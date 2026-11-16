# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only views, for whoever wants to know where things stand.

  show            the published index as a client sees it: its signature
                  and trust checked as verify checks them (without
                  fetching any artifact), then its channels, series and
                  releases.
  status VERSION  where one release stands, end to end: tagged, staged by
                  CI, the maintainer's stable stage, uploaded, listed; for a
                  nightly or beta, staged, promoted, listed. It ends with
                  what to do next.
  status          the channels, and each branch's newest staged build.

Nothing here writes anything. What cannot be read (a store not set, a base
that does not answer) is "unknown", never "no".
"""

from __future__ import annotations

import json
from pathlib import Path

import layout
import stable as stable_mod
import trust
import verify as verify_mod
from common import (DESCRIPTOR, STAGED, Product, ReleaseError, parse_iso,
                    parse_version, refs, run, utcnow)

YES, NO, UNKNOWN = "yes", "no", "unknown"


def _days(text: str) -> float | None:
    try:
        return (parse_iso(text) - utcnow()).total_seconds() / 86400
    except ReleaseError:
        return None


def _live_index(product: Product, update) -> tuple[dict | None, str | None]:
    """The published index, verified, and why not when it cannot be had.
    None published yet is an index that lists nothing."""
    try:
        return verify_mod.read_index(product, update), None
    except ReleaseError as e:
        text = str(e)
        if text.endswith("not found") or ": not found (" in text or \
                "HTTP Error 404" in text:
            return {"releases": {}, "channels": {}, "series": {}}, \
                "no index published yet"
        return None, text


def _version_of(entry) -> str | None:
    return entry.get("version") if isinstance(entry, dict) else None


# --- show ---------------------------------------------------------------

def show(product: Product, update) -> dict:
    index = verify_mod.read_index(product, update)
    series = {}
    for key, entry in sorted(index.get("series", {}).items(),
                             key=lambda kv: [int(p) for p in
                                             kv[0].split(".")],
                             reverse=True):
        series[key] = {"status": entry.get("status"),
                       "eol": entry.get("eol"),
                       "latest": entry.get("latest"),
                       "beta": _version_of(entry.get("beta"))}
    return {
        "product": product.slug,
        "serial": index["serial"],
        "generated_at": index["generated_at"],
        "expires": index["expires"],
        "days_left": _days(index["expires"]),
        "root_version": index.get("root_version"),
        "index_keys_version": index.get("index_keys_version"),
        "channels": index["channels"],
        "series": series,
        "releases": {v: {"notes": bool(e.get("notes"))}
                     for v, e in sorted(index["releases"].items(),
                                        key=lambda kv: parse_version(kv[0]),
                                        reverse=True)},
        "withdrawn": index.get("withdrawn", []),
    }


def render_show(view: dict) -> list[str]:
    days = view["days_left"]
    lines = [
        f"{view['product']} update index, serial {view['serial']}",
        f"  signed   {view['generated_at']}",
        f"  expires  {view['expires']}"
        + (f" (in {days:.1f} days)" if days is not None else ""),
        f"  trust    root {view['root_version']}, index-keys "
        f"{view['index_keys_version']}",
        "",
        "channels",
    ]
    for name in ("stable", "beta", "prerelease", "nightly"):
        lines.append(f"  {name:<8} {view['channels'].get(name) or '-'}")
    lines += ["", "series"]
    for key, s in view["series"].items():
        parts = [f"  {key:<6} {s['status'] or '-':<10}"]
        if s["latest"]:
            parts.append(f"latest {s['latest']}")
        if s["beta"]:
            parts.append(f"beta {s['beta']}")
        if s["eol"]:
            parts.append(f"eol {s['eol']}")
        lines.append("  ".join(parts).rstrip())
    if view["releases"]:
        lines += ["", "releases"]
        for version, r in view["releases"].items():
            lines.append(f"  {version:<10} "
                         f"{'notes' if r['notes'] else 'no notes'}")
    if view["withdrawn"]:
        lines += ["", "withdrawn"]
        for w in view["withdrawn"]:
            lines.append(f"  {w.get('version'):<10} {w.get('reason', '')}")
    return lines


def show_next(view: dict) -> str | None:
    days = view["days_left"]
    if days is not None and days < 7:
        return ("the index expires soon: the signing host re-signs it "
                "daily; check it is running (its status page, /healthz)")
    return None


# --- status -------------------------------------------------------------

def _exists(source, path: str) -> str:
    """YES, NO or UNKNOWN: whether @p source (a Store, an HttpSource, or
    None when not set) holds @p path."""
    if source is None:
        return UNKNOWN
    if hasattr(source, "exists"):
        try:
            return YES if source.exists(path) else NO
        except ReleaseError:
            return UNKNOWN
    try:
        source.read(path)
        return YES
    except ReleaseError as e:
        return NO if "404" in str(e) else UNKNOWN


def _tagged(git_dir, version: str) -> str:
    if not git_dir:
        return UNKNOWN
    try:
        out = run(["git", "-C", git_dir, "rev-parse", "-q", "--verify",
                   f"refs/tags/{refs.release_tag(version)}"], check=False)
    except ReleaseError:
        return UNKNOWN
    return YES if out.returncode == 0 else NO


def _stage_state(work, version: str) -> dict | None:
    if not work:
        return None
    path = Path(work) / version / "state.json"
    try:
        return json.loads(path.read_text())
    except (OSError, ValueError):
        return None


def status(product: Product, version: str, *, update, builds, staging=None,
           work=None, git_dir=None) -> dict:
    v = parse_version(version)
    version = str(v)
    index, index_problem = _live_index(product, update)
    facts: list[dict] = []

    def fact(name, state, detail=""):
        facts.append({"step": name, "state": state, "detail": detail})
        return state

    if v.is_release:
        tagged = fact("tagged", _tagged(git_dir, version),
                      refs.release_tag(version))
        staged = fact("staged by CI", _exists(
            staging, f"{layout.staged_tag(v.filesafe)}/{STAGED}"),
            "" if staging else "no --staging")
        stage = _stage_state(work, version)
        done = (stage or {}).get("done", [])
        fact("stable stage", YES if "upload" in done else
             (f"{len(done)}/{len(stable_mod.STEPS)}" if stage else UNKNOWN),
             ", ".join(done) if stage else "not run on this machine")
        uploaded = fact("uploaded", _exists(
            builds, layout.descriptor_of(layout.release_dir(version))))
        listed = UNKNOWN if index is None else \
            (YES if version in index["releases"] else NO)
        on = [c for c, value in ((index or {}).get("channels") or {}).items()
              if value == version]
        fact("listed", listed, f"the {', '.join(on)} channel" if on else
             (index_problem or ""))
        if listed == YES:
            nxt = None
        elif uploaded == YES:
            nxt = ("wait for the signing host's next round (it admits a "
                   "release within minutes), then `gfrelease status "
                   f"{version}` again")
        elif stage and done:
            following = next(s for s in stable_mod.STEPS if s not in done)
            nxt = f"gfrelease stable {version} (resumes at {following})"
        elif staged == YES:
            nxt = f"gfrelease stable {version}"
        elif tagged == NO:
            nxt = (f"tag {refs.release_tag(version)} (signed) and push it: "
                   "CI builds and stages it")
        else:
            nxt = ("wait for CI to stage the tag's build (the Build "
                   "workflow of the tag)")
    else:
        candidates = []
        if staging is not None and hasattr(staging, "files"):
            try:
                candidates = staging.files(
                    layout.BRANCHES, include=f"*/{v.filesafe}/{STAGED}")
            except ReleaseError:
                candidates = None
        fact("staged by CI", UNKNOWN if staging is None or
             candidates is None else (YES if candidates else NO),
             "" if staging else "no --staging")
        promoted = fact("promoted", _exists(
            builds, f"{layout.nightly_dir(version)}/{DESCRIPTOR}"))
        named = {}
        if index is not None:
            named["nightly"] = _version_of(index.get("nightly"))
            for key, s in index.get("series", {}).items():
                named[f"beta of {key}"] = _version_of(s.get("beta"))
        on = [k for k, value in named.items() if value == version]
        listed = fact("listed", UNKNOWN if index is None else
                      (YES if on else NO),
                      ", ".join(on) or (index_problem or ""))
        newer = named.get("nightly")
        if listed == YES:
            nxt = None
        elif promoted == YES and newer and \
                parse_version(newer) > v and v.series == \
                parse_version(newer).series:
            nxt = f"none: {newer} is newer and listed in its place"
        elif promoted == YES:
            nxt = "wait for the signing host's next index"
        else:
            nxt = ("wait for CI to stage the build and the signing host to "
                   "promote it (it promotes each branch's newest build)")
    return {"version": version, "facts": facts, "next": nxt,
            "index_problem": index_problem}


def render_status(view: dict) -> list[str]:
    marks = {YES: "✓", NO: "✗", UNKNOWN: "?"}
    lines = [view["version"]]
    for f in view["facts"]:
        mark = marks.get(f["state"], "…")
        state = "" if f["state"] in marks else f" {f['state']}"
        detail = f"  ({f['detail']})" if f["detail"] else ""
        lines.append(f"  {mark} {f['step']}{state}{detail}")
    return lines


def overview(product: Product, *, update, builds=None, staging=None) -> dict:
    """The channels, and each branch's newest staged build."""
    view = {"channels": None, "index_problem": None, "branches": {}}
    index, view["index_problem"] = _live_index(product, update)
    if index is not None:
        view["channels"] = index["channels"]
    if staging is not None and hasattr(staging, "files"):
        newest: dict[str, str] = {}
        for path in staging.files(layout.BRANCHES, include=f"*/*/{STAGED}"):
            branch = path.split("/")[0]
            try:
                staged = json.loads(staging.read(f"{layout.BRANCHES}/{path}"))
                v = parse_version(staged["version"])
            except (ReleaseError, ValueError, KeyError, TypeError):
                continue
            if branch not in newest or v > parse_version(newest[branch]):
                newest[branch] = str(v)
        for branch, version in sorted(newest.items()):
            view["branches"][branch] = {
                "newest": version,
                "promoted": _exists(builds, f"{layout.nightly_dir(version)}/"
                                    f"{DESCRIPTOR}")}
    return view


