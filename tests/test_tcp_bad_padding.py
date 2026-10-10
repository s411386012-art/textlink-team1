
import socket
import struct
from pathlib import Path

HOST = "127.0.0.1"
PORT = 5000

ROOT = Path(__file__).resolve().parent.parent
HUFF_FILE = ROOT / "tests" / "edge_files" / "padding_bad.huff"
OUTPUT_FILE = ROOT / "out" / "padding_bad_output.bin"

T_FILE_BEGIN = 0x10
T_FILE_DATA = 0x11
T_FILE_END = 0x12


def make_frame(frame_type, payload=b""):
    length = 1 + len(payload)
    return struct.pack(">I", length) + bytes([frame_type]) + payload


def recv_exact(sock, size):
    data = bytearray()

    while len(data) < size:
        part = sock.recv(size - len(data))
        if not part:
            raise ConnectionError("Connection closed unexpectedly")
        data.extend(part)

    return bytes(data)


def main():
    encoded = HUFF_FILE.read_bytes()

    assert len(encoded) == 20, "Unexpected encoded size"
    assert encoded[-1] == 0x01, "Padding was not corrupted"

    if OUTPUT_FILE.exists():
        raise RuntimeError(
            "Old output exists. Delete out/padding_bad_output.bin first."
        )

    # FILE_BEGIN:
    # mode=1 (HUFF)
    # original size=5 bytes
    # encoded size=20 bytes
    # filename
    filename = b"padding_bad_output.bin"

    begin_payload = (
        bytes([1])
        + struct.pack(">Q", 5)
        + struct.pack(">Q", len(encoded))
        + filename
    )

    with socket.create_connection((HOST, PORT), timeout=10) as sock:
        sock.settimeout(10)

        sock.sendall(make_frame(T_FILE_BEGIN, begin_payload))
        sock.sendall(make_frame(T_FILE_DATA, encoded))
        sock.sendall(make_frame(T_FILE_END))

        header = recv_exact(sock, 5)

        length = struct.unpack(">I", header[:4])[0]
        frame_type = header[4]
        payload = recv_exact(sock, length - 1)

    print("ACK Type:", hex(frame_type))
    print("ACK Payload:", payload.hex())
    print("Output exists:", OUTPUT_FILE.exists())

    if (
        frame_type == T_FILE_END
        and payload == b"\x01"
        and not OUTPUT_FILE.exists()
    ):
        print("PASS: Invalid Huffman padding correctly rejected")
    else:
        raise AssertionError(
            "FAIL: Invalid padding was not correctly rejected"
        )


if __name__ == "__main__":
    main()
