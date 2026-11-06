#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Check the committed entitlement policies before a macOS build is spent: both
# parse, and neither has quietly regained a hardening exception.
#
# The signing job (.github/workflows/sign-apple.yml) reads these exact files
# from its own sparse checkout of the commit, never from the build artifact,
# so they are the single source of what a signed bundle carries. Drift in one
# direction ships an app missing an entitlement it needs, which is loud. A
# restored exception is not loud at all, and is exactly the kind of thing
# nobody notices for a year -- hence this check.
#
# Usage: scripts/check_entitlements.sh

set -u -o pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(dirname "$HERE")"

python3 - "$REPO" <<'PY'
import plistlib
import sys

repo = sys.argv[1]
failures = []

# Entitlements that must never come back without redoing the experiment that
# justified removing them. See §15 of the packaging plan.
FORBIDDEN = {
    "com.apple.security.cs.disable-library-validation":
        "module code ships inside Contents/Frameworks signed with the app's "
        "own identity, which Library Validation permits; if modules stop "
        "loading the cause is a wrong Team ID, not a missing exception",
}

POLICIES = {
    "Developer ID": "resource/entitlements/Normal.entitlements",
    "App Store sandbox": "resource/entitlements/Sandbox.entitlements",
}

for name, path in POLICIES.items():
    try:
        with open(f"{repo}/{path}", "rb") as f:
            policy = plistlib.load(f)
    except Exception as exc:                                  # noqa: BLE001
        failures.append(f"{name}: {path} does not parse: {exc}")
        continue

    bad = [f"{key} is back. {why}" for key, why in FORBIDDEN.items()
           if key in policy]
    if bad:
        failures.extend(f"{name}: {b}" for b in bad)
        continue
    print(f"  ok    {name}: {len(policy)} entitlement(s)")

if failures:
    for f in failures:
        print(f"  FAIL  {f}", file=sys.stderr)
    sys.exit(1)
PY
