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
 *
 *  為什麼要這樣分？把「多」拆成三個 byte 分開統計，等於丟掉「E5 後面常接 A4」這種資訊；
 *  把一個 sample 拆成高、低兩個 byte 也是。符號定得對，同一套 Huffman 演算法壓縮率差很多
 *  ——報告要你們用數字比較（規格「壓縮率」一節）。代價是符號種類變多（sample 最多 65,536 種、
 *  字元上千種），codebook 變大、建樹也不能再用「每輪線性找最小」的寫法。
 *
 *  huff_encode 產生的那一塊資料必須「自己帶 codebook、可以獨立解碼」，建議的長相：
 *
 *      +------+----------+--------+----------------+-------------------+------------------+
 *      | 符號 | 原始長度 | 符號數 | codebook       | （SYM_S16）檔頭等 | bitstream        |
 *      | 種類 | （bytes）|        | （怎麼存自訂） | 非 sample 的 bytes| （末尾補 0）     |
 *      +------+----------+--------+----------------+-------------------+------------------+
 *===========================================================================*/
#include "textlink.h"
#include <string.h> 
#include <stdlib.h> 

// -----------------------------------------------------------------------------
// 1. 內部資料結構與自訂格式設計
// -----------------------------------------------------------------------------

// 我們的自訂封包格式設計 (總共固定 13 bytes 的標頭 + 動態大小的 Codebook + Bitstream)：
// [0]    : sym (1 byte) - 記錄當下是哪種符號模式 (SYM_BYTE/CHAR/S16)
// [1..4] : orig_len (4 bytes, Big-endian) - 壓縮前的原始總 bytes 數
// [5..8] : symbol_count (4 bytes, Big-endian) - 壓縮前的總符號數量 (SYM_BYTE 時等於 orig_len)
// [9..12]: unique_symbols (4 bytes, Big-endian) - 有幾種不同的符號 (K)
// [Codebook 區塊]: 每個唯一符號存 (1 byte 符號值 + 1 byte 編碼長度 + 4 bytes Code 值) -> 共 6 * K bytes
// [Bitstream 區塊]: 實際壓縮後的資料 (結尾補 0)

// Huffman Tree 節點結構
typedef struct {
    uint32_t symbol; // 符號 (BYTE模式為0~255)
    uint64_t freq;   // 出現頻率
    int left;        // 左子節點的陣列索引 (-1 代表無)
    int right;       // 右子節點的陣列索引 (-1 代表無)
} HuffNode;

// 儲存每個符號的最終編碼結果
typedef struct {
    uint32_t code;   // 編碼的二進位值
    int length;      // 編碼長度 (有幾個 bit)
} HuffCode;

// -----------------------------------------------------------------------------
// 2. 核心演算法：建樹與遞迴產生 Codebook
// -----------------------------------------------------------------------------

// 給定一組頻率與符號陣列，建立 Huffman Tree 並回傳根節點索引
static int build_huffman_tree(HuffNode *nodes, int num_nodes) {
    if (num_nodes == 0) return -1;
    if (num_nodes == 1) return 0;

    int total_nodes = num_nodes;
    
    // 每次找兩個頻率最小的合併，直到剩一個根節點
    for (int step = 1; step < num_nodes; step++) {
        int min1 = -1, min2 = -1;
        
        // 尋找最小與次小
        for (int i = 0; i < total_nodes; i++) {
            if (nodes[i].freq > 0) { // 還沒被合併
                if (min1 == -1 || nodes[i].freq < nodes[min1].freq) {
                    min2 = min1;
                    min1 = i;
                } else if (min2 == -1 || nodes[i].freq < nodes[min2].freq) {
                    min2 = i;
                }
            }
        }
        
        // 建立新父節點
        nodes[total_nodes].symbol = 0; 
        nodes[total_nodes].freq = nodes[min1].freq + nodes[min2].freq;
        nodes[total_nodes].left = min1;
        nodes[total_nodes].right = min2;
        
        // 將舊節點標記為已合併
        nodes[min1].freq = 0;
        nodes[min2].freq = 0;
        
        total_nodes++;
    }
    
    return total_nodes - 1; // 回傳最後一個建立的節點 (Root)
}

// 遞迴走訪 Tree 產生每個符號的編碼
static void generate_codes(HuffNode *nodes, int node_idx, uint32_t current_code, int current_len, HuffCode *codebook) {
    if (node_idx == -1) return;
    
    // 如果是葉節點，記錄它的 Code
    if (nodes[node_idx].left == -1 && nodes[node_idx].right == -1) {
        codebook[nodes[node_idx].symbol].code = current_code;
        codebook[nodes[node_idx].symbol].length = (current_len == 0) ? 1 : current_len; 
        return;
    }
    
    // 往左走加 0，往右走加 1
    generate_codes(nodes, nodes[node_idx].left, (current_code << 1) | 0, current_len + 1, codebook);
    generate_codes(nodes, nodes[node_idx].right, (current_code << 1) | 1, current_len + 1, codebook);
}

