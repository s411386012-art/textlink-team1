# TextLink 介面文件（Team 1 實作版）

> 目標：**別組只看這份文件，就能寫出與我們互通的程式。** 每個欄位寫清楚幾 bytes、什麼順序、
> 位元組順序與位元順序。

組別：Team 1 成員與角色：陳皓祥 (程式實作與架構設計)，陳立哲(程式架構修正) 文件版本／日期：v1.1 修訂草稿 (2026-10-07)

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

傳送端先驗證聊天輸入是否為合法 UTF-8。若合法，使用 `huff_encode(SYM_CHAR)` 進行壓縮，並將完整 Huffman 區塊（Header、Codebook、Bitstream）放入 Payload。

若壓縮後資料量大於原始文字，仍維持 HUFF 模式傳送，不會自動切換 RAW。

若使用者輸入的文字不是合法 UTF-8，聊天傳送端拒絕送出並顯示錯誤訊息。接收端解碼後仍須驗證文字合法性。

BYTE fallback 主要適用於檔案編碼：當文字資料不符合 UTF-8 要求，或 WAV 不符合支援的 16-bit PCM 格式時，可使用 `SYM_BYTE` 保留原始 bytes。

### 0x10 FILE_BEGIN（傳送端 → 接收端）

| 欄位 | bytes | 說明 |
|---|---|---|
| mode | 1 | 0＝raw、1＝huff |
| orig_size | 8 | big-endian；原檔 bytes 數；上限 64 MiB |
| data_size | 8 | big-endian；之後所有 FILE_DATA 的 payload 總 bytes 數；raw 時必須等於 orig_size |
| name | 其餘 | 檔名，不含路徑；接收端只保留 `0-9 A-Z a-z . - _`，其餘換成 `_` |

`FILE_BEGIN` 的固定 Payload 為 17 bytes，之後必須至少包含 1 byte 的檔名。

檔名以 UTF-8 bytes 傳送，不包含 `\0` 結尾；接收端最多讀取前 127 bytes 作為原始檔名，並進行安全字元過濾。傳送端及接收端皆不得將原始檔案路徑直接用於目的地寫入。

`orig_size` 和 `data_size` 都不能超過 64 MiB。RAW 模式還要求兩者相等，不符合條件時拒絕該 `FILE_BEGIN`。

### 0x11 FILE_DATA（傳送端 → 接收端）

傳輸資料的一段，每個最多 65,536 bytes，依序接起來共 data_size bytes。
- raw 模式：直接為原始檔案內容片段。
- huff 模式：為第 4 節 Huffman 區塊的一段（將整個壓縮結果切割發送）。

### 0x12 FILE_END

傳送端 → 接收端：payload 為空，表示資料送完。
接收端 → 傳送端：payload 1 byte，0＝解碼與寫檔成功、其他＝失敗。傳送端收到 0 才以結束碼 0 結束。

**檔案傳輸確認機制**

傳送端送出 `FILE_END` 後，必須等待接收端的確認 Frame。確認 Frame 的 Type 為 `0x12`，Payload 為 1 byte，其中 `0x00` 表示接收、解碼及存檔成功；`0x01` 表示處理失敗。

只有收到 Type 正確、Payload 長度為 1 byte 且值為 `0x00` 的確認 Frame，命令列傳送端才以結束碼 0 回報成功。其他回覆或接收失敗都視為傳輸失敗。

接收端收到異常 Huffman Codebook 時，應拒絕解碼並回報失敗，而非產生錯誤的正式輸出檔案。

**FILE_END 格式與驗證**

依通訊協定設計，傳送端送出的 `FILE_END` Payload 應為空；接收端回覆的 `FILE_END` Payload 則固定為 1 byte，其中 `0x00` 表示成功，`0x01` 表示失敗。

目前命令列接收端已檢查傳送端 `FILE_END` 的 Payload 長度，僅接受空 Payload（0 byte）；若收到非空 Payload，則回報協定錯誤並拒絕存檔。聊天模式也已加入相應檢查，遇到非法 `FILE_END` 時回報失敗，不將該次傳輸視為成功。

2026-10-10 已使用 Python TCP 測試程式分別對 CLI Receiver 與 Chat Server 傳送非法 `FILE_END`（1-byte Payload）。兩者均正確拒絕，且未產生對應的正式輸出檔案。

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

**WAV 邊界與對齊處理**

解析 WAV 時，應以 RIFF chunk 的實際長度欄位定位 `data` 區，而非假設音訊資料固定從第 44 byte 開始。

若音訊資料無法完整切分為 16-bit sample，必須保留剩餘的原始 byte。若 RIFF chunk 含有 padding byte 或後續其他 chunk，也必須確保解碼後仍能逐 byte 還原原始 WAV。

`total_hdr`、`tail_len` 與 sample 資料的實際切分方式，應與編碼器及解碼器的實作一致。

#### C. Codebook 區塊 (長度 = K 筆紀錄大小的總和)
連續出現 K 次的編碼紀錄。每筆紀錄包含：
1. **符號值**：BYTE 佔 1 byte；S16 佔 2 bytes；CHAR 佔 4 bytes。
2. **編碼長度**：1 byte，合法值為 1–64，單位為 bit。
3. **編碼值**：長度 1–32 時佔 **4 bytes**；長度 33–64 時佔 **8 bytes**。整數為 big-endian，編碼對齊右側 LSB，超出編碼長度的高位必須為 0。

