
from pathlib import Path
import subprocess
import tempfile
import csv

ROOT = Path(__file__).resolve().parent.parent
DUMPER = ROOT / "dump_huffman.exe"
OUTPUT = ROOT / "benchmarks" / "figures" / "chat_compression.csv"

CHINESE = (ROOT / "benchmark_real" / "real_chinese.txt").read_text(
    encoding="utf-8-sig"
)
ENGLISH = (ROOT / "benchmark_real" / "real_english.txt").read_text(
    encoding="utf-8-sig"
)

LENGTHS = [10, 50, 100, 500, 1000, 2000, 2500]

CASES = {
    "Chinese natural": CHINESE,
    "English natural": ENGLISH,
    "Repeated A": "A" * 3000,
}

MAX_TEXT_BYTES = 8191


def encode_with_c(data, temp_dir):
    src = temp_dir / "chat_input.txt"
    dst = temp_dir / "chat_encoded.bin"

    src.write_bytes(data)

    result = subprocess.run(
        [str(DUMPER), str(src), "char", str(dst)],
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        raise RuntimeError(result.stderr)

    return dst.stat().st_size


def main():
    if not DUMPER.is_file():
        raise FileNotFoundError(
            "Please compile dump_huffman.exe first."
        )

    rows = []

    with tempfile.TemporaryDirectory() as folder:
        temp_dir = Path(folder)

        for case_name, text in CASES.items():
            for count in LENGTHS:
                message = text[:count]
                data = message.encode("utf-8")

                if len(data) > MAX_TEXT_BYTES:
                    continue

                encoded_bytes = encode_with_c(data, temp_dir)

                raw_wire = len(data) + 5
                huff_wire = encoded_bytes + 5

                ratio = huff_wire / len(data) if data else None

                rows.append({
                    "case": case_name,
                    "characters": len(message),
                    "file_bytes": len(data),
                    "raw_wire_bytes": raw_wire,
                    "huff_encoded_bytes": encoded_bytes,
                    "huff_wire_bytes": huff_wire,
                    "huff_ratio": round(ratio, 6),
                    "huff_percentage": round(ratio * 100, 2),
                    "huff_smaller_than_raw": huff_wire < raw_wire,
                    "huff_below_100pct": huff_wire < len(data),
                })

                print(
                    f"{case_name:16} "
                    f"chars={len(message):4} "
                    f"raw={len(data):5} "
                    f"huff_wire={huff_wire:5} "
                    f"ratio={ratio * 100:7.2f}%"
                )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)

    with OUTPUT.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    print()
    print("CSV created:", OUTPUT)


if __name__ == "__main__":
    main()
