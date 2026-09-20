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


def entitlements_from_workflow(path, job, marker, terminator):
    """The verbatim copy the signing job writes, lifted out of its heredoc."""
    import yaml
    d = yaml.safe_load(open(path))
    for step in d["jobs"][job]["steps"]:
        run = step.get("run") or ""
        if marker not in run:
            continue
        after = run.split(marker, 1)[1]
        lines = []
        for line in after.split("\n"):
            if line.strip() == terminator:
                break
            lines.append(line[10:] if line.startswith(" " * 10) else line)
        return plistlib.loads("\n".join(lines).encode())
    return None


pairs = [
    (
        "Developer ID",
        f"{repo}/resource/entitlements/Normal.entitlements",
        f"{repo}/.github/workflows/build.yml",
        "sign-macos",
        "<<'PLIST'\n",
        "PLIST",
    ),
]

for name, repo_file, workflow, job, marker, terminator in pairs:
    try:
        committed = plistlib.load(open(repo_file, "rb"))
    except Exception as exc:                                  # noqa: BLE001
        failures.append(f"{name}: {repo_file} does not parse: {exc}")
        continue

    applied = entitlements_from_workflow(workflow, job, marker, terminator)
    if applied is None:
        failures.append(f"{name}: no entitlements heredoc found in {job}")
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
