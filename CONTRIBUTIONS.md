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
- 整理 `benchmarks/formal_raw_40.csv`、`formal_medians.csv`、`formal_benchmark_40.xlsx` 等統計資料。
- 維護 `benchmarks/run_benchmark.ps1`，驗證 RAW/HUFF 傳輸、STATS 紀錄與量測重現性。
- 使用 `fc /b` 驗證接收檔案與原始檔案逐 byte 相同。
- 修正 Windows MinGW 環境下的 IPv4 轉換及終端機旗標相容性問題。
- 更新 `docs/testing.md`、README 測試結果與相關說明。
- 新增 TCP-05、TCP-06 拆包與黏包整合測試，以及 TCP-07 損壞 Huffman Codebook 測試。
- 完成七組 Huffman 特殊檔案及 BYTE fallback 測試。
- 補充 WAV 奇數 `data` 長度、零 Sample WAV，以及正常與異常 Huffman Padding 測試。
- 使用 C Huffman 編碼結果分析 Shannon Entropy（H）、平均碼長（L）、Codebook 大小及實際壓縮比例。
- 比較中文 CHAR/BYTE、英文 CHAR/BYTE 及 WAV S16/BYTE 的編碼效能。
- 分析聊天短訊息的 Huffman 壓縮效果，並估計 RAW/HUFF 的理論損益平衡頻寬。
- 整理 `docs/compression_report.md`、分析程式、CSV 及 Excel 工作簿。
- 更新 `docs/verification_checklist.md`，依照實際測試證據記錄各項驗收狀態。

### 後續整合與驗證成果（2026-10-10）

- 完成兩台實體電腦的 RAW/HUFF 功能回歸，以及 40 次正式雙機 Benchmark，連同 localhost 共取得 80 次正式量測。
- 修正 Windows Socket 重複監聽問題，使用 `SO_EXCLUSIVEADDRUSE`。
- 修正 CLI 監聽 Port 驗證，使監聽端符合 1024–65535。
- 補強 CLI 與 Chat 的非法 `FILE_END` Payload 驗證，新增 `tests/test_invalid_file_end.py`。
- 完成修改後的 RAW/HUFF CLI 與聊天傳檔回歸測試。
- 補充 Huffman 壓縮理論分析、雙機效能數據與研究限制。
- 新增 TCP-08 逐 byte 傳送測試，將 5 個 Frame 共 4,155 bytes 分成 4,155 次寫入，成功還原 4,096 bytes。
- 完成 20 MiB 單一符號二進位檔案的 RAW/HUFF localhost 傳輸測試，兩種模式均逐 byte 還原成功。

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

**【通訊協定與封包層】**
- **制定通訊協定**：撰寫 `docs/interface.md`，定義 Big-endian 傳輸規範與 5 種 TL-Frame 狀態機。
- **實作封包層 (Frame)**：完成 TL-Frame (`length`+`type`) 封裝與解析，解決 TCP 黏包/半包問題，實現網路與應用層解耦。

**【Huffman 核心壓縮演算法】**
- **三模式編解碼器**：實作 `SYM_BYTE`、`SYM_CHAR` (UTF-8) 與 `SYM_S16` (16-bit WAV PCM) 三種 Huffman 模式，最佳化不同資料型態的壓縮率。
- **自帶 Codebook 設計**：設計 13-bytes 共通標頭與動態 Codebook (4/8 bytes)，實現接收端 100% 獨立解碼。
- **WAV 邊界處理**：解決 data chunk 奇數長度截斷問題，並透過 `parse_wav` 雙重校驗，精確保留非樣本資料。
- **解碼端安全防護**：重建解碼前綴樹 (Prefix Tree) 時加入惡意封包防護，攔截「重複編碼」與「前綴衝突」以防止系統崩潰。

**【字元驗證與系統測試】**
- **UTF-8 合法性驗證**：實作 `utf8.c` (RFC 3629)，嚴格攔截 Overlong Encoding、UTF-16 代理區段等非法輸入。
- **單元測試 (Unit Tests)**：完成並通過專案全部 82 項單元測試，達成 0 failures 綠燈標準。
- **跨裝置實機測試**：編譯 `textlink.exe`，並完成 Server/Client 跨電腦 TCP Socket 通訊、文字聊天與大檔傳輸驗證。

