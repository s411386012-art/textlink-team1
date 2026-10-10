# TEAM_LOG.md — TextLink 團隊開發紀錄

## 專案資訊

- 專案名稱：TextLink
- 專案功能：TCP 文字聊天與檔案傳輸，支援 RAW 與 Huffman 編碼模式
- 團隊分工：P（報告）、D（展示整合）、V（測試驗證）

## 開發與驗證紀錄

| 日期 | 工作項目 | 執行角色 | 結果 |
|---|---|---|---|
| 2026/10/08 | 建立 TXT、WAV 效能測試資料 | V | 完成四種測試檔案 |
| 2026/10/08 | 執行 RAW/HUFF 效能比較 | V | 完成 40 次 localhost 量測 |
| 2026/10/08 | 整理傳輸量、壓縮率與時間數據 | V | 完成 CSV 統計及比較圖 |
| 2026/10/09 | 新增 Frame、UTF-8、Huffman 邊界測試 | V | 單元測試達 114 PASS |
| 2026/10/09 | 測試 TCP 異常封包與中斷連線 | V | 完成 TCP-01～TCP-04 測試 |
| 2026/10/09 | 驗證 Windows MinGW 編譯相容性 | V／整合 | 修正 IPv4 轉換與終端機旗標問題 |
| 2026/10/09 | 重現 RAW/HUFF 傳輸及檔案比對 | V | 傳輸成功、逐 byte 一致 |
| 2026/10/09 | 整理測試報告與量測腳本 | V | 更新測試文件與 GitHub |
| 2026/10/10 | 完成兩台實體電腦 RAW/HUFF 正式量測 | V | 四種檔案、兩種模式、各五次，共 40 次雙機 Benchmark；累計正式量測 80 次 |
| 2026/10/10 | 雙機功能及檔案完整性驗證 | V／D | TXT/WAV RAW/HUFF 傳輸、SHA-256 驗證、聊天功能及 `--bind` 回歸測試通過 |
| 2026/10/10 | 修正 Windows 重複監聽 Port 問題 | V／整合 | Windows 使用 `SO_EXCLUSIVEADDRUSE`；第二個 Receiver 正確回報 Port 被占用 |
| 2026/10/10 | CLI 監聽 Port 邊界驗證 | V | 拒絕 Port 1023、接受 Port 1024，符合課程要求 |
| 2026/10/10 | 補強 `FILE_END` Payload 驗證 | V | CLI 與 Chat 均能拒絕非法非空結束封包，不產生錯誤輸出檔 |
| 2026/10/10 | RAW/HUFF 正常傳輸回歸 | V | CLI 與 Chat RAW/HUFF 正常傳檔及確認流程均通過 |
| 2026/10/10 | 更新壓縮率分析與雙機報告 | V | 補充理論壓縮率、Codebook 成本、40 次雙機效能分析與研究限制 |

## V — 測試驗證成果

### 單元測試

使用 `mingw32-make test` 執行 `tests/test_codec.c`。

結果：

- PASS：114
- FAIL：0
- TODO：0

測試涵蓋 Frame 封包長度、UTF-8 合法性、Huffman 編碼與解碼，以及不合法資料和邊界條件。

### TCP 異常輸入測試

使用 `tests/test_tcp_malformed.ps1` 產生不完整或不合法的 TCP 資料，觀察接收端能否安全處理異常情況。

測試案例：TCP-01～TCP-04。

詳細測試方式及觀察結果請參閱 `docs/testing.md`。

### 效能量測

使用四種測試檔案：

- `real_chinese.txt`
- `real_english.txt`
- `audio_music_20s.wav`
- `audio_noise.wav`

每種檔案分別執行 RAW 與 HUFF 模式，每種模式重複五次，共 40 次正式量測。

量測項目包含：

- `wire_bytes`
- `ratio`
- `encode_ms`
- `send_ms`
- `total_ms`
- `decode_ms`

量測原始資料與統計結果保存在 `benchmarks/`。

### 傳輸完整性驗證

使用 `fc /b` 比對傳輸前後檔案，確認 RAW 與 HUFF 模式均能正確還原原始資料。

另以 Trial 99 進行量測腳本重現驗證，確認 PowerShell 腳本能正常執行並保存傳送端 `STATS`。

### 測試限制

正式效能量測包含 localhost 40 次及兩台實體電腦 40 次。雙機測試使用 Windows 電腦 A（Ethernet，`192.168.0.140`）與電腦 B（Wi-Fi，`192.168.0.188`）。

不同環境的耗時受到電腦效能、網路條件及測試資料版本影響，因此不將兩種環境的量測差異完全歸因於網路頻寬。詳細結果參閱 `docs/testing.md` 及 `docs/compression_report.md`。

## 其他團隊工作

以下內容由團隊成員依實際工作補充：

- P：系統架構、Huffman 設計說明、效能分析與口頭報告
- D：命令列整合、聊天與檔案傳輸展示、跨電腦操作驗證
- 跨電腦測試日期、環境與結果
- 團隊討論紀錄及重要開發決策

## 相關文件

- `README.md`：專案使用說明
- `docs/interface.md`：通訊介面與資料格式
- `docs/testing.md`：測試方法及驗證結果
- `benchmarks/README.md`：效能量測說明
- `benchmarks/formal_raw_40.csv`：原始量測數據
- `benchmarks/formal_benchmark_40.xlsx`：統計摘要
