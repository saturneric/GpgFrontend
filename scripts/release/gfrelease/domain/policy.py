# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""release/policy.toml: validate it, and keep SECURITY.md in step with it.

  gfrelease policy check            validate the policy (and product.json)
  gfrelease policy render-security  rewrite SECURITY.md's generated sections
  gfrelease policy check-security   fail if SECURITY.md is out of date
  gfrelease policy mainline         print the mainline series ("" for none)

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
import textwrap
import tomllib
from pathlib import Path

from ..paths import ROOT
from . import semver

POLICY = ROOT / "release" / "policy.toml"
PRODUCT = ROOT / "release" / "product.json"
TRUST_ROOT = ROOT / "release" / "root"
SECURITY = ROOT / "SECURITY.md"

STATUSES = ("mainline", "stable", "legacy", "eol")
# The series users are meant to be on: their newest release is the stable
# channel, and their release branch's builds are offered as a beta.
CURRENT_STATUSES = ("stable", "mainline")
# Used with fullmatch throughout: "$" would also match before a trailing
# newline. FINGERPRINT, REPOSITORY and WORKFLOW are the trust root's
# (gfrelease/trust.py).
SERIES_KEY = re.compile(r"([0-9]+)(?:\.([0-9]+))?")
PLATFORM_KEYS = {"windows": str, "macos": str, "linux_glibc": str,
                 "android_api": int}
