# TextLink 測試與驗證紀錄（V）

## 1. 測試範圍與環境

本文件記錄 TextLink 專案由 V（Verification，測試驗證）負責的單元測試、異常輸入測試、檔案完整性驗證、效能量測與測試重現作業。

### 1.1 測試環境

- 程式語言：C99
- 建置工具：GCC / MinGW、`mingw32-make`
- 作業系統：Windows
- 操作介面：Windows PowerShell
- 正式效能量測網路：TCP Loopback，`127.0.0.1:5000`
- 正式量測次數：四種檔案 × RAW/HUFF 兩種模式 × 各五次，共 40 次
- 統計方法：每組五次量測取中位數（Median）

本次正式效能量測使用同一台 Windows 電腦進行，所得結果不代表兩台實體電腦之間的網路效能。

團隊曾進行雙機 TCP 連線測試，但完整的雙機測試環境、IP、操作紀錄與效能數據仍需由相關組員補充。雙機功能測試與本文件的 40 次 localhost 效能量測應分開看待。

### 1.2 單元測試結果

2026-10-09 執行 `mingw32-make test`，結果如下：

| 結果 | 數量 |
|---|---:|
| PASS | 114 |
| FAIL | 0 |
| TODO | 0 |

原先為 104 PASS，本次增加十項邊界及異常輸入測試，結果達到 114 PASS。

代表性 Git commits：

- `dc3a506`：新增 V 邊界與異常輸入測試
- `5f8d19f`：修正 Windows MinGW IPv4 位址轉換相容性
- `1441af1`：更新 114 PASS 測試驗證報告
- `7852c4b`：記錄並重現 TCP 異常封包測試
- `7395bc5`：修正舊版 MinGW 終端機旗標相容性
- `9b7a139`：修正 PowerShell Benchmark stderr 處理

## 2. 測試項目與驗證結果

| 測試類別 | 驗證方式 | 結果 |
|---|---|---|
| Frame、UTF-8、Huffman | `mingw32-make test` | 114 PASS、0 FAIL、0 TODO |
| RAW/HUFF 聊天 | 本機 TCP server/client 互傳文字 | 已完成本機功能測試 |
| TXT/WAV 檔案傳輸 | `send` / `recv`，RAW/HUFF | 已進行功能及正式效能量測 |
| 檔案完整性 | Windows `fc /b` 逐 byte 比對 | 已驗證測試檔案可正確還原 |
| 邊界與壞輸入 | `tests/test_codec.c` | 新增十項測試並通過 |
| TCP 異常輸入 | `tests/test_tcp_malformed.ps1` | TCP-01～TCP-04 已執行 |
| Benchmark | 四檔案 × 兩模式 × 五次 | 40 筆 localhost 量測完成 |
| 雙機 TCP 連線 | 團隊跨電腦實際操作 | 曾執行，詳細證據待補 |

單元測試通過不代表已完成所有整合測試、記憶體安全檢查或網路壓力測試。

### 2.1 V 新增的十項邊界與異常輸入測試

本次擴充 `tests/test_codec.c` 中的 `test_v_boundaries()`，測試方向包含：

**Frame**

- 空 Payload
- 最大允許長度
- 超過允許上限的長度

**UTF-8**

- 包含 NUL（`0x00`）的輸入
- NUL 後仍須檢查的非法位元組
- 截斷的四位元組 UTF-8 序列

**Huffman**

- 截斷的固定標頭前綴
- `max_out = 0` 時拒絕產生非空輸出

完整測試輸入、斷言與程式實作請參閱 `tests/test_codec.c` 及 Git commit `dc3a506`。

### 2.2 Windows 編譯相容性驗證

在重新 Clone 專案後，曾遇到以下 Windows MinGW 相容性問題：

1. `inet_ntop` / `inet_pton` 未宣告及連結失敗。
2. `ENABLE_VIRTUAL_TERMINAL_PROCESSING` 在部分 MinGW 環境中未定義。

修正後已重新編譯與執行測試，取得 114 PASS、0 FAIL、0 TODO。

相關修改位於 `src/net.c`、`src/main.c`，可參閱相應 Git commit。

## 3. TCP 異常封包與中途斷線測試

### 3.1 測試方式

測試環境：

