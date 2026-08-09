#!/usr/bin/env python3
"""
HTTP receiver for Titan face-triggered camera frames.

Board flow:
    cam_upload_set <PC_IP> 5000
    cam_face_upload_loop

PC flow:
    python script/receive_face_http.py --host 0.0.0.0 --port 5000
"""

from __future__ import annotations

import argparse
import datetime as _dt
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


def rgb565_to_rgb888(raw: bytes, width: int, height: int) -> bytes:
    expected = width * height * 2
    if len(raw) != expected:
        raise ValueError(f"bad raw size: got {len(raw)}, expected {expected}")

    out = bytearray(width * height * 3)
    j = 0
    for i in range(0, len(raw), 2):
        p = raw[i] | (raw[i + 1] << 8)
        r5 = (p >> 11) & 0x1F
        g6 = (p >> 5) & 0x3F
        b5 = p & 0x1F
        out[j] = (r5 << 3) | (r5 >> 2)
        out[j + 1] = (g6 << 2) | (g6 >> 4)
        out[j + 2] = (b5 << 3) | (b5 >> 2)
        j += 3
    return bytes(out)


def save_image(rgb: bytes, width: int, height: int, faces: int, out_dir: Path) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    stamp = _dt.datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
    try:
        from PIL import Image

        path = out_dir / f"face_http_{stamp}_{faces}faces.png"
        Image.frombytes("RGB", (width, height), rgb).save(path)
        return path
    except ImportError:
        path = out_dir / f"face_http_{stamp}_{faces}faces.ppm"
        with path.open("wb") as f:
            f.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
            f.write(rgb)
        return path


class UploadHandler(BaseHTTPRequestHandler):
    output_dir: Path = Path("captures_http")

    def do_POST(self) -> None:
        if self.path != "/upload":
            self.send_error(404, "use /upload")
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
            width = int(self.headers.get("X-Width", "640"))
            height = int(self.headers.get("X-Height", "480"))
            faces = int(self.headers.get("X-Faces", "0"))
            fmt = self.headers.get("X-Format", "RGB565")
        except ValueError:
            self.send_error(400, "bad metadata headers")
            return

        if fmt.upper() != "RGB565":
            self.send_error(415, f"unsupported format {fmt}")
            return

        raw = self.rfile.read(length)
        try:
            rgb = rgb565_to_rgb888(raw, width, height)
            path = save_image(rgb, width, height, faces, self.output_dir)
        except Exception as exc:
            self.send_error(400, str(exc))
            return

        body = f"saved {path}\n".encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)
        print(f"saved {path} ({width}x{height}, faces={faces}, raw={length} bytes)")

    def log_message(self, fmt: str, *args) -> None:
        print("%s - %s" % (self.address_string(), fmt % args))


def main() -> int:
    parser = argparse.ArgumentParser(description="Receive Titan RGB565 face frames over HTTP")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=5000)
    parser.add_argument("--out", default="captures_http")
    args = parser.parse_args()

    UploadHandler.output_dir = Path(args.out)
    server = ThreadingHTTPServer((args.host, args.port), UploadHandler)
    print(f"Listening on http://{args.host}:{args.port}/upload")
    print("Use Windows ipconfig to find your PC IPv4, then on board:")
    print(f"  cam_upload_set <PC_IP> {args.port}")
    print("  cam_face_upload_loop")
    print("Press Ctrl+C to stop.")
    server.serve_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
