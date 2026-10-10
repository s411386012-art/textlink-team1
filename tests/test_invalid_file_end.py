
import socket
import struct
import sys

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 5012

def frame(frame_type, payload):
    return struct.pack(">I", len(payload) + 1) + bytes([frame_type]) + payload

name = b"invalid_end_test.bin"
begin = bytes([0]) + struct.pack(">Q", 4) + struct.pack(">Q", 4) + name

with socket.create_connection(("127.0.0.1", PORT), timeout=5) as sock:
    sock.sendall(frame(0x10, begin))
    sock.sendall(frame(0x11, b"ABCD"))
    sock.sendall(frame(0x12, b"\x99"))

print("Invalid FILE_END sent")