- Windows PowerShell
- 接收端：`textlink.exe recv 5000 out`
- 網路：`127.0.0.1:5000`
- 測試腳本：`tests/test_tcp_malformed.ps1`

接收端啟動：

```powershell
.\textlink.exe recv 5000 out
```

另一個終端機執行：

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\test_tcp_malformed.ps1 -Case TCP-01
```

可將 `TCP-01` 改為 `TCP-02`、`TCP-03` 或 `TCP-04`。

每個案例執行前，都必須重新啟動接收端。

### 3.2 測試結果

| 編號 | 異常輸入 | 接收端觀察結果 | 判定 |
|---|---|---|---|
| TCP-01 | 僅送出 2 bytes Header 後斷線 | 接收失敗，未產生輸出檔，顯示對方已關閉連線 | 符合預期 |
| TCP-02 | Header 宣告 Length=11，但 Payload 只送出 3/10 bytes | 接收失敗，未產生輸出檔，顯示對方已關閉連線 | 安全失敗，細部階段待確認 |
| TCP-03 | 宣告 Length=`0x01000001`，超過 16 MiB 上限 | 拒絕不合規格封包，未產生輸出檔 | 符合預期 |
| TCP-04 | 合法 Length=1，未知 Type=`0xFF` | 拒絕不合規格封包，未產生輸出檔 | 符合預期 |

以上結果由人工觀察接收端終端機取得。

測試腳本成功送出異常資料，不代表接收端測試自動通過；必須另外檢查接收端反應。

**測試限制：**

- 所有案例均使用本機 Loopback。
- TCP-02 尚未獨立證明錯誤確實發生於 Payload 讀取階段。
- 本次未搭配 AddressSanitizer 或其他記憶體分析工具。
- 此四項 TCP 測試不包含在 114 項離線單元測試的數量內。

### TCP-05／TCP-06：TCP 半包與黏包測試

**測試環境：** Windows PowerShell、TCP Loopback `127.0.0.1:5000`。

使用 `tests/test_tcp_stream.ps1` 產生符合 TextLink 協定的 `FILE_BEGIN`、`FILE_DATA`、`FILE_END` Frame，測試接收端對分段與連續 TCP 串流的處理能力。

| 測試編號 | 測試方式 | 測試結果 |
|---|---|---|
| TCP-05 | 將 5 個 Frame、合計 4,155 bytes 拆成 385 次 TCP 寫入 | 成功接收 4,096 bytes、收到成功 ACK，`fc /b` 無差異 |
| TCP-06 | 將 5 個 Frame、合計 4,155 bytes 合併為一次 TCP 寫入 | 成功接收 4,096 bytes、收到成功 ACK，`fc /b` 無差異 |

**重現指令：**

接收端（每次重新啟動）：

```powershell
.\textlink.exe recv 5000 out
```

測試端：

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\test_tcp_stream.ps1 -Case TCP-05
powershell -ExecutionPolicy Bypass -File .\tests\test_tcp_stream.ps1 -Case TCP-06
```

每個案例分別執行，不可在同一個接收端程序中連續執行。

**檔案完整性驗證：**

```powershell
cmd /c fc /b "$env:TEMP\tcp_stream_05.bin" out\tcp_stream_05.bin
cmd /c fc /b "$env:TEMP\tcp_stream_06.bin" out\tcp_stream_06.bin
```

兩項測試均顯示「FC: 找不到相異處」。

**測試結論：** TextLink 接收端能正確處理本次分段寫入與連續 Frame 串流，並完整還原資料。此測試未直接記錄底層 `recv()` 的實際分段邊界，因此不宣稱涵蓋所有 TCP 封包切分情況。

## Huffman 特殊檔案與 BYTE 自動回退整合測試（2026-10-09）

### 測試目的與環境

驗證 TextLink 在 Huffman 模式下處理特殊檔案、符號邊界及格式不相容情況的能力，包含自動回退至 `SYM_BYTE`、完成傳輸以及逐 byte 還原。

測試環境為 Windows PowerShell、TCP Loopback `127.0.0.1:5000`。每次測試均重新啟動接收端，使用 `textlink.exe send ... --huff` 傳送，並透過 `fc /b` 比對原始與接收檔案。

### 測試結果

