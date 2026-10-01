/*============================================================================
 *  huffman.c  —  Huffman 編碼與解碼（本專題的主菜）
 *----------------------------------------------------------------------------
 *  Huffman coding 的第一個問題不是「怎麼建樹」，而是「符號是什麼」——
 *  你對什麼東西統計出現機率，就決定了能壓到多小。本專題規定：
 *
 *    SYM_CHAR  文字：符號 = UTF-8 字元（code point）
 *              先把 bytes 切成一個一個「字」（MP1 做過的事），統計每個字出現的機率，再編碼。
 *              「多」是一個符號，不是 E5、A4、9A 三個符號。
 *    SYM_S16   WAV ：符號 = 16-bit sample value
 *              解析 RIFF 檔頭找到 data 區，把每 2 bytes（little-endian）當成一個 sample，
 *              對 sample 值做 histogram，再編碼。雙聲道就是左右交錯的 sample，一樣處理。
 *              data 區以外的 bytes（檔頭、其他 chunk）不是 sample，要原樣保留在區塊裡。
 *    SYM_BYTE  其他：符號 = byte。任何資料都適用；也是上面兩種不適用時的退路。
 *===========================================================================*/
#include "textlink.h"
#include <string.h> 
#include <stdlib.h> 

// -----------------------------------------------------------------------------
// 1. 內部資料結構與自訂格式設計
// -----------------------------------------------------------------------------
typedef struct {
    uint32_t symbol; 
    uint64_t freq;   
    int left;        
    int right;       
} HuffNode;

typedef struct {
    uint32_t code;   
    int length;      
} HuffCode;

// -----------------------------------------------------------------------------
// 2. 核心演算法：建樹與遞迴產生 Codebook
// -----------------------------------------------------------------------------
static int build_huffman_tree(HuffNode *nodes, int num_nodes) {
    if (num_nodes == 0) return -1;
    if (num_nodes == 1) return 0;
    int total_nodes = num_nodes;
    
    for (int step = 1; step < num_nodes; step++) {
        int min1 = -1, min2 = -1;
        for (int i = 0; i < total_nodes; i++) {
            if (nodes[i].freq > 0) { 
                if (min1 == -1 || nodes[i].freq < nodes[min1].freq) {
                    min2 = min1; min1 = i;
                } else if (min2 == -1 || nodes[i].freq < nodes[min2].freq) {
                    min2 = i;
                }
            }
        }
        nodes[total_nodes].symbol = 0; 
        nodes[total_nodes].freq = nodes[min1].freq + nodes[min2].freq;
        nodes[total_nodes].left = min1;
        nodes[total_nodes].right = min2;
        nodes[min1].freq = 0;
        nodes[min2].freq = 0;
        total_nodes++;
    }
    return total_nodes - 1; 
}

static void generate_codes(HuffNode *nodes, int node_idx, uint32_t current_code, int current_len, HuffCode *codebook) {
    if (node_idx == -1) return;
    if (nodes[node_idx].left == -1 && nodes[node_idx].right == -1) {
        codebook[nodes[node_idx].symbol].code = current_code;
        codebook[nodes[node_idx].symbol].length = (current_len == 0) ? 1 : current_len; 
        return;
    }
    generate_codes(nodes, nodes[node_idx].left, (current_code << 1) | 0, current_len + 1, codebook);
    generate_codes(nodes, nodes[node_idx].right, (current_code << 1) | 1, current_len + 1, codebook);
}

// -----------------------------------------------------------------------------
// 3. Bit Writer / Reader 工具
// -----------------------------------------------------------------------------
typedef struct { uint8_t *buf; size_t cap; size_t byte_pos; int bit_offset; } BitWriter;
static void bw_write_bit(BitWriter *bw, int bit) {
    if (bw->byte_pos >= bw->cap) return; 
    if (bit) bw->buf[bw->byte_pos] |= (1 << (7 - bw->bit_offset));
    bw->bit_offset++;
    if (bw->bit_offset == 8) { bw->bit_offset = 0; bw->byte_pos++; }
}

