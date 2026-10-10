# TextLink — TCP 聊天與 Huffman 檔案傳輸系統

TextLink 是使用 C99 開發的命令列 TCP 通訊程式，支援 UTF-8 文字聊天、TXT/WAV 檔案傳輸，以及 RAW（不壓縮）與 Huffman（壓縮）兩種傳輸模式。

本專案完成 Frame Header 封裝與解析、UTF-8 合法性驗證、Huffman 編碼與解碼、TCP 聊天及檔案傳輸功能，並提供單元測試、異常輸入測試與可重現的效能量測資料。

## 1. 專案功能

| 功能 | 說明 |
|---|---|
| TCP 文字聊天 | 支援 UTF-8 中文與英文訊息 |
| RAW 模式 | 不壓縮資料，直接傳輸 |
| Huffman 模式 | 使用 Huffman 編碼減少資料量 |
| TXT 檔案傳輸 | 支援以 UTF-8 字元作為 Huffman 符號 |
| WAV 檔案傳輸 | 支援以 16-bit PCM sample 作為 Huffman 符號 |
| Frame 封包 | 使用 4-byte Big-endian 長度及 1-byte Type |
| 傳輸統計 | 輸出檔案大小、上線傳輸量、壓縮比例、編解碼與總耗時 |
| 測試驗證 | 提供單元測試、異常封包測試與 Benchmark 腳本 |

Huffman 模式不保證所有資料都能壓縮變小。對於高熵資料或符號種類較多的音訊，壓縮後資料量可能因 Codebook 等額外開銷而增加。

## 2. 專案完成與驗證狀態

| 驗證項目 | 結果 |
|---|---|
| C 單元測試 | **114 PASS、0 FAIL、0 TODO** |
| Frame、UTF-8、Huffman | 已完成實作及單元測試 |
| TCP 半包／黏包 | TCP-05、TCP-06 本機測試 PASS |
| TCP 異常輸入 | TCP-01～04、TCP-07、非法 Padding 及中途斷線等已測案例 PASS |
| RAW/HUFF 文字聊天 | 中文、英文及 Emoji 雙機測試 PASS |
| TXT 檔案傳輸 | CLI 與聊天 `/send` 雙機測試 PASS |
| WAV 檔案傳輸 | CLI 與聊天 `/send` 雙機測試 PASS；音樂可正常播放 |
| 檔案完整性 | 雙機 TXT/WAV 使用 SHA-256 驗證內容一致 |
| Localhost 正式量測 | 4 種檔案 × 2 種模式 × 5 次，共 **40 筆** |
| 雙實體電腦正式量測 | 4 種檔案 × 2 種模式 × 5 次，共 **40 筆** |
| **正式效能量測合計** | **80 筆** |
| 雙機測試環境 | 電腦 A：Ethernet；電腦 B：Wi-Fi；TCP IPv4 |
| 數據與分析 | 原始 CSV、中位數、Excel 圖表及 Huffman 壓縮分析報告已完成 |

114 PASS 為 C 單元測試結果，80 筆 Benchmark 為兩種網路環境下的正式效能量測，兩者屬於不同驗證工作。

TCP 異常輸入及 Huffman 邊界測試主要在 localhost 執行，不能直接視為所有情境都已完成雙機驗證。詳細測試證據參閱 `docs/testing.md`。

## 3. 編譯與測試

本專案使用 C99，編譯時啟用 `-Wall -Wextra`。

### Windows PowerShell

在專案根目錄執行：

```powershell
mingw32-make
```

執行單元測試：

```powershell
mingw32-make test
```

預期測試結果：

```text
結果：PASS 114、FAIL 0、TODO 0
```

清除編譯產物：

```powershell
mingw32-make clean
```

### macOS / Linux / WSL

```bash
make
make test
make clean
```

實際編譯環境需要具備 C 編譯器與 Make 工具。Windows 版本需要 MinGW 及 Winsock2 函式庫。

## 4. TCP 聊天操作

同一台電腦可以使用兩個終端機測試；跨電腦測試時，將 `127.0.0.1` 換成接收端的實際區域網路 IP。

### 終端機 A：建立伺服器

```powershell
.\textlink.exe chat server 5000 --raw
```

### 終端機 B：連線到伺服器

```powershell
.\textlink.exe chat client 127.0.0.1 5000 --raw
```

聊天過程中可使用：

| 指令 | 功能 |
|---|---|
| `/raw` | 切換 RAW 模式 |
| `/huff` | 切換 Huffman 模式 |
| `/help` | 顯示可用指令 |
| `/quit` | 離開聊天 |
| `/files samples` | 列出指定資料夾的檔案 |
| `/send 1` | 傳送列出的第 1 個檔案 |
| `/send 路徑` | 傳送指定檔案 |