| 測試檔案 | 原始大小（bytes） | 傳送端 sym | wire_bytes | ratio | 結果 |
|---|---:|---|---:|---:|---|
| `invalid_utf8.txt` | 6 | byte | 99 | 16.5000 | PASS |
| `audio_8bit.wav` | 8,044 | byte | 2,612 | 0.3247 | PASS |
| `single_symbol.bin` | 4,096 | byte | 580 | 0.1416 | PASS |
| `empty.bin` | 0 | byte | 54 | 0.0000* | PASS |
| `all_256_bytes.bin` | 4,096 | byte | 5,694 | 1.3901 | PASS |
| `utf8_bom_crlf.txt` | 21 | char | 130 | 6.1905 | PASS |
| `wav_extra_chunk.wav` | 8,056 | s16 | 635 | 0.0788 | PASS |

*空檔案的 `file_bytes = 0`，因此 `wire_bytes / file_bytes` 在數學上未定義。程式顯示的 `ratio=0.0000` 是零分母的特殊處理值，不代表實際壓縮率為 0%。*

### 自動回退驗證

- `invalid_utf8.txt`：副檔名為 `.txt`，但包含非法 UTF-8，程式由原本的 CHAR 選擇回退至 BYTE。
- `audio_8bit.wav`：檔案為 8-bit PCM，不符合 16-bit sample 編碼條件，因此回退至 BYTE。
- `utf8_bom_crlf.txt`：包含 BOM、CRLF、1～4 bytes UTF-8 字元，正確使用 CHAR。
- `wav_extra_chunk.wav`：包含額外 LIST Chunk 的 16-bit PCM WAV，正確使用 S16。

### 檔案完整性

七個案例的接收端皆成功存檔，且使用 `fc /b` 比對後顯示「找不到相異處」。

### 重現方式

接收端：

```powershell
.\textlink.exe recv 5000 out
```

傳送端（以 8-bit WAV 為例）：

```powershell
.\textlink.exe send 127.0.0.1 5000 tests\edge_files\audio_8bit.wav --huff
```

檔案比對：

```powershell
cmd /c fc /b tests\edge_files\audio_8bit.wav out\audio_8bit.wav
```

其餘六個案例只需替換檔名，每次傳輸前重新啟動接收端。

測試資料位於 `tests/edge_files/`。

### 測試結論

本次七種特殊檔案均完成 Huffman 傳輸與逐 byte 還原。非法 UTF-8 TXT 與 8-bit WAV 也成功驗證 `SYM_BYTE` 自動回退。

本測試屬於 localhost 整合測試，不等同於記憶體安全性檢測，也不代表所有可能的惡意輸入均已涵蓋。

### WAV 奇數 data chunk 長度測試（2026-10-10）

**測試目的：** 驗證 WAV 的 `data` chunk 長度為奇數時，Huffman S16 模式能否保留最後不足一個 16-bit sample 的 byte，並在解碼後完整還原原始檔案。

**測試環境：** Windows PowerShell、TCP Loopback `127.0.0.1:5000`。

| 驗證項目 | 實際結果 |
|---|---|
| 測試檔案 | `tests/edge_files/audio_odd.wav` |
| 原始大小 | 245 bytes |
| WAV `data` chunk 宣告長度 | 201 bytes |
| 傳輸模式 | HUFF |
| 實際 Huffman 符號模式 | `sym=s16` |
| Huffman 編碼資料 | 86 bytes |
| 實際上線資料量 | 131 bytes |
| 傳輸比例 | 0.5347（53.47%） |
| 接收端解碼及存檔 | 成功，245 bytes |
| `fc /b` 逐 byte 比對 | 找不到相異處 |
| **整體測試結果** | **PASS** |

**測試結論：**

本次在 localhost 環境下，使用 `data` chunk 宣告長度為 201 bytes 的 WAV 檔案進行 Huffman S16 傳輸。傳送端成功編碼，接收端完成解碼及存檔，最終使用 `fc /b` 確認原始與還原檔案逐 byte 完全相同。

結果證實，本次測試的奇數長度 WAV 可透過 S16 模式無損還原，包含最後不足一個 16-bit sample 的剩餘 byte。

本項為單一特殊檔案的 localhost 整合測試，不代表所有 WAV 格式或 RIFF 邊界情況皆已涵蓋。

**重現指令：**

接收端：

