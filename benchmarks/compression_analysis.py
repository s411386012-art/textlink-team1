
from pathlib import Path
from collections import Counter
import csv
import math
import struct
import wave

ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = ROOT / "benchmark_real"
OUTPUT = ROOT / "benchmarks" / "figures" / "compression_entropy.csv"

FILES = [
    ("real_chinese.txt", "CHAR"),
    ("real_english.txt", "CHAR"),
    ("audio_music_20s.wav", "S16"),
    ("audio_noise.wav", "S16"),
]


def get_symbols(data, mode, path):
    if mode == "BYTE":
        return list(data)

    if mode == "CHAR":
        text = data.decode("utf-8", errors="strict")
        return [ord(ch) for ch in text]

    if mode == "S16":
        with wave.open(str(path), "rb") as wav:
            if wav.getsampwidth() != 2:
                raise ValueError("S16 requires 16-bit PCM")
            if wav.getcomptype() != "NONE":
                raise ValueError("WAV must be uncompressed PCM")
            pcm = wav.readframes(wav.getnframes())

        if len(pcm) % 2:
            raise ValueError("Invalid 16-bit sample length")

        # C 程式以 little-endian 讀取 WAV PCM sample
        return [
            value[0]
            for value in struct.iter_unpack("<H", pcm)
        ]

    raise ValueError("Unknown symbol mode")


def calculate_entropy(symbols):
    n = len(symbols)
    counts = Counter(symbols)
    k = len(counts)

    if n == 0:
        return n, k, None

    entropy = 0.0

    for count in counts.values():
        p = count / n
        entropy -= p * math.log2(p)

    return n, k, entropy


def main():
    rows = []

    for filename, native_mode in FILES:
        path = DATA_DIR / filename
        data = path.read_bytes()

        # 原本的符號模式，以及 BYTE 對照模式
        for mode in [native_mode, "BYTE"]:
            symbols = get_symbols(data, mode, path)
            n, k, h = calculate_entropy(symbols)

            theoretical_bits = h * n if h is not None else 0
            theoretical_ratio = (
                theoretical_bits / 8 / len(data)
                if len(data) else None
            )

            row = {
                "filename": filename,
                "mode": mode,
                "file_bytes": len(data),
                "N": n,
                "K": k,
                "entropy_H": round(h, 6) if h is not None else "",
                "entropy_ratio": round(theoretical_ratio, 6)
                if theoretical_ratio is not None else "",
            }

            rows.append(row)

            print(
                f"{filename:23} "
                f"{mode:4} "
                f"N={n:9} "
                f"K={k:6} "
                f"H={h:.4f}"
                if h is not None
                else f"{filename} {mode} N=0 H=undefined"
            )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)

    with OUTPUT.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print()
    print("CSV created:", OUTPUT)


if __name__ == "__main__":
    main()