聊天介面會顯示傳輸模式，以及原始資料量與上線傳輸量的比較。

## 5. 命令列檔案傳輸

### 接收端

開啟第一個 PowerShell：

```powershell
.\textlink.exe recv 5000 out
```

### 傳送端：RAW

開啟第二個 PowerShell：

```powershell
.\textlink.exe send 127.0.0.1 5000 benchmark_real\real_chinese.txt --raw
```

### 傳送端：Huffman

接收端重新啟動後執行：

```powershell
.\textlink.exe send 127.0.0.1 5000 benchmark_real\real_chinese.txt --huff
```

每次獨立檔案傳輸後，接收端需要重新啟動，才能進行下一次測試。

### 檔案完整性驗證

```powershell
cmd /c fc /b benchmark_real\real_chinese.txt out\real_chinese.txt
```

如果顯示：

```text
FC: 找不到相異處
```

表示接收到的檔案與原始檔案逐 byte 相同。

程式在傳送端與接收端輸出 `STATS`，包含 `file_bytes`、`wire_bytes`、`ratio`、`encode_ms`、`decode_ms` 及 `total_ms` 等統計資訊。

## 6. 通訊協定與 Huffman 設計

### Frame 格式

每個 Frame 的基本格式如下：

| 欄位 | 大小 | 說明 |
|---|---|---|
| Length | 4 bytes | Big-endian，包含 Type 與 Payload |
| Type | 1 byte | Frame 類型 |
| Payload | 變動長度 | 實際資料 |

Frame Length 必須介於 1 與 16 MiB 之間。

### Huffman 符號類型

| 符號模式 | 說明 |
|---|---|
| `SYM_CHAR` | UTF-8 字元 |
| `SYM_S16` | 16-bit PCM sample |
| `SYM_BYTE` | 一般位元組 |

Huffman 編碼流程包含符號切分、頻率統計、Huffman Tree、Codebook 建立，以及 Bitstream 打包。

解碼端依照 Codebook 還原符號，並重新組合原始 bytes。

詳細封包格式、Codebook 欄位及錯誤處理規範請參閱 [介面文件](docs/interface.md)。

## 7. 測試驗證

### 7.1 單元測試

測試程式：`tests/test_codec.c`

執行：

```powershell
mingw32-make test
```

測試涵蓋：

- Frame Header 封裝、解析及長度邊界
- UTF-8 合法性與異常編碼
- Huffman 編碼、解碼與 Round-trip
- Huffman 不完整資料及異常輸入
- 其他邊界值與錯誤處理

測試結果：

**114 PASS、0 FAIL、0 TODO**

### 7.2 TCP 通訊與異常封包測試

測試腳本：`tests/test_tcp_malformed.ps1`

第一個 PowerShell 啟動接收端：

```powershell
.\textlink.exe recv 5000 out
```

