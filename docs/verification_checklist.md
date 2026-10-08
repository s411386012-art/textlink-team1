# TextLink 驗收檢查表（V）

請依實際執行結果填寫，未執行不得標 PASS。

| 驗收 | 測試 | 結果 | 證據 |
|---|---|---|---|
| 0 | 雙機指定 IP/port 連線 | 待填 | |
| 1 | RAW/HUFF 聊天（中文、emoji、4-byte） | 待填 | |
| 2 | TCP 半包與黏包 | 待填 | |
| 3 | TXT：send/recv 與聊天 /send；RAW/HUFF 逐 byte | 待填 | |
| 4 | WAV：send/recv 與聊天 /send；RAW/HUFF 逐 byte、播放 | 待填 | |
| 5 | 空檔、單符號、256 byte、BOM/CRLF、異常 WAV、byte fallback | 待填 | |
| 5a | sym=char / sym=s16；中文 ratio 門檻 | 待填 | |
| 6 | 長度 0/超限、未知 type、斷線、壞 codebook、非法 UTF-8 | 待填 | |
| 7 | STATS 欄位、ratio 驗算、失敗 exit code | 待填 | |
| 7a | RAW ratio 1.0000–1.0100；中文 HUFF ratio < 1 | 待填 | |
| 8 | 乾淨 clone：make、make test、無警告 | 待填 | |

## 雙機測試紀錄
- 日期：
- 發送端 OS / 接收端 OS：
- 網路類型：
- 發送端 IP 網段 / 接收端 IP 網段：
- 指令與結果：
- 截圖或 log：
- 對應 commit SHA：