def render_overview(view: dict) -> list[str]:
    lines = ["channels"]
    if view["channels"] is None:
        lines.append(f"  unknown: {view['index_problem']}")
    else:
        for name in ("stable", "beta", "prerelease", "nightly"):
            lines.append(f"  {name:<8} {view['channels'].get(name) or '-'}")
    if view["branches"]:
        lines += ["", "staged (each branch's newest)"]
        for branch, b in view["branches"].items():
            lines.append(f"  {branch:<16} {b['newest']}  promoted: "
                         f"{b['promoted']}")
    return lines


# --- trust root status --------------------------------------------------

def root_status(root_dir: Path, slug: str) -> dict:
    """The newest root file in @p root_dir, trusted or still collecting
    signatures: which root keys signed it, against the thresholds of the
    root before it and its own."""
    names = sorted(Path(root_dir).glob("*.root.json"),
                   key=lambda p: int(p.name.split(".")[0])
                   if p.name.split(".")[0].isdigit() else -1)
    if not names:
        raise ReleaseError(f"{root_dir}: no root")
    path = names[-1]
    read = trust.read_dir(root_dir)
    data = path.read_bytes()
    doc = trust.load(data, path.name, trust.ROOT_KIND, slug)
    n = doc["version"]
    previous = trust.walk(read, slug, upto=n - 1, steps=None) if n > 1 \
        else None
    comment = layout.root_comment(slug, n)
    groups = []
    for whose, signer in (("root " + str(n - 1), previous), ("its own", doc)):
        if signer is None:
            continue
        role = signer["roles"]["root"]
        keys = {i: signer["keys"][i] for i in role["keyids"]}
        found = trust.signers(
            data, {i: read(layout.root_sig_name(n, i)) for i in keys}, keys,
            comment)
        groups.append({"whose": whose, "threshold": role["threshold"],
                       "signed": sorted(found),
                       "missing": sorted(set(keys) - found)})
    try:
        # Untracked or changed: what is still to be committed.
        pending = run(["git", "-C", path.parent, "status", "--porcelain",
                       "--", f"{n}.root.json*"], check=False).stdout.strip()
    except ReleaseError:
        pending = ""
    return {"file": str(path), "version": n, "expires": doc["expires"],
            "days_left": trust.days_left(doc), "groups": groups,
            "complete": all(len(g["signed"]) >= g["threshold"]
                            for g in groups),
            "uncommitted": bool(pending)}


def render_root_status(view: dict) -> list[str]:
    lines = [f"root {view['version']} ({view['file']}): expires "
             f"{view['expires']}, in {view['days_left']:.0f} days"]
    for g in view["groups"]:
        lines.append(f"  {g['whose']} root keys: {len(g['signed'])} of "
                     f"{g['threshold']} needed have signed")
        for key in g["signed"]:
            lines.append(f"    ✓ {key}")
        for key in g["missing"]:
            lines.append(f"    · {key}")
    return lines


def root_status_next(view: dict) -> str | None:
    if view["complete"] and view["uncommitted"]:
        return (f"commit {view['file']} and its signatures; the signing "
                "host takes it on its next round")
    if view["complete"]:
        if view["days_left"] < 60:
            return ("renew it: gfrelease trust root new (gfrelease help "
                    "root-ceremony)")
        return None
    short = [f"{g['threshold'] - len(g['signed'])} more of {g['whose']} "
             "root keys" for g in view["groups"]
             if len(g["signed"]) < g["threshold"]]
    return (f"{' and '.join(short)} sign it: gfrelease trust root sign "
            f"{view['file']} -s <that root key>")