```powershell
.\textlink.exe recv 5000 out
```

傳送端：

```powershell
.\textlink.exe send 127.0.0.1 5000 tests\edge_files\audio_odd.wav --huff
```

檔案完整性比對：

```powershell
cmd /c fc /b tests\edge_files\audio_odd.wav out\audio_odd.wav
```

**通過條件：** 傳送端顯示 `mode=huff sym=s16`，接收端成功解碼及存檔，且 `fc /b` 顯示找不到相異處。

**測試備註：** 目前已確認 C 編碼器可以對此測試檔案產生 86 bytes 的 Huffman 區塊，但尚未取得接收端還原及檔案比對成功的結果，因此不將整體測試標記為 PASS。

### TCP-07：損壞 Huffman Codebook 整合測試（2026-10-09）

**測試目的：** 驗證接收端遇到非法 Huffman Codebook 時，能拒絕解碼、不產生錯誤輸出檔案，並回傳失敗狀態與非零結束碼。

**測試環境：** Windows PowerShell、TCP Loopback `127.0.0.1:5000`。

**測試工具：** `tests/test_tcp_bad_codebook.py`

測試腳本產生一份包含無效 Codebook 的 Huffman 資料，其中符號 `A` 的編碼長度被設定為 0，並透過 `FILE_BEGIN`、`FILE_DATA`、`FILE_END` 傳送至 TextLink 接收端。

| 驗證項目 | 實際結果 |
|---|---|
| Huffman 資料大小 | 20 bytes |
| 宣告原始大小 | 1 byte |
| 接收端錯誤 | 資料內容不合法 |
| 回覆 Frame Type | `0x12`（FILE_END） |
| 回覆 Payload | `01`（失敗） |
| 輸出檔案 | 未產生 |
| 接收端結束碼 | 1 |
| 判定 | PASS |

**重現方式：**

接收端：

```powershell
.\textlink.exe recv 5000 out
```

測試端：

```powershell
py .\tests\test_tcp_bad_codebook.py
```

接收端執行完成後，可使用 `$LASTEXITCODE` 確認結束碼。

**結論：** 接收端正確拒絕本次損壞的 Huffman Codebook，回覆失敗 ACK（`FILE_END`、Payload=`01`），沒有產生輸出檔案，並以結束碼 1 正常結束。測試未發現程式崩潰。

本案例屬於協定及解碼錯誤處理的整合驗證，不等同於使用記憶體分析工具證明完全沒有越界讀寫。

### STATS 格式與程式結束碼驗證（2026-10-09）

**測試環境：** Windows PowerShell、TCP Loopback `127.0.0.1:5000`。

**測試檔案：** `benchmark_real/real_chinese.txt`，原始大小為 1,200,335 bytes。

本次分別執行 RAW 與 HUFF 傳輸，確認傳送端、接收端的 `STATS` 欄位、資料一致性及成功結束碼；另使用 TCP-07 損壞 Huffman Codebook 測試驗證失敗結束碼。

| 驗證項目 | RAW | HUFF |
|---|---:|---:|
| `file_bytes` | 1,200,335 | 1,200,335 |
| `wire_bytes` | 1,200,473 | 388,786 |
| `ratio` | 1.0001 | 0.3239 |
| 傳送端 `sym` | none | char |
| 傳送端 `encode_ms` | 0.0 | 28.8 |
| 傳送端 `send_ms` | 37.9 | 11.3 |
| 傳送端 `total_ms` | 41.0 | 52.8 |
| 接收端 `decode_ms` | 0.0 | 9.4 |
| 接收端 `total_ms` | 40.3 | 23.3 |
| 傳送端結束碼 | 0 | 0 |
| 接收端結束碼 | 0 | 0 |
| 判定 | PASS | PASS |

本表為額外功能驗證的單次測試結果，不納入正式 40 次 Benchmark 的中位數統計。

**失敗結束碼驗證：**

在 TCP-07 測試中，接收端成功辨識損壞的 Huffman Codebook，回覆 `FILE_END` 失敗狀態（Payload `01`），沒有產生輸出檔案，並以結束碼 1 結束。

**結論：** 本次 RAW/HUFF 傳輸均正常輸出指定的 STATS 欄位，成功時兩端回傳結束碼 0；故意提供損壞 Huffman Codebook 時，接收端回傳失敗狀態及非零結束碼。資料比例與 `wire_bytes / file_bytes` 的計算一致。

