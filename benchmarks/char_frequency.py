
from pathlib import Path
from collections import Counter
import csv

files = [
    Path("benchmark_real/real_chinese.txt"),
    Path("benchmark_real/real_english.txt"),
]

output_dir = Path("benchmarks/figures")
output_dir.mkdir(parents=True, exist_ok=True)

for file in files:
    text = file.read_text(encoding="utf-8-sig")

    # 統計全部字元，包括空白、換行和標點
    counts = Counter(text)
    total = sum(counts.values())

    top30 = counts.most_common(30)
    output = output_dir / f"{file.stem}_top30.csv"

    with output.open("w", encoding="utf-8-sig", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["character", "count", "probability", "percentage"])

        for char, count in top30:
            # 讓 Excel 容易辨識空白和換行
            label = {
                " ": "[SPACE]",
                "\n": "[LF]",
                "\r": "[CR]",
                "\t": "[TAB]",
            }.get(char, char)

            probability = count / total

            writer.writerow([
                label,
                count,
                probability,
                probability * 100,
            ])

    print(f"檔案：{file}")
    print(f"總字元數：{total}")
    print(f"不同字元數：{len(counts)}")
    print(f"CSV 已產生：{output}")
    print()
