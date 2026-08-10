"""
Odoo -> Qwen bridge, for the Edgi-Talk board (Jolin) to poll.

One HTTP endpoint, plus a background poller:

  GET  /api/announcement       -> reads the latest x_face_captures record,
                                   asks Qwen for a line, returns it as JSON.
                                   For the Edgi-Talk board to poll.

  Background poll loop         -> every POLL_INTERVAL_SECONDS, checks Odoo
                                   for any record with "Generate now" ticked.
                                   For each one found: reads its face count +
                                   idiom/poem category, asks Qwen, writes the
                                   result into "Generated Answer", and unticks
                                   "Generate now" so it isn't reprocessed.

                                   Odoo Server Actions can't make outbound
                                   HTTP calls (imports are blocked in that
                                   sandbox), so this script polls Odoo
                                   instead of Odoo calling the script.

Usage:
    1. Fill in the CONFIG section below - CATEGORY_FIELD, GENERATED_FIELD,
       and TRIGGER_FIELD must match the real technical names of the fields
       you added in Studio.
    2. Run:  python titan_qwen_bridge.py
    3. In Odoo, tick "Generate now" on a record and save. Within
       POLL_INTERVAL_SECONDS, "Generated Answer" should fill in and the
       checkbox should untick itself.
    4. Give Jolin this PC's IP + LISTEN_PORT for /api/announcement.
"""

import json
import re
import threading
import time
import urllib.request
import urllib.error
import xmlrpc.client
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# --- CONFIG -----------------------------------------------------------------
ODOO_URL = "https://www.lingkail.com"
ODOO_DB = ""
ODOO_USERNAME = "cheahwei.leow@lingkail.com"
ODOO_API_KEY = ""

QWEN_API_URL = "https://llm-j5uad7hmmdjz4217.ap-southeast-1.maas.aliyuncs.com/compatible-mode/v1/chat/completions"
QWEN_MODEL = "qwen-plus"
QWEN_API_KEY = ""

PROMPT_TEMPLATE = (
    "Say hello and mention that {count} people have been detected today. "
    "Keep it short and natural, suitable for text-to-speech."
)

# Technical field names on x_face_captures - CONFIRM THESE in Studio before
# running. Click the field, open its properties, and check the technical
# name (or view page source with ?debug=1 like you did for the db name).
CATEGORY_FIELD = "x_studio_selection_field_9lq_1jumeqhb3"  # "Idiom / Poem"
GENERATED_FIELD = "x_studio_generated_answer"               # "Generated Answer"
TRIGGER_FIELD = "x_studio_generate_now"                     # "Generate now" checkbox

CATEGORY_PROMPTS = {
    "idioms": "Write one short, punchy idiom in English about {count} people being detected today.",
    "poems": "Write a short 2-line poem in English about {count} people being detected today.",
}

LISTEN_PORT = 5002
POLL_INTERVAL_SECONDS = 5
# ------------------------------------------------------------------------------


class OdooClient:
    """Wrapper around Odoo's XML-RPC External API - reads and writes."""

    def __init__(self, url, db, username, api_key):
        self.db = db
        self.api_key = api_key
        self.common = xmlrpc.client.ServerProxy(f"{url}/xmlrpc/2/common")
        self.models = xmlrpc.client.ServerProxy(f"{url}/xmlrpc/2/object")
        self.uid = self.common.authenticate(db, username, api_key, {})
        if not self.uid:
            raise SystemExit(
                "Odoo login failed. Check ODOO_URL / ODOO_DB / ODOO_USERNAME / ODOO_API_KEY."
            )
        print(f"Connected to Odoo as uid={self.uid}")

    @staticmethod
    def _as_int(raw):
        """x_studio_face_count stores text like '2 face(s) detected', not a
        plain number - pull out the leading digits instead of assuming it's
        already an int."""
        if raw is None:
            return 0
        match = re.match(r'\s*(\d+)', str(raw))
        return int(match.group(1)) if match else 0

    def latest_face_count(self):
        records = self.models.execute_kw(
            self.db, self.uid, self.api_key,
            'x_face_captures', 'search_read',
            [[]],
            {
                'fields': ['x_studio_face_count', 'create_date'],
                'order': 'create_date desc',
                'limit': 1,
            },
        )
        if not records:
            return 0
        return self._as_int(records[0].get('x_studio_face_count'))

    def read_record(self, record_id):
        """Read one record's face count + idiom/poem category by id."""
        records = self.models.execute_kw(
            self.db, self.uid, self.api_key,
            'x_face_captures', 'read',
            [[record_id]],
            {'fields': ['x_studio_face_count', CATEGORY_FIELD]},
        )
        if not records:
            raise ValueError(f"No x_face_captures record with id={record_id}")
        rec = records[0]
        count = self._as_int(rec.get('x_studio_face_count'))
        category = (rec.get(CATEGORY_FIELD) or 'Idioms').lower()
        return count, category

    def find_pending_records(self):
        """Return ids of records with Generate now ticked."""
        return self.models.execute_kw(
            self.db, self.uid, self.api_key,
            'x_face_captures', 'search',
            [[(TRIGGER_FIELD, '=', True)]],
        )

    def clear_trigger(self, record_id):
        self.models.execute_kw(
            self.db, self.uid, self.api_key,
            'x_face_captures', 'write',
            [[record_id], {TRIGGER_FIELD: False}],
        )

    def write_generated_answer(self, record_id, text):
        self.models.execute_kw(
            self.db, self.uid, self.api_key,
            'x_face_captures', 'write',
            [[record_id], {GENERATED_FIELD: text}],
        )


