# TextLink 測試與驗證紀錄（V）

## 1. 範圍與測試環境

- 語言／建置：C99、GCC／MinGW，Windows PowerShell。
- **最新單元測試結果：`PASS 114、FAIL 0、TODO 0`**（2026-10-09，依執行 `mingw32-make test` 的終端機截圖）。原先為 104 PASS；本次增加 10 項邊界及壞輸入測試。
- 相關 Git 提交：`dc3a506`（`test: add V boundary and malformed input tests`）、`5f8d19f`（`fix: support IPv4 conversion on Windows MinGW`）；兩筆已推送至 `main`。
- 本次 40 次效能量測：**同一台 Windows 電腦，TCP loopback `127.0.0.1:5000`**；不是兩台實體電腦間的網路效能量測。
- 實體雙機連線：組員表示已測試；仍需補雙方作業系統、網路類型、IP 網段、實際操作及證據截圖。**雙機連線成功不等於已完成雙機效能量測。**

## 2. 測試項目與結果

| 類別 | 驗證方式 | 已知結果／待補證據 |
|---|---|---|
| Frame / UTF-8 / Huffman 單元測試 | `mingw32-make test` | **114 PASS、0 FAIL、0 TODO**；請保存完整 log |
| RAW／HUFF 聊天 | server/client 互傳 ASCII、中文 | 曾完成本機測試；請附截圖 |
| TXT／WAV 傳檔 | `send`／`recv`，RAW／HUFF | 曾完成本機測試；請附截圖 |
| 位元組一致性 | `cmd /c fc /b <source> <destination>` | 先前測試顯示無差異；請保留各模式驗證 log |
| 壞輸入與邊界 | `tests/test_codec.c` | 新增 10 項測試已包含於 114 PASS；不以 PASS 數直接宣稱完整覆蓋率 |
| TCP 異常封包／中途斷線 | 獨立 Client 送出異常資料 | **尚未執行整合測試；不得標示 PASS** |
| 效能 | 四個大於 1 MiB 檔案 × RAW/HUFF × 五次 | 40 筆原始數據，見 `benchmarks/raw_results.csv` |

### 2.1 V 新增的 10 項邊界與壞輸入測試

本次於 `tests/test_codec.c` 擴充 `test_v_boundaries()`，驗證方向包括：

- **Frame**：空 payload、最大允許長度，以及超過允許上限的長度。
- **UTF-8**：內含 NUL (`0x00`) 的輸入、NUL 後仍需檢查非法位元組，以及截斷的四位元組 UTF-8 序列。
- **Huffman**：截斷的固定標頭前綴，以及 `max_out = 0` 時拒絕產生非空輸出。

以上為新增測試的範圍摘要；單項斷言與實際輸入值以版本 `dc3a506` 的 `tests/test_codec.c` 為準。此次 `mingw32-make test` 執行結果為 **114 PASS、0 FAIL、0 TODO**。

> 注意：單元測試成功不代表 TCP 異常封包、斷線恢復、記憶體安全性等整合／動態檢測已全部完成。

### 2.2 Windows 編譯相容性

- 在重新 clone 的專案中，MinGW 曾對 `inet_ntop`／`inet_pton` 出現未宣告與連結失敗問題。
- 已修改 `src/net.c` 的 Windows IPv4 轉換相容性處理，提交 `5f8d19f`。
- 修改後在使用者 Windows 環境重新執行 `mingw32-make test`，得到 114 PASS。
- **尚需**在乾淨 clone 上保留完整建置 log，確認所有編譯警告與執行結果。

## 3. 測試指令（Windows PowerShell）

```powershell
mingw32-make
mingw32-make test

# 終端機 A：接收端
.\textlink.exe recv 5000 out

# 終端機 B：發送端（每次傳輸重新啟動接收端）
.\textlink.exe send 127.0.0.1 5000 benchmark_files\text_repeat.txt --huff

# 檔案內容逐 byte 比對
cmd /c fc /b benchmark_files\text_repeat.txt out\text_repeat.txt
```

## 4. 效能資料定義與來源

- `file_bytes`：原始檔案 bytes。
- `wire_bytes`：程式 STATS 回報的線上 bytes（含協定額外資訊）。
- `ratio`：`wire_bytes / file_bytes`；越小表示傳輸量越少。
- `encode_ms`：發送端編碼耗時。
- `send_ms`：發送端傳送耗時。
- `total_ms`：**發送端**統計的總耗時；不是端到端延遲。
- `decode_ms`：接收端解碼耗時；不能直接加到 `total_ms` 當作精確端到端延遲。
- 原始資料由 PowerShell 終端機 STATS 手動抄錄至 Excel，非自動採集；`raw_results.csv` 由既有紀錄整理。
- `benchmarks/median_results.csv`：每組五次測試的**中位數**，優先用於正式報告。
- `benchmarks/summary.csv`：平均值摘要，供補充分析。