typedef struct { const uint8_t *buf; size_t len; size_t byte_pos; int bit_offset; } BitReader;
static int br_read_bit(BitReader *br) {
    if (br->byte_pos >= br->len) return -1;
    int bit = (br->buf[br->byte_pos] >> (7 - br->bit_offset)) & 1;
    br->bit_offset++;
    if (br->bit_offset == 8) { br->bit_offset = 0; br->byte_pos++; }
    return bit;
}

// -----------------------------------------------------------------------------
// 4. UTF-8 切字與還原工具 (SYM_CHAR 專用)
// -----------------------------------------------------------------------------
static int get_next_char(const uint8_t *s, size_t n, size_t *pos) {
    if (*pos >= n) return -1;
    uint8_t c = s[*pos];
    int bytes = 0;
    uint32_t cp = 0, min_cp = 0;

    if ((c & 0x80) == 0x00) { bytes = 1; cp = c; min_cp = 0; }
    else if ((c & 0xE0) == 0xC0) { bytes = 2; cp = c & 0x1F; min_cp = 0x80; }
    else if ((c & 0xF0) == 0xE0) { bytes = 3; cp = c & 0x0F; min_cp = 0x800; }
    else if ((c & 0xF8) == 0xF0) { bytes = 4; cp = c & 0x07; min_cp = 0x10000; }
    else return -1;

    if (*pos + bytes > n) return -1;
    for (int j = 1; j < bytes; j++) {
        if ((s[*pos + j] & 0xC0) != 0x80) return -1;
        cp = (cp << 6) | (s[*pos + j] & 0x3F);
    }
    if (cp < min_cp || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) return -1;

    *pos += bytes;
    return cp;
}

static void put_char(uint32_t cp, uint8_t *buf, size_t *pos) {
    if (cp <= 0x7F) {
        buf[(*pos)++] = cp;
    } else if (cp <= 0x7FF) {
        buf[(*pos)++] = 0xC0 | (cp >> 6);
        buf[(*pos)++] = 0x80 | (cp & 0x3F);
    } else if (cp <= 0xFFFF) {
        buf[(*pos)++] = 0xE0 | (cp >> 12);
        buf[(*pos)++] = 0x80 | ((cp >> 6) & 0x3F);
        buf[(*pos)++] = 0x80 | (cp & 0x3F);
    } else {
        buf[(*pos)++] = 0xF0 | (cp >> 18);
        buf[(*pos)++] = 0x80 | ((cp >> 12) & 0x3F);
        buf[(*pos)++] = 0x80 | ((cp >> 6) & 0x3F);
        buf[(*pos)++] = 0x80 | (cp & 0x3F);
    }
}

// -----------------------------------------------------------------------------
// 5. WAV 解析工具 (SYM_S16 專用)
// -----------------------------------------------------------------------------
typedef struct {
    size_t data_offset; // data chunk 的 payload 起點
    size_t data_len;    // data payload 長度 (bytes)
    size_t total_hdr;   // data payload 之前的全部 bytes
} WavInfo;

