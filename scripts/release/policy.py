#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""release/policy.toml: validate it, and keep SECURITY.md in step with it.

  policy.py check            validate the policy (and release/product.json)
  policy.py render-security  rewrite SECURITY.md's generated sections
  policy.py check-security   fail if SECURITY.md is out of date
  policy.py mainline         print the mainline series ("" when there is none)

The generated sections sit between `<!-- policy:<name>:begin -->` and
`<!-- policy:<name>:end -->` markers; everything else in SECURITY.md is prose
and left alone.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import re
import sys
import tomllib
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE))

import semver  # noqa: E402

POLICY = ROOT / "release" / "policy.toml"
PRODUCT = ROOT / "release" / "product.json"
SECURITY = ROOT / "SECURITY.md"

STATUSES = ("mainline", "stable", "legacy", "eol")
SERIES_KEY = re.compile(r"^(\d+)(?:\.(\d+))?$")
PLATFORM_KEYS = {"windows": str, "macos": str, "linux_glibc": str,
                 "android_api": int}
SLUG = re.compile(r"^[a-z][a-z0-9-]{1,31}$")
FINGERPRINT = re.compile(r"^[0-9A-F]{40}([0-9A-F]{24})?$")


class PolicyError(Exception):
    pass


def load_policy(path: Path = POLICY) -> dict:
    with open(path, "rb") as f:
        return tomllib.load(f)


def load_product(path: Path = PRODUCT) -> dict:
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def series_order(key: str) -> tuple[int, int]:
    m = SERIES_KEY.match(key)
    return (int(m.group(1)), int(m.group(2) or -1))


def check_policy(policy: dict, today: dt.date | None = None) -> list[str]:
    """Raises PolicyError on a broken policy; returns warnings."""
    errors: list[str] = []
    warnings: list[str] = []
    today = today or dt.date.today()

    if policy.get("schema") != 1:
        errors.append("schema must be 1")

    signing = policy.get("signing", {})
    try:
        floor = semver.parse(signing.get("legacy_unsigned_before", ""))
        if not floor.is_final:
            errors.append("signing.legacy_unsigned_before must be a final version")
    except ValueError:
        errors.append("signing.legacy_unsigned_before must be a SemVer version")

    series = policy.get("series", {})
    if not series:
        errors.append("no series")

    by_status: dict[str, list[str]] = {s: [] for s in STATUSES}
    for key, entry in series.items():
        where = f'series."{key}"'
        if not SERIES_KEY.match(key):
            errors.append(f"{where}: the key must be MAJOR or MAJOR.MINOR")
            continue
        status = entry.get("status")
        if status not in STATUSES:
            errors.append(f"{where}: status must be one of {', '.join(STATUSES)}")
            continue
        by_status[status].append(key)

        if status in ("legacy", "eol"):
            if entry.get("successor") not in series:
                errors.append(f"{where}: a {status} series needs a successor "
                              "that is itself listed")
        elif "successor" in entry:
            errors.append(f"{where}: only legacy and eol series have a successor")

        if status == "legacy":
            try:
                eol = dt.date.fromisoformat(str(entry.get("eol", "")))
                if eol < today:
                    warnings.append(f"{where}: its eol date {eol} has passed; "
                                    "mark it eol")
            except ValueError:
                errors.append(f"{where}: a legacy series needs eol = YYYY-MM-DD")
        elif "eol" in entry and status != "eol":
            errors.append(f"{where}: only legacy and eol series have an eol date")

        if status in ("mainline", "stable", "legacy"):
            branch = entry.get("branch", "")
            want = "main" if status == "mainline" else f"release/{key}"
            if branch != want:
                errors.append(f'{where}: a {status} series lives on "{want}"')

        platforms = entry.get("platforms")
        if status in ("mainline", "stable") and platforms is None:
            errors.append(f"{where}: a {status} series needs a platforms table")
        for name, value in (platforms or {}).items():
            kind = PLATFORM_KEYS.get(name)
            if kind is None:
                errors.append(f"{where}.platforms: unknown key {name}")
            elif not isinstance(value, kind):
                errors.append(f"{where}.platforms.{name}: must be a {kind.__name__}")

    if len(by_status["mainline"]) > 1:
        errors.append("more than one mainline series: "
                      + ", ".join(by_status["mainline"]))
    if len(by_status["stable"]) > 1:
        errors.append("more than one stable series: "
                      + ", ".join(by_status["stable"]))
    if by_status["stable"] and by_status["mainline"]:
        if series_order(by_status["mainline"][0]) <= series_order(by_status["stable"][0]):
            errors.append("the mainline series must be newer than the stable one")
    if by_status["stable"]:
        stable = series_order(by_status["stable"][0])
        for key in by_status["legacy"] + by_status["eol"]:
            if series_order(key) >= stable:
                errors.append(f'series."{key}": an older line cannot be newer '
                              "than the stable series")

    for i, item in enumerate(policy.get("withdrawn", [])):
        try:
            v = semver.parse(item.get("version", ""))
            if v.build:
                raise ValueError
        except ValueError:
            errors.append(f"withdrawn[{i}]: version must be a release version")
        if not item.get("reason"):
            errors.append(f"withdrawn[{i}]: a withdrawal needs a reason")

    if errors:
        raise PolicyError("\n".join(errors))
    return warnings


