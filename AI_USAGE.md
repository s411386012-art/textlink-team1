# AI_USAGE.md — TextLink AI 工具使用紀錄

## 1. 使用原則

本專案在開發與測試過程中使用 AI 工具輔助理解程式、分析錯誤、規劃測試案例及整理文件。

AI 提供的建議與程式修改，均須經由組員檢查，並透過編譯、測試或實際執行確認結果。AI 產生的內容不直接視為已驗證的成果。

## 2. V（測試驗證）AI 使用紀錄

- 負責成員：s411386011
- Git 作者：s411386011-art
- AI 工具：ChatGPT

### 2.1 單元測試與邊界測試

AI 協助分析 Frame、UTF-8、Huffman 模組可能遇到的邊界條件與異常輸入，並提供測試案例及程式修改建議。

實際測試由組員在 Windows MinGW 環境執行。

驗證結果：

- 測試指令：`mingw32-make test`
- PASS：114
- FAIL：0
- TODO：0

### 2.2 TCP 異常輸入測試

AI 協助規劃不完整封包、超出長度限制、未知 Frame Type 等測試情境，並協助建立 PowerShell 測試腳本。

組員實際啟動接收端、傳送測試資料，觀察接收端對異常輸入的反應。

相關檔案：

- `tests/test_tcp_malformed.ps1`
- `docs/testing.md`

### 2.3 Benchmark 效能量測

AI 協助規劃 RAW/HUFF 效能測試流程、測試資料安排、STATS 數據解析、中位數計算，以及不同環境的效能比較。

正式量測涵蓋兩種環境：

- Localhost：四種檔案 × RAW/HUFF × 各五次，共 40 次。
- 雙實體電腦：四種檔案 × RAW/HUFF × 各五次，共 40 次。
- 正式量測合計：**80 次**。

雙機測試使用電腦 A（Ethernet，Receiver）與電腦 B（Wi-Fi，Sender）。

AI 協助提供 PowerShell 自動化腳本及 Python 數據合併程式，組員實際執行測試、收集兩端 STATS、檢查 40 對紀錄的一致性，再整理原始 CSV、中位數與 Excel 圖表。

相關檔案：

- `benchmarks/run_benchmark.ps1`
- `benchmarks/two_pc_receiver.ps1`
- `benchmarks/two_pc_sender.ps1`
- `benchmarks/merge_two_pc.py`
- `benchmarks/formal_raw_40.csv`
- `benchmarks/formal_medians.csv`
- `benchmarks/two_pc_raw_40.csv`
- `benchmarks/two_pc_medians.csv`
- `benchmarks/TextLink_Benchmark_80.xlsx`

所有量測數據均以實際程式輸出為準，AI 提出的效能原因分析屬於推論，不直接視為已證實的事實。

### 2.4 程式除錯與相容性修正

AI 協助分析及提出修正建議，包括：

- Windows MinGW 的 IPv4 位址轉換問題。
- 舊版 MinGW 的終端機旗標相容性問題。
- PowerShell 對原生程式 stderr 輸出的錯誤處理問題。
- Git 版本管理與多人協作操作。

組員依建議修改程式及腳本，並透過實際編譯、測試與傳輸驗證修正結果。

### 2.5 文件撰寫

AI 協助整理測試方法、結果分析、Benchmark 重現流程及專案文件初稿。

文件內容由組員依實際執行結果、終端機輸出與 Git commit 紀錄檢查後再提交。

### 2.6 Huffman 壓縮率與理論分析

AI 協助規劃及撰寫 Huffman 壓縮分析程式，包含：

- 計算符號數 N、不同符號數 K 與 Shannon Entropy H。
- 計算 Huffman 平均碼長 L，檢查理論不等式。
- 解析 C Huffman 編碼器輸出的資料，區分 Header、Codebook 與 Bitstream。
- 比較中文、英文的 CHAR/BYTE 符號模式。
- 比較 WAV 的 S16/BYTE 符號模式。
- 分析不同長度聊天訊息的壓縮比例。
- 根據量測資料建立簡化的 RAW/HUFF 損益平衡頻寬模型。

組員實際執行 C 與 Python 程式，檢查輸出數據，並整理分析報告與 Excel。

相關檔案包括：

- `benchmarks/compression_analysis.py`
- `benchmarks/compression_codebook.py`
- `benchmarks/analyze_encoded.py`
- `benchmarks/chat_compression_analysis.py`
- `benchmarks/break_even_analysis.py`
- `docs/compression_report.md`
- `benchmarks/TextLink_Huffman_Compression_Analysis.xlsx`

分析結果及理論估計均需依據實際資料與公式驗證，不將 AI 建議直接視為實驗結論。

### 2.7 TCP 傳輸中途斷線測試

AI 協助設計 `tests/test_tcp_disconnect.py`，模擬傳送端宣告 4096 bytes、僅傳送 1024 bytes 後提前關閉 TCP 連線的情境。

組員實際執行測試，確認接收端偵測斷線、以 Exit Code `1` 結束，且沒有產生不完整的正式輸出檔案。

測試結果已記錄於 `docs/testing.md`，並作為驗收第 6 項的補充證據。

## 3. 人工驗證方式

本專案 V 工作採用以下方式確認 AI 建議的正確性：

1. 使用 `mingw32-make` 確認程式可以編譯。
2. 使用 `mingw32-make test` 驗證單元測試結果。
3. 使用 TCP 異常輸入腳本檢查接收端行為。
4. 實際執行 RAW 與 HUFF 檔案傳輸。
5. 使用 `fc /b` 比對原始與接收檔案。
6. 保存並檢查 Benchmark 原始量測數據。
7. 使用 Git commit 紀錄追蹤程式及文件修改。

## 4. 其他組員 AI 使用紀錄

以下由各組員依實際使用情況自行補充。

| 成員 | 角色 | AI 工具 | 使用內容 |
|---|---|---|---|
| 陳皓祥 | P1 | 待確認 | 待補充 |
| s411386008 | P2 | 待確認 | 待補充 |
| LiJheChen | D | 待確認 | 待補充 |

若未使用 AI，應明確註明「未使用」，不應填寫未經確認的使用紀錄。

## 5. 聲明

本文件如實記錄目前已確認的 AI 輔助工作。AI 協助不取代組員對程式正確性、測試結果及提交內容的責任。

其他組員的使用紀錄將在確認後更新。
