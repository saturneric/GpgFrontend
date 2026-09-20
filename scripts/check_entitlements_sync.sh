#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Check that the entitlements the signing job applies match the ones committed
# to the repository, and that neither has quietly regained an exception.
#
# ## Why there are two copies at all
#
# `sign-macos` holds the Apple credentials and treats everything the build job
# hands it as untrusted payload. Reading entitlements out of that payload would
# let a compromised build job choose its own entitlements and then have them
# verified against its own choice -- so the signing job carries a verbatim copy
# and the two are kept in step by hand.
#
# "By hand" is the part that needs a check. Drift in one direction ships an app
# missing an entitlement it needs, which is loud. Drift in the other silently
# restores a hardening exception, which is not loud at all, and is exactly the
# kind of thing nobody notices for a year.
#
# Usage: scripts/check_entitlements_sync.sh

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


def entitlements_from_workflow(path, marker, terminator):
    """The verbatim copy the signing job writes, lifted out of its heredoc.

    Read as TEXT, not through a YAML parser. A heredoc body is a textual
    construct -- the workflow's own shell finds it exactly this way -- and
    reaching for PyYAML to locate it added a dependency the macOS runner's
    python3 does not have. This runs on stdlib alone, which is the only thing
    a check that gates a build should need.
    """
    text = open(path, encoding="utf-8").read()
    if text.count(marker) != 1:
        return None, f"expected exactly one {marker.strip()} heredoc"

    after = text.split(marker, 1)[1]
    body = []
    for line in after.split("\n"):
        if line.strip() == terminator:
            # De-indent by whatever the terminator is indented by: a heredoc
            # closed with <<- or written at a different depth still works.
            pad = len(line) - len(line.lstrip())
            return "\n".join(
                l[pad:] if l.startswith(" " * pad) else l for l in body
            ), None
        body.append(line)
    return None, f"heredoc opened with {marker.strip()} is never closed"


pairs = [
    (
        "Developer ID",
        f"{repo}/resource/entitlements/Normal.entitlements",
        f"{repo}/.github/workflows/build.yml",
        "<<'PLIST'\n",
        "PLIST",
    ),
    (
        # The same hazard, the same hand-sync comment, and until now the same
        # lack of a check. A sandboxed build's entitlements decide its keychain
        # access group and its application identifier, so the two drifting is
        # not a cosmetic difference.
        "App Store sandbox",
        f"{repo}/resource/entitlements/Sandbox.entitlements",
        f"{repo}/.github/workflows/mas-sandbox.yml",
        "<<'PLIST'\n",
        "PLIST",
    ),
]

for name, repo_file, workflow, marker, terminator in pairs:
    try:
        committed = plistlib.load(open(repo_file, "rb"))
    except Exception as exc:                                  # noqa: BLE001
        failures.append(f"{name}: {repo_file} does not parse: {exc}")
        continue

    body, why = entitlements_from_workflow(workflow, marker, terminator)
    if body is None:
        failures.append(f"{name}: {why} in {workflow}")
        continue
    try:
        applied = plistlib.loads(body.encode())
    except Exception as exc:                                  # noqa: BLE001
        failures.append(f"{name}: the copy in {workflow} does not parse: {exc}")
        continue

    if committed != applied:
        only_repo = set(committed) - set(applied)
        only_ci = set(applied) - set(committed)
        failures.append(
            f"{name}: the committed file and the signing job disagree"
            + (f"; only in the repository: {sorted(only_repo)}" if only_repo else "")
            + (f"; only in the signing job: {sorted(only_ci)}" if only_ci else "")
        )
        continue

    for key, why in FORBIDDEN.items():
        if key in committed:
            failures.append(f"{name}: {key} is back. {why}")

    print(f"  ok    {name}: {len(committed)} entitlement(s), in sync")

if failures:
    for f in failures:
        print(f"  FAIL  {f}", file=sys.stderr)
    sys.exit(1)
PY