## 5. 結果與限制

| 檔案 | HUFF 傳輸量／原始檔案大小 | RAW 發送端 total_ms 中位數 | HUFF 發送端 total_ms 中位數 |
|---|---:|---:|---:|
| `text_repeat.txt` | 48.73% | 53.7 ms | 65.6 ms |
| `text_mixed.txt` | 58.82% | 70.3 ms | 174.6 ms |
| `audio_sine.wav` | 42.14% | 39.0 ms | 34.7 ms |
| `audio_noise.wav` | 140.62% | 38.9 ms | 175.8 ms |

- 隨機音訊使用 Huffman 反而增加線上傳輸量；壓縮效果取決於資料分布及 codebook 開銷。
- 本機 loopback 的結果不能代表跨實體網路的吞吐量與延遲。
- `total_ms` 為發送端指標，不能與接收端 `decode_ms` 直接相加視為完整傳輸時間。
- 目前數據不能直接推斷資料熵與編碼耗時的因果關係，只能提出合理解釋。
- 上表壓縮比例沿用 STATS 的 `wire_bytes / file_bytes` 定義，並非以 RAW `wire_bytes` 為分母的流量節省百分比。

## TCP 異常封包與中途斷線整合測試（2026-10-09）

測試環境：Windows PowerShell、TextLink `recv 5000 out`、localhost `127.0.0.1:5000`。使用 `tests/test_tcp_malformed.ps1` 產生異常 TCP 輸入；每個案例均重新啟動接收端。以下結果來自人工執行時的終端機觀察，**不是腳本自動判定**。

| 編號 | 輸入內容 | 接收端實際訊息 | 結果 |
|---|---|---|---|
| TCP-01 | 僅送出 2 bytes Header，立即斷線 | 接收失敗；沒有產生輸出檔；對方已關閉連線 | PASS：異常連線安全失敗 |
| TCP-02 | Header 宣告 Length=11、Type=0x01，實際只送 3/10 bytes Payload 後斷線 | 接收失敗；沒有產生輸出檔；對方已關閉連線 | PASS：接收端安全失敗；尚未確認失敗是否發生在 Payload 讀取階段 |
| TCP-03 | Header 宣告 Length=`0x01000001`，超過 16 MiB 上限 | 接收失敗；沒有產生輸出檔；收到不合規格的封包 | PASS：拒絕超長 Frame |
| TCP-04 | Header Length=1（合法），Type=`0xFF`（未知） | 接收失敗；沒有產生輸出檔；收到不合規格的封包 | PASS：接收流程拒絕未知 Type |

**限制：** 四項測試均在本機 loopback 執行，不代表雙機網路壓力測試；沒有使用記憶體分析工具驗證記憶體安全性。TCP-02 的封包 Type=0x01，若要確認確實執行到 Payload 讀取階段，需搭配 `src/transfer.c` 的實作流程檢查。這四項整合測試與 `mingw32-make test` 的 114 項單元測試分開統計。

重現方式：

```powershell
# 終端機 A（每次測試重新啟動）
.\textlink.exe recv 5000 out

# 終端機 B（依序替換 TCP-01 ~ TCP-04）
powershell -ExecutionPolicy Bypass -File .\tests\test_tcp_malformed.ps1 -Case TCP-01
```

腳本僅負責發送測試資料，是否 PASS 須觀察終端機 A 的接收端訊息。

## 6. 提交前待辦

- [x] 新增 10 項 V 邊界與壞輸入測試，並取得 `114 PASS、0 FAIL、0 TODO`。
- [x] 將測試及 Windows 相容性修改提交 GitHub（`dc3a506`、`5f8d19f`）。
- [ ] 保存 `mingw32-make test` 完整輸出、GCC 版本、日期與 commit SHA（目前已有終端機結果截圖）。
- [ ] 核對 `tests/` 各個壞輸入案例的實際覆蓋範圍，必要時補測。
- [ ] 執行 TCP 異常封包、截斷 payload、斷線等整合測試並記錄結果。
- [ ] 以 Hash 或 `fc /b` 保留四個檔案在 RAW／HUFF 下的完整性證據。
- [ ] 補上兩台實體電腦測試環境與證據；如課程要求雙機效能數據，另行量測。
- [ ] 把三張 Excel 圖表匯出 PNG 放入 `benchmarks/figures/`，並確認正式報告採用中位數。
- [ ] 在根目錄 `README.md` 連結本文件及 benchmark 資料。
