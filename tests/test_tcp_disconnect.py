
import argparse
import socket
import struct
import time
from pathlib import Path

TYPE_FILE_BEGIN = 0x10
TYPE_FILE_DATA = 0x11

DECLARED_SIZE = 4096
ACTUAL_SIZE = 1024
FILENAME = b"disconnect_test.bin"


def send_frame(sock, frame_type, payload):
    # length includes 1-byte type and payload, but not itself
    length = 1 + len(payload)
    frame = struct.pack(">I", length) + bytes([frame_type]) + payload
    sock.sendall(frame)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=5002)
    args = parser.parse_args()

    # FILE_BEGIN payload:
    # mode(1) + orig_size(8) + data_size(8) + filename
    mode = 0  # RAW
    begin_payload = (
        bytes([mode])
        + struct.pack(">Q", DECLARED_SIZE)
        + struct.pack(">Q", DECLARED_SIZE)
        + FILENAME
    )

    data_payload = b"A" * ACTUAL_SIZE

    with socket.create_connection(
        (args.host, args.port), timeout=10
    ) as sock:
        send_frame(sock, TYPE_FILE_BEGIN, begin_payload)
        send_frame(sock, TYPE_FILE_DATA, data_payload)

        print("Connected:", args.host, args.port)
        print("FILE_BEGIN declared:", DECLARED_SIZE, "bytes")
        print("FILE_DATA sent:", ACTUAL_SIZE, "bytes")
        print("FILE_END: not sent")

        # Allow receiver to consume the partial data.
        time.sleep(0.3)

        # Closing without FILE_END simulates interrupted transfer.

    print("TCP connection closed intentionally.")
    print("Expected: receiver fails with nonzero exit code.")
    print("Expected: no completed output file.")


if __name__ == "__main__":
    main()