**【專案管理與報告】**
- **版控管理**：負責 GitHub Repository 權限與可見度設定，確保團隊協作同步。
- **口頭報告 (P1)**：主講「架構設計與通訊協定」環節，涵蓋模組化資料流、封包規格及 Huffman 演算法決策。

### Git 作者：LiJheChen

- 修正 Frame header 長度溢位問題。
- 改善 Huffman 樹建立與解碼安全性。
- 增加 Frame 與 Huffman 修正後的回歸測試。
- 更新 Huffman 介面規格文件。

### 其他 Repository 貢獻

- `s411386012-art`：建立初始 Repository。
- `s411386011-create`：透過 GitHub 上傳檔案，詳細內容需依提交內容確認。

### 3.1 MP4 程式來源與 Huffman 實作說明

本專案的 Huffman 編解碼功能以組員**陳皓祥的個人 MP4 作業**作為部分程式實作與演算法基礎，並由團隊依照 TextLink 的通訊與檔案傳輸需求進行擴充、整合及修正。

TextLink 的 Huffman 功能除基本編解碼外，還需要支援 `SYM_BYTE`、`SYM_CHAR`、`SYM_S16` 三種符號模式、Codebook 二進位傳輸、WAV 格式解析、特殊邊界資料及錯誤處理。

以下為 TextLink Repository 中可追溯的相關開發紀錄：

| Commit | 作者 | 工作內容 |
|---|---|---|
| `3278144` | 陳皓祥 | Huffman `SYM_BYTE` 編解碼 |
| `f600929` | 陳皓祥 | Huffman `SYM_CHAR` 編解碼 |
| `2232c74` | 陳皓祥 | WAV `data` chunk 奇數長度邊界修正 |
| `edfed16` | LiJheChen | Huffman Tree 建立與解碼安全性改善 |
| `dc3a506` | s411386011-art | Frame、UTF-8、Huffman 邊界與異常測試 |

**程式來源說明：** Huffman 的部分實作參考或沿用陳皓祥個人 MP4 作業，並非全部由團隊從零開始撰寫。TextLink 版本經過團隊後續修改與整合；實際直接沿用的函式及程式碼範圍，仍須以 MP4 與 TextLink 原始碼比對結果為準。

## 4. 成果與驗證

- 單元測試：114 PASS、0 FAIL、0 TODO。
- TCP 異常輸入：已執行 TCP-01～TCP-04，結果記錄於 `docs/testing.md`。
- 效能量測：完成 localhost 40 次及雙實體電腦 40 次正式 RAW/HUFF Benchmark，合計 80 次；保存原始 CSV、中位數及兩端 STATS。
- 檔案完整性：RAW 與 HUFF 傳輸後均完成逐 byte 比對。
- 測試程式、量測腳本、原始數據及文件已納入 Git 版本管理。

## 5. 團隊驗證與協作

- P1、P2：負責整理專案技術內容與效能分析，準備 10 分鐘口頭報告。
- D：負責整合與展示聊天、RAW/HUFF 檔案傳輸功能。
- V：負責測試案例、114 項單元測試、TCP 異常輸入測試、80 次正式效能量測及資料分析。
- 團隊共同確認程式可編譯、功能可執行，並準備評測當天的程式操作與問題回答。

## 6. Git 貢獻紀錄

- `s411386011-art`：測試、Benchmark、異常封包測試、Windows 相容性修正與驗證文件。
- `陳皓祥`：Frame、UTF-8、Huffman 編解碼與 WAV 邊界處理。
- `LiJheChen`：Frame 溢位修正、Huffman 安全性改善、回歸測試及介面文件。
- `s411386008`（P2）：負責壓縮率分析、效能結果說明與口頭報告。截至本次 Git 紀錄檢查，尚未找到以該成員身分提交的 C 程式修改；實際完成的工作仍須依其本人提供的成果確認。
- `s411386012-art` 與 `陳皓祥` 為同一位成員的不同 Git 作者身分，相關貢獻合併歸屬陳皓祥。


本文件依據 Git commit 紀錄與已完成的驗證工作整理，未確認事項不視為已完成。
