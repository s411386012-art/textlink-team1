# TextLink 驗收檢查表（V）

請依實際執行結果填寫，未執行不得標 PASS。

| 驗收 | 測試 | 結果 | 證據 |
|---|---|---|---|
| 0 | 雙機指定 IP/Port 連線 | PASS（雙機） | 電腦 B `192.168.0.188` 連線至電腦 A `192.168.0.140:5000`，TCP 連線及資料傳輸成功 |
| 1 | RAW/HUFF 聊天（中文、英文、Emoji、4-byte UTF-8） | PASS（雙機） | 雙機聊天截圖；中文、英文及 Emoji 正確傳送，RAW/HUFF 切換成功 |
| 2 | TCP 半包與黏包 | PASS（本機） | TCP-05、TCP-06；拆包與黏包均成功還原 4096 bytes，ACK 成功 |
| 3 | TXT：CLI `send/recv` 與聊天 `/send`；RAW/HUFF 無損傳輸 | PASS（雙機） | 中文及英文 TXT CLI RAW/HUFF 傳輸成功；中文 TXT 聊天 `/send` RAW/HUFF 成功；SHA-256 一致 |
| 4 | WAV：CLI `send/recv` 與聊天 `/send`；RAW/HUFF 無損傳輸及播放 | PASS（雙機） | 音樂 WAV CLI 與聊天 `/send` RAW/HUFF 成功，SHA-256 一致且可播放；雜訊 WAV CLI RAW/HUFF SHA-256 一致 |
| 5 | 空檔、單符號、256 bytes、BOM/CRLF、特殊 WAV、BYTE fallback | PASS（已測案例） | `tests/edge_files/` 邊界資料及本機測試；奇數 WAV、零 Sample WAV 等補充案例 |
| 5a | `sym=char` / `sym=s16`；中文 Ratio 門檻 | PASS（本機＋雙機） | 中文 HUFF `sym=char`；16-bit WAV HUFF `sym=s16`；雙機中文 HUFF ratio=0.3254 |
| 6 | 長度 0／超限、未知 Type、斷線、損壞 Codebook、非法 UTF-8、Padding | PASS（已測案例） | TCP-01～04、TCP-07、損壞 Padding 拒絕測試、UTF-8 與 Huffman 邊界測試；詳細案例參閱 `docs/testing.md` TCP-01～04、TCP-07、損壞 Padding、非法 UTF-8、Huffman 邊界測試；另以 `tests/test_tcp_disconnect.py` 驗證 RAW 傳輸至 25% 時斷線，Receiver Exit Code 為 1，沒有留下不完整檔案。詳細紀錄參閱 `docs/testing.md`。|
| 7 | STATS 欄位、Ratio 驗算、成功／失敗 Exit Code | PASS（本機＋雙機已測案例） | 114 項單元測試及相關 TCP 驗證；雙機 40 對 Sender/Receiver STATS 配對成功 |
| 7a | RAW ratio 1.0000～1.0100；中文 HUFF ratio 小於 1 | PASS（本機＋雙機） | 雙機 RAW ratio≈1.0001、中文 HUFF ratio≈0.3254 |
| 8 | 乾淨 Clone：Make、Make Test、無警告 | PASS（已驗證環境） | 乾淨 Clone 編譯及測試成功，114 PASS、0 FAIL、0 TODO |

**驗收範圍說明：** 上述「PASS（雙機）」代表已完成實際跨電腦驗證；「PASS（本機）」或「PASS（已測案例）」代表已有相應測試證據，不表示所有異常情境均已於雙機環境重新執行。完整測試過程、原始資料及紀錄請參閱 `docs/testing.md`。

## 雙機測試紀錄（2026-10-10）

**測試環境**

- 測試日期：2026-10-10
- 接收端 IP：`192.168.0.140`
- 傳送端 IP：`192.168.0.188`
- TCP Port：`5000`
- 網路：兩台實體電腦、區域網路
- 傳輸協定：TCP
- 實際 OS、網路連線方式及兩端 Git Commit SHA：待補

### 雙機功能驗收結果

| 驗收項目 | 測試內容 | 結果 |
|---|---|---|
| 0 | 指定 IP/Port 建立雙機連線 | PASS |
| 1 | RAW/HUFF 中文、英文、4-byte Emoji 聊天 | PASS |
| 3 | 中文及英文 TXT：CLI RAW/HUFF、SHA-256 | PASS |
| 3 | 中文 TXT：聊天 `/send` RAW/HUFF、SHA-256 | PASS |
| 4 | 音樂 WAV：CLI RAW/HUFF、SHA-256、播放 | PASS |
| 4 | 雜訊 WAV：CLI RAW/HUFF、SHA-256 | PASS |
| 4 | 音樂 WAV：聊天 `/send` RAW/HUFF、SHA-256、播放 | PASS |
| 7 | STATS 正常輸出與傳輸比例驗算 | PASS（已驗證案例） |
| 7a | RAW ratio 約 1.0001、中文 HUFF ratio 小於 1 | PASS |

### 雙機測試結論

TextLink 已在兩台實體電腦之間完成 TCP 連線、RAW/HUFF 文字聊天、TXT/WAV 命令列檔案傳輸，以及聊天介面的 `/send` 檔案傳輸。

測試中的檔案均以 SHA-256 比對確認傳送前及接收後內容相同。音樂 WAV 可正常播放，聊天介面在傳送大型檔案後仍可持續雙向通訊。

本節為跨電腦功能驗證；正式效能量測的重複次數、環境及結果應另行記錄，不與 localhost Benchmark 混用。