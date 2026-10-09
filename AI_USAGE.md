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

AI 協助規劃測試流程、整理統計資料及分析 RAW 與 Huffman 的效能差異。

正式測試使用四種檔案、兩種模式、每種五次，共 40 次量測。

組員實際執行測試、收集 `STATS`、整理 CSV，並使用 Excel 製作比較圖表。

相關檔案：

- `benchmarks/run_benchmark.ps1`
- `benchmarks/formal_raw_40.csv`
- `benchmarks/formal_medians.csv`
- `benchmarks/formal_benchmark_40.xlsx`
- 初期探索性量測使用 `benchmark_files/`（舊資料集，現已從 Repository 最新版本移除）；正式驗收改用 `benchmark_real/`。

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