第二個 PowerShell 執行：

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\test_tcp_malformed.ps1 -Case TCP-01
```

其他情境可改成 `TCP-02`、`TCP-03`、`TCP-04`。

測試包含：

- 不完整 Frame Header
- 不完整 Payload
- 超過 16 MiB 的 Frame Length
- 未知 Frame Type

接收端能回報相應的接收失敗情況。這些測試仍需依實際接收端輸出判讀，不能只因測試腳本成功送出資料就視為測試通過。

詳細紀錄請參閱 [測試驗證報告](docs/testing.md)。

此外，本專案另進行以下本機 TCP 驗證：

- TCP-05：將多個 Frame 拆成多次 TCP 寫入，確認接收端能正確重組。
- TCP-06：將多個 Frame 合併為一次 TCP 寫入，確認接收端能依 Frame 邊界正確解析。
- TCP-07：傳送包含異常 Huffman Codebook 的資料，確認接收端能拒絕錯誤編碼。

上述測試均已完成本機驗證，詳細測試方式、接收端結果及檔案完整性檢查請參閱 [測試驗證報告](docs/testing.md)。

此外，2026-10-10 新增 TCP 傳輸中途斷線測試，使用 `tests/test_tcp_disconnect.py` 模擬 RAW 檔案傳輸：

- `FILE_BEGIN` 宣告 4096 bytes。
- 實際僅傳送 1024 bytes，接收進度為 25%。
- 不傳送 `FILE_END`，直接關閉 TCP 連線。
- 接收端正確顯示「對方已關閉連線」。
- 接收端 Exit Code 為 `1`。
- 未產生不完整的正式輸出檔案，也未留下暫存檔。

**測試結果：PASS（localhost）。**

## 8. 正式 Benchmark 效能量測

### 8.1 測試目的與資料

本專案以 RAW（不壓縮）及 HUFF（Huffman 壓縮）兩種模式，測量 TextLink 傳輸不同資料類型時的壓縮比例、編解碼時間、傳輸時間及有效傳輸速率。

正式 Benchmark 使用四種檔案，每份原始檔案皆大於 1 MB，包括中文文章、英文文章、音樂 WAV 及高熵雜訊 WAV。

本次分別在 localhost 與雙實體電腦區域網路兩種環境進行量測。

| 測試檔案 | 資料類型 | Localhost 原始大小（bytes） | 雙機原始大小（bytes） |
|---|---|---:|---:|
| `real_chinese.txt` | 中文為主的合成自然語言文章 | 1,200,335 | 1,202,641 |
| `real_english.txt` | 英文為主的合成自然語言文章 | 1,258,581 | 1,261,850 |
| `audio_music_20s.wav` | 音樂 WAV，16-bit PCM | 3,528,044 | 3,528,044 |
| `audio_noise.wav` | 高熵雜訊 WAV，16-bit PCM | 1,120,044 | 1,120,044 |

中文及英文測試資料為人工合成的自然語言語料，並非真實出版文章。

音樂 WAV 為 44.1 kHz、16-bit PCM、雙聲道、長度 20 秒；雜訊 WAV 為 8 kHz、16-bit PCM、單聲道。

**資料版本注意事項：** Localhost 與雙機測試的中文、英文 TXT 檔案大小不同，因此兩種環境的文字測試並非完全相同的輸入資料。實驗比較時，不能將耗時差異全部歸因於網路環境。

### 8.2 測試環境與方式

#### 8.2.1 Localhost 測試環境

| 項目 | 設定 |
|---|---|
| 測試電腦 | 電腦 A |
| 作業系統 | Windows |
| IP | `127.0.0.1` |
| TCP Port | 5000 |
| GCC | MinGW.org GCC 6.3.0 |
| 傳輸環境 | 同一台電腦的 TCP Loopback |
| 測試模式 | RAW、HUFF |
| 每組重複次數 | 5 次 |
| 總量測次數 | 40 次 |

#### 8.2.2 雙實體電腦測試環境

| 項目 | 電腦 A（Receiver） | 電腦 B（Sender） |
|---|---|---|
| 作業系統 | Windows | Windows |
| IPv4 | `192.168.0.140` | `192.168.0.188` |
| GCC | MinGW.org GCC 6.3.0 | MinGW-Builds GCC 14.2.0 |
| TCP Port | 5000 | 連線至 A 的 Port 5000 |
| Git Commit | `0179c758612702dd27618b0132ca893bf193499f` | 相同 Commit |
| 功能 | 接收、解碼、寫入檔案 | 讀檔、編碼、傳送 |

雙機測試使用區域網路進行 TCP 傳輸，不使用 localhost IP。

雙機測試使用混合式區域網路連線：電腦 A 透過乙太網路（Ethernet）連接網路，電腦 B 透過 Wi-Fi 連接網路，兩端以 TCP IPv4 進行傳輸。本次未獨立量測可用網路頻寬、延遲及封包遺失率。

#### 8.2.3 正式量測次數

兩種環境皆使用四份測試檔案，每份檔案分別執行 RAW 與 HUFF，每種模式重複五次。

| 測試環境 | 檔案數 | 模式數 | 每組重複次數 | 量測總數 |
|---|---:|---:|---:|---:|
| Localhost | 4 | 2 | 5 | 40 |
| 雙實體電腦 | 4 | 2 | 5 | 40 |
| **合計** | | | | **80** |

每次量測保存 Sender 與 Receiver 的 STATS，並將原始資料整理成 CSV。

正式報告採用每組五次測量結果的中位數（Median），避免只以單次量測代表整體效能。

兩種環境的原始量測資料分開保存，不覆蓋彼此的 CSV。

### 8.3 壓縮比例與有效傳輸速率

本專案以實際上線傳輸位元組數 `wire_bytes` 與原始檔案大小 `file_bytes` 計算傳輸比例。

`wire_bytes` 包含 Frame Header、檔案協定欄位、Huffman Header、Codebook 及 Bitstream 等實際傳送的資料。

| 指標 | 計算公式 | 說明 |
|---|---|---|
| 傳輸比例（ratio） | `wire_bytes / file_bytes` | 小於 1 代表上線資料量小於原始檔案 |
| 壓縮後比例（%） | `ratio × 100` | 上線資料量占原始檔案的百分比 |
| 節省傳輸量（%） | `(1 - ratio) × 100` | 負值表示資料量增加 |
| 傳送端有效速率（MB/s） | `file_bytes / (sender_total_ms × 1000)` | 依傳送端總耗時計算 |
| 接收端有效速率（MB/s） | `file_bytes / (receiver_total_ms × 1000)` | 依接收端總耗時計算 |

有效傳輸速率採十進位 MB/s，亦即 1 MB = 1,000,000 bytes。

#### 範例：雙機中文 TXT Huffman

- 原始大小：1,202,641 bytes
- 實際上線資料量：391,389 bytes
- 傳輸比例：約 0.3254
- 壓縮後比例：約 32.54%
- 節省傳輸量：約 67.46%

Huffman 可有效減少中文文字檔的上線資料量，但傳輸量減少不必然代表總耗時也會縮短。

**注意：** 傳送端及接收端的 `total_ms` 為各自獨立的量測值，不可以相加作為端對端延遲。根據 `total_ms` 換算的有效速率也不等於實際物理網路頻寬。

### 8.4 正式 Benchmark 測試結果

以下結果均為每組五次量測的中位數。

#### 8.4.1 Localhost Benchmark（40 次）

| 測試檔案 | HUFF 傳輸比例 | RAW Sender Total（ms） | HUFF Sender Total（ms） |
|---|---:|---:|---:|
| 中文文章 | 32.39% | 35.0 | 51.8 |
| 英文文章 | 53.96% | 37.1 | 85.0 |
| 音樂 WAV | 108.51% | 92.8 | 389.1 |
| 雜訊 WAV | 140.62% | 34.0 | 162.5 |

Localhost 傳送端有效傳輸速率：

| 測試檔案 | RAW（MB/s） | HUFF（MB/s） |
|---|---:|---:|
| 中文文章 | 34.30 | 23.17 |
| 英文文章 | 33.92 | 14.81 |
| 音樂 WAV | 38.02 | 9.07 |
| 雜訊 WAV | 32.94 | 6.89 |

在 localhost 環境中，四份檔案的 RAW 傳送端總耗時中位數均低於 HUFF。

#### 8.4.2 雙實體電腦 Benchmark（40 次）

雙機傳輸方向為電腦 B（`192.168.0.188`）至電腦 A（`192.168.0.140`）。

| 測試檔案 | HUFF 傳輸比例 | RAW Sender Total（ms） | HUFF Sender Total（ms） |
|---|---:|---:|---:|
| 中文文章 | 32.54% | 74.6 | 113.2 |
| 英文文章 | 54.15% | 136.9 | 116.5 |
| 音樂 WAV | 108.51% | 214.7 | 3,956.0 |
| 雜訊 WAV | 140.62% | 136.0 | 7,059.5 |

雙機接收端總耗時中位數：

| 測試檔案 | RAW Receiver Total（ms） | HUFF Receiver Total（ms） |
|---|---:|---:|
| 中文文章 | 71.7 | 87.4 |
| 英文文章 | 133.0 | 80.5 |
| 音樂 WAV | 209.8 | 328.0 |
| 雜訊 WAV | 132.6 | 150.4 |

雙機正式量測已完成四種檔案、RAW/HUFF 各五次，共 40 筆傳輸。

全部 40 筆 Sender 與 Receiver STATS 均已配對驗證，兩端記錄的 `mode`、`file_bytes` 與 `wire_bytes` 一致。

此外，雙機功能驗收另以 SHA-256 驗證 TXT/WAV 接收檔案與傳送端原始檔案內容一致，音樂 WAV 也已確認可以正常播放。

### 8.5 實驗結果分析

#### 8.5.1 中文文章

中文文章在兩種環境下均具有良好的 Huffman 壓縮效果。

- Localhost：HUFF 傳輸比例為 32.39%，減少約 67.61% 的上線資料量。
- 雙機：HUFF 傳輸比例為 32.54%，減少約 67.46% 的上線資料量。

然而，雙機測試中的傳送端總耗時由 RAW 的 74.6 ms 增加至 HUFF 的 113.2 ms。

結果顯示，中文 Huffman 雖能有效節省傳輸量，但本次測試中額外的編碼處理時間仍影響整體完成時間。

#### 8.5.2 英文文章

英文文章在兩種環境下亦具有壓縮效果。

- Localhost：HUFF 傳輸比例為 53.96%。
- 雙機：HUFF 傳輸比例為 54.15%。

特別的是，雙機英文 TXT 的傳送端總耗時中位數：

- RAW：136.9 ms
- HUFF：116.5 ms

**在本次雙機英文文章測試中，HUFF 的傳送端總耗時中位數低於 RAW，表示壓縮後確實出現傳送端完成時間縮短的現象。**

不過 localhost 的英文文章測試則是 RAW 較快，且兩個環境的測試檔案大小、傳送端電腦及編譯環境不同，因此不能僅憑這兩組結果推論 Huffman 在所有網路環境中都會比較快。

#### 8.5.3 音樂 WAV

音樂 WAV 使用 Huffman S16 符號模式後，HUFF 上線資料量為原始檔案的 108.51%，增加約 8.51%。

雙機測試中：

- RAW Sender Total 中位數：214.7 ms
- HUFF Sender Total 中位數：3,956.0 ms

進一步檢查五次 HUFF 原始數據，發現 `encode_ms` 中位數為 3,624.6 ms，明顯高於 `send_ms` 中位數 233.7 ms。

因此本次雙機音樂 WAV 的 HUFF 效能瓶頸主要出現在傳送端編碼處理階段。

#### 8.5.4 雜訊 WAV

雜訊 WAV 的 HUFF 傳輸比例達到 140.62%，代表上線資料量增加約 40.62%。

雙機測試中：

- RAW Sender Total 中位數：136.0 ms
- HUFF Sender Total 中位數：7,059.5 ms
- HUFF Encode 中位數：6,890.7 ms
- HUFF Send 中位數：97.9 ms

結果顯示，雜訊 WAV 不僅無法透過目前的 S16 Huffman 格式減少傳輸量，傳送端也需要付出顯著的編碼運算成本。

由於此檔案接近均勻分布且包含大量不同的 S16 Sample 值，Codebook 的大小與符號處理成本可能影響效能，但具體瓶頸仍須透過程式分析進一步確認。

#### 8.5.5 雙機 WAV HUFF 的額外控制實驗

為釐清雙機 WAV 編碼耗時偏高的現象，另外在電腦 B 進行 `127.0.0.1:5001` 的 localhost 控制測試。

以 `audio_noise.wav`、HUFF S16 模式進行單次測試，得到：

| 指標 | 電腦 B Localhost 單次測試 |
|---|---:|
| Encode | 7,568.2 ms |
| Send | 23.3 ms |
| Decode | 325,680.8 ms |
| Sender Total | 約 33,278.2 ms |
| Receiver Total | 325,708.6 ms |

該次測試最終成功完成傳輸及存檔，但電腦 B 的本機解碼時間明顯異常偏高。

由於雙機正式測試與此控制實驗使用相同的傳送端電腦，而編碼時間仍達數秒，結果支持編碼耗時主要來自本機處理，而非純粹由區域網路頻寬造成。

不過，電腦 A 與電腦 B 使用不同的硬體環境及 GCC 版本，因此尚不能確認異常效能差異的直接原因。

**此控制實驗屬於額外分析，不納入正式 80 次 Benchmark 統計。**

### 8.6 實驗結論與限制

根據 localhost 與雙實體電腦兩種環境、共 80 次正式量測，可得到以下結論：

1. Huffman 對中文與英文自然語言測試資料具有明顯壓縮效果，能有效減少實際上線資料量。
2. 壓縮後資料量較小，不代表整體傳輸時間一定較短；仍須考慮編碼、傳送、解碼及通訊協定開銷。
3. Localhost 的四種測試檔案均由 RAW 取得較低的傳送端總耗時中位數。
4. 雙機環境下，英文 TXT 的 HUFF 傳送端總耗時中位數低於 RAW；其餘三種檔案則為 RAW 較快。
5. 音樂及雜訊 WAV 的 Huffman S16 模式出現資料膨脹，其中高熵雜訊 WAV 的膨脹比例較大。
6. 雙機 WAV HUFF 的大量耗時集中於編碼階段，顯示目前實作仍有優化空間。
7. 雙機 TXT/WAV 的 RAW/HUFF 傳輸均已通過 SHA-256 完整性驗證，音樂 WAV 亦可正常播放。
8. 兩種網路環境的測試檔案版本及執行電腦不完全相同，應避免將耗時差異完全解釋為網路因素。

**實驗限制：**

- Localhost 與雙機的中文、英文 TXT 檔案大小不同。
- Localhost 測試的傳送端主要為電腦 A；雙機測試的傳送端為電腦 B。
- 電腦 A 使用 GCC 6.3.0；電腦 B 使用 GCC 14.2.0。
- 尚未在相同硬體條件下進行不同 GCC 版本的控制比較。
- 本次雙機測試未針對網路頻寬、延遲或封包遺失率進行獨立量測。
- `sender_total_ms` 與 `receiver_total_ms` 具有不同計時區間，不能直接相加作為端對端延遲。
- 本次結果適用於已測試的資料、電腦及網路條件，不代表所有環境下的效能。

**正式原始數據與工作簿：**

- [Localhost 原始量測 CSV](benchmarks/formal_raw_40.csv)
- [Localhost 中位數 CSV](benchmarks/formal_medians.csv)
- [Localhost Benchmark Excel](benchmarks/formal_benchmark_40.xlsx)
- [雙機原始量測 CSV](benchmarks/two_pc_raw_40.csv)
- [雙機中位數 CSV](benchmarks/two_pc_medians.csv)
- [兩種環境 Benchmark Excel](benchmarks/TextLink_Benchmark_80.xlsx)
- [Benchmark 測試方法](benchmarks/README.md)

### 8.7 Huffman 壓縮率與理論分析

除正式 Benchmark 外，本專案另行分析 Huffman 編碼的理論與實際壓縮效果，包括：

- Shannon Entropy（H）
- Huffman 平均碼長（L）
- Codebook 大小與比例
- 純 Bitstream 與完整上線資料量的差異
- 中文及英文的 CHAR/BYTE 符號模式比較
- WAV 的 S16/BYTE 符號模式比較
- 短聊天訊息的 Huffman 固定開銷
- RAW/HUFF 理論損益平衡頻寬

根據先前 localhost 量測資料建立的簡化模型，中文文章與英文文章的理論損益平衡頻寬分別約為 **172.70 Mbps** 與 **73.59 Mbps**。

這些數值是基於當時的檔案版本與程式耗時建立的理論估計，並未經不同頻寬條件的實驗驗證，也不能直接套用到本次雙機 Benchmark。

詳細公式、Codebook 分析及實驗結論請參閱：

- [Huffman 壓縮率完整分析報告](docs/compression_report.md)
- [Huffman 壓縮分析 Excel](benchmarks/TextLink_Huffman_Compression_Analysis.xlsx)

## 9. 資料分布分析

除 RAW/HUFF 傳輸比較外，本專案另提供：

- 中文文字檔最常見前 30 個字元及其機率
- 英文文字檔最常見前 30 個字元及其機率
- 音樂 WAV 的 16-bit PCM Sample Histogram

對應檔案：

- `benchmarks/char_frequency.py`
- `benchmarks/wav_histogram.py`
- `benchmarks/figures/real_chinese_top30.csv`
- `benchmarks/figures/real_english_top30.csv`
- `benchmarks/figures/audio_music_histogram.csv`

上述資料用於觀察符號頻率分布與 Huffman 壓縮效果之間的關係。

## 10. 專案資料夾結構

本專案採用模組化架構，將 C 程式實作、測試工具、正式量測資料、壓縮分析及專案文件分別存放，以利程式維護、測試重現與團隊協作。

```text
textlink-team1/
│
├── src/                                   # C 程式主要實作
│   ├── main.c                             # 主程式與命令列參數處理
│   ├── net.c                              # TCP 網路連線
│   ├── frame.c                            # Frame 封裝與解析
│   ├── utf8.c                             # UTF-8 合法性驗證
│   ├── huffman.c                          # Huffman 編碼與解碼
│   ├── chat.c                             # TCP 聊天功能
│   └── transfer.c                         # RAW/HUFF 檔案傳輸
│
├── include/                               # C Header 檔案
│   ├── platform.h
│   └── textlink.h
│
├── tests/                                 # 單元測試與異常測試
│   ├── test_codec.c                       # C 單元測試（114 PASS）
│   ├── test_tcp_malformed.ps1             # TCP 異常 Frame 測試
│   ├── test_tcp_stream.ps1                # TCP 半包／黏包測試
│   ├── test_tcp_bad_codebook.py           # 損壞 Huffman Codebook 測試
│   ├── test_tcp_bad_padding.py            # 非法 Huffman Padding 測試
│   ├── test_tcp_disconnect.py             # TCP 傳輸中途斷線測試
│   ├── dump_huffman.c                     # 匯出 C Huffman 實際編碼結果
│   ├── make_samples.py                    # 測試資料產生工具
│   └── edge_files/                        # Huffman 邊界測試資料
│
├── benchmark_real/                        # 四份正式 Benchmark 測試檔案
│   ├── real_chinese.txt                   # 中文自然語言測試資料
│   ├── real_english.txt                   # 英文自然語言測試資料
│   ├── audio_music_20s.wav                # 20 秒 16-bit PCM 音樂
│   └── audio_noise.wav                    # 16-bit PCM 雜訊
│
├── benchmarks/                            # 正式效能量測與統計分析
│   ├── README.md                          # Benchmark 執行說明
│   │
│   ├── run_benchmark.ps1                  # Localhost 單次量測輔助腳本
│   ├── formal_raw_40.csv                  # Localhost 40 筆原始數據
│   ├── formal_medians.csv                 # Localhost 五次中位數
│   ├── formal_benchmark_40.xlsx           # Localhost Benchmark Excel
│   │
│   ├── two_pc_receiver.ps1                # 雙機自動接收腳本
│   ├── two_pc_sender.ps1                  # 雙機自動傳送腳本
│   ├── merge_two_pc.py                    # 配對與合併 40 對 STATS
│   ├── two_pc_raw_40.csv                  # 雙機 40 筆原始量測
│   ├── two_pc_medians.csv                 # 雙機五次中位數
│   ├── TextLink_Benchmark_80.xlsx         # 兩種環境共 80 次比較
│   │
│   ├── logs_two_pc_sender/                # 雙機 Sender 原始 Log
│   ├── logs_two_pc_receiver/              # 雙機 Receiver 原始 Log
│   │
│   ├── char_frequency.py                  # 中文／英文字符頻率分析
│   ├── wav_histogram.py                   # WAV Sample Histogram
│   ├── compression_analysis.py            # N、K、Shannon Entropy H
│   ├── compression_codebook.py            # 平均碼長 L 與 Codebook 分析
│   ├── analyze_encoded.py                 # C Huffman 實際壓縮結果解析
│   ├── chat_compression_analysis.py       # 短聊天訊息壓縮率分析
│   ├── break_even_analysis.py             # 理論損益平衡頻寬計算
│   ├── TextLink_Huffman_Compression_Analysis.xlsx
│   │                                       # Huffman 壓縮分析工作簿
│   │
│   └── figures/                           # 頻率分布及壓縮分析資料
│       ├── real_chinese_top30.csv         # 中文前 30 常見字元
│       ├── real_english_top30.csv         # 英文前 30 常見字元
│       ├── audio_music_histogram.csv      # WAV Sample 分布
│       ├── compression_entropy.csv        # Shannon Entropy 分析
│       ├── compression_codebook.csv       # 平均碼長與 Codebook
│       ├── compression_actual.csv         # 實際編碼及封包開銷
│       ├── chat_compression.csv           # 聊天壓縮比例
│       └── break_even.csv                 # 損益平衡頻寬
│
├── docs/                                  # 技術與驗收文件
│   ├── interface.md                       # TCP Frame 與 Huffman 介面規格
│   ├── testing.md                         # 單元、異常及雙機測試報告
│   ├── verification_checklist.md          # 教授 0～8 驗收檢查表
│   ├── compression_report.md              # Huffman 壓縮率完整分析
│   └── report_data_requirements.md        # 實驗報告資料需求
│
├── Makefile                               # C 程式編譯與測試規則
├── README.md                              # 專案說明與操作指南
├── README_V_SUBMISSION.md                 # V 角色資料說明
├── TEAM_LOG.md                            # 團隊開發與測試紀錄
├── CONTRIBUTIONS.md                       # 組員工作與程式貢獻
├── AI_USAGE.md                            # AI 工具使用紀錄
└── slides.pdf                             # 最終口頭報告簡報（待確認）
```

### 資料夾說明

- **`src/` 與 `include/`**：存放 TextLink 的 C99 核心程式，包括 TCP、Frame、UTF-8、Huffman、聊天及檔案傳輸。
- **`tests/`**：存放 114 項單元測試、TCP 半包／黏包、異常封包、損壞 Codebook、非法 Padding、中途斷線及 Huffman 邊界測試。
- **`benchmark_real/`**：存放四種正式測試檔案，涵蓋中文、英文、音樂 WAV 與雜訊 WAV。
- **`benchmarks/`**：存放 localhost 40 次及雙機 40 次正式效能量測的 CSV、Excel、執行腳本與統計分析。
- **`benchmarks/logs_two_pc_sender/` 與 `logs_two_pc_receiver/`**：保存正式雙機量測的 Sender／Receiver 原始 STATS 紀錄。
- **`benchmarks/figures/`**：保存字符機率分布、WAV Sample Histogram、Entropy、Codebook 及壓縮分析結果。
- **`docs/`**：存放通訊協定、驗收檢查表、完整測試報告與 Huffman 壓縮率分析。
- **`TEAM_LOG.md`、`CONTRIBUTIONS.md`、`AI_USAGE.md`**：記錄團隊協作、成員實際貢獻及 AI 工具使用情況。

### 正式 Benchmark 資料說明

本專案已完成以下量測：

| 測試環境 | 網路條件 | 正式量測 |
|---|---|---:|
| Localhost | 電腦 A，`127.0.0.1` | 40 次 |
| 雙實體電腦 | 電腦 B（Wi-Fi）→ 電腦 A（Ethernet） | 40 次 |
| **合計** | | **80 次** |

每個環境皆使用四種測試檔案、RAW/HUFF 各五次，並分別保存傳送端與接收端 STATS。

兩種環境的比較結果可參閱 `benchmarks/TextLink_Benchmark_80.xlsx`，完整實驗分析則記錄於 `README.md` 第 8 節與 `docs/testing.md`。

**注意：** 本節列示主要專案檔案與正式成果，未包含編譯產生的 `.exe`、本機接收資料夾、暫存檔案及其他不需提交的測試產物。

## 11. 團隊分工

本組採 P＋P＋D＋V 四人分工。

| 成員 | 角色 | 負責工作 |
|---|---|---|
| 陳皓祥 | P | 技術內容整理、口頭報告 |
| s411386008 | P | 效能分析、口頭報告 |
| LiJheChen | D | 模組整合、功能展示 |
| s411386011 | V | 單元測試、異常輸入測試、Benchmark 及原始數據 |

詳細程式貢獻、團隊紀錄與 AI 使用情況，請參閱下列文件。

## 12. 相關文件與繳交資料

### 技術與測試文件

- [通訊介面規格](docs/interface.md)
- [測試驗證報告](docs/testing.md)
- [驗證檢查表](docs/verification_checklist.md)
- [效能量測資料與操作說明](benchmarks/README.md)
- [正式原始數據](benchmarks/formal_raw_40.csv)
- [正式中位數](benchmarks/formal_medians.csv)
- [正式量測 Excel](benchmarks/formal_benchmark_40.xlsx)
- [Benchmark 量測腳本](benchmarks/run_benchmark.ps1)
- [TCP 異常輸入測試腳本](tests/test_tcp_malformed.ps1)
- [正式四種測試資料](benchmark_real/)
- [Huffman 壓縮率分析報告](docs/compression_report.md)
- [Huffman 壓縮分析 Excel](benchmarks/TextLink_Huffman_Compression_Analysis.xlsx)
- [雙機 40 筆原始量測](benchmarks/two_pc_raw_40.csv)
- [雙機 Benchmark 中位數](benchmarks/two_pc_medians.csv)
- [80 次 Benchmark 比較 Excel](benchmarks/TextLink_Benchmark_80.xlsx)
- [雙機自動傳送腳本](benchmarks/two_pc_sender.ps1)
- [雙機自動接收腳本](benchmarks/two_pc_receiver.ps1)
- [雙機數據合併程式](benchmarks/merge_two_pc.py)
- [TCP 中途斷線測試](tests/test_tcp_disconnect.py)

### 團隊與繳交文件

- [TEAM_LOG.md](TEAM_LOG.md)：團隊開發與驗證紀錄
- [CONTRIBUTIONS.md](CONTRIBUTIONS.md)：成員工作分配及實際貢獻
- [AI_USAGE.md](AI_USAGE.md)：AI 輔助使用紀錄
- `slides.pdf`：10 分鐘口頭報告簡報，完成後放於 Repository 根目錄

## 13. 已知限制

- Huffman 壓縮不保證輸出一定小於原始資料。
- 部分音訊檔因符號種類與 Codebook 開銷，可能出現壓縮後膨脹。
- 效能數據受到 CPU、記憶體、系統負載與網路環境影響。
- 正式 Benchmark 已涵蓋 localhost 與雙實體電腦區域網路兩種環境，各執行 40 次。
- 雙機採混合網路連線，電腦 A 使用 Ethernet，電腦 B 使用 Wi-Fi。
- 兩台電腦的 CPU 硬體與 GCC 版本不同，無法直接將兩種環境的耗時差異完全歸因於網路。
- Localhost 與雙機測試的中文、英文 TXT 檔案大小不同，跨環境比較須註明測試資料版本差異。
- 電腦 B 的額外 WAV Huffman 控制實驗曾出現顯著偏高的編解碼時間，目前尚未確認直接原因。
- 傳送端與接收端的 `total_ms` 為個別量測值，不能直接相加視為端對端時間。
- 中文檔名及部分終端機字元顯示可能受到作業系統與編碼環境限制。

---

TextLink — Team 1

本 README 以目前完成的程式功能、測試紀錄與正式 Benchmark 數據為準。後續更新請依照實際 Git commit、測試結果及團隊確認內容修改。