static int parse_wav(const uint8_t *in, size_t in_len, WavInfo *info) {
    if (in_len < 12) return TL_ERR_DATA;
    if (memcmp(in, "RIFF", 4) != 0 || memcmp(in + 8, "WAVE", 4) != 0) return TL_ERR_DATA;

    size_t pos = 12;
    int found_fmt = 0, found_data = 0;

    while (pos + 8 <= in_len) {
        const char *id = (const char *)(in + pos);
        uint32_t sz = (uint32_t)in[pos+4] | ((uint32_t)in[pos+5] << 8) |
                      ((uint32_t)in[pos+6] << 16) | ((uint32_t)in[pos+7] << 24);
        pos += 8;

        if (memcmp(id, "fmt ", 4) == 0) {
            if (sz < 16 || pos + sz > in_len) return TL_ERR_DATA;
            uint16_t audio_fmt = (uint16_t)in[pos] | ((uint16_t)in[pos+1] << 8);
            uint16_t bits = (uint16_t)in[pos+14] | ((uint16_t)in[pos+15] << 8);
            if (audio_fmt != 1 || bits != 16) return TL_ERR_DATA; // 必須是 16-bit PCM
            found_fmt = 1;
            pos += sz + (sz & 1); // RIFF padding，奇數長度要補 1 byte
        } else if (memcmp(id, "data", 4) == 0) {
            if (!found_fmt) return TL_ERR_DATA;
            
            // 如果檔案結尾被截斷，只算到實際檔案結尾為止
            if (pos + sz > in_len) sz = (uint32_t)(in_len - pos);
            
            info->data_offset = pos;
            // 關鍵修正：扣除湊不成 16-bit sample 的奇數 byte，讓它自然落入 tail_len
            info->data_len = sz - (sz % 2); 
            info->total_hdr = pos;
            
            found_data = 1;
            break;
        } else {
            pos += sz + (sz & 1); // 略過其他 chunk，一樣要處理 padding
        }
    }

    if (!found_fmt || !found_data) return TL_ERR_DATA;
    return TL_OK;
}