例如長度為 3 的編碼 `101` 存成 `00 00 00 05`；長度為 33 的全 1 編碼存成 `00 00 00 01 FF FF FF FF`。
每筆大小 = 符號 bytes + 1 + (碼長 <= 32 ? 4 : 8)，因此不能再假定全部紀錄固定大小。

編碼器按符號值遞增寫出紀錄。解碼器接受任意紀錄順序，但拒絕重複符號、重複碼、前綴衝突及非完整二元樹（單一符號另依 4.3）。CHAR 符號必須為合法 Unicode scalar，不能位於代理區。

**版本相容性**：1–32 bits 的紀錄格式維持 v1.0；33–64 bits 是 v1.1 擴充。測試及展示時兩端請使用同一修訂版本；不保證舊版解碼器能讀長碼。新版只接受規範中的 13-byte 空區塊，拒絕舊實作多出的 8 bytes。

#### D. Bitstream (壓縮位元流)
- **位元順序**：每個 byte 從 `bit 7` (MSB) 填寫至 `bit 0` (LSB)。
- **位元對齊與補零**：最後一個 byte 若未填滿，剩餘的低位元將自動補 `0`。解碼端透過 `sym_count` 判斷終止條件，不會多讀到補零產生的無效符號。
- **平手規則**：本實作直接儲存 Codebook 值，解碼時先依 Code 長度與數值建立前綴樹，再逐 bit 走樹，不需依賴建樹時的平手規則。編碼器使用 min-heap，頻率相同時以節點索引較小者優先；葉節點依符號值建立。

### 4.3 邊界狀況設計
- **空輸入 (0 bytes)**：BYTE／CHAR 僅回傳 13 bytes 的共通標頭，`orig_len`、`sym_count`、`unique_syms` 皆為 0。無 Codebook 與 Bitstream，也不接受多餘 bytes。0-byte 輸入不符合 WAV，S16 回傳 `TL_ERR_DATA` 供傳輸層退回 BYTE。
- **只有檔頭的 WAV**：保留 WAV 前後資料，`orig_len` 非零、`sym_count` 和 `unique_syms` 為 0；沒有 Codebook 與 Bitstream。
- **輸出長度**：在配置記憶體前檢查 `orig_len <= max_out` 且不超過 64 MiB。每次寫入前確認完整字元／sample 都放得下。
- **結尾**：解出指定符號數後，最多只允許 7 個補零位元，拒絕非零 padding 或附加 byte。
- **僅一種符號**：`unique_syms` 為 1。該符號長度設為 `1`，編碼值為 `0`。Bitstream 為全 0。

## 5. 錯誤處理

| 情況 | 我們的行為 |
|---|---|
| length 為 0 或超過上限 | 丟出 `TL_ERR_PROTO` 關閉連線，程式結束碼非 0 |
| 不認得的 type、frame 順序不對 | 丟出 `TL_ERR_PROTO` 關閉連線，程式結束碼非 0 |
| 傳到一半斷線 | 不留下輸出檔（接收端先將副檔名標為 `.part`，接收成功才改名為正式檔名），結束碼非 0 |
| Huffman 區塊損壞 | 檢查長度與符號數一致性、Codebook 紀錄範圍、碼長、符號、前綴樹、輸出空間及 padding；不合法回傳 `TL_ERR_DATA`。失敗時 `*out = NULL`、`*out_len = 0`。這些是結構驗證，並非 checksum，不能保證偵測所有資料位元翻轉 |
| 非法 UTF-8 | 丟棄該則訊息並顯示系統訊息，不導致主程式崩潰 |

## 6. 已知限制

1. **記憶體用量**：WAV 雙聲道高取樣率或極大文字檔，可能導致 `SYM_CHAR` 與 `SYM_S16` 模式下 `calloc` 配置出極大記憶體。若超出系統上限，會回傳 `TL_ERR_NOMEM`。
2. **WAV 格式支援**：本工具的 `SYM_S16` 僅支援 16-bit PCM (AudioFormat = 1)。8-bit PCM 或非 WAV 檔案強制作為 `SYM_BYTE` 處理。
3. **碼長與輸入上限**：編碼使用 `uint64_t`，支援 1–64 bits；遞迴建碼在超出上限前回報錯誤，解碼也拒絕超過 64 的碼長。原始輸入限制為 64 MiB，因此符號頻率總和至多 2^26；正整數頻率的 Huffman 最壞深度受 Fibonacci 型增長限制，在這個資料量下不會需要 64 bits 以上的碼長。

### 6.4 驗證範圍

本專案已完成 114 項 C 單元測試，以及本機 TCP 異常封包、拆包／黏包、Huffman 邊界資料與 RAW/HUFF 無損傳輸測試。

上述結果僅代表已執行的本機驗證，不能取代正式跨實體電腦的互通性測試。

詳細測試方法與結果請參閱 [測試驗證報告](testing.md)，各項驗收狀態請參閱 [驗收檢查表](verification_checklist.md)。