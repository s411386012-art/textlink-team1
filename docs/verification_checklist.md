# TextLink 驗收檢查表（V）

請依實際執行結果填寫，未執行不得標 PASS。

| 驗收 | 測試 | 結果 | 證據 |
|---|---|---|---|
| 0 | 雙機指定 IP/port 連線 | 待測 | 待補雙機 IP、port、連線結果 |
| 1 | RAW/HUFF 聊天（中文、emoji、4-byte） | 待測 | 待補雙機聊天測試 |
| 2 | TCP 半包與黏包 | PASS（本機） | TCP-05、TCP-06；4096 bytes 還原一致，ACK 成功 |
| 3 | TXT：send/recv 與聊天 `/send`；RAW/HUFF 逐 byte | 部分完成 | 本機 CLI RAW/HUFF `fc /b` 成功；雙機及 `/send` 待補 |
| 4 | WAV：send/recv 與聊天 `/send`；RAW/HUFF 逐 byte、播放 | 部分完成 | 本機 RAW/HUFF 完整性驗證；雙機與播放證據待補 |
| 5 | 空檔、單符號、256 byte、BOM/CRLF、異常 WAV、BYTE fallback | PASS（已測案例） | `tests/edge_files/` 七組資料 |
| 5a | sym=char / sym=s16；中文 ratio 門檻 | PASS（本機） | 中文 HUFF sym=char，WAV HUFF sym=s16，中文 ratio≈0.3239 |
| 6 | 長度 0/超限、未知 type、斷線、壞 Codebook、非法 UTF-8 | PASS（已測案例） | TCP-01～04、07；非法 UTF-8 fallback 測試 |
| 7 | STATS 欄位、ratio 驗算、失敗 exit code | PASS（本機） | 正式 Benchmark CSV、成功 exit 0、失敗 exit 1 |
| 7a | RAW ratio 1.0000～1.0100；中文 HUFF ratio < 1 | PASS（本機） | RAW≈1.000115；HUFF≈0.3239 |
| 8 | 乾淨 clone：make、make test、無警告 | PASS | 乾淨 clone 編譯成功；114 PASS、0 FAIL、0 TODO |

## 雙機測試紀錄
- 日期：
- 發送端 OS / 接收端 OS：
- 網路類型：
- 發送端 IP 網段 / 接收端 IP 網段：
- 指令與結果：
- 截圖或 log：
- 對應 commit SHA：
