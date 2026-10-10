
from pathlib import Path
import csv
import re
import statistics

ROOT = Path(__file__).resolve().parent
SEND_DIR = ROOT / "logs_two_pc_sender"
RECV_DIR = ROOT / "logs_two_pc_receiver"

RAW_CSV = ROOT / "two_pc_raw_40.csv"
MEDIAN_CSV = ROOT / "two_pc_medians.csv"

FILES = [
    "real_chinese.txt",
    "real_english.txt",
    "audio_music_20s.wav",
    "audio_noise.wav",
]
MODES = ["raw", "huff"]
TRIALS = range(1, 6)

EXPECTED_SYMBOL = {
    "real_chinese.txt": "char",
    "real_english.txt": "char",
    "audio_music_20s.wav": "s16",
    "audio_noise.wav": "s16",
}

STATS_RE = re.compile(
    r"STATS\s+role=(send|recv)\b[^\r\n]*"
)
FIELD_RE = re.compile(r"([a-z_]+)=([^\s]+)")

INT_FIELDS = ("file_bytes", "wire_bytes")
FLOAT_FIELDS = (
    "ratio", "encode_ms", "send_ms",
    "decode_ms", "total_ms"
)


def parse_log(path, expected_role):
    if not path.is_file():
        raise FileNotFoundError(f"Missing log: {path}")

    raw = path.read_bytes()

    # Handle Windows PowerShell and other text encodings.
    if raw.startswith(b"\xff\xfe"):
        content = raw.decode("utf-16")
    elif raw.startswith(b"\xfe\xff"):
        content = raw.decode("utf-16")
    elif raw.startswith(b"\xef\xbb\xbf"):
        content = raw.decode("utf-8-sig")
    elif raw[:200].count(b"\x00") > 10:
        content = raw.decode("utf-16-le")
    else:
        content = raw.decode("utf-8", errors="replace")

    full_matches = list(STATS_RE.finditer(content))

    if len(full_matches) != 1:
        raise ValueError(
            f"{path.name}: expected exactly 1 STATS line, "
            f"found {len(full_matches)}"
        )

    line = full_matches[0].group(0)
    fields = dict(FIELD_RE.findall(line))

    if fields.get("role") != expected_role:
        raise ValueError(
            f"{path.name}: expected role={expected_role}, "
            f"got {fields.get('role')}"
        )

    for name in INT_FIELDS:
        if name not in fields:
            raise ValueError(f"{path.name}: missing {name}")
        fields[name] = int(fields[name])

    for name in FLOAT_FIELDS:
        if name in fields:
            fields[name] = float(fields[name])

    if "mode" not in fields or "total_ms" not in fields:
        raise ValueError(f"{path.name}: missing mode or total_ms")

    return fields


def mbps(file_bytes, total_ms):
    if total_ms <= 0:
        return 0.0
    return file_bytes / (total_ms * 1000.0)


