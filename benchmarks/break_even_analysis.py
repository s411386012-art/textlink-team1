
import csv
import statistics
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / "benchmarks" / "formal_raw_40.csv"
OUTPUT = ROOT / "benchmarks" / "figures" / "break_even.csv"


def median(values):
    return statistics.median(values)


def main():
    groups = defaultdict(lambda: {"raw": [], "huff": []})

    with INPUT.open("r", encoding="utf-8-sig", newline="") as f:
        for row in csv.DictReader(f):
            groups[row["file"]][row["mode"].lower()].append(row)

    results = []

    for filename, modes in groups.items():
        raw = modes["raw"]
        huff = modes["huff"]

        if not raw or not huff:
            raise ValueError(f"Missing RAW/HUFF data: {filename}")

        raw_wire = median([int(r["wire_bytes"]) for r in raw])
        huff_wire = median([int(r["wire_bytes"]) for r in huff])

        # Each HUFF trial has its own encode/decode measurements.
        extra_times = [
            float(r["encode_ms"]) + float(r["decode_ms"])
            for r in huff
        ]
        extra_ms = median(extra_times)

        encode_ms = median([float(r["encode_ms"]) for r in huff])
        decode_ms = median([float(r["decode_ms"]) for r in huff])

        raw_total_ms = median([
            float(r["sender_total_ms"]) for r in raw
        ])
        huff_total_ms = median([
            float(r["sender_total_ms"]) for r in huff
        ])

        saved_bytes = raw_wire - huff_wire

        if saved_bytes > 0 and extra_ms > 0:
            break_even_mbps = 0.008 * saved_bytes / extra_ms
            status = "theoretical_break_even"
        elif saved_bytes <= 0 and extra_ms > 0:
            break_even_mbps = ""
            status = "no_positive_break_even"
        else:
            break_even_mbps = ""
            status = "requires_review"

        result = {
            "file": filename,
            "raw_trials": len(raw),
            "huff_trials": len(huff),
            "raw_wire_bytes": raw_wire,
            "huff_wire_bytes": huff_wire,
            "saved_bytes": saved_bytes,
            "encode_median_ms": round(encode_ms, 3),
            "decode_median_ms": round(decode_ms, 3),
            "extra_median_ms": round(extra_ms, 3),
            "raw_sender_median_ms": round(raw_total_ms, 3),
            "huff_sender_median_ms": round(huff_total_ms, 3),
            "break_even_mbps": (
                round(break_even_mbps, 3)
                if break_even_mbps != "" else ""
            ),
            "status": status,
        }

        results.append(result)

        bw = (
            f"{break_even_mbps:.2f} Mbps"
            if break_even_mbps != ""
            else "N/A"
        )

        print(
            f"{filename:22} "
            f"saved={saved_bytes:>9,.0f} B  "
            f"extra={extra_ms:>7.2f} ms  "
            f"break-even={bw}"
        )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)

    with OUTPUT.open("w", encoding="utf-8-sig", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)

    print()
    print("CSV created:", OUTPUT)


if __name__ == "__main__":
    main()