### 乾淨 Clone 建置與單元測試驗證（2026-10-09）

本次在獨立資料夾 `textlink-clean-test` 重新 Clone GitHub Repository，確認下載後不存在既有編譯產物，並依照 README 的 Windows PowerShell 建置方式執行測試。

**驗證項目與結果：**

| 項目 | 結果 |
|---|---|
| Git Clone | 成功 |
| Commit SHA | 與驗證指定版本一致 |
| 舊編譯產物 | 無 |
| `mingw32-make` | 成功，無編譯警告 |
| `mingw32-make test` | PASS 114、FAIL 0、TODO 0 |
| 測試結束碼 | 0 |

**結論：** 專案可在本次測試使用的 Windows / MinGW 環境中，從乾淨的 Git Clone 重新建置並通過全部單元測試。

驗證 Commit SHA：`（填入實際 git rev-parse HEAD 的完整 SHA）`

本測試證明目前環境中的建置可重現性，不代表已驗證所有作業系統及編譯器版本。

## 4. 正式 Benchmark 測試資料

### 4.1 資料集

本次正式效能量測使用以下四種檔案：

| 測試檔案 | 類別 | 大小（bytes） |
|---|---|---:|
| `real_chinese.txt` | 中文為主的合成自然語言文章 | 1,200,335 |
| `real_english.txt` | 英文為主的合成自然語言文章 | 1,258,581 |
| `audio_music_20s.wav` | 音樂 WAV，16-bit PCM | 3,528,044 |
| `audio_noise.wav` | 自選高熵雜訊 WAV | 1,120,044 |

資料存放位置：`benchmark_real/`。

四種檔案都超過 1,000,000 bytes，符合至少 1 MB 的檔案大小要求。

中文與英文檔案是人工合成的自然語言測試語料，並非出版文章。音樂 WAV 為 44.1 kHz、雙聲道、16-bit PCM，使用 20 秒音樂片段。雜訊 WAV 為 8 kHz、單聲道、16-bit PCM。

### 4.2 測試設計

每個檔案分別進行：

- RAW：五次
- HUFF：五次

總計：

4 檔案 × 2 模式 × 5 次 = **40 次正式量測**。

每次測試重新啟動接收端，於傳送端執行 `run_benchmark.ps1`，收集 `STATS`，並將兩端數據整理至正式 CSV。

測試環境統一使用 `127.0.0.1:5000`。

### 4.3 正式量測重現方式

接收端：

```powershell
.\textlink.exe recv 5000 out
```

傳送端 RAW 範例：

```powershell
powershell -ExecutionPolicy Bypass -File .\benchmarks\run_benchmark.ps1 -File benchmark_real\real_chinese.txt -Mode raw -Trial 1
```

傳送端 HUFF 範例：

```powershell
powershell -ExecutionPolicy Bypass -File .\benchmarks\run_benchmark.ps1 -File benchmark_real\real_chinese.txt -Mode huff -Trial 1
```

每次測試均需重新啟動接收端，並依序執行 Trial 1～5。

測試完成後可使用：

```powershell
cmd /c fc /b benchmark_real\real_chinese.txt out\real_chinese.txt
```

確認接收檔案與原始檔案逐 byte 相同。

`run_benchmark.ps1` 負責執行單次傳送端測試並保存傳送端輸出。接收端的 `STATS` 由人工觀察、記錄並併入正式數據。

## 5. 效能指標定義

### 5.1 傳輸量與壓縮比例

| 指標 | 定義 |
|---|---|
| `file_bytes` | 原始檔案大小（bytes） |
| `wire_bytes` | TextLink STATS 回報的上線傳輸量（bytes），包含協定開銷 |
| `ratio` | `wire_bytes / file_bytes` |
| `compressed_pct` | `ratio × 100` |
| `saved_pct` | `(1 - ratio) × 100` |

當 `ratio` 小於 1，代表傳輸量減少。

當 `ratio` 大於 1，代表壓縮後傳輸量反而增加。

例如：

- `ratio=0.3239`：上線資料量為原始檔案的約 32.39%。
- `ratio=1.4062`：上線資料量為原始檔案的約 140.62%，增加約 40.62%。

