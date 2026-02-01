#!/usr/bin/env python3
import http.server
import os
import re

PORT = 9090
SAVE_DIR = os.path.expanduser("~/Downloads/wifi_transfer")

class UploadHandler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path != "/upload":
            self.send_response(404)
            self.end_headers()
            return

        content_length = int(self.headers.get("Content-Length", 0))
        content_type = self.headers.get("Content-Type", "")

        boundary = None
        for part in content_type.split(";"):
            part = part.strip()
            if part.startswith("boundary="):
                boundary = part[9:]
                break

        if not boundary:
            self.send_response(400)
            self.end_headers()
            return

        body = self.rfile.read(content_length)

        os.makedirs(SAVE_DIR, exist_ok=True)

        delimiter = ("--" + boundary).encode()
        parts = body.split(delimiter)
        count = 0

        for part in parts:
            if part in (b"", b"--\r\n", b"\r\n"):
                continue

            header_end = part.find(b"\r\n\r\n")
            if header_end == -1:
                continue

            headers = part[:header_end].decode(errors="replace")
            data = part[header_end + 4:]

            if data.endswith(b"\r\n"):
                data = data[:-2]

            match = re.search(r'filename="([^"]+)"', headers)
            if not match:
                continue

            filename = os.path.basename(match.group(1))
            count += 1
            path = os.path.join(SAVE_DIR, filename)
            with open(path, "wb") as f:
                f.write(data)
            print(f"Saved: {path} ({len(data)} bytes)")

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        resp = f'{{"status":"ok","files":{count}}}'
        self.send_header("Content-Length", str(len(resp)))
        self.end_headers()
        self.wfile.write(resp.encode())

print(f"Receiver listening on port {PORT}")
print(f"Files will be saved to {SAVE_DIR}")
http.server.HTTPServer(("0.0.0.0", PORT), UploadHandler).serve_forever()
