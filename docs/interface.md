# TextLink 介面文件（Team 1 實作版）

> 目標：**別組只看這份文件，就能寫出與我們互通的程式。** 每個欄位寫清楚幾 bytes、什麼順序、
> 位元組順序與位元順序。

組別：Team 1 成員與角色：陳皓祥 (程式實作與架構設計) 文件版本／日期：v1.0 (2026-10-05)

## 1. 模組與資料流

本專案採用模組化設計，核心資料流如下：
1. **傳輸層 (`net.c`)**：負責建立 TCP 連線與收發 Socket 位元組流。
2. **封包層 (`frame.c`)**：負責將應用層資料（文字/檔案）打包成帶有 `length` 與 `type` 的 TL-Frame，解決 TCP 黏包/半包問題。
3. **驗證與壓縮層 (`utf8.c` & `huffman.c`)**：
   - 收到 TEXT_RAW 時，透過 `utf8.c` 驗證字串合法性。
   - 啟用壓縮時，透過 `huffman.c` 進行三種模式 (`SYM_BYTE`, `SYM_CHAR`, `SYM_S16`) 的編解碼。
4. **應用層 (`chat`/`transfer`)**：處理使用者輸入，呼叫底層 API，並管理檔案 I/O。

## 2. frame 外框（規格固定）

| 欄位 | bytes | 說明 |
|---|---|---|
| length | 4 | big-endian 無號整數＝type＋payload 的 bytes 數；合法範圍 1–16,777,216 |
| type | 1 | 見下表 |
| payload | length − 1 | 依 type 而定 |

## 3. 各 type 的 payload

### 0x01 TEXT_RAW

UTF-8 文字，不含結尾 `\0`；最長 4095 bytes。接收端以 RFC 3629 檢查，不合法則丟棄並顯示系統訊息。

### 0x02 TEXT_HUFF

將輸入文字呼叫 `huff_encode(SYM_CHAR)` 壓縮，產生的**完整 Huffman 區塊（含 Header、Codebook、Bitstream）原樣放入 payload**。若壓完比原文大，仍強制使用壓縮後的區塊，以維持實作單純與封包格式的一致性，且實驗證明文字資料壓縮率通常低於 40%。若輸入包含非法 UTF-8，則退回使用 `SYM_BYTE` 模式壓縮。

### 0x10 FILE_BEGIN（傳送端 → 接收端）

| 欄位 | bytes | 說明 |
|---|---|---|
| mode | 1 | 0＝raw、1＝huff |
| orig_size | 8 | big-endian；原檔 bytes 數；上限 64 MiB |
| data_size | 8 | big-endian；之後所有 FILE_DATA 的 payload 總 bytes 數；raw 時必須等於 orig_size |
| name | 其餘 | 檔名，不含路徑；接收端只保留 `0-9 A-Z a-z . - _`，其餘換成 `_` |

### 0x11 FILE_DATA（傳送端 → 接收端）

傳輸資料的一段，每個最多 65,536 bytes，依序接起來共 data_size bytes。
- raw 模式：直接為原始檔案內容片段。
- huff 模式：為第 4 節 Huffman 區塊的一段（將整個壓縮結果切割發送）。

### 0x12 FILE_END

傳送端 → 接收端：payload 為空，表示資料送完。
接收端 → 傳送端：payload 1 byte，0＝解碼與寫檔成功、其他＝失敗。傳送端收到 0 才以結束碼 0 結束。

## 4. Huffman 區塊格式（`huff_encode` 的輸出）

本專題設計之 Huffman 區塊採用**「自帶 Codebook」**架構，不依賴外部資訊即可獨立解碼。所有整數欄位皆採用 **Big-endian** 儲存。位元流 (Bitstream) 的打包順序為 **從最高位元 (MSB) 填至最低位元 (LSB)**。

### 4.1 符號切分與標記
- `SYM_BYTE (0x00)`：以單一 byte (0-255) 為單位。
- `SYM_CHAR (0x01)`：以 UTF-8 Code Point 為單位。
- `SYM_S16 (0x02)`：解析 WAV 檔的 `data` chunk，以 16-bit PCM sample (2 bytes, little-endian) 為單位。

### 4.2 區塊結構總覽
區塊由四個部分依序組成：**共通標頭** + **WAV 額外資料 (僅 S16)** + **Codebook** + **Bitstream**。

