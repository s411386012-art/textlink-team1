# CONTRIBUTIONS.md — TextLink 成員貢獻紀錄

## 1. 團隊角色分工

本組共四位成員，採用 P＋P＋D＋V 分工。

| 成員 | 角色 | 主要責任 |
|---|---|---|
| 陳皓祥 | P1（口頭報告） | 系統架構、封包格式與 Huffman 設計說明 |
| s411386008 | P2（口頭報告） | 壓縮率、效能分析與測試結果說明 |
| LiJheChen | D（展示整合） | 命令列與模組整合、程式展示及跨電腦操作 |
| s411386011 | V（測試驗證） | 單元測試、異常輸入測試、效能量測腳本與原始數據 |

上述為團隊角色及報告責任分配；各成員的實際程式貢獻另依 Git commit 紀錄列示。

## 2. V — 測試驗證

**Git 作者：`s411386011-art`**

主要工作：

- 擴充 `tests/test_codec.c`，新增 Frame、UTF-8 與 Huffman 邊界及異常輸入測試。
- 執行 `mingw32-make test`，確認 114 PASS、0 FAIL、0 TODO。
- 建立 `tests/test_tcp_malformed.ps1`，重現 TCP-01～TCP-04 異常封包測試。
- 建立四種 Benchmark 測試資料，涵蓋重複文字、混合文字、規律音訊及高熵音訊。
- 執行四種檔案、兩種模式、每種五次，共 40 次效能量測。
- 整理 `benchmarks/raw_results.csv`、`median_results.csv`、`summary.csv` 等統計資料。
- 維護 `benchmarks/run_benchmark.ps1`，驗證 RAW/HUFF 傳輸、STATS 紀錄與量測重現性。
- 使用 `fc /b` 驗證接收檔案與原始檔案逐 byte 相同。
- 修正 Windows MinGW 環境下的 IPv4 轉換及終端機旗標相容性問題。
- 更新 `docs/testing.md`、README 測試結果與相關說明。

代表性 Git commits：

| Commit | 工作內容 |
|---|---|
| `dc3a506` | 新增 V 邊界與異常輸入測試 |
| `5f8d19f` | 修正 Windows MinGW IPv4 轉換 |
| `45a1325` | 新增可重現的 Benchmark 測試資料 |
| `1441af1` | 更新 114 PASS 測試報告 |
| `7852c4b` | 記錄 TCP 異常封包測試 |
| `7395bc5` | 修正舊版 MinGW 終端機旗標相容性 |
| `9b7a139` | 修正 PowerShell Benchmark stderr 處理 |
| `93927a5` | 補充 Benchmark 重現驗證 |
| `941de3d` | 忽略本機測試產物 |

## 3. 其他程式實作貢獻

### Git 作者：陳皓祥

- 實作 Frame 標頭封裝與解析。
- 實作 UTF-8 合法性驗證。
- 實作 Huffman `SYM_BYTE`、`SYM_CHAR` 編碼與解碼。
- 修正 WAV data chunk 奇數長度截斷的邊界問題。
- 完成相關單元測試。
- 補充 Huffman 封包及 WAV 邊界處理規格。

### Git 作者：LiJheChen

- 修正 Frame header 長度溢位問題。
- 改善 Huffman 樹建立與解碼安全性。
- 增加 Frame 與 Huffman 修正後的回歸測試。
- 更新 Huffman 介面規格文件。

### 其他 Repository 貢獻

- `s411386012-art`：建立初始 Repository。
- `s411386011-create`：透過 GitHub 上傳檔案，詳細內容需依提交內容確認。

## 4. 成果與驗證

- 單元測試：114 PASS、0 FAIL、0 TODO。
- TCP 異常輸入：已執行 TCP-01～TCP-04，結果記錄於 `docs/testing.md`。
- 效能量測：40 次正式 RAW/HUFF 測試。
- 檔案完整性：RAW 與 HUFF 傳輸後均完成逐 byte 比對。
- 測試程式、量測腳本、原始數據及文件已納入 Git 版本管理。

## 5. 團隊驗證與協作

- P1、P2：負責整理專案技術內容與效能分析，準備 10 分鐘口頭報告。
- D：負責整合與展示聊天、RAW/HUFF 檔案傳輸功能。
- V：負責測試案例、114 項單元測試、TCP 異常輸入測試及 40 次效能量測。
- 團隊共同確認程式可編譯、功能可執行，並準備評測當天的程式操作與問題回答。

## 6. Git 貢獻紀錄

- `s411386011-art`：測試、Benchmark、異常封包測試、Windows 相容性修正與驗證文件。
- `陳皓祥`：Frame、UTF-8、Huffman 編解碼與 WAV 邊界處理。
- `LiJheChen`：Frame 溢位修正、Huffman 安全性改善、回歸測試及介面文件。
- `s411386008`：P2 角色已確認，具體程式與文件貢獻待依其實際工作補充。


本文件依據 Git commit 紀錄與已完成的驗證工作整理，未確認事項不視為已完成。