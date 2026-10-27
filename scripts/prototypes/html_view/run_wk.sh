#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Stage 0 of the HTML View plan, macOS only: build the WKWebView policy probe,
# run it against probe_mail.html with the probe logger listening, and fail if
# anything reached the logger.
#
#   scripts/prototypes/html_view/run_wk.sh                 automatic, 8 s
#   scripts/prototypes/html_view/run_wk.sh --interactive   stays open; click
#                                                          the links, close
#                                                          the window to end
#
# DNS is not visible to the logger. To check the DNS probes as well, run this
# in a second terminal first and expect no gfprobe-* lines:
#   sudo tcpdump -i any -n -l port 53 | grep --line-buffered gfprobe

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
out="${TMPDIR:-/tmp}/gf-html-view-wk"
mkdir -p "$out"

seconds=8
[[ "${1:-}" == "--interactive" ]] && seconds=0

clang++ -std=c++17 -fobjc-arc -mmacosx-version-min=12.0 \
  -framework Cocoa -framework WebKit \
  "$here/wk_probe.mm" -o "$out/probe"

python3 "$here/probe_logger.py" 8765 "$out/hits.txt" &
logger=$!
trap 'kill $logger 2>/dev/null || true' EXIT
sleep 0.5

"$out/probe" "$here/probe_mail.html" "$seconds" | tee "$out/policy.log"

# Late leaks: a refresh or a prefetch can fire after the window is gone.
sleep 1

echo
if [[ -s "$out/hits.txt" ]]; then
  echo "FAIL: requests reached the network:"
  sort "$out/hits.txt" | uniq -c
  exit 1
fi
if ! grep -q "part serve" "$out/policy.log"; then
  echo "FAIL: the gfpart image was never requested"
  exit 1
fi
echo "PASS: no request reached the logger; the gfpart part was served."
echo "Policy log: $out/policy.log"