### 5.2 編解碼與總耗時

| 指標 | 定義 |
|---|---|
| `encode_ms` | 傳送端編碼耗時 |
| `send_ms` | 傳送端傳送耗時 |
| `decode_ms` | 接收端解碼耗時 |
| `sender_total_ms` | 傳送端 STATS 回報的總耗時 |
| `receiver_total_ms` | 接收端 STATS 回報的總耗時 |

傳送端與接收端的 `total_ms` 為個別計時結果，不可直接相加視為精確的端對端延遲。

### 5.3 有效傳輸速率

以原始檔案大小及各端總耗時換算有效傳輸速率，單位為十進位 MB/s。

傳送端：

`sender_MBps = file_bytes / (sender_total_ms × 1000)`

接收端：

`receiver_MBps = file_bytes / (receiver_total_ms × 1000)`

此速率包含程式處理時間的影響，不能直接視為實際網路鏈路頻寬。

### 5.4 中位數統計

每個檔案與模式分別執行五次，對每個指標取中位數。

注意：不同欄位的中位數不一定來自同一個 Trial，因此不應將各欄中位數當成單次傳輸的完整計時拆解。

## 6. 正式 Benchmark 結果

### 6.1 五次量測中位數

以下八組結果均使用正式四種測試檔案，不包含先前探索性資料的測試。

| 檔案 | 模式 | wire_bytes | encode_ms | send_ms | decode_ms | 傳送端 total_ms | 接收端 total_ms |
|---|---|---:|---:|---:|---:|---:|---:|
| 中文文章 | RAW | 1,200,473 | 0.0 | 24.4 | 0.0 | 35.0 | 33.8 |
| 中文文章 | HUFF | 388,786 | 27.9 | 3.7 | 8.9 | 51.8 | 22.6 |
| 英文文章 | RAW | 1,258,724 | 0.0 | 24.3 | 0.0 | 37.1 | 35.9 |
| 英文文章 | HUFF | 679,187 | 42.6 | 9.4 | 20.5 | 85.0 | 40.9 |
| 音樂 WAV | RAW | 3,528,360 | 0.0 | 76.1 | 0.0 | 92.8 | 91.3 |
| 音樂 WAV | HUFF | 3,828,268 | 201.1 | 85.7 | 82.4 | 389.1 | 184.1 |
| 雜訊 WAV | RAW | 1,120,176 | 0.0 | 21.8 | 0.0 | 34.0 | 33.0 |
| 雜訊 WAV | HUFF | 1,574,998 | 82.4 | 35.8 | 34.1 | 162.5 | 79.3 |

時間單位：ms。

### 6.2 RAW 與 HUFF 傳輸量比較

| 檔案 | RAW wire_bytes | HUFF wire_bytes | HUFF 傳輸比例 | HUFF 相對原始檔案的節省比例 |
|---|---:|---:|---:|---:|
| 中文文章 | 1,200,473 | 388,786 | 32.39% | 67.61% |
| 英文文章 | 1,258,724 | 679,187 | 53.96% | 46.04% |
| 音樂 WAV | 3,528,360 | 3,828,268 | 108.51% | -8.51% |
| 雜訊 WAV | 1,120,176 | 1,574,998 | 140.62% | -40.62% |

節省比例的分母為原始檔案 `file_bytes`，並非 RAW 的 `wire_bytes`。

### 6.3 RAW 與 HUFF 傳送端總耗時比較

| 檔案 | RAW 中位數 | HUFF 中位數 | HUFF 比 RAW 增加 |
|---|---:|---:|---:|
| 中文文章 | 35.0 ms | 51.8 ms | 16.8 ms |
| 英文文章 | 37.1 ms | 85.0 ms | 47.9 ms |
| 音樂 WAV | 92.8 ms | 389.1 ms | 296.3 ms |
| 雜訊 WAV | 34.0 ms | 162.5 ms | 128.5 ms |

四種正式測試檔案中，RAW 的傳送端總耗時中位數都低於 HUFF。

### 6.4 有效傳輸速率

以下使用原始檔案大小及傳送端 `total_ms` 中位數換算。

| 檔案 | RAW (MB/s) | HUFF (MB/s) |
|---|---:|---:|
| 中文文章 | 34.30 | 23.17 |
| 英文文章 | 33.92 | 14.81 |
| 音樂 WAV | 38.02 | 9.07 |
| 雜訊 WAV | 32.94 | 6.89 |

