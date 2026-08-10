"""
Titan Board -> Odoo bridge.

Receives the same HTTP POST upload the Titan Board firmware already sends
(raw RGB565 image bytes + X-Width / X-Height / X-Faces / X-Format headers),
converts the image to PNG, and creates a record in Odoo's "Face Captures"
(x_face_captures) Studio model via Odoo's External API (XML-RPC).

Usage:
    1. Fill in the four values in the CONFIG section below.
    2. Run:  python titan_odoo_bridge.py
    3. Point the Titan Board at this PC's IP, port 5000 (same as before):
           cam_upload_set <THIS_PC_IP> 5000
           cam_face_upload_loop 5
    4. Watch this window - each accepted upload prints a line, and a new
       record should appear in Odoo under Face Captures.
"""

import io
import xmlrpc.client
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# ─── CONFIG - fill these in ──────────────────────────────────────────────
ODOO_URL = "https://www.lingkail.com"        # your Odoo site, no trailing slash, no /odoo
ODOO_DB = "lingkail-sdn-bhd-odoo-lingkail-2025-26947647"
ODOO_USERNAME = "cheahwei.leow@lingkail.com"  # the email you log into Odoo with
ODOO_API_KEY = ""  # from the "API Key Ready" popup
LISTEN_PORT = 5000                           # must match cam_upload_set port
# ──────────────────────────────────────────────────────────────────────────


def discover_db_name(url):
    """Try to auto-detect the database name via Odoo's db-list XML-RPC service.
    Some Odoo Online databases disable this for security; in that case we
    fall back to asking the person to fill ODOO_DB in manually."""
    try:
        db_proxy = xmlrpc.client.ServerProxy(f"{url}/xmlrpc/2/db")
        dbs = db_proxy.list()
        if len(dbs) == 1:
            print(f"Auto-detected database name: {dbs[0]}")
            return dbs[0]
        elif len(dbs) > 1:
            print(f"Multiple databases found: {dbs}")
            print("Set ODOO_DB to the correct one from this list and rerun.")
            raise SystemExit(1)
    except Exception:
        pass
    return None

try:
    from PIL import Image
except ImportError:
    raise SystemExit("Missing dependency. Run: pip install pillow")


def rgb565_to_png_bytes(raw_bytes, width, height):
    """Convert raw RGB565 (little-endian) bytes into PNG bytes."""
    expected_len = width * height * 2
    if len(raw_bytes) < expected_len:
        return None

    img = Image.new('RGB', (width, height))
    pixels = img.load()

    for y in range(height):
        row_offset = y * width * 2
        for x in range(width):
            idx = row_offset + x * 2
            pixel = raw_bytes[idx] | (raw_bytes[idx + 1] << 8)
            r = (pixel >> 11) & 0x1F
            g = (pixel >> 5) & 0x3F
            b = pixel & 0x1F
            r = (r << 3) | (r >> 2)
            g = (g << 2) | (g >> 4)
            b = (b << 3) | (b >> 2)
            pixels[x, y] = (r, g, b)

    buf = io.BytesIO()
    img.save(buf, format='PNG')
    return buf.getvalue()


class OdooConnection:
    """Thin wrapper around Odoo's XML-RPC External API."""

    def __init__(self, url, db, username, api_key):
        if not db:
            db = discover_db_name(url)
            if not db:
                raise SystemExit(
                    "Could not auto-detect the database name. "
                    "Ask whoever manages Odoo for the exact database name, "
                    "then set ODOO_DB to it manually and rerun."
                )
        self.db = db
        self.username = username
        self.api_key = api_key
        self.common = xmlrpc.client.ServerProxy(f"{url}/xmlrpc/2/common")
        self.models = xmlrpc.client.ServerProxy(f"{url}/xmlrpc/2/object")
        self.uid = self.common.authenticate(db, username, api_key, {})
        if not self.uid:
            raise SystemExit(
                "Odoo login failed. Check ODOO_URL / ODOO_DB / ODOO_USERNAME / ODOO_API_KEY."
            )
        print(f"Connected to Odoo as uid={self.uid}")

    def create_face_capture(self, face_count, png_bytes):
        import base64
        image_b64 = base64.b64encode(png_bytes).decode('ascii') if png_bytes else False
        values = {
            'x_name': f"{face_count} face(s) detected",
            'x_studio_face_count': str(face_count),
            'x_studio_image': image_b64,
            'x_studio_binary_field_370_1jujdu716': image_b64,  # add this line
        }
        return self.models.execute_kw(
            self.db, self.uid, self.api_key,
            'x_face_captures', 'create',
            [values],
        )


class UploadHandler(BaseHTTPRequestHandler):
    odoo = None  # set in main()

    def do_POST(self):
        if self.path != '/upload':
            self.send_response(404)
            self.end_headers()
            return

        length = int(self.headers.get('Content-Length', 0))
        raw_body = self.rfile.read(length)

        width = int(self.headers.get('X-Width', '640') or 640)
        height = int(self.headers.get('X-Height', '480') or 480)
        face_count = int(self.headers.get('X-Faces', '0') or 0)
        image_format = self.headers.get('X-Format', 'RGB565')

        print(f"Received upload: {width}x{height}, faces={face_count}, format={image_format}")

        png_bytes = None
        if raw_body and width and height:
            if image_format.upper() == 'RGB565':
                png_bytes = rgb565_to_png_bytes(raw_body, width, height)
            else:
                png_bytes = raw_body  # already a standard image format

        try:
            record_id = self.odoo.create_face_capture(face_count, png_bytes)
            print(f"  -> saved to Odoo as record id {record_id}")
            self.send_response(200)
            self.send_header('Content-Type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"OK\n")
        except Exception as exc:
            print(f"  -> FAILED to save to Odoo: {exc}")
            self.send_response(500)
            self.send_header('Content-Type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"ERROR\n")

    def log_message(self, format, *args):
        pass  # keep console output clean; we print our own status lines above


def main():
    odoo = OdooConnection(ODOO_URL, ODOO_DB, ODOO_USERNAME, ODOO_API_KEY)
    UploadHandler.odoo = odoo

    server = ThreadingHTTPServer(('0.0.0.0', LISTEN_PORT), UploadHandler)
    print(f"Listening on http://0.0.0.0:{LISTEN_PORT}/upload")
    print("On the board terminal, run:")
    print(f"  cam_upload_set <THIS_PC_IP> {LISTEN_PORT}")
    print("  cam_face_upload_loop 5")
    print("Press Ctrl+C to stop.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping.")


if __name__ == '__main__':
    main()
