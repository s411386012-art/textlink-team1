
import socket
import struct
from pathlib import Path

HOST = "127.0.0.1"
PORT = 5000
NAME = "tcp_bad_codebook.bin"

# TextLink Frame Type
FILE_BEGIN = 0x10
FILE_DATA = 0x11
FILE_END = 0x12


def frame(frame_type, payload=b""):
    length = 1 + len(payload)
    return struct.pack(">I", length) + bytes([frame_type]) + payload


def recv_exact(sock, size):
    result = bytearray()
    while len(result) < size:
        part = sock.recv(size - len(result))
        if not part:
            raise ConnectionError("Connection closed before ACK")
        result.extend(part)
    return bytes(result)


# Huffman SYM_BYTE: symbol type = 0
# Declare: original size=1, symbol count=1, unique symbols=1
huff_header = bytes([0]) + struct.pack(">III", 1, 1, 1)

# Corrupted codebook:
# Symbol 'A' (0x41) has an invalid code length of 0.
bad_record = bytes([0x41, 0, 0, 0, 0, 0])

# One dummy bitstream byte
bad_huffman = huff_header + bad_record + b"\x00"

# FILE_BEGIN payload:
# mode=HUFF(1), original size=1, encoded size, filename
begin = (
    bytes([1])
    + struct.pack(">QQ", 1, len(bad_huffman))
    + NAME.encode("utf-8")
)

packet = (
    frame(FILE_BEGIN, begin)
    + frame(FILE_DATA, bad_huffman)
    + frame(FILE_END)
)

output = Path("out") / NAME
if output.exists():
    raise SystemExit(
        "Remove the previous test output before running TCP-07."
    )

print("[TCP-07] Sending corrupted Huffman codebook.")
print("[TCP-07] Encoded bytes:", len(bad_huffman))

with socket.create_connection((HOST, PORT), timeout=10) as sock:
    sock.settimeout(10)
    sock.sendall(packet)

    header = recv_exact(sock, 5)
    length = struct.unpack(">I", header[:4])[0]
    reply_type = header[4]

    if length < 1 or length > 1024:
        raise RuntimeError("Invalid ACK frame length")

    payload = recv_exact(sock, length - 1)

    print("[TCP-07] Reply type:", hex(reply_type))
    print("[TCP-07] Reply payload:", payload.hex())

    if reply_type != FILE_END or payload != b"\x01":
        raise RuntimeError("Expected FILE_END failure ACK (01)")

if output.exists():
    raise RuntimeError("Receiver unexpectedly created output file")

print("[TCP-07] PASS: corrupted codebook rejected.")
print("[TCP-07] No output file created.")
