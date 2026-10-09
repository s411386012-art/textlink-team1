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
| TCP 異常封包測試 | 已執行 TCP-01～TCP-04 |
| RAW/HUFF 檔案傳輸 | 已進行功能測試 |
| 檔案完整性 | 使用 `fc /b` 驗證傳輸後內容一致 |
| 正式效能量測 | 4 種檔案 × 2 種模式 × 5 次，共 **40 筆** |
| 正式量測環境 | `127.0.0.1`（localhost） |
| 雙機 TCP 連線 | 團隊曾進行跨電腦連線測試，詳細環境與量測證據待補充 |

單元測試與 Benchmark 是不同的驗證工作。114 PASS 代表程式單元測試通過，40 筆 Benchmark 則用於分析傳輸效能。

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

### 7.2 TCP 異常封包測試

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

## 8. 正式 Benchmark 效能量測

### 8.1 測試資料

本次正式驗收使用四種檔案，每個檔案皆大於 1 MB。

測試資料位於 `benchmark_real/`。

| 測試檔案 | 內容類型 | 原始大小（bytes） |
|---|---|---:|
| `real_chinese.txt` | 中文為主的合成自然語言文章 | 1,200,335 |
| `real_english.txt` | 英文為主的合成自然語言文章 | 1,258,581 |
| `audio_music_20s.wav` | 20 秒音樂 WAV，16-bit PCM | 3,528,044 |
| `audio_noise.wav` | 自選高熵雜訊 WAV，16-bit PCM | 1,120,044 |

中文與英文文字檔為人工合成的自然語言測試語料，並非真實出版文章。音樂 WAV 為 44.1 kHz、16-bit、雙聲道 PCM；雜訊 WAV 為 8 kHz、16-bit、單聲道 PCM。

### 8.2 測試環境與方式

- 測試系統：Windows
- 網路：localhost（`127.0.0.1:5000`）
- 傳輸模式：RAW、HUFF
- 每個檔案及模式：重複五次
- 正式量測總數：4 × 2 × 5 = **40 次**
- 統計方法：每組五次取中位數（Median）

每次量測記錄傳送端與接收端的 `STATS`，並將原始資料保存為 CSV。

### 8.3 壓縮比例與有效傳輸速率

本專案的傳輸比例定義為：

\[
\text{ratio}=\frac{\text{wire\_bytes}}{\text{file\_bytes}}
\]

壓縮後比例 (%)：

\[
\text{Compressed Percentage}=\text{ratio}\times100\%
\]

節省傳輸量比例 (%)：

\[
\text{Saved Percentage}=(1-\text{ratio})\times100\%
\]

若結果為負值，代表壓縮後的資料量反而增加。

有效傳輸速率（十進位 MB/s）：

\[
\text{Effective Throughput}=
\frac{\text{file\_bytes}}{\text{total\_ms}\times1000}
\]

傳送端與接收端分別使用各自的 `total_ms` 計算，不將兩端時間直接相加。

### 8.4 正式測試結果

以下為五次量測的中位數。

| 測試檔案 | HUFF 傳輸比例 | RAW 傳送端總耗時 (ms) | HUFF 傳送端總耗時 (ms) |
|---|---:|---:|---:|
| 中文文章 | 32.39% | 35.0 | 51.8 |
| 英文文章 | 53.96% | 37.1 | 85.0 |
| 音樂 WAV | 108.51% | 92.8 | 389.1 |
| 雜訊 WAV | 140.62% | 34.0 | 162.5 |

有效傳輸速率（MB/s，以傳送端總耗時中位數換算）：

| 測試檔案 | RAW (MB/s) | HUFF (MB/s) |
|---|---:|---:|
| 中文文章 | 34.30 | 23.17 |
| 英文文章 | 33.92 | 14.81 |
| 音樂 WAV | 38.02 | 9.07 |
| 雜訊 WAV | 32.94 | 6.89 |

### 8.5 實驗結果分析

**中文文章**

Huffman 將傳輸量降低約 67.61%，但傳送端總耗時中位數由 RAW 的 35.0 ms 增加至 51.8 ms。

**英文文章**

Huffman 將傳輸量降低約 46.04%，但傳送端總耗時由 37.1 ms 增加至 85.0 ms。

**音樂 WAV**

Huffman 的傳輸比例為 108.51%，表示壓縮後反而增加約 8.51% 的資料量，且編解碼帶來額外運算成本。

**雜訊 WAV**

Huffman 的傳輸比例達 140.62%，顯示高熵音訊資料不一定適合使用目前實作的 Huffman 格式。

### 8.6 實驗結論與限制

在本次 localhost 測試中：

1. Huffman 對中文與英文自然語言測試語料能有效減少傳輸量。
2. Huffman 對部分音訊資料不一定能有效壓縮，並可能增加上線傳輸量。
3. 四種檔案的 RAW 傳送端總耗時中位數均低於 HUFF。
4. 編碼、解碼與 Codebook 開銷會影響實際傳輸效能。
5. localhost 測試結果不能直接代表跨電腦網路環境的效能。

詳細數據請參閱：

- [正式原始量測 CSV](benchmarks/formal_raw_40.csv)
- [正式中位數 CSV](benchmarks/formal_medians.csv)
- [正式 Benchmark Excel](benchmarks/formal_benchmark_40.xlsx)
- [Benchmark 方法與資料說明](benchmarks/README.md)

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

```text
textlink-team1/
├── src/                         C 程式實作
│   ├── main.c
│   ├── net.c
│   ├── frame.c
│   ├── utf8.c
│   ├── huffman.c
│   ├── chat.c
│   └── transfer.c
├── include/                     Header 檔
│   ├── platform.h
│   └── textlink.h
├── tests/
│   ├── test_codec.c
│   ├── test_tcp_malformed.ps1
│   └── make_samples.py
├── benchmark_real/              正式驗收測試資料
├── benchmarks/
│   ├── formal_raw_40.csv
│   ├── formal_medians.csv
│   ├── formal_benchmark_40.xlsx
│   ├── run_benchmark.ps1
│   ├── char_frequency.py
│   ├── wav_histogram.py
│   ├── figures/
│   └── README.md
├── docs/
│   ├── interface.md
│   ├── testing.md
│   ├── verification_checklist.md
│   └── report_data_requirements.md
├── Makefile
├── README.md
├── TEAM_LOG.md
├── CONTRIBUTIONS.md
├── AI_USAGE.md
└── slides.pdf                    正式報告，完成後放入
```

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

### 團隊與繳交文件

- [TEAM_LOG.md](TEAM_LOG.md)：團隊開發與驗證紀錄
- [CONTRIBUTIONS.md](CONTRIBUTIONS.md)：成員工作分配及實際貢獻
- [AI_USAGE.md](AI_USAGE.md)：AI 輔助使用紀錄
- `slides.pdf`：10 分鐘口頭報告簡報，完成後放於 Repository 根目錄

## 13. 已知限制

- Huffman 壓縮不保證輸出一定小於原始資料。
- 部分音訊檔因符號種類與 Codebook 開銷，可能出現壓縮後膨脹。
- 效能數據受到 CPU、記憶體、系統負載與網路環境影響。
- 本次正式 Benchmark 使用 localhost，不能直接推論實際區域網路傳輸表現。
- 傳送端與接收端的 `total_ms` 為個別量測值，不能直接相加視為端對端時間。
- 中文檔名及部分終端機字元顯示可能受到作業系統與編碼環境限制。

---

TextLink — Team 1

本 README 以目前完成的程式功能、測試紀錄與正式 Benchmark 數據為準。後續更新請依照實際 Git commit、測試結果及團隊確認內容修改。