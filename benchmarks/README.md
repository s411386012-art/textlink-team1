# TextLink benchmark 資料

- `formal_raw_40.csv`：4 檔案 × 2 模式 × 5 次，共 40 筆，從實際 STATS／Excel 紀錄整理。
- `formal_benchmark_40.xlsx`：每組數據以及 `total_ms` 的中位數。
- `figures/`：請將已完成的 Excel 圖表匯出為 `wire_bytes.png`、`total_ms.png`、`codec_time.png`。
- `run_benchmark.ps1`：互動式單次傳送與 STATS 存檔腳本；**不是既有 40 次資料的採集來源**。

## 重現測試

1. Windows PowerShell 在專題根目錄執行 `mingw32-make`。
2. 在接收端啟動 `./textlink.exe recv 5000 out`。
3. 在另一終端機執行 `.\textlink.exe send 127.0.0.1 5000 benchmark_real/real_chinese.txt --raw（或 `--huff`）。
4. 檔案比對：`cmd /c fc /b benchmark_real\real_chinese.txt out\real_chinese.txt`。
5. 重新啟動接收端，依序執行各模式各五次；記錄 `STATS`。

> 注意：目前效能數據是 loopback 測試，不代表實體跨機網路效能。測試檔案需自行保留；不要把過大的輸出檔或 `.exe` 不必要地加入 Git。

