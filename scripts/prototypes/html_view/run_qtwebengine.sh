#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The HTML View policy probe for QtWebEngine (m_web_engine): run the module's
# own WebEngineView against probe_mail.html with the probe logger listening,
# and fail if anything reached the logger or the cid part never loaded.
#
# Needs a tree configured with GPGFRONTEND_MODULES_WEB_ENGINE=ON and
# GPGFRONTEND_MODULES_BUILD_TESTS=ON; builds the probe target first.
#
#   scripts/prototypes/html_view/run_qtwebengine.sh                 automatic
#   scripts/prototypes/html_view/run_qtwebengine.sh --interactive   click
#   scripts/prototypes/html_view/run_qtwebengine.sh --control       must leak
#   BUILD_DIR=build-x scripts/prototypes/html_view/run_qtwebengine.sh
#
# Without a display it runs offscreen. DNS is not visible to the logger; for
# the DNS probes, in a second terminal first, and expect no gfprobe-* lines:
#   sudo tcpdump -i any -n -l port 53 | grep --line-buffered gfprobe

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
build="${BUILD_DIR:-$root/build}"
out="${TMPDIR:-/tmp}/gf-html-view-qtwebengine"
mkdir -p "$out"

seconds=15
control=0
case "${1:-}" in
  --interactive) seconds=0 ;;
  # The same mail in a stock view: proves the logger would see a leak.
  --control) control=1; export GF_PROBE_CONTROL=1 ;;
esac
if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
  export QT_QPA_PLATFORM=offscreen
fi

cmake --build "$build" --target gf_web_engine_probe >/dev/null
probe="$build/test-bin/gf_web_engine_probe"

python3 -I "$here/probe_logger.py" 8765 "$out/hits.txt" &
logger=$!
trap 'kill $logger 2>/dev/null || true' EXIT
sleep 0.5

"$probe" "$here/probe_mail.html" "$seconds" 2>"$out/engine.log" |
  tee "$out/policy.log"

# Late leaks: a refresh or a prefetch can fire after the window is gone.
sleep 1

echo
if [[ "$control" -eq 1 ]]; then
  if [[ -s "$out/hits.txt" ]]; then
    echo "CONTROL OK: an unlocked view reached the logger, as it must:"
    sort "$out/hits.txt" | uniq -c
    exit 0
  fi
  echo "CONTROL FAIL: nothing reached the logger even with no policy"
  exit 1
fi
if [[ -s "$out/hits.txt" ]]; then
  echo "FAIL: requests reached the network:"
  sort "$out/hits.txt" | uniq -c
  exit 1
fi
if ! grep -q "part serve" "$out/policy.log"; then
  echo "FAIL: the gfpart image was never requested"
  echo "Engine log: $out/engine.log"
  exit 1
fi
echo "PASS: no request reached the logger; the gfpart part was served."
echo "Policy log: $out/policy.log"