// -----------------------------------------------------------------------------
// 3. Bit Writer / Reader 工具
// -----------------------------------------------------------------------------
typedef struct {
    uint8_t *buf;
    size_t cap;
    size_t byte_pos;
    int bit_offset; // 0~7, 0代表最高位 (MSB)
} BitWriter;

static void bw_write_bit(BitWriter *bw, int bit) {
    if (bw->byte_pos >= bw->cap) return; 
    if (bit) bw->buf[bw->byte_pos] |= (1 << (7 - bw->bit_offset));
    bw->bit_offset++;
    if (bw->bit_offset == 8) {
        bw->bit_offset = 0;
        bw->byte_pos++;
    }
}

typedef struct {
    const uint8_t *buf;
    size_t len;
    size_t byte_pos;
    int bit_offset;
} BitReader;

static int br_read_bit(BitReader *br) {
    if (br->byte_pos >= br->len) return -1;
    int bit = (br->buf[br->byte_pos] >> (7 - br->bit_offset)) & 1;
    br->bit_offset++;
    if (br->bit_offset == 8) {
        br->bit_offset = 0;
        br->byte_pos++;
    }
    return bit;
}

/*--------------------------------------------------------------------------
 * ★ TODO 4：編碼
 *   sym 指定符號種類。資料不適用（見步驟 0）回傳 TL_ERR_DATA，殼會改用 SYM_BYTE 再呼叫一次。
 *   成功：*out = malloc 出來的結果、*out_len = 它的長度，回傳 TL_OK。
 *   記憶體不足回傳 TL_ERR_NOMEM。
 *-------------------------------------------------------------------------*/
int huff_encode(const uint8_t *in, size_t in_len, tl_sym_t sym, uint8_t **out, size_t *out_len) {
    if (sym != SYM_BYTE) return TL_ERR_DATA; // 暫時只解 BYTE

    // 空輸入的特殊處理：回傳只有標頭的封包
    if (in_len == 0) {
        *out = calloc(13, 1);
        (*out)[0] = sym;
        *out_len = 13;
        return TL_OK;
    }

    // 1. 統計頻率 (SYM_BYTE 只有 256 種可能)
    uint64_t freq_table[256] = {0};
    for (size_t i = 0; i < in_len; i++) {
        freq_table[in[i]]++;
    }

    // 2. 準備 Tree 節點 (256個葉節點 + 最多255個內部節點)
    HuffNode nodes[512] = {0};
    int unique_symbols = 0;
    
    // 初始化葉節點並集中到陣列前端
    for (int i = 0; i < 256; i++) {
        if (freq_table[i] > 0) {
            nodes[unique_symbols].symbol = i;
            nodes[unique_symbols].freq = freq_table[i];
            nodes[unique_symbols].left = -1;
            nodes[unique_symbols].right = -1;
            unique_symbols++;
        }
    }

    // 3. 建樹與產生 Codebook
    int root_idx = build_huffman_tree(nodes, unique_symbols);
    HuffCode codebook[256] = {0};
    if (root_idx != -1) {
        // 特別處理：如果檔案裡全都是同一個字元
        if (unique_symbols == 1) {
            codebook[nodes[0].symbol].code = 0;
            codebook[nodes[0].symbol].length = 1;
        } else {
            generate_codes(nodes, root_idx, 0, 0, codebook);
        }
    }

    // 4. 計算輸出大小並配置記憶體
    size_t header_size = 13; 
    size_t cb_size = unique_symbols * 6; 
    
    // 計算 Bitstream 總 bits 數
    size_t total_bits = 0;
    for (size_t i = 0; i < in_len; i++) {
        total_bits += codebook[in[i]].length;
    }
    size_t bitstream_bytes = (total_bits + 7) / 8;
    
    size_t final_size = header_size + cb_size + bitstream_bytes;
    uint8_t *buf = calloc(final_size, 1);
    if (!buf) return TL_ERR_NOMEM;

    // 5. 寫入標頭 (Big-endian)
    buf[0] = sym;
    buf[1] = (in_len >> 24) & 0xFF; buf[2] = (in_len >> 16) & 0xFF; 
    buf[3] = (in_len >> 8) & 0xFF;  buf[4] = in_len & 0xFF;
    
    uint32_t sym_count = (uint32_t)in_len; // BYTE 模式下，符號數 = byte 數
    buf[5] = (sym_count >> 24) & 0xFF; buf[6] = (sym_count >> 16) & 0xFF; 
    buf[7] = (sym_count >> 8) & 0xFF;  buf[8] = sym_count & 0xFF;

    buf[9] = (unique_symbols >> 24) & 0xFF; buf[10] = (unique_symbols >> 16) & 0xFF; 
    buf[11] = (unique_symbols >> 8) & 0xFF; buf[12] = unique_symbols & 0xFF;

    // 6. 寫入 Codebook
    size_t pos = 13;
    for (int i = 0; i < 256; i++) {
        if (codebook[i].length > 0) {
            buf[pos++] = i; 
            buf[pos++] = codebook[i].length; 
            uint32_t c = codebook[i].code;
            buf[pos++] = (c >> 24) & 0xFF; buf[pos++] = (c >> 16) & 0xFF;
            buf[pos++] = (c >> 8) & 0xFF;  buf[pos++] = c & 0xFF;
        }
    }

    // 7. 寫入壓縮後的 Bitstream
    BitWriter bw = { .buf = buf, .cap = final_size, .byte_pos = pos, .bit_offset = 0 };
    for (size_t i = 0; i < in_len; i++) {
        uint32_t c = codebook[in[i]].code;
        int len = codebook[in[i]].length;
        for (int b = len - 1; b >= 0; b--) {
            bw_write_bit(&bw, (c >> b) & 1);
        }
    }

    *out = buf;
    *out_len = final_size;
    return TL_OK;
}