#### A. 共通標頭 (13 bytes)
| 偏移量 | 欄位名稱 | bytes | 說明 |
| :--- | :--- | :--- | :--- |
| `0` | `sym_type` | 1 | `0x00` (BYTE), `0x01` (CHAR), 或 `0x02` (S16) |
| `1` | `orig_len` | 4 | 解壓縮還原後的原始資料總長度 (bytes) |
| `5` | `sym_count` | 4 | 壓縮前的總符號數量 (N) |
| `9` | `unique_syms`| 4 | 唯一出現的符號種類數量 (K) |

#### B. WAV 額外資料區塊 (僅當 sym_type == 0x02 時存在)
WAV 的檔頭與非樣本資料必須原封不動夾帶。若 `data` chunk 長度為奇數，最後無法湊成 sample 的 1 byte 將被歸入 `tail_len` 區塊保留。
| 欄位名稱 | bytes | 說明 |
| :--- | :--- | :--- |
| `total_hdr` | 4 | `data` chunk 之前的全部位元組數 |
| `tail_len` | 4 | `data` chunk 之後的全部位元組數 (含奇數長度截斷的 1 byte) |
| `hdr_bytes` | `total_hdr` | 原始 WAV 檔頭資料 |
| `tail_bytes`| `tail_len` | 原始 WAV 結尾資料 |

#### C. Codebook 區塊 (長度 = K × 單一紀錄大小)
連續出現 K 次的編碼紀錄。每筆紀錄包含：
1. **符號值**：BYTE 佔 1 byte；S16 佔 2 bytes；CHAR 佔 4 bytes。
2. **編碼長度**：1 byte，該符號對應之 Huffman Code 的 bit 數。
3. **編碼值**：4 bytes，該符號的二進位碼，對齊右側 LSB（高位補 0）。

#### D. Bitstream (壓縮位元流)
- **位元順序**：每個 byte 從 `bit 7` (MSB) 填寫至 `bit 0` (LSB)。
- **位元對齊與補零**：最後一個 byte 若未填滿，剩餘的低位元將自動補 `0`。解碼端透過 `sym_count` 判斷終止條件，不會多讀到補零產生的無效符號。
- **平手規則**：本實作直接儲存 Codebook 值，解碼時依據傳來的 Code 長度與數值精確比對，不需依賴建樹時的平手規則。

### 4.3 邊界狀況設計
- **空輸入 (0 bytes)**：僅回傳 13 bytes 的共通標頭，`orig_len`、`sym_count`、`unique_syms` 皆為 0。無 Codebook 與 Bitstream。
- **僅一種符號**：`unique_syms` 為 1。該符號長度設為 `1`，編碼值為 `0`。Bitstream 為全 0。

## 5. 錯誤處理

| 情況 | 我們的行為 |
|---|---|
| length 為 0 或超過上限 | 丟出 `TL_ERR_PROTO` 關閉連線，程式結束碼非 0 |
| 不認得的 type、frame 順序不對 | 丟出 `TL_ERR_PROTO` 關閉連線，程式結束碼非 0 |
| 傳到一半斷線 | 不留下輸出檔（接收端先將副檔名標為 `.part`，接收成功才改名為正式檔名），結束碼非 0 |
| Huffman 區塊損壞 | 檢查標頭欄位合理性、`unique_syms` 是否超出該模式極限、Codebook 大小是否超出封包剩餘長度，若損壞回傳 `TL_ERR_DATA` |
| 非法 UTF-8 | 丟棄該則訊息並顯示系統訊息，不導致主程式崩潰 |

## 6. 已知限制

1. **記憶體用量**：WAV 雙聲道高取樣率或極大文字檔，可能導致 `SYM_CHAR` 與 `SYM_S16` 模式下 `calloc` 配置出極大記憶體。若超出系統上限，會回傳 `TL_ERR_NOMEM`。
2. **WAV 格式支援**：本工具的 `SYM_S16` 僅支援 16-bit PCM (AudioFormat = 1)。8-bit PCM 或非 WAV 檔案強制作為 `SYM_BYTE` 處理。
3. **位元流長度限制**：Codebook 的編碼值最大只支援 32 bits。若分佈極度偏斜導致 Huffman 樹深度超過 32，可能會發生溢位。