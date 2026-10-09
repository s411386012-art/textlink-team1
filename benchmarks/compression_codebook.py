
from collections import Counter
from pathlib import Path
import csv
import heapq
import itertools
import math

from compression_analysis import (
    FILES, DATA_DIR, get_symbols, calculate_entropy
)

OUTPUT = Path(__file__).resolve().parent / "figures" / "compression_codebook.csv"


def huffman_lengths(counts):
    if not counts:
        return {}

    if len(counts) == 1:
        return {next(iter(counts)): 1}

    order = itertools.count()
    heap = []

    for symbol, freq in counts.items():
        heapq.heappush(heap, (freq, next(order), symbol))

    while len(heap) > 1:
        f1, _, a = heapq.heappop(heap)
        f2, _, b = heapq.heappop(heap)
        heapq.heappush(heap, (f1 + f2, next(order), (a, b)))

    lengths = {}
    stack = [(heap[0][2], 0)]

    while stack:
        node, depth = stack.pop()

        if isinstance(node, tuple):
            left, right = node
            stack.append((left, depth + 1))
            stack.append((right, depth + 1))
        else:
            lengths[node] = depth

    return lengths


rows = []

for filename, native_mode in FILES:
    path = DATA_DIR / filename
    data = path.read_bytes()
    file_bytes = len(data)

    for mode in (native_mode, "BYTE"):
        symbols = get_symbols(data, mode, path)
        counts = Counter(symbols)
        n, k, h = calculate_entropy(symbols)

        lengths = huffman_lengths(counts)

        total_bits = sum(
            freq * lengths[sym]
            for sym, freq in counts.items()
        )

        L = total_bits / n if n else 0

        symbol_bytes = {
            "BYTE": 1,
            "CHAR": 4,
            "S16": 2,
        }[mode]

        codebook_bytes = sum(
            symbol_bytes + 1 + (4 if length <= 32 else 8)
            for length in lengths.values()
        )

        bitstream_bytes = math.ceil(total_bits / 8)

        row = {
            "filename": filename,
            "mode": mode,
            "file_bytes": file_bytes,
            "N": n,
            "K": k,
            "H": round(h, 6) if h is not None else "",
            "L": round(L, 6),
            "entropy_ratio": round(h * n / 8 / file_bytes, 6)
                if h is not None and file_bytes else "",
            "pure_huffman_ratio": round(total_bits / 8 / file_bytes, 6)
                if file_bytes else "",
            "codebook_bytes_est": codebook_bytes,
            "bitstream_bytes": bitstream_bytes,
            "max_code_length": max(lengths.values(), default=0),
            "H_L_check": (
                h - 1e-9 <= L < h + 1 + 1e-9
                if h is not None and n else "N/A"
            ),
        }

        rows.append(row)

        print(
            f"{filename:23} {mode:4} "
            f"H={h:.4f} L={L:.4f} "
            f"Codebook={codebook_bytes} bytes "
            f"Check={row['H_L_check']}"
        )

OUTPUT.parent.mkdir(parents=True, exist_ok=True)

with OUTPUT.open("w", newline="", encoding="utf-8-sig") as f:
    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)

print("\nCSV created:", OUTPUT)
