# TextLink Benchmark 資料與重現方式

## 1. 正式效能量測總覽

本專案針對四種大於 1 MB 的測試檔案，分別執行 RAW 與 HUFF 傳輸，每種模式重複五次，並以五次測量結果的中位數比較效能。

正式 Benchmark 包含兩種環境：

| 測試環境 | 檔案數 | 傳輸模式 | 每組次數 | 總次數 |
|---|---:|---|---:|---:|
| Localhost（127.0.0.1） | 4 | RAW、HUFF | 5 | 40 |
| 雙實體電腦（Ethernet／Wi-Fi） | 4 | RAW、HUFF | 5 | 40 |
| **合計** | | | | **80** |

雙機測試使用 Windows 電腦 A（Ethernet，Receiver）及 Windows 電腦 B（Wi-Fi，Sender）。

兩種環境的設備與網路條件不同，部分測試檔案版本也存在差異，因此不得將兩者的時間差異完全歸因於網路頻寬。

## 2. 測試檔案

正式效能分析使用下列資料類型：

| 檔案 | 資料類型 |
|---|---|
| `real_chinese.txt` | 中文自然語言文字 |
| `real_english.txt` | 英文自然語言文字 |
| `audio_music_20s.wav` | 音樂 WAV（16-bit PCM） |
| `audio_noise.wav` | 雜訊 WAV（16-bit PCM） |

實際檔案位置請參考專案根目錄的 `benchmark_real/`。

## 3. 正式量測數據

### Localhost

- `formal_raw_40.csv`：40 次原始量測紀錄。
- `formal_medians.csv`：各組五次量測的中位數。
- `formal_benchmark_40.xlsx`：Localhost 的統計與圖表。

### 雙實體電腦

- `two_pc_raw_40.csv`：40 次雙機原始量測紀錄。
- `two_pc_medians.csv`：雙機各組中位數。
- `two_pc_sender.ps1`：雙機 Sender 量測腳本。
- `two_pc_receiver.ps1`：雙機 Receiver 量測腳本。
- `merge_two_pc.py`：整理雙機兩端量測資料。

### 整合工作簿

- `TextLink_Benchmark_80.xlsx`：80 次正式 Benchmark 的整合資料。
- `TextLink_Huffman_Compression_Analysis.xlsx`：Huffman 壓縮率與理論分析。

## 4. 量測指標

本專案記錄下列指標：

| 指標 | 說明 |
|---|---|
| `file_bytes` | 原始檔案大小 |
| `wire_bytes` | TCP 上線資料量，包含 Frame Header 與 Codebook |
| `ratio` | `wire_bytes / file_bytes` |
| `encode_ms` | Huffman 編碼時間 |
| `decode_ms` | Huffman 解碼時間 |
| `send_ms` | 傳送端資料傳輸時間 |
| `total_ms` | 傳送端或接收端總耗時 |

所有時間均以實際 TextLink 的 STATS 輸出為準。

## 5. Localhost 重現測試

以下為 Windows PowerShell 的單次傳輸範例。

### 第一步：編譯

在 Repository 根目錄執行：

```powershell
mingw32-make
```

### 第二步：啟動 Receiver

在第一個 PowerShell 視窗執行：

```powershell
.\textlink.exe recv 5000 out
```

### 第三步：啟動 Sender

在第二個 PowerShell 視窗執行：

```powershell
.\textlink.exe send 127.0.0.1 5000 benchmark_real/real_chinese.txt --raw
```

若要測試 HUFF，將最後的 `--raw` 改成 `--huff`。

每次傳輸後重新啟動 Receiver，並記錄 Sender 與 Receiver 的 STATS。

### 第四步：驗證檔案完整性

```powershell
cmd /c fc /b benchmark_real\real_chinese.txt out\real_chinese.txt
```

若顯示「找不到相異處」，代表本次檔案無損還原成功。

正式批次測試可參考 `run_benchmark.ps1`。

## 6. 雙實體電腦重現測試

本次使用 Windows 電腦 A 作為 Receiver、電腦 B 作為 Sender。

- 電腦 A：Ethernet，192.168.0.140。
- 電腦 B：Wi-Fi，192.168.0.188。

執行前確認兩台電腦可互相連線，並允許防火牆中的 TextLink TCP 連線。

雙機測試相關腳本：

- `two_pc_receiver.ps1`
- `two_pc_sender.ps1`
- `merge_two_pc.py`

具體腳本參數、操作順序與結果請參考 `docs/testing.md`。

正式測試以四種檔案、RAW/HUFF 兩種模式各執行五次，共 40 次，並保存雙方 STATS。

## 7. 圖表與壓縮分析

`figures/` 包含已完成的實驗圖表與分析結果，例如：

- `RAW_vs_HUFF_sender_total_ms_median.png`：RAW/HUFF 傳送端總時間中位數比較。
- `real_chinese_top30.png`：中文字元前 30 名機率分布。
- `real_english_top30.png`：英文字元前 30 名機率分布。
- `audio_music_histogram.png`：音樂 WAV 的 16-bit PCM Sample Histogram。

Huffman 理論分析另包含：

- Shannon Entropy（H）。
- Huffman 平均碼長（L）。
- CHAR/BYTE 與 S16/BYTE 符號比較。
- Codebook 與 Frame Header 開銷。
- 短訊息 Huffman 壓縮效益。
- RAW/HUFF 損益平衡頻寬。

完整計算方式與結果請參考 `docs/compression_report.md`。

## 8. 測試結果與限制

本專案正式量測共 80 次，分別在 localhost 與雙實體電腦環境完成。

結果顯示，Huffman 對中文及英文文字通常能有效減少傳輸 Bytes，但並非所有情況都能降低總耗時。對於音樂及高熵 WAV 資料，Codebook 和編解碼開銷可能使實際傳輸量及總時間增加。

本次量測結果受到硬體效能、作業系統、背景程序、網路條件與資料分布影響，不應將個別案例推廣成所有檔案或網路環境的普遍結論。

本資料夾保存正式量測與分析成果，其他補充功能測試（如 TCP 逐 byte 傳送、20 MiB RAW/HUFF 傳輸）不納入這 80 次正式 Benchmark。