本次 localhost 實驗中，四種檔案的 RAW 有效速率都高於 HUFF。

## 7. 實驗分析與討論

### 7.1 中文文章

中文文章在 Huffman 模式下使用 `SYM_CHAR`，上線傳輸量為原始檔案的約 32.39%，減少約 67.61%。

然而，傳送端總耗時中位數從 RAW 的 35.0 ms 增加到 HUFF 的 51.8 ms。

顯示 Huffman 雖然有效減少資料量，但在本機高速 Loopback 環境中，額外編碼成本仍可能高於節省的傳送時間。

### 7.2 英文文章

英文文章也使用 `SYM_CHAR`。

Huffman 將傳輸量降至約 53.96%，但傳送端總耗時中位數由 37.1 ms 增加到 85.0 ms。

文字資料的可壓縮程度與符號出現頻率、字元分布及 Codebook 開銷有關，不能僅以中文或英文判斷壓縮效率。

### 7.3 音樂 WAV

音樂檔使用 `SYM_S16`，以 16-bit PCM sample 作為 Huffman 符號。

HUFF 傳輸比例約 108.51%，代表傳輸量反而增加約 8.51%。

傳送端總耗時中位數也從 92.8 ms 增加到 389.1 ms。

可能原因包含實際 sample 值種類較多、頻率分布較分散，以及 Codebook 等額外開銷。單靠壓縮比例及 Histogram，尚不能確認各種成本分別占多少。

### 7.4 雜訊 WAV

雜訊 WAV 的 HUFF 傳輸比例約 140.62%，顯示資料量增加約 40.62%。

傳送端總耗時中位數從 RAW 的 34.0 ms 增加至 HUFF 的 162.5 ms。

此結果符合高熵資料通常較難使用 Huffman 有效壓縮的預期，但實際膨脹原因仍須結合 Codebook 大小與編碼資料長度進一步分析。

### 7.5 實驗限制

- 正式 40 次效能量測均使用 localhost。
- 目前沒有將雙機功能測試誤列為雙機效能量測。
- 各端 `total_ms` 不可直接相加視為完整端對端時間。
- 測試檔案的資料分布會影響 Huffman 表現。
- 正式中文與英文檔案為人工合成語料，不能直接代表所有真實出版文章。
- 未執行記憶體分析及網路壓力測試。
- 量測結果會受到電腦負載及執行環境影響。

## 8. 字元機率與 WAV Histogram 分析

除傳輸效能外，本次另外分析兩種文字檔的字元出現機率，以及音樂 WAV 的 PCM sample 分布。

### 8.1 文字字元統計

使用 `benchmarks/char_frequency.py` 讀取 UTF-8 文字檔並統計字元頻率。

| 項目 | 中文文章 | 英文文章 |
|---|---:|---:|
| 總字元數 | 403,185 | 1,256,253 |
| 不同字元數 | 373 | 57 |
| 統計圖 | 前 30 字元機率 | 前 30 字元機率 |

對應原始資料：

- `benchmarks/figures/real_chinese_top30.csv`
- `benchmarks/figures/real_english_top30.csv`

字元機率定義為：

`P(c) = 該字元出現次數 / 總字元數`

本次統計以 Unicode 字元為單位，而非 UTF-8 bytes。

### 8.2 音樂 WAV Sample Histogram

使用 `benchmarks/wav_histogram.py` 讀取 `audio_music_20s.wav`。

音訊參數：

- 取樣率：44,100 Hz
- 聲道：2
- Sample Width：16-bit
- 長度：20 秒
- 總 Frames：882,000
- 總 Samples：1,764,000

將 PCM sample 值範圍 -32768 至 32767 分成 64 個等寬區間，統計各區間的 sample 數量。

輸出資料：

`benchmarks/figures/audio_music_histogram.csv`

Histogram 用來觀察 sample 值的分布情況，但因每個區間包含多個實際取樣值，不能直接代替 Huffman 精確符號頻率或 Codebook 大小分析。

## 9. 正式數據與重現檔案

