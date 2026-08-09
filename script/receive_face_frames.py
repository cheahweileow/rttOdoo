#!/usr/bin/env python3
"""
Receive RGB565 frames streamed by the Titan RT-Thread camera demo.

Board command:
    cam_face_send_loop

PC command:
    python script/receive_face_frames.py --port COM3

The script saves PNG when Pillow is installed. If Pillow is missing, it saves
PPM files instead, which can still be opened by many image tools.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import os
import re
import sys
from pathlib import Path


BEGIN_RE = re.compile(
    rb"@@FRAME_BEGIN width=(\d+) height=(\d+) format=RGB565 size=(\d+) faces=(\d+)@@\n"
)
END_MARKER = b"\n@@FRAME_END@@"


def rgb565_to_rgb888(raw: bytes, width: int, height: int) -> bytes:
    expected = width * height * 2
    if len(raw) != expected:
        raise ValueError(f"bad raw size: got {len(raw)}, expected {expected}")

    out = bytearray(width * height * 3)
    j = 0
    for i in range(0, len(raw), 2):
        # Firmware stores uint16_t RGB565 in little-endian memory.
        p = raw[i] | (raw[i + 1] << 8)
        r5 = (p >> 11) & 0x1F
        g6 = (p >> 5) & 0x3F
        b5 = p & 0x1F

        out[j] = (r5 << 3) | (r5 >> 2)
        out[j + 1] = (g6 << 2) | (g6 >> 4)
        out[j + 2] = (b5 << 3) | (b5 >> 2)
        j += 3

    return bytes(out)


def save_image(rgb: bytes, width: int, height: int, faces: int, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    stamp = _dt.datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]

    try:
        from PIL import Image

        path = output_dir / f"face_{stamp}_{faces}faces.png"
        img = Image.frombytes("RGB", (width, height), rgb)
        img.save(path)
        return path
    except ImportError:
        path = output_dir / f"face_{stamp}_{faces}faces.ppm"
        with path.open("wb") as f:
            f.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
            f.write(rgb)
        return path


def read_exact(ser, size: int) -> bytes:
    data = bytearray()
    while len(data) < size:
        chunk = ser.read(size - len(data))
        if not chunk:
            raise TimeoutError(f"serial timeout while reading frame: {len(data)}/{size}")
        data.extend(chunk)
    return bytes(data)


def main() -> int:
    parser = argparse.ArgumentParser(description="Receive face-triggered RGB565 frames from Titan board")
    parser.add_argument("--port", default="COM3", help="Serial port, default COM3")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate, default 115200")
    parser.add_argument("--out", default="captures", help="Output folder, default captures")
    args = parser.parse_args()

    try:
        import serial
    except ImportError:
        print("Missing pyserial. Install with: python -m pip install pyserial", file=sys.stderr)
        return 2

    output_dir = Path(args.out)

    print(f"Opening {args.port} @ {args.baud}. Close RT-Thread terminal first if COM is busy.")
    print("Waiting for @@FRAME_BEGIN... Press Ctrl+C to stop.")

    with serial.Serial(args.port, args.baud, timeout=10) as ser:
        buffer = bytearray()

        while True:
            chunk = ser.read(256)
            if not chunk:
                print("waiting...")
                continue

            buffer.extend(chunk)

            match = BEGIN_RE.search(buffer)
            if not match:
                # Keep enough bytes to match a split header, but drop old terminal logs.
                if len(buffer) > 4096:
                    del buffer[:-256]
                continue

            width = int(match.group(1))
            height = int(match.group(2))
            size = int(match.group(3))
            faces = int(match.group(4))

            del buffer[: match.end()]
            already = bytes(buffer)
            buffer.clear()

            if len(already) >= size:
                raw = already[:size]
                buffer.extend(already[size:])
            else:
                raw = already + read_exact(ser, size - len(already))

            # Consume footer if it is already in the buffer or read until it appears.
            while END_MARKER not in buffer:
                more = ser.read(64)
                if not more:
                    break
                buffer.extend(more)

            marker_index = buffer.find(END_MARKER)
            if marker_index >= 0:
                del buffer[: marker_index + len(END_MARKER)]

            rgb = rgb565_to_rgb888(raw, width, height)
            path = save_image(rgb, width, height, faces, output_dir)
            print(f"saved {path} ({width}x{height}, faces={faces}, raw={size} bytes)")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\nstopped")
