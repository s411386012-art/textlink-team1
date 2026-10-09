> **文件定位：V 角色工作交接紀錄**
>
> 本文件保留測試驗證角色原先的工作範圍、
> 資料交接要求與提交注意事項。
>
> 最新的專案完成狀態請以根目錄 README.md 為準；
> 詳細測試結果請參閱 docs/testing.md；
> Huffman 壓縮分析請參閱 docs/compression_report.md。

# TextLink — V（測試驗證）提交資料

本資料包只包含 **V 角色的附加文件與數據**，請合併到既有 team repo，**不要覆蓋組員的 src/、include/、tests/、Makefile、README.md**。

- `docs/testing.md`：既有測試方法與結果，部分證據待補。
- `docs/verification_checklist.md`：依老師 0–8 項最低驗收逐項填寫。
- `docs/report_data_requirements.md`：報告所需圖表與尚缺的數據。
- `benchmarks/formal_raw_40.csv`：從既有截圖轉錄的 40 筆原始數據。
- `benchmarks/formal_medians.csv`：**老師要求的中位數**，報告主要採用。
- `benchmarks/formal_benchmark_40.xlsx`：平均值供補充，不能取代中位數。
- `benchmarks/run_benchmark.ps1`：單次 sender 記錄輔助腳本；**不是已使用的自動量測工具**，須先實測。
- `benchmarks/figures/`：請放 Excel 匯出的三張圖，另補規格要求的 WAV histogram 和文字 top-30 字元機率圖。

團隊根目錄仍須備妥：`src/`、`include/`、`tests/`、建置檔、`README.md`、`docs/interface.md`、`TEAM_LOG.md`、`CONTRIBUTIONS.md`、`AI_USAGE.md`、`slides.pdf`。

每人都須有實際 C 程式 commit；請在 `CONTRIBUTIONS.md` 依真實貢獻記錄，不要把模板當作完成證據。提交時登錄 repo URL 和完整 commit SHA。

