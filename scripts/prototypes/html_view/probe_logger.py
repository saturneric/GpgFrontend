#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The far end of every probe in probe_mail.html. Anything that reaches this
# server is a request the HTML View policy promised would never be made, so
# every hit is a finding. Prints one line per request and writes the probe
# names to the hits file the runner reads afterwards.

import http.server
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
HITS = sys.argv[2] if len(sys.argv) > 2 else "probe_hits.txt"

# A 1x1 GIF, so an image probe that does get through renders as one.
GIF = bytes.fromhex(
    "47494638396101000100800000000000ffffff21f90401000000002c00000000"
    "010001000002024401003b")


class Handler(http.server.BaseHTTPRequestHandler):
    def _hit(self):
        probe = self.path.lstrip("/").split("?")[0] or "(root)"
        host = self.headers.get("Host", "")
        line = f"{time.strftime('%H:%M:%S')} {self.command} {probe} host={host}"
        print("HIT", line, flush=True)
        with open(HITS, "a", encoding="utf-8") as f:
            f.write(probe + "\n")
        self.send_response(200)
        self.send_header("Content-Type", "image/gif")
        self.send_header("Content-Length", str(len(GIF)))
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(GIF)

    do_GET = do_POST = do_HEAD = do_OPTIONS = _hit

    def log_message(self, *_):
        pass


if __name__ == "__main__":
    open(HITS, "w", encoding="utf-8").close()
    server = http.server.ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print(f"probe logger on 127.0.0.1:{PORT}, hits -> {HITS}", flush=True)
    server.serve_forever()
