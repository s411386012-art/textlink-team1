# TextLink 測試與驗證紀錄（V）

## 1. 範圍與測試環境

- 語言／建置：C99、GCC／MinGW，Windows PowerShell。
- 已觀察到的單元測試結果：`PASS 104、FAIL 0、TODO 0`（依先前終端機截圖；請附原始 log、執行日期、GCC 版本與 commit SHA）。
- 本次 40 次效能量測：**同一台 Windows 電腦，TCP loopback `127.0.0.1:5000`**；非兩台電腦網路效能。
- 實體雙機連線：組員表示已測試；請另補雙方作業系統、網路類型、IP 網段、操作及證據截圖。

## 2. 測試項目

| 類別 | 驗證方式 | 已知結果／待補證據 |
|---|---|---|
| Frame / UTF-8 / Huffman 單元測試 | `mingw32-make test` | 104 PASS、0 FAIL、0 TODO；需保存 log |
| RAW／HUFF 聊天 | server/client 互傳 ASCII、中文 | 曾完成本機測試；請附截圖 |
| TXT／WAV 傳檔 | `send`／`recv`，RAW／HUFF | 曾完成本機測試；請附截圖 |
| 位元組一致性 | `cmd /c fc /b <source> <destination>` | 先前測試顯示無差異；請保留驗證 log |
| 壞輸入 | `tests/test_codec.c` | 需核對原始測試案例與完整涵蓋範圍，不以 PASS 數代替覆蓋率 |
| 效能 | 四個 >1 MiB 檔案 × 兩模式 × 五次 | 40 筆原始數據，見 `benchmarks/raw_results.csv` |

## 3. 測試指令（Windows PowerShell）

```powershell
mingw32-make
mingw32-make test
# 終端機 A
.\textlink.exe recv 5000 out
# 終端機 B（每次傳輸重新啟動接收端）
.\textlink.exe send 127.0.0.1 5000 benchmark_files\text_repeat.txt --huff
cmd /c fc /b benchmark_files\text_repeat.txt out\text_repeat.txt
```

## 4. 效能資料定義

- `file_bytes`：原始檔案 bytes。
- `wire_bytes`：程式 STATS 回報的線上 bytes（含協定額外資訊）。
- `ratio`：`wire_bytes / file_bytes`；越小表示傳輸量越少。
- `encode_ms`：發送端編碼耗時。
- `send_ms`：發送端傳送耗時。
- `total_ms`：**發送端**統計的總耗時；不是端到端延遲。
- `decode_ms`：接收端解碼耗時，不能直接加到 `total_ms` 當成精確端到端延遲。
- 原始資料由 PowerShell 終端機 STATS 手動抄錄至 Excel，非自動採集；`raw_results.csv` 由既有紀錄整理。

## 5. 結果與限制

- 重複文字 HUFF/RAW 傳輸比例約 48.73%；混合文字約 58.82%；正弦波 WAV 約 42.14%；隨機音訊 WAV 約 140.62%。
- 隨機音訊使用 Huffman 傳輸量增加；壓縮效果取決於資料分布和 codebook 開銷。
- loopback 測試不能代表跨實體網路的吞吐與延遲。
- `total_ms` 為傳送端指標，不能與接收端 `decode_ms` 直接相加視為完整傳輸時間。
- 本次資料無法直接推斷高熵與編碼耗時的因果關係，僅能提出合理解釋。

## 6. 提交前待辦

- [ ] 保留 `mingw32-make test` 完整輸出及 gcc 版本。
- [ ] 檢查 `tests/` 內壞輸入案例與實際覆蓋內容。
- [ ] 以 Hash 或 `fc /b` 保留四個檔案在 RAW／HUFF 下的完整性證據。
- [ ] 補上兩台實體電腦測試環境與證據。
- [ ] 把三張 Excel 圖表匯出 PNG 放入 `benchmarks/figures/`。
- [ ] 在 README 連結本文件及 benchmark 資料。