def build_rows():
    rows = []
    index = 0

    for filename in FILES:
        for mode in MODES:
            for trial in TRIALS:
                index += 1

                sender_name = (
                    f"{index:02d}_{filename}_{mode}_{trial}.txt"
                )
                receiver_name = f"receiver_{index:02d}.txt"

                sender = parse_log(
                    SEND_DIR / sender_name, "send"
                )
                receiver = parse_log(
                    RECV_DIR / receiver_name, "recv"
                )

                # Ensure sender and receiver correspond
                # to the same transfer.
                if sender["mode"] != mode:
                    raise ValueError(
                        f"Transfer {index}: sender mode mismatch"
                    )

                if receiver["mode"] != mode:
                    raise ValueError(
                        f"Transfer {index}: receiver mode mismatch"
                    )

                for key in ("file_bytes", "wire_bytes"):
                    if sender[key] != receiver[key]:
                        raise ValueError(
                            f"Transfer {index}: {key} mismatch: "
                            f"send={sender[key]}, "
                            f"recv={receiver[key]}"
                        )

                expected_sym = (
                    "none" if mode == "raw"
                    else EXPECTED_SYMBOL[filename]
                )

                if sender.get("sym") != expected_sym:
                    raise ValueError(
                        f"Transfer {index}: expected sym="
                        f"{expected_sym}, got {sender.get('sym')}"
                    )

                file_bytes = sender["file_bytes"]
                wire_bytes = sender["wire_bytes"]

                if file_bytes <= 0:
                    raise ValueError(
                        f"Transfer {index}: invalid file size"
                    )

                actual_ratio = wire_bytes / file_bytes

                for side, stats in (
                    ("sender", sender),
                    ("receiver", receiver),
                ):
                    if abs(stats["ratio"] - actual_ratio) > 0.00011:
                        raise ValueError(
                            f"Transfer {index}: {side} ratio mismatch"
                        )

                sender_total = sender["total_ms"]
                receiver_total = receiver["total_ms"]

                row = {
                    "file": filename,
                    "mode": mode,
                    "trial": trial,
                    "environment": "two_pc_lan",
                    "sym": sender["sym"],
                    "file_bytes": file_bytes,
                    "wire_bytes": wire_bytes,
                    "ratio": actual_ratio,
                    "compressed_pct": actual_ratio * 100,
                    "saved_pct": (1 - actual_ratio) * 100,
                    "encode_ms": sender.get("encode_ms", 0.0),
                    "send_ms": sender.get("send_ms", 0.0),
                    "decode_ms": receiver.get("decode_ms", 0.0),
                    "sender_total_ms": sender_total,
                    "receiver_total_ms": receiver_total,
                    "sender_MBps": mbps(file_bytes, sender_total),
                    "receiver_MBps": mbps(file_bytes, receiver_total),
                }

                rows.append(row)

                print(
                    f"[{index:02d}/40] PASS "
                    f"{filename:<20} "
                    f"{mode:<4} "
                    f"trial={trial} "
                    f"sender={sender_total:.1f} ms "
                    f"receiver={receiver_total:.1f} ms"
                )

    return rows


def write_csv(path, rows, columns):
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)


def make_medians(rows):
    result = []

    metric_columns = [
        "file_bytes",
        "wire_bytes",
        "ratio",
        "compressed_pct",
        "saved_pct",
        "encode_ms",
        "send_ms",
        "decode_ms",
        "sender_total_ms",
        "receiver_total_ms",
        "sender_MBps",
        "receiver_MBps",
    ]

    for filename in FILES:
        for mode in MODES:
            group = [
                row for row in rows
                if row["file"] == filename
                and row["mode"] == mode
            ]

            if len(group) != 5:
                raise ValueError(
                    f"{filename} {mode}: expected 5 trials"
                )

            record = {
                "file": filename,
                "mode": mode,
                "environment": "two_pc_lan",
                "trials": len(group),
                "sym": group[0]["sym"],
            }

            for key in metric_columns:
                record[key] = statistics.median(
                    row[key] for row in group
                )

            result.append(record)

    return result


def main():
    if not SEND_DIR.is_dir() or not RECV_DIR.is_dir():
        raise FileNotFoundError(
            "Sender or receiver log directory is missing"
        )

    send_logs = list(SEND_DIR.glob("*.txt"))
    recv_logs = list(RECV_DIR.glob("*.txt"))

    if len(send_logs) != 40 or len(recv_logs) != 40:
        raise ValueError(
            f"Expected 40 sender and 40 receiver logs; "
            f"found {len(send_logs)} and {len(recv_logs)}"
        )

    rows = build_rows()

    if len(rows) != 40:
        raise ValueError("Expected exactly 40 merged rows")

    raw_columns = list(rows[0].keys())
    medians = make_medians(rows)
    median_columns = list(medians[0].keys())

    write_csv(RAW_CSV, rows, raw_columns)
    write_csv(MEDIAN_CSV, medians, median_columns)

    print()
    print("All 40 sender/receiver pairs verified.")
    print(f"Raw CSV: {RAW_CSV}")
    print(f"Median CSV: {MEDIAN_CSV}")

    print()
    print("=== MEDIAN SUMMARY ===")
    for row in medians:
        print(
            f"{row['file']:<20} "
            f"{row['mode']:<4} "
            f"ratio={row['ratio']:.4f} "
            f"sender={row['sender_total_ms']:.1f} ms "
            f"receiver={row['receiver_total_ms']:.1f} ms"
        )


if __name__ == "__main__":
    main()