| 檔案 | 用途 |
|---|---|
| `benchmarks/formal_raw_40.csv` | 40 次正式原始量測紀錄 |
| `benchmarks/formal_medians.csv` | 8 組中位數統計 |
| `benchmarks/formal_benchmark_40.xlsx` | 正式原始數據、統計與比較圖表 |
| `benchmarks/run_benchmark.ps1` | 單次傳送端量測與 Log 保存 |
| `benchmarks/char_frequency.py` | 文字字元頻率分析 |
| `benchmarks/wav_histogram.py` | 音樂 WAV Sample Histogram |
| `tests/test_tcp_malformed.ps1` | TCP 異常封包重現測試 |
| `benchmark_real/` | 正式四種測試檔案 |

正式 CSV 由實際傳送端及接收端 STATS 整理而成。原始量測的 `ratio` 在程式輸出中只顯示四位小數，正式 CSV 可依 `wire_bytes / file_bytes` 計算較精確比例。

## 10. 歷史補充：量測腳本重現驗證（Trial 99）

本節為正式驗收資料確立前執行的額外流程測試，**不納入新的 40 筆正式量測數據**。

### 10.1 測試目的

確認 `benchmarks/run_benchmark.ps1` 可以執行 RAW 與 HUFF 傳輸、保存傳送端 STATS，並搭配檔案比對驗證內容一致。

### 10.2 測試設定

- 測試檔案：`benchmark_files/text_repeat.txt`
- 原始大小：1,740,000 bytes
- 測試編號：Trial 99
- 網路：`127.0.0.1:5000`
- 模式：RAW、HUFF

### 10.3 歷史資料保存說明

本次 Trial 99 原使用 `benchmark_files/text_repeat.txt` 進行 RAW 與 HUFF 測試。

由於後續正式驗收資料集已改為 `benchmark_real/`，原 `benchmark_files/` 已從 Repository 最新版本移除，因此 Trial 99 僅作為歷史測試紀錄，不能直接使用原指令重現。

目前若要重新執行效能量測，請依第 4.3 節使用正式測試檔案與 `benchmarks/run_benchmark.ps1`。

以下保留當時量測結果，供測試流程及 PowerShell 相容性修正的歷史追溯。

### 10.4 歷史結果

| 項目 | RAW | HUFF |
|---|---:|---:|
| file_bytes | 1,740,000 | 1,740,000 |
| wire_bytes | 1,740,177 | 847,944 |
| ratio | 1.0001 | 0.4873 |
| encode_ms | 0.0 ms | 24.4 ms |
| send_ms | 29.0 ms | 14.3 ms |
| sender total_ms | 47.7 ms | 69.9 ms |
| 檔案比對 | PASS | PASS |

接收端 HUFF 解碼時間為 14.5 ms。

測試期間曾因 PowerShell 對原生程式 stderr 輸出的處理而遇到 `NativeCommandError`。修正腳本後，RAW/HUFF 均能正常保存 Log。

本項僅用來證明量測腳本的重現流程，不代表正式資料集的壓縮表現。

## 11. 提交前驗證清單

- [x] 新增十項 V 邊界與異常輸入測試。
- [x] 取得 114 PASS、0 FAIL、0 TODO。
- [x] 提交相關程式與相容性修正至 Git。
- [x] 執行 TCP-01～TCP-04 異常封包測試。
- [x] 完成正式四種檔案的 RAW/HUFF 各五次量測。
- [x] 整理 40 筆原始數據及八組中位數。
- [x] 完成中文字元、英文字元頻率及 WAV Histogram 統計。
- [x] 使用 `fc /b` 進行傳輸檔案完整性驗證。
- [x] 建立正式 Benchmark CSV 與 Excel。
- [ ] 保存完整 `mingw32-make test` 輸出、GCC 版本及對應 Commit SHA。
- [ ] 補充兩台實體電腦的測試環境與證據。
- [ ] 確認三份正式統計檔案、圖表及相關文件均已推送 GitHub。
- [ ] 將正式 Benchmark 圖表與分析整合至 `slides.pdf`。
- [ ] 與其他組員核對最終分工、文件與口頭展示內容。

---

本文件以已執行的測試、保留的 STATS 及 Git 紀錄為依據。正式效能報告優先使用 `benchmarks/formal_raw_40.csv` 與 `benchmarks/formal_medians.csv`，歷史探索性量測不與正式 40 筆資料混用。