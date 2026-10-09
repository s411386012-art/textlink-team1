# 提供給口頭報告 P 的數據與待補項目

## 已有：Loopback 效能
- `benchmarks/formal_raw_40.csv`：4 檔 × RAW/HUFF × 5 次，40 筆。
- `benchmarks/formal_medians.csv`：依課程要求，**中位數**為主要報告數據。
- `benchmarks/formal_benchmark_40.xlsx`：原先整理的平均值，只能作補充，不能取代中位數。
- `benchmarks/figures/`：請將已做好的 3 張 Excel 圖表匯出 PNG。
- 測試環境：Windows 單機 127.0.0.1:5000，非雙機量測。

## 仍需補齊（課程規格要求）
1. 四個檔案須確認有「中文為主」及「英文為主」的文字檔、語音/音樂 WAV、自選檔。`text_repeat`、`text_mixed` 的名稱不足以證明語言比例。
2. **雙機量測**：若先前僅測試能連線，尚須各檔案 RAW/HUFF 各 5 次的雙機數據；記錄兩端 OS、網路、IP 網段。
3. **兩端 total_ms**：目前 CSV 只有發送端 total_ms，接收端 total_ms 尚未納入。
4. 壓縮率分析表：N、K、H、L、理論/純編碼壓縮率、codebook bytes、byte-symbol 對照、實際 ratio；需從程式/分析指令取得，不能由 STATS 推測。
5. WAV sample histogram、文字最常見 30 字元機率圖；三張既有圖不取代這兩張。
6. 至少 5 則不同長度聊天訊息的原始 bytes / 上線 bytes（含 codebook）。
7. 損益平衡頻寬計算與壓縮是否加速的討論。
8. 測試截圖、make test log、異常輸入證據、SHA。

## 主要觀察
- text_repeat HUFF ratio=0.4873；text_mixed=0.5882；audio_sine=0.4214；audio_noise=1.4062。
- 在 loopback，Huffman 不保證較快；高熵噪聲音訊反而膨脹。
- 本文件不能當作已完成的測試證明。