def ask_qwen(prompt):
    """Call the Qwen compatible-mode chat endpoint and return the reply text."""
    body = json.dumps({
        "model": QWEN_MODEL,
        "messages": [{"role": "user", "content": prompt}],
    }).encode("utf-8")

    req = urllib.request.Request(
        QWEN_API_URL,
        data=body,
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {QWEN_API_KEY}",
        },
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=20) as resp:
        data = json.loads(resp.read().decode("utf-8"))
    return data["choices"][0]["message"]["content"].strip()


class AnnouncementHandler(BaseHTTPRequestHandler):
    odoo = None  # set in main()

    def do_GET(self):
        if self.path != '/api/announcement':
            self.send_response(404)
            self.end_headers()
            return

        try:
            count = self.odoo.latest_face_count()
            prompt = PROMPT_TEMPLATE.format(count=count)
            text = ask_qwen(prompt)

            body = json.dumps({"count": count, "text": text}).encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            print(f"count={count} -> \"{text}\"")
        except urllib.error.HTTPError as exc:
            print(f"FAILED Qwen call: HTTP {exc.code} {exc.read()}")
            self.send_response(502)
            self.send_header('Content-Type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"QWEN_ERROR\n")
        except Exception as exc:
            print(f"FAILED: {exc}")
            self.send_response(500)
            self.send_header('Content-Type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"ERROR\n")

    def log_message(self, format, *args):
        pass  # keep console output clean; we print our own status lines above


def poll_loop(odoo):
    """Every POLL_INTERVAL_SECONDS, process any record with Generate now ticked."""
    while True:
        try:
            pending_ids = odoo.find_pending_records()
            for record_id in pending_ids:
                try:
                    count, category = odoo.read_record(record_id)
                    template = CATEGORY_PROMPTS.get(category, CATEGORY_PROMPTS['idioms'])
                    prompt = template.format(count=count)
                    text = ask_qwen(prompt)

                    odoo.write_generated_answer(record_id, text)
                    odoo.clear_trigger(record_id)
                    print(f"record_id={record_id} category={category} count={count} -> \"{text}\"")
                except urllib.error.HTTPError as exc:
                    print(f"record_id={record_id} FAILED Qwen call: HTTP {exc.code} {exc.read()}")
                except Exception as exc:
                    print(f"record_id={record_id} FAILED: {exc}")
        except Exception as exc:
            print(f"Poll cycle FAILED: {exc}")

        time.sleep(POLL_INTERVAL_SECONDS)


def main():
    odoo = OdooClient(ODOO_URL, ODOO_DB, ODOO_USERNAME, ODOO_API_KEY)
    AnnouncementHandler.odoo = odoo

    poller = threading.Thread(target=poll_loop, args=(odoo,), daemon=True)
    poller.start()
    print(f"Polling Odoo every {POLL_INTERVAL_SECONDS}s for '{TRIGGER_FIELD}' = True")

    server = ThreadingHTTPServer(('0.0.0.0', LISTEN_PORT), AnnouncementHandler)
    print(f"Listening on http://0.0.0.0:{LISTEN_PORT}")
    print(f"  GET  /api/announcement  - for Jolin's Edgi-Talk board")
    print("Press Ctrl+C to stop.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping.")


if __name__ == '__main__':
    main()