def check_product(product: dict) -> None:
    errors: list[str] = []
    if product.get("schema") != 1:
        errors.append("schema must be 1")
    if not SLUG.match(product.get("slug", "")):
        errors.append("slug must match " + SLUG.pattern)
    if not re.match(r"^[\w.-]+/[\w.-]+$", product.get("repository", "")):
        errors.append("repository must be owner/name")
    for role in ("update", "builds"):
        bases = product.get("bases", {}).get(role, [])
        if not bases:
            errors.append(f"bases.{role} is empty")
        for base in bases:
            if not base.startswith("https://") or not base.endswith("/"):
                errors.append(f"bases.{role}: {base} must be https:// and end in /")
    signing = product.get("signing", {})
    for fpr in signing.get("maintainer_fingerprints", []):
        if not FINGERPRINT.match(fpr):
            errors.append(f"signing.maintainer_fingerprints: {fpr} is not an "
                          "upper-case v4/v6 fingerprint")
    # A key or certificate replaced: what it signed still verifies, nothing new
    # is signed with it. The current one is never also retired.
    for field, current in (("artifact_keys_retired", "artifact_key"),
                           ("authenticode_subjects_retired",
                            "authenticode_subject")):
        retired = signing.get(field, [])
        if not isinstance(retired, list) or \
                not all(isinstance(v, str) and v for v in retired):
            errors.append(f"signing.{field} must be a list of strings")
        elif signing.get(current) and signing[current] in retired:
            errors.append(f"signing.{field} lists the current {current}")
    if len(signing.get("index_keys", [])) > 2:
        errors.append("signing.index_keys holds the current and the next key, "
                      "no more")
    if errors:
        raise PolicyError("\n".join(errors))


def mainline(policy: dict) -> str:
    for key, entry in policy.get("series", {}).items():
        if entry.get("status") == "mainline":
            return key
    return ""


# --- SECURITY.md --------------------------------------------------------

def _label(key: str) -> str:
    return f"{key}.x" if "." in key else f"{key}.x.x"


def render_versions(policy: dict) -> str:
    describe = {
        "mainline": "In development: nightly and beta builds",
        "stable": "Current release: bug and security fixes",
        "legacy": "Security fixes only",
        "eol": "End of life",
    }
    rows = ["| Version | Status | Supported | Until |",
            "| ------- | ------ | --------- | ----- |"]
    series = policy["series"]
    for key in sorted(series, key=series_order, reverse=True):
        entry = series[key]
        status = describe[entry["status"]]
        if entry.get("note"):
            status += f" ({entry['note']})"
        if entry["status"] in ("legacy", "eol"):
            status += f"; move to {_label(entry['successor'])}"
        ok = ":x:" if entry["status"] == "eol" else ":white_check_mark:"
        until = str(entry.get("eol", "")) if entry["status"] == "legacy" else ""
        rows.append(f"| {_label(key)} | {status} | {ok} | {until} |")

    withdrawn = policy.get("withdrawn", [])
    if withdrawn:
        rows.append("")
        rows.append("Withdrawn releases (do not use; update instead):")
        rows.append("")
        for item in withdrawn:
            rows.append(f"- {item['version']}: {item['reason']}")
    return "\n".join(rows)


def render_keys(policy: dict, product: dict) -> str:
    floor = policy["signing"]["legacy_unsigned_before"]
    signing = product.get("signing", {})
    fprs = signing.get("maintainer_fingerprints", [])
    artifact = signing.get("artifact_key", "")
    lines = [f"Releases from {floor} on carry a detached signature for every "
             "file:", "",
             "| Releases | Signed with | Verify with |",
             "| -------- | ----------- | ----------- |"]
    lines.append("| Stable (vX.Y.Z) | OpenPGP, detached `.sig` per file | "
                 + (", ".join(f"`{f}`" for f in fprs) if fprs
                    else "not yet published") + " |")
    lines.append("| Beta, rc and nightly | minisign, `.minisig` per file | "
                 + (f"`{artifact}`" if artifact else "not yet published")
                 + " |")
    lines.append("")
    lines.append("Check a stable download with `gpg --verify FILE.sig FILE`, "
                 "a beta or nightly one with `minisign -Vm FILE -P <key>`. "
                 "Windows installers of stable releases are also "
                 "Authenticode-signed.")
    return "\n".join(lines)


def _replace(text: str, name: str, body: str) -> str:
    begin = f"<!-- policy:{name}:begin -->"
    end = f"<!-- policy:{name}:end -->"
    pattern = re.compile(re.escape(begin) + r".*?" + re.escape(end), re.S)
    if not pattern.search(text):
        raise PolicyError(f"SECURITY.md has no {begin} ... {end} section")
    return pattern.sub(lambda _: f"{begin}\n{body}\n{end}", text)


def render_security(text: str, policy: dict, product: dict) -> str:
    text = _replace(text, "versions", render_versions(policy))
    return _replace(text, "keys", render_keys(policy, product))


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", choices=["check", "render-security",
                                        "check-security", "mainline"])
    ap.add_argument("--policy", type=Path, default=POLICY)
    ap.add_argument("--product", type=Path, default=PRODUCT)
    ap.add_argument("--security", type=Path, default=SECURITY)
    args = ap.parse_args(argv)

    try:
        policy = load_policy(args.policy)
        if args.command == "mainline":
            print(mainline(policy))
            return 0
        for warning in check_policy(policy):
            print(f"warning: {warning}", file=sys.stderr)
        product = load_product(args.product)
        check_product(product)
        if args.command == "check":
            return 0

        current = args.security.read_text(encoding="utf-8")
        rendered = render_security(current, policy, product)
        if args.command == "render-security":
            if rendered != current:
                args.security.write_text(rendered, encoding="utf-8")
            return 0
        if rendered != current:
            print(f"{args.security} is out of date; run "
                  "scripts/release/policy.py render-security", file=sys.stderr)
            return 1
        return 0
    except PolicyError as e:
        print(f"policy: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
