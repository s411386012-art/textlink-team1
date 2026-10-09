
from pathlib import Path
from collections import Counter
import csv
import math
import struct

from compression_analysis import (
    FILES, DATA_DIR, get_symbols, calculate_entropy
)

ROOT = Path(__file__).resolve().parent
ENCODED_DIR = ROOT / "encoded"
OUTPUT = ROOT / "figures" / "compression_actual.csv"

ENCODED_NAMES = {
    "real_chinese.txt": "real_chinese",
    "real_english.txt": "real_english",
    "audio_music_20s.wav": "audio_music",
    "audio_noise.wav": "audio_noise",
}

WIRE_BYTES = {
    "real_chinese.txt": 388786,
    "real_english.txt": 679187,
    "audio_music_20s.wav": 3828268,
    "audio_noise.wav": 1574998,
}

MODE_ID = {"BYTE": 0, "CHAR": 1, "S16": 2}
SYM_SIZE = {"BYTE": 1, "CHAR": 4, "S16": 2}


def be32(data, offset):
    return struct.unpack_from(">I", data, offset)[0]


def analyze(filename, mode):
    path = DATA_DIR / filename
    original = path.read_bytes()

    prefix = ENCODED_NAMES[filename]
    encoded_path = ENCODED_DIR / (
        prefix + "_" + mode.lower() + ".bin"
    )
    encoded = encoded_path.read_bytes()

    if len(encoded) < 13:
        raise ValueError("Encoded header too short")

    sym = encoded[0]
    orig_len = be32(encoded, 1)
    n_header = be32(encoded, 5)
    k_header = be32(encoded, 9)

    assert sym == MODE_ID[mode], "Symbol mode mismatch"
    assert orig_len == len(original), "File size mismatch"

    pos = 13
    wav_extra_bytes = 0

    if mode == "S16":
        if len(encoded) < pos + 8:
            raise ValueError("Incomplete WAV metadata")

        hdr_len = be32(encoded, pos)
        tail_len = be32(encoded, pos + 4)
        wav_extra_bytes = 8 + hdr_len + tail_len
        pos += wav_extra_bytes

        if pos > len(encoded):
            raise ValueError("Invalid WAV metadata length")

    codebook_start = pos
    lengths = {}

    for _ in range(k_header):
        sym_size = SYM_SIZE[mode]

        if pos + sym_size + 1 > len(encoded):
            raise ValueError("Truncated codebook record")

        symbol = int.from_bytes(
            encoded[pos:pos + sym_size], "big"
        )
        pos += sym_size

        code_length = encoded[pos]
        pos += 1

        if not 1 <= code_length <= 64:
            raise ValueError("Invalid code length")

        code_size = 4 if code_length <= 32 else 8

        if pos + code_size > len(encoded):
            raise ValueError("Truncated Huffman code")

        pos += code_size

        if symbol in lengths:
            raise ValueError("Duplicate symbol")

        lengths[symbol] = code_length

    codebook_bytes = pos - codebook_start
    bitstream_bytes = len(encoded) - pos

    symbols = get_symbols(original, mode, path)
    counts = Counter(symbols)
    n, k, h = calculate_entropy(symbols)

    assert n == n_header, "Symbol count mismatch"
    assert k == k_header, "Unique symbol count mismatch"
    assert set(counts) == set(lengths), "Codebook mismatch"

    total_bits = sum(
        freq * lengths[symbol]
        for symbol, freq in counts.items()
    )

    expected_bitstream = (total_bits + 7) // 8
    assert bitstream_bytes == expected_bitstream, (
        "Bitstream size mismatch"
    )

    padding_bits = bitstream_bytes * 8 - total_bits
    L = total_bits / n if n else None

    if n:
        assert h - 1e-8 <= L < h + 1 + 1e-8, (
            "Huffman entropy bound failed"
        )

    fixed_header_bytes = 13

    reconstructed = (
        fixed_header_bytes
        + wav_extra_bytes
        + codebook_bytes
        + bitstream_bytes
    )
    assert reconstructed == len(encoded)

    file_bytes = len(original)
    native_mode = dict(FILES)[filename]

    wire_bytes = (
        WIRE_BYTES[filename]
        if mode == native_mode else None
    )

    row = {
        "filename": filename,
        "mode": mode,
        "file_bytes": file_bytes,
        "N": n,
        "K": k,
        "H": round(h, 6) if h is not None else "",
        "L": round(L, 6) if L is not None else "",
        "pure_bits": total_bits,
        "pure_bitstream_bytes": total_bits / 8,
        "bitstream_bytes": bitstream_bytes,
        "padding_bits": padding_bits,
        "codebook_bytes": codebook_bytes,
        "fixed_header_bytes": fixed_header_bytes,
        "wav_extra_bytes": wav_extra_bytes,
        "encoded_bytes": len(encoded),
        "entropy_ratio": (
            round(h * n / 8 / file_bytes, 6)
            if h is not None and file_bytes else ""
        ),
        "pure_ratio": (
            round(total_bits / 8 / file_bytes, 6)
            if file_bytes else ""
        ),
        "encoded_ratio": (
            round(len(encoded) / file_bytes, 6)
            if file_bytes else ""
        ),
        "wire_bytes": (
            wire_bytes if wire_bytes is not None else ""
        ),
        "frame_overhead_bytes": (
            wire_bytes - len(encoded)
            if wire_bytes is not None else ""
        ),
        "wire_ratio": (
            round(wire_bytes / file_bytes, 6)
            if wire_bytes is not None and file_bytes else ""
        ),
    }

    print(
        f"{filename:23} {mode:4} "
        f"N={n:8} K={k:6} "
        f"H={h:.4f} L={L:.4f} "
        f"Codebook={codebook_bytes:7} "
        f"Bitstream={bitstream_bytes:8} "
        f"Encoded={len(encoded):8}"
    )

    return row


def main():
    rows = []

    for filename, native_mode in FILES:
        for mode in (native_mode, "BYTE"):
            rows.append(analyze(filename, mode))

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)

    with OUTPUT.open(
        "w", newline="", encoding="utf-8-sig"
    ) as f:
        writer = csv.DictWriter(
            f, fieldnames=list(rows[0].keys())
        )
        writer.writeheader()
        writer.writerows(rows)

    print()
    print("All 8 encoded files verified.")
    print("CSV created:", OUTPUT)


if __name__ == "__main__":
    main()

