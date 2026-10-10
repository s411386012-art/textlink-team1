# 提供給口頭報告 P 的數據與簡報準備清單

更新日期：2026-10-10

本文件整理 TextLink 專案已完成的測試、正式 Benchmark、Huffman 壓縮分析，以及口頭報告仍需準備的內容。

本文件為團隊內部的簡報準備清單，詳細測試方法與結果以 `README.md`、`docs/testing.md`、`docs/compression_report.md` 及原始 CSV 為準。

## 一、已完成的測試與數據

| 項目 | 完成狀態 | 對應資料 |
|---|---|---|
| C 單元測試 | 完成，114 PASS、0 FAIL、0 TODO | `tests/test_codec.c`、`docs/testing.md` |
| TCP 異常封包與邊界測試 | 已測案例完成 | `docs/testing.md` |
| TCP 中途斷線 | PASS（localhost） | `tests/test_tcp_disconnect.py` |
| 雙機 RAW/HUFF 聊天 | PASS | `docs/verification_checklist.md` |
| 雙機 TXT/WAV RAW/HUFF 傳輸 | PASS，SHA-256 一致 | `docs/testing.md` |
| Localhost Benchmark | 40 次完成 | `benchmarks/formal_raw_40.csv` |
| 雙實體電腦 Benchmark | 40 次完成 | `benchmarks/two_pc_raw_40.csv` |
| Benchmark 統計 | 16 組中位數 | 兩種環境的 Median CSV |
| Excel 整理與比較 | 已完成 | `benchmarks/TextLink_Benchmark_80.xlsx` |
| Huffman 理論與實際壓縮分析 | 已完成 | `docs/compression_report.md` |
| 字元頻率與 WAV Histogram | 已完成統計 | `benchmarks/figures/` |

## 二、正式 Benchmark 測試環境

| 項目 | Localhost | 雙實體電腦 |
|---|---|---|
| 網路 | `127.0.0.1` | B → A，TCP 區域網路 |
| Sender | 電腦 A | 電腦 B：Wi-Fi |
| Receiver | 電腦 A | 電腦 A：Ethernet |
| GCC | 6.3.0 | B：14.2.0，A：6.3.0 |
| 檔案種類 | 4 | 4 |
| 模式 | RAW、HUFF | RAW、HUFF |
| 每組次數 | 5 | 5 |
| 總量測 | 40 | 40 |

雙機 IP：Sender `192.168.0.188`，Receiver `192.168.0.140`，Port 5000。

注意：兩種環境的中文與英文 TXT 測試檔案大小不同，且傳送端電腦不同，不能把所有耗時差異歸因於網路。

## 三、正式 Benchmark 核心結果

以下為各組五次測量的中位數。

| 檔案 | Localhost RAW (ms) | Localhost HUFF (ms) | 雙機 RAW (ms) | 雙機 HUFF (ms) |
|---|---:|---:|---:|---:|
| 中文 TXT | 35.0 | 51.8 | 74.6 | 113.2 |
| 英文 TXT | 37.1 | 85.0 | 136.9 | 116.5 |
| 音樂 WAV | 92.8 | 389.1 | 214.7 | 3,956.0 |
| 雜訊 WAV | 34.0 | 162.5 | 136.0 | 7,059.5 |

上表單位為傳送端 `total_ms`，不能與接收端 `total_ms` 相加作為端對端延遲。

各檔案 Huffman 傳輸比例：

| 檔案 | Localhost | 雙機 |
|---|---:|---:|
| 中文 TXT | 32.39% | 32.54% |
| 英文 TXT | 53.96% | 54.15% |
| 音樂 WAV | 108.51% | 108.51% |
| 雜訊 WAV | 140.62% | 140.62% |

**主要發現：**

- 中文及英文文字的 Huffman 傳輸量明顯減少。
- WAV 使用 S16 Huffman 時可能發生資料膨脹。
- Localhost 四種檔案皆為 RAW 的傳送端總耗時較短。
- 雙機英文 TXT 的 HUFF 傳送端總耗時中位數低於 RAW。
- 雙機 WAV HUFF 的主要耗時集中於傳送端編碼階段。

## 四、Huffman 壓縮分析

已完成以下項目：

- 符號數 N、不同符號數 K、Shannon Entropy H。
- Huffman 平均碼長 L 與理論不等式。
- Bitstream、Codebook 及 Header 開銷分析。
- 中文／英文 CHAR 與 BYTE 模式比較。
- WAV S16 與 BYTE 模式比較。
- 不同長度聊天訊息的原始大小及上線傳輸量。
- 理論損益平衡頻寬及壓縮是否加速的討論。

完整數據：

- `docs/compression_report.md`
- `benchmarks/TextLink_Huffman_Compression_Analysis.xlsx`
- `benchmarks/figures/compression_actual.csv`
- `benchmarks/figures/chat_compression.csv`
- `benchmarks/figures/break_even.csv`

理論損益平衡頻寬是依特定測試條件建立的估計值，不代表已經在不同網路頻寬下完成實驗驗證。

## 五、提供給 P 組員的簡報內容

建議在 10 分鐘報告中說明：

1. 系統架構與 TCP Frame 協定。
2. 三種 Huffman 符號（CHAR、S16、BYTE）及 Codebook 格式。
3. 主要功能：RAW/HUFF 聊天與 TXT/WAV 檔案傳輸。
4. 測試設計：四份檔案、兩種模式、兩個環境，共 80 次。
5. 壓縮比例及 RAW/HUFF 傳送端耗時比較。
6. WAV Sample Histogram 及文字前 30 常見字元機率圖。
7. 為何 Huffman 對文字有效、對部分 WAV 反而膨脹。
8. 雙機驗收結果、實驗限制及改進方向。

## 六、仍待確認的項目

- [ ] 將正式 Excel 圖表匯出為清晰圖片並放入簡報。
- [ ] 將雙機操作、SHA-256 驗證及重要測試結果截圖整理為簡報證據。
- [ ] 完成 10 分鐘簡報並匯出 `slides.pdf`。
- [ ] 與 P1、P2、D、V 確認各自報告及展示內容。
- [ ] 確認 `CONTRIBUTIONS.md` 記錄實際程式貢獻與 MP4 程式沿用來源。
- [ ] 確認最後的 Repository URL、Commit SHA 及繳交狀態。

## 七、資料使用原則

正式量測請優先引用原始 CSV 及中位數統計，Excel 圖表用於視覺化呈現。

歷史探索性資料（例如舊版 `text_repeat`、`text_mixed`）不得混入正式 80 次 Benchmark 統計。

報告中應清楚區分 localhost 與雙機結果，並註明不同 GCC 版本、電腦硬體、網路媒介以及測試文字檔大小差異。