// -----------------------------------------------------------------------------
// ★ TODO 4：編碼 (支援 SYM_BYTE, SYM_CHAR, SYM_S16)
// -----------------------------------------------------------------------------
int huff_encode(const uint8_t *in, size_t in_len, tl_sym_t sym, uint8_t **out, size_t *out_len) {
    WavInfo winfo = {0};
    if (sym == SYM_S16) {
        int rc = parse_wav(in, in_len, &winfo);
        if (rc != TL_OK) return rc;
    }

    if (in_len == 0) {
        *out = calloc(21, 1);
        if (!*out) return TL_ERR_NOMEM;
        (*out)[0] = sym;
        *out_len = 21;
        return TL_OK;
    }

    uint32_t max_sym = 256;
    if (sym == SYM_CHAR) max_sym = 0x110000;
    else if (sym == SYM_S16) max_sym = 65536;

    uint64_t *freq = calloc(max_sym, sizeof(uint64_t));
    if (!freq) return TL_ERR_NOMEM;

    uint32_t sym_count = 0;
    if (sym == SYM_BYTE) {
        for (size_t i = 0; i < in_len; i++) freq[in[i]]++;
        sym_count = (uint32_t)in_len;
    } else if (sym == SYM_CHAR) {
        size_t pos = 0;
        while (pos < in_len) {
            int cp = get_next_char(in, in_len, &pos);
            if (cp == -1) { free(freq); return TL_ERR_DATA; }
            freq[cp]++;
            sym_count++;
        }
    } else { // SYM_S16
        const uint8_t *d = in + winfo.data_offset;
        for (size_t i = 0; i < winfo.data_len; i += 2) {
            uint16_t s = (uint16_t)d[i] | ((uint16_t)d[i+1] << 8);
            freq[s]++;
            sym_count++;
        }
    }

    HuffNode *nodes = calloc(max_sym * 2, sizeof(HuffNode));
    if (!nodes) { free(freq); return TL_ERR_NOMEM; }

    int unique_symbols = 0;
    for (uint32_t i = 0; i < max_sym; i++) {
        if (freq[i] > 0) {
            nodes[unique_symbols].symbol = i;
            nodes[unique_symbols].freq = freq[i];
            nodes[unique_symbols].left = -1;
            nodes[unique_symbols].right = -1;
            unique_symbols++;
        }
    }
    free(freq);

    int root_idx = build_huffman_tree(nodes, unique_symbols);
    HuffCode *codebook = calloc(max_sym, sizeof(HuffCode));
    if (!codebook) { free(nodes); return TL_ERR_NOMEM; }

    if (root_idx != -1) {
        if (unique_symbols == 1) {
            codebook[nodes[0].symbol].code = 0;
            codebook[nodes[0].symbol].length = 1;
        } else {
            generate_codes(nodes, root_idx, 0, 0, codebook);
        }
    }

    size_t sym_size = (sym == SYM_BYTE) ? 1 : ((sym == SYM_S16) ? 2 : 4);
    size_t cb_size = unique_symbols * (sym_size + 1 + 4);

    size_t total_bits = 0;
    if (sym == SYM_BYTE) {
        for (size_t i = 0; i < in_len; i++) total_bits += codebook[in[i]].length;
    } else if (sym == SYM_CHAR) {
        size_t pos = 0;
        while (pos < in_len) {
            uint32_t cp = (uint32_t)get_next_char(in, in_len, &pos);
            total_bits += codebook[cp].length;
        }
    } else {
        const uint8_t *d = in + winfo.data_offset;
        for (size_t i = 0; i < winfo.data_len; i += 2) {
            uint16_t s = (uint16_t)d[i] | ((uint16_t)d[i+1] << 8);
            total_bits += codebook[s].length;
        }
    }

    size_t bitstream_bytes = (total_bits + 7) / 8;
    size_t wav_extra = 0;
    size_t tail_len = 0;
    if (sym == SYM_S16) {
        tail_len = in_len - (winfo.data_offset + winfo.data_len);
        wav_extra = 8 + winfo.total_hdr + tail_len; // 8 bytes 存長度 + hdr + tail
    }

    size_t final_size = 13 + wav_extra + cb_size + bitstream_bytes;
    uint8_t *buf = calloc(final_size, 1);
    if (!buf) { free(nodes); free(codebook); return TL_ERR_NOMEM; }

    buf[0] = sym;
    buf[1] = (in_len >> 24) & 0xFF; buf[2] = (in_len >> 16) & 0xFF; buf[3] = (in_len >> 8) & 0xFF; buf[4] = in_len & 0xFF;
    buf[5] = (sym_count >> 24) & 0xFF; buf[6] = (sym_count >> 16) & 0xFF; buf[7] = (sym_count >> 8) & 0xFF; buf[8] = sym_count & 0xFF;
    buf[9] = (unique_symbols >> 24) & 0xFF; buf[10] = (unique_symbols >> 16) & 0xFF; buf[11] = (unique_symbols >> 8) & 0xFF; buf[12] = unique_symbols & 0xFF;

    size_t out_pos = 13;
    if (sym == SYM_S16) {
        buf[out_pos++] = (winfo.total_hdr >> 24) & 0xFF; buf[out_pos++] = (winfo.total_hdr >> 16) & 0xFF;
        buf[out_pos++] = (winfo.total_hdr >> 8) & 0xFF;  buf[out_pos++] = winfo.total_hdr & 0xFF;
        buf[out_pos++] = (tail_len >> 24) & 0xFF; buf[out_pos++] = (tail_len >> 16) & 0xFF;
        buf[out_pos++] = (tail_len >> 8) & 0xFF;  buf[out_pos++] = tail_len & 0xFF;
        memcpy(buf + out_pos, in, winfo.total_hdr);
        out_pos += winfo.total_hdr;
        if (tail_len > 0) {
            memcpy(buf + out_pos, in + winfo.data_offset + winfo.data_len, tail_len);
            out_pos += tail_len;
        }
    }

    for (uint32_t i = 0; i < max_sym; i++) {
        if (codebook[i].length > 0) {
            if (sym == SYM_BYTE) {
                buf[out_pos++] = i & 0xFF;
            } else if (sym == SYM_S16) {
                buf[out_pos++] = (i >> 8) & 0xFF; buf[out_pos++] = i & 0xFF;
            } else {
                buf[out_pos++] = (i >> 24) & 0xFF; buf[out_pos++] = (i >> 16) & 0xFF;
                buf[out_pos++] = (i >> 8) & 0xFF; buf[out_pos++] = i & 0xFF;
            }
            buf[out_pos++] = codebook[i].length;
            uint32_t c = codebook[i].code;
            buf[out_pos++] = (c >> 24) & 0xFF; buf[out_pos++] = (c >> 16) & 0xFF;
            buf[out_pos++] = (c >> 8) & 0xFF; buf[out_pos++] = c & 0xFF;
        }
    }

    BitWriter bw = { .buf = buf, .cap = final_size, .byte_pos = out_pos, .bit_offset = 0 };
    if (sym == SYM_BYTE) {
        for (size_t i = 0; i < in_len; i++) {
            uint32_t c = codebook[in[i]].code;
            int len = codebook[in[i]].length;
            for (int b = len - 1; b >= 0; b--) bw_write_bit(&bw, (c >> b) & 1);
        }
    } else if (sym == SYM_CHAR) {
        size_t pos = 0;
        while (pos < in_len) {
            uint32_t cp = (uint32_t)get_next_char(in, in_len, &pos);
            uint32_t c = codebook[cp].code;
            int len = codebook[cp].length;
            for (int b = len - 1; b >= 0; b--) bw_write_bit(&bw, (c >> b) & 1);
        }
    } else {
        const uint8_t *d = in + winfo.data_offset;
        for (size_t i = 0; i < winfo.data_len; i += 2) {
            uint16_t s = (uint16_t)d[i] | ((uint16_t)d[i+1] << 8);
            uint32_t c = codebook[s].code;
            int len = codebook[s].length;
            for (int b = len - 1; b >= 0; b--) bw_write_bit(&bw, (c >> b) & 1);
        }
    }

    free(nodes); free(codebook);
    *out = buf;
    *out_len = final_size;
    return TL_OK;
}