SLUG = re.compile(r"[a-z][a-z0-9-]{1,31}")
FINGERPRINT = re.compile(r"[0-9A-F]{40}([0-9A-F]{24})?")
REPOSITORY = re.compile(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")
WORKFLOW = re.compile(r"\.github/workflows/[A-Za-z0-9_.-]+\.ya?ml")
ARTIFACT_PREFIX = re.compile(r"[A-Za-z0-9][A-Za-z0-9._+-]*")
# The trust files' names (gfrelease/trust.py), versions from 1, in decimal.
ROOT_FILE = re.compile(r"([1-9][0-9]*)\.root\.json")
ROOT_SIG_FILE = re.compile(r"([1-9][0-9]*)\.root\.json\.[0-9A-F]{16}\.minisig")
INDEX_KEYS_FILE = re.compile(r"([1-9][0-9]*)\.json")


class PolicyError(Exception):
    pass


def load_policy(path: Path = POLICY) -> dict:
    with open(path, "rb") as f:
        return tomllib.load(f)


def unique_keys(pairs: list[tuple]) -> dict:
    """json's object_pairs_hook refusing a key given twice: it reads
    differently in different parsers, so the checks here would pass on one
    value and a client act on the other."""
    keys = [k for k, _ in pairs]
    if len(keys) != len(set(keys)):
        raise ValueError("a key is given twice")
    return dict(pairs)


def load_json_file(path: Path):
    return json.loads(Path(path).read_text(encoding="utf-8"),
                      object_pairs_hook=unique_keys)


def load_product(path: Path = PRODUCT) -> dict:
    return load_json_file(path)


def is_https_base(base) -> bool:
    """A store's public base: https://, ending in /."""
    return isinstance(base, str) and base.startswith("https://") and \
        base.endswith("/")


def numbered(names, pattern: re.Pattern) -> list[int]:
    """The versions of @p names that are a whole @p pattern (ROOT_FILE,
    INDEX_KEYS_FILE), oldest first. A name spelled any other way, such as
    01.json or 1.root.json.new, is not one."""
    found = (pattern.fullmatch(str(n)) for n in names)
    return sorted(int(m.group(1)) for m in found if m)


def series_order(key: str) -> tuple[int, int]:
    m = SERIES_KEY.fullmatch(key)
    return (int(m.group(1)), int(m.group(2) or -1))


def successor_chain(series: dict, key: str) -> list[str]:
    """The series a legacy or eol @p key's users are sent through, in
    order: its successor, followed on while that is itself legacy or eol, as
    clients from 2.2.5 on walk it. Stops before a series not listed, one
    already in the chain, or one not newer than the last; the last entry is
    where the chain ends."""
    chain: list[str] = []
    seen = {key}
    current = key
    while True:
        entry = series.get(current)
        nxt = entry.get("successor") if isinstance(entry, dict) else None
        if not isinstance(nxt, str) or nxt not in series or nxt in seen or \
                not SERIES_KEY.fullmatch(nxt) or \
                series_order(nxt) <= series_order(current):
            return chain
        chain.append(nxt)
        seen.add(nxt)
        nxt_entry = series[nxt]
        if not isinstance(nxt_entry, dict) or \
                nxt_entry.get("status") not in ("legacy", "eol"):
            return chain
        current = nxt


def check_policy(policy: dict, today: dt.date | None = None) -> list[str]:
    """Raises PolicyError on a broken policy; returns warnings."""
    errors: list[str] = []
    warnings: list[str] = []
    # UTC, as every other date here: not the day it happens to be where
    # this runs.
    today = today or dt.datetime.now(dt.timezone.utc).date()

    def table(value, where: str) -> dict:
        """A table read as one; anything else is an error, and reads as
        empty so the checks after it still run."""
        if isinstance(value, dict):
            return value
        errors.append(f"{where} must be a table")
        return {}

    if policy.get("schema") != 1:
        errors.append("schema must be 1")

    signing = table(policy.get("signing", {}), "signing")
    try:
        floor = semver.parse(signing.get("legacy_unsigned_before", ""))
        if not floor.is_release:
            errors.append("signing.legacy_unsigned_before must be a final version")
    except (ValueError, AttributeError):
        errors.append("signing.legacy_unsigned_before must be a SemVer version")

    series = table(policy.get("series", {}), "series")
    if not series:
        errors.append("no series")

    by_status: dict[str, list[str]] = {s: [] for s in STATUSES}
    for key, entry in series.items():
        where = f'series."{key}"'
        if not SERIES_KEY.fullmatch(key):
            errors.append(f"{where}: the key must be MAJOR or MAJOR.MINOR")
            continue
        if not isinstance(entry, dict):
            errors.append(f"{where} must be a table")
            continue
        status = entry.get("status")
        if status not in STATUSES:
            errors.append(f"{where}: status must be one of {', '.join(STATUSES)}")
            continue
        by_status[status].append(key)

        if status in ("legacy", "eol"):
            successor = entry.get("successor")
            if not isinstance(successor, str) or successor not in series:
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
        if status in CURRENT_STATUSES and platforms is None:
            errors.append(f"{where}: a {status} series needs a platforms table")
        for name, value in table(platforms or {},
                                 f"{where}.platforms").items():
            kind = PLATFORM_KEYS.get(name)
            if kind is None:
                errors.append(f"{where}.platforms: unknown key {name}")
            elif not isinstance(value, kind) or isinstance(value, bool):
                errors.append(f"{where}.platforms.{name}: must be a {kind.__name__}")

    if len(by_status["mainline"]) > 1:
        errors.append("more than one mainline series: "
                      + ", ".join(by_status["mainline"]))
    # Several series may be stable at once: an older one keeps its fixes
    # until the maintainer moves it to legacy.
    stables = sorted(by_status["stable"], key=series_order)
    if stables and by_status["mainline"]:
        if series_order(by_status["mainline"][0]) <= series_order(stables[-1]):
            errors.append("the mainline series must be newer than every "
                          "stable one")
    if stables:
        newest = series_order(stables[-1])
        for key in by_status["legacy"] + by_status["eol"]:
            if series_order(key) >= newest:
                errors.append(f'series."{key}": an older line cannot be newer '
                              "than the newest stable series")

    # Where each old line's users are sent: the whole chain of successors
    # must end at a stable series (or the mainline, before anything is
    # stable). Clients before 2.2.5 follow one step only, so a successor
    # that is not stable itself still works there only once it is updated.
    ends = set(stables) or set(by_status["mainline"])
    for key in by_status["legacy"] + by_status["eol"]:
        successor = series[key].get("successor")
        if not isinstance(successor, str) or successor not in series:
            continue  # said above
        where = f'series."{key}"'
        if not SERIES_KEY.fullmatch(successor) or \
                series_order(successor) <= series_order(key):
            errors.append(f"{where}: its successor {successor} is not newer")
            continue
        chain = successor_chain(series, key)
        last = chain[-1] if chain else successor
        last_entry = series.get(last, {})
        last_status = last_entry.get("status") \
            if isinstance(last_entry, dict) else None
        if last_status in ("legacy", "eol"):
            # The walk stopped on an old line: a loop or a step back after it.
            nxt = last_entry.get("successor")
            errors.append(f"{where}: its successor chain "
                          f"({' -> '.join([key, *chain])}) stops at {last}, "
                          f"whose successor {nxt} "
                          + ("loops back" if nxt in [key, *chain]
                             else "is not newer"))
        elif last not in ends:
            errors.append(f"{where}: its successor chain "
                          f"({' -> '.join([key, *chain])}) ends at {last}, "
                          "not at a stable series")
        elif successor not in ends:
            warnings.append(f"{where}: successor {successor} is not a stable "
                            f"series (the chain ends at {last}); clients "
                            "before 2.2.5 follow one step only")

    withdrawn = policy.get("withdrawn", [])
    if not isinstance(withdrawn, list):
        errors.append("withdrawn must be an array of tables")
        withdrawn = []
    for i, item in enumerate(withdrawn):
        item = table(item, f"withdrawn[{i}]")
        try:
            v = semver.parse(item.get("version", ""))
            if v.build:
                raise ValueError
        except (ValueError, AttributeError):
            errors.append(f"withdrawn[{i}]: version must be a release version")
        if not item.get("reason") or not isinstance(item["reason"], str):
            errors.append(f"withdrawn[{i}]: a withdrawal needs a reason")

    if errors:
        raise PolicyError("\n".join(errors))
    return warnings


def check_product(product: dict) -> None:
    errors: list[str] = []

    def table(value, where: str) -> dict:
        if isinstance(value, dict):
            return value
        errors.append(f"{where} must be an object")
        return {}

    def strings(value, where: str) -> list[str]:
        if isinstance(value, list) and \
                all(isinstance(v, str) and v for v in value):
            return value
        errors.append(f"{where} must be a list of strings")
        return []

    def matches(pattern: re.Pattern, field: str) -> bool:
        value = product.get(field)
        return isinstance(value, str) and pattern.fullmatch(value) is not None

    if product.get("schema") != 1:
        errors.append("schema must be 1")
    if not matches(SLUG, "slug"):
        errors.append("slug must match " + SLUG.pattern)
    # What every published file's name starts with, and so part of every
    # path in the stores.
    if not matches(ARTIFACT_PREFIX, "artifact_prefix"):
        errors.append("artifact_prefix must be a file name prefix")
    bases = table(product.get("bases", {}), "bases")
    for role in ("update", "builds"):
        listed = strings(bases.get(role, []), f"bases.{role}")
        if not listed:
            errors.append(f"bases.{role} is empty")
        for base in listed:
            if not is_https_base(base):
                errors.append(f"bases.{role}: {base} must be https:// and end in /")
    if len(bases.get("update", [])) > 1:
        # One store holds the index and the trust it names: a second base
        # would serve clients what nothing here writes.
        errors.append("bases.update lists more than the one update store")
    if "notes_link_hosts" in product:
        strings(product["notes_link_hosts"], "notes_link_hosts")
    # Which programs inside a Windows package are Authenticode-signed:
    # extensions, and folders whose programs stay as their upstream built
    # them (gfrelease's authenticode.py).
    if "authenticode" in product:
        rule = table(product["authenticode"], "authenticode")
        for e in strings(rule.get("sign_inside", []),
                         "authenticode.sign_inside"):
            if not re.fullmatch(r"\.[A-Za-z0-9]+", e):
                errors.append(f"authenticode.sign_inside: {e!r} is not a "
                              "file extension")
        if not rule.get("sign_inside"):
            errors.append("authenticode.sign_inside is empty")
        for d in (strings(rule["leave_unsigned"], "authenticode."
                          "leave_unsigned")
                  if "leave_unsigned" in rule else []):
            if "/" in d or "\\" in d or d in (".", ".."):
                errors.append(f"authenticode.leave_unsigned: {d!r} is not a "
                              "folder name")
        unknown = set(rule) - {"sign_inside", "leave_unsigned"}
        if unknown:
            errors.append(f"authenticode: unknown {sorted(unknown)}")
    # Every published file is classified by these rules (gfrelease refuses
    # one none matches): each must compile, and name a kind clients know.
    kinds = strings(product.get("artifact_kinds", []), "artifact_kinds")
    rules = product.get("artifacts", [])
    if not isinstance(rules, list):
        errors.append("artifacts must be a list of rules")
        rules = []
    for i, rule in enumerate(rules):
        where = f"artifacts[{i}]"
        rule = table(rule, where)
        try:
            re.compile(rule.get("match", ""))
        except (re.error, TypeError) as e:
            errors.append(f"{where}.match does not compile: {e}")
        for key in ("match", "os", "arch", "kind", "flavor"):
            if not rule.get(key) or not isinstance(rule[key], str):
                errors.append(f"{where} lacks {key}")
        if "min_os" in rule and not (
                isinstance(rule["min_os"], dict) and
                all(isinstance(v, str) for v in rule["min_os"].values())):
            errors.append(f"{where}.min_os must map a platform to a version")
        if rule.get("kind") not in kinds:
            errors.append(f"{where}.kind {rule.get('kind')!r} is not one of "
                          "artifact_kinds")
    if "nightly_retention_days" in product:
        # Never silently ignored: what replaced it lives elsewhere.
        errors.append("nightly_retention_days is gone: the signing host "
                      "keeps the newest nightlies (Release-Server's "
                      "GFR_NIGHTLY_KEEP)")
    if errors:
        raise PolicyError("\n".join(errors))


# How long a signed index is valid.
INDEX_LIFETIME_DAYS = 14


def mainline(policy: dict) -> str:
    for key, entry in policy.get("series", {}).items():
        if isinstance(entry, dict) and entry.get("status") == "mainline":
            return key
    return ""


# --- SECURITY.md --------------------------------------------------------

def _label(key: str) -> str:
    return f"{key}.x" if "." in key else f"{key}.x.x"


# SECURITY.md is read as plain text as often as rendered: what is generated
# into it stays within this width, and its tables line up.
WIDTH = 79


def _wrap(text: str, hanging: str = "") -> list[str]:
    """@p text in lines of WIDTH, every one after the first indented by
    @p hanging (a list item's)."""
    return textwrap.wrap(text, WIDTH, subsequent_indent=hanging,
                         break_long_words=False, break_on_hyphens=False)


def _table(rows: list[list[str]]) -> list[str]:
    """A Markdown table whose columns line up in plain text too."""
    widths = [max(len(row[c]) for row in rows) for c in range(len(rows[0]))]

    def line(cells):
        return "| " + " | ".join(c.ljust(w) for c, w in
                                 zip(cells, widths)) + " |"
    return [line(rows[0]), line(["-" * w for w in widths]),
            *(line(row) for row in rows[1:])]


def render_versions(policy: dict) -> str:
    describe = {
        "mainline": "In development: nightly builds",
        "stable": "Current release: bug and security fixes",
        "legacy": "Security fixes only",
        "eol": "End of life",
    }
    rows = [["Version", "Status", "Supported", "Until"]]
    notes = []
    series = policy["series"]
    for key in sorted(series, key=series_order, reverse=True):
        entry = series[key]
        status = describe[entry["status"]]
        if entry["status"] in ("legacy", "eol"):
            # Where its users end up: the whole chain, as clients walk it.
            chain = successor_chain(series, key)
            status += f"; move to {_label(chain[-1] if chain else entry['successor'])}"
        if entry.get("note"):
            # Below the table: in a cell it would make every row as wide.
            notes.append(f"{_label(key)}: {entry['note']}.")
        supported = "No" if entry["status"] == "eol" else "Yes"
        until = str(entry.get("eol", "")) if entry["status"] == "legacy" else ""
        rows.append([_label(key), status, supported, until])
    lines = _table(rows)
    if notes:
        lines.append("")
        for note in notes:
            lines += _wrap(note)

    withdrawn = policy.get("withdrawn", [])
    if withdrawn:
        lines += ["", "Withdrawn releases (do not use; update instead):", ""]
        for item in withdrawn:
            lines += _wrap(f"- {item['version']}: {item['reason']}", "  ")
    return "\n".join(lines)


def newest_root(directory: Path = TRUST_ROOT) -> dict:
    """The newest <N>.root.json that any root key has signed: what
    SECURITY.md shows. A draft from `gfrelease trust root new` nobody
    signed yet is not shown. Whether the signatures verify is `gfrelease
    trust root verify`'s to check."""
    directory = Path(directory)
    names = [p.name for p in directory.iterdir()] if directory.is_dir() \
        else []
    signed = {int(m.group(1)) for m in map(ROOT_SIG_FILE.fullmatch, names)
              if m}
    found = [n for n in numbered(names, ROOT_FILE) if n in signed]
    if not found:
        raise PolicyError(f"{directory}: no signed root yet")
    path = directory / f"{found[-1]}.root.json"
    root = load_json_file(path)
    host = root.get("host_policy") if isinstance(root, dict) else None
    if not isinstance(host, dict) or \
            not isinstance(host.get("stable_pgp"), dict) or \
            not all(isinstance(host.get(f), str)
                    for f in ("repository", "workflow")):
        raise PolicyError(f"{path}: no host_policy with repository, "
                          "workflow and stable_pgp")
    return root


def render_keys(policy: dict, root: dict) -> str:
    floor = policy["signing"]["legacy_unsigned_before"]
    host = root["host_policy"]
    fprs = host["stable_pgp"]["fingerprints"]
    workflow = (f"https://github.com/{host['repository']}/"
                f"{host['workflow']}@<ref>")
    # Paragraphs, not a table: a fingerprint and a URL each on a line of
    # their own (an indented code block) read and copy the same either way.
    lines = [f"Releases from {floor} on can be checked file by file.", ""]
    lines += _wrap("Stable releases (vX.Y.Z) are signed with OpenPGP, a "
                   "detached `.sig` per file, by the maintainer's key:")
    lines.append("")
    lines += [f"    {f}" for f in fprs] or ["    (not yet published)"]
    lines.append("")
    lines += _wrap("Nightly builds (main), betas (release/X.Y) and release "
                   "candidates (vX.Y.0-rc.N) are signed "
                   "with Sigstore, on the build's manifest "
                   "(`build/SHA256SUMS.txt.sigstore.json`), by the build "
                   "workflow's identity:")
    lines += ["", f"    {workflow}", ""]
    lines += _wrap("Check a stable download by hand with `gpg --verify "
                   "FILE.sig FILE`, a nightly's or beta's manifest with "
                   "`cosign verify-blob` and its files against that manifest. "
                   "The application relies on the signed update index, which "
                   "names each release's descriptor by its hash, and the "
                   "descriptor each file's; a stable release is listed there "
                   "only once its OpenPGP signatures have been checked. "
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


def render_security(text: str, policy: dict, root: dict) -> str:
    text = _replace(text, "versions", render_versions(policy))
    return _replace(text, "keys", render_keys(policy, root))


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(prog="gfrelease policy", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", choices=["check", "render-security",
                                        "check-security", "mainline"])
    ap.add_argument("--policy", type=Path, default=POLICY)
    ap.add_argument("--product", type=Path, default=PRODUCT)
    ap.add_argument("--security", type=Path, default=SECURITY)
    ap.add_argument("--root-dir", type=Path, default=TRUST_ROOT)
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
        rendered = render_security(current, policy,
                                   newest_root(args.root_dir))
        if args.command == "render-security":
            if rendered != current:
                args.security.write_text(rendered, encoding="utf-8")
            return 0
        if rendered != current:
            print(f"{args.security} is out of date; run "
                  "scripts/gfrelease policy render-security", file=sys.stderr)
            return 1
        return 0
    except PolicyError as e:
        print(f"policy: {e}", file=sys.stderr)
        return 1
    except (OSError, ValueError) as e:
        # A file that is missing, or not TOML or JSON at all.
        print(f"policy: {e}", file=sys.stderr)
        return 1