/*--------------------------------------------------------------------------
 * ★ TODO 5：解碼
 *   符號種類由區塊自己記載，所以這裡不需要 sym 參數。
 *   in 是「對方送來的」，要當成可能是壞的：
 *-------------------------------------------------------------------------*/
int huff_decode(const uint8_t *in, size_t in_len, size_t max_out, uint8_t **out, size_t *out_len) {
    if (in_len < 13) return TL_ERR_DATA; // 連標頭都不夠
    if (in[0] != SYM_BYTE) return TL_ERR_DATA; // 暫時只處理 BYTE

    // 1. 讀取標頭 (Big-endian)
    uint32_t orig_len = ((uint32_t)in[1] << 24) | ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 8) | in[4];
    uint32_t sym_count = ((uint32_t)in[5] << 24) | ((uint32_t)in[6] << 16) | ((uint32_t)in[7] << 8) | in[8];
    uint32_t unique_symbols = ((uint32_t)in[9] << 24) | ((uint32_t)in[10] << 16) | ((uint32_t)in[11] << 8) | in[12];

    // 空輸入的特殊處理
    if (orig_len == 0) {
        *out = calloc(1, 1);
        *out_len = 0;
        return TL_OK;
    }

    if (orig_len > max_out) return TL_ERR_DATA; // 安全防護
    if (unique_symbols > 256) return TL_ERR_DATA; // BYTE 模式最多 256 種符號
    
    size_t cb_size = unique_symbols * 6;
    if (13 + cb_size > in_len) return TL_ERR_DATA; // 檢查 Codebook 區塊是否完整

    // 2. 讀取 Codebook 
    HuffCode codebook[256] = {0};
    int active_symbols[256];
    
    size_t pos = 13;
    for (uint32_t i = 0; i < unique_symbols; i++) {
        uint8_t sym_val = in[pos++];
        uint8_t len_val = in[pos++];
        uint32_t code_val = ((uint32_t)in[pos] << 24) | ((uint32_t)in[pos+1] << 16) | ((uint32_t)in[pos+2] << 8) | in[pos+3];
        pos += 4;
        
        codebook[sym_val].code = code_val;
        codebook[sym_val].length = len_val;
        active_symbols[i] = sym_val;
    }

    // 3. 配置輸出記憶體
    uint8_t *buf = calloc(orig_len + 1, 1); // 多1 byte避免奇怪操作
    if (!buf) return TL_ERR_NOMEM;

    // 4. 讀取 Bitstream 並解碼
    BitReader br = { .buf = in, .len = in_len, .byte_pos = pos, .bit_offset = 0 };
    uint32_t current_code = 0;
    int current_len = 0;
    size_t decoded_syms = 0;
    size_t output_pos = 0;

    while (decoded_syms < sym_count) {
        int bit = br_read_bit(&br);
        if (bit == -1) { // Bitstream 提早結束
            free(buf);
            return TL_ERR_DATA;
        }

        current_code = (current_code << 1) | bit;
        current_len++;

        // 在 Codebook 中尋找匹配
        for (uint32_t i = 0; i < unique_symbols; i++) {
            uint8_t s = active_symbols[i];
            if (codebook[s].length == current_len && codebook[s].code == current_code) {
                buf[output_pos++] = s;
                decoded_syms++;
                current_code = 0;
                current_len = 0;
                break;
            }
        }
    }

    *out = buf;
    *out_len = orig_len;
    return TL_OK;
}