// -----------------------------------------------------------------------------
// ★ TODO 5：解碼 (支援 SYM_BYTE, SYM_CHAR, SYM_S16)
// -----------------------------------------------------------------------------
int huff_decode(const uint8_t *in, size_t in_len, size_t max_out, uint8_t **out, size_t *out_len) {
    if (in_len < 13) return TL_ERR_DATA;
    uint8_t sym = in[0];
    if (sym != SYM_BYTE && sym != SYM_CHAR && sym != SYM_S16) return TL_ERR_DATA;

    uint32_t orig_len = ((uint32_t)in[1] << 24) | ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 8) | in[4];
    uint32_t sym_count = ((uint32_t)in[5] << 24) | ((uint32_t)in[6] << 16) | ((uint32_t)in[7] << 8) | in[8];
    uint32_t unique_symbols = ((uint32_t)in[9] << 24) | ((uint32_t)in[10] << 16) | ((uint32_t)in[11] << 8) | in[12];

    if (orig_len == 0) {
        *out = calloc(1, 1);
        if (!*out) return TL_ERR_NOMEM;
        *out_len = 0;
        return TL_OK;
    }

    if (orig_len > max_out) return TL_ERR_DATA;
    uint32_t max_sym = (sym == SYM_BYTE) ? 256 : ((sym == SYM_S16) ? 65536 : 0x110000);
    if (unique_symbols > max_sym) return TL_ERR_DATA;

    size_t pos = 13;
    uint32_t total_hdr = 0, tail_len = 0;
    const uint8_t *hdr_bytes = NULL, *tail_bytes = NULL;

    if (sym == SYM_S16) {
        if (pos + 8 > in_len) return TL_ERR_DATA;
        total_hdr = ((uint32_t)in[pos] << 24) | ((uint32_t)in[pos+1] << 16) | ((uint32_t)in[pos+2] << 8) | in[pos+3];
        tail_len = ((uint32_t)in[pos+4] << 24) | ((uint32_t)in[pos+5] << 16) | ((uint32_t)in[pos+6] << 8) | in[pos+7];
        pos += 8;
        if (pos + total_hdr + tail_len > in_len) return TL_ERR_DATA;
        hdr_bytes = in + pos; pos += total_hdr;
        tail_bytes = in + pos; pos += tail_len;
    }

    size_t sym_size = (sym == SYM_BYTE) ? 1 : ((sym == SYM_S16) ? 2 : 4);
    size_t cb_size = unique_symbols * (sym_size + 1 + 4);
    if (pos + cb_size > in_len) return TL_ERR_DATA;

    HuffCode *codebook = calloc(max_sym, sizeof(HuffCode));
    uint32_t *active_symbols = calloc(unique_symbols, sizeof(uint32_t));
    if (!codebook || !active_symbols) { free(codebook); free(active_symbols); return TL_ERR_NOMEM; }

    for (uint32_t i = 0; i < unique_symbols; i++) {
        uint32_t sym_val = 0;
        if (sym == SYM_BYTE) {
            sym_val = in[pos++];
        } else if (sym == SYM_S16) {
            sym_val = ((uint32_t)in[pos] << 8) | in[pos+1];
            pos += 2;
        } else {
            sym_val = ((uint32_t)in[pos] << 24) | ((uint32_t)in[pos+1] << 16) | ((uint32_t)in[pos+2] << 8) | in[pos+3];
            pos += 4;
            if (sym_val >= max_sym) { free(codebook); free(active_symbols); return TL_ERR_DATA; }
        }
        uint8_t len_val = in[pos++];
        uint32_t code_val = ((uint32_t)in[pos] << 24) | ((uint32_t)in[pos+1] << 16) | ((uint32_t)in[pos+2] << 8) | in[pos+3];
        pos += 4;
        
        codebook[sym_val].code = code_val;
        codebook[sym_val].length = len_val;
        active_symbols[i] = sym_val;
    }

    uint8_t *buf = calloc(orig_len + 1, 1);
    if (!buf) { free(codebook); free(active_symbols); return TL_ERR_NOMEM; }

    size_t output_pos = 0;
    if (sym == SYM_S16) {
        if (total_hdr > orig_len) { free(buf); free(codebook); free(active_symbols); return TL_ERR_DATA; }
        memcpy(buf, hdr_bytes, total_hdr);
        output_pos = total_hdr;
    }

    BitReader br = { .buf = in, .len = in_len, .byte_pos = pos, .bit_offset = 0 };
    uint32_t current_code = 0;
    int current_len = 0;
    size_t decoded_syms = 0;

    while (decoded_syms < sym_count) {
        int bit = br_read_bit(&br);
        if (bit == -1) { free(buf); free(codebook); free(active_symbols); return TL_ERR_DATA; }

        current_code = (current_code << 1) | bit;
        current_len++;

        for (uint32_t i = 0; i < unique_symbols; i++) {
            uint32_t s = active_symbols[i];
            if (codebook[s].length == current_len && codebook[s].code == current_code) {
                if (sym == SYM_BYTE) {
                    if (output_pos >= orig_len) { free(buf); free(codebook); free(active_symbols); return TL_ERR_DATA; }
                    buf[output_pos++] = s;
                } else if (sym == SYM_CHAR) {
                    if (output_pos >= orig_len) { free(buf); free(codebook); free(active_symbols); return TL_ERR_DATA; }
                    put_char(s, buf, &output_pos);
                } else { // SYM_S16
                    if (output_pos + 2 > orig_len) { free(buf); free(codebook); free(active_symbols); return TL_ERR_DATA; }
                    buf[output_pos++] = s & 0xFF;
                    buf[output_pos++] = (s >> 8) & 0xFF;
                }
                decoded_syms++;
                current_code = 0;
                current_len = 0;
                break;
            }
        }
    }
    
    if (sym == SYM_S16 && tail_len > 0) {
        if (output_pos + tail_len > orig_len) { free(buf); free(codebook); free(active_symbols); return TL_ERR_DATA; }
        memcpy(buf + output_pos, tail_bytes, tail_len);
        output_pos += tail_len;
    }

    free(codebook); free(active_symbols);
    if (output_pos != orig_len) { free(buf); return TL_ERR_DATA; }

    *out = buf;
    *out_len = orig_len;
    return TL_OK;
}