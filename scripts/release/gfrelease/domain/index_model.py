# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What the index says, from what was admitted: the series table, each
channel and each series' beta and release candidate. Decided here, apart
from how anything is admitted or published (app/index.py), and nothing here
reads a store or runs a tool."""

from __future__ import annotations

import datetime as dt

from . import descriptor, layout
from . import policy as policy_mod
from .formats import SCHEMA, iso, parse_version, sha256_bytes
from .limits import INDEX_LIFETIME

LIFETIME = INDEX_LIFETIME
VOLATILE = ("serial", "generated_at", "expires")


def build_entry(version: str, path: str, raw: bytes) -> dict:
    """How the index names a build (the nightly, a beta, a release
    candidate): its version, its descriptor at @p path and that
    descriptor's hash (@p raw its bytes)."""
    return {"version": version, "descriptor": path,
            "sha256": sha256_bytes(raw)}


def notes_fields(version: str, notes: bytes) -> dict:
    """A release entry's notes: where they are published, their hash and
    the maintainer's signature beside them."""
    digest = sha256_bytes(notes)
    notes_path = layout.notes_path(version, digest)
    return {"notes": notes_path, "notes_sha256": digest,
            "notes_sig": layout.sig_of(notes_path)}


def iso_of(created: int) -> str:
    """A signature's time (Unix seconds) as the index spells times."""
    return iso(dt.datetime.fromtimestamp(created, dt.timezone.utc))


def release_entry(path: str, descriptor_sha256: str, notes: dict | None,
                  signatures: dict) -> dict:
    """How the index lists a release: its descriptor at @p path and that
    descriptor's hash, its @p notes (notes_fields()), and when each file's
    maintainer signature was made and by whom (the manifest's is not a
    file a client gets)."""
    return {"descriptor": path, "sha256": descriptor_sha256} | \
        (notes or {}) | {"signatures": {
            name: {"signer": sig["signer"], "created": iso_of(sig["created"])}
            for name, sig in sorted(signatures.items())
            if name != layout.SUMS}}


def withdrawn(policy: dict) -> set:
    """The versions @p policy withdraws: never listed, whatever is admitted."""
    return {parse_version(w["version"]) for w in policy.get("withdrawn", [])}


def newer(a: dict, b: dict | None) -> bool:
    return b is None or \
        parse_version(a["version"]) > parse_version(b["version"])


def kept_builds(previous: dict | None, path: str
                 ) -> tuple[dict | None, list[dict], list[dict]]:
    """What the published index lists for the descriptor at @p path, which
    could not be read this round: (as the nightly, as betas, as release
    candidates)."""
    previous = previous or {}
    nightly = previous.get("nightly")
    nightly = nightly if descriptor.names_build(nightly) and \
        nightly["descriptor"] == path else None
    series = previous.get("series")
    entries = [s for s in (series.values() if isinstance(series, dict)
                           else []) if isinstance(s, dict)]

    def named(builds: list) -> list[dict]:
        return [b for b in builds
                if descriptor.names_build(b) and b["descriptor"] == path][:1]
    return (nightly,
            named([previous.get("beta")] + [s.get("beta") for s in entries]),
            named([s.get("rc") for s in entries]))


def series_table(policy: dict, releases: dict[str, dict],
                  withdrawn: set) -> dict[str, dict]:
    """Each series as the policy states it, with its newest release that is
    not withdrawn (latest); its beta is chosen later (choose_betas)."""
    series = {}
    for key, entry in policy["series"].items():
        series[key] = {k: entry.get(k) for k in ("status", "eol", "successor")}
        series[key]["platforms"] = entry.get("platforms", {})
        series[key]["latest"] = None
        series[key]["beta"] = None
        series[key]["rc"] = None
        series[key]["prerelease"] = False
        if series[key]["eol"] is not None:
            series[key]["eol"] = str(series[key]["eol"])
    # The stable channel is then the newest release of the series users are
    # meant to be on (stable, or mainline once its .0 shipped and before the
    # policy moved on). The nightly and beta channels are builds, never
    # releases.
    for version in sorted(releases, key=parse_version):
        if parse_version(version) in withdrawn:
            continue
        key = parse_version(version).series
        if key in series:
            series[key]["latest"] = version
    return series


def choose_rcs(series: dict[str, dict], rcs: list[dict]) -> None:
    """Sets each series' release candidate: its newest X.Y.0-rc.N, while
    the series has no release and users are meant to move to it (stable,
    or mainline before the cut). Such a series is a prerelease: offered to
    those who ask for betas. Its release makes it an ordinary series again,
    and the candidate is no longer listed."""
    for b in rcs:
        v = parse_version(b["version"])
        entry = series.get(v.series)
        if entry is None or not v.is_release_candidate or \
                entry["status"] not in policy_mod.CURRENT_STATUSES or \
                entry["latest"] is not None:
            continue
        if newer(b, entry["rc"]):
            entry["rc"] = b
            entry["prerelease"] = True


def choose_betas(series: dict[str, dict], betas: list[dict]) -> None:
    """Sets each series' beta: its release branch's newest build, when users
    are meant to be on that series and the build is newer than its latest
    release. Admitted as a nightly is; a user opts in, and is only ever
    offered the beta of the series they run."""
    for b in betas:
        v = parse_version(b["version"])
        entry = series.get(v.series)
        if entry is None or \
                entry["status"] not in policy_mod.CURRENT_STATUSES or \
                (entry["latest"] is not None and
                 v <= parse_version(entry["latest"])):
            continue
        if newer(b, entry["beta"]):
            entry["beta"] = b


def content(slug: str, policy: dict, root_version: int,
            index_keys_version: int, *, releases: dict[str, dict],
            nightly: dict | None, betas: list[dict],
            rcs: list[dict]) -> dict:
    """The index's content, without serial and times: @p releases ({version:
    entry}), the newest build of main (@p nightly), and the builds of
    release branches (@p betas) and of release candidates' tags (@p rcs)
    that were admitted, as @p policy has the series."""
    series = series_table(policy, releases, withdrawn(policy))
    current = [s["latest"] for s in series.values()
               if s["status"] in policy_mod.CURRENT_STATUSES and s["latest"]]
    stable = max(current, key=parse_version) if current else None
    choose_betas(series, betas)
    choose_rcs(series, rcs)
    # Two channels for all: the stable release and main's nightly. Betas and
    # release candidates are each series' own (series.<X.Y>.beta and .rc);
    # the beta and the prerelease that once stood for all of them here were
    # read by no released client, and are gone.
    return {
        "schema": SCHEMA, "product": slug,
        # Where a client finds what it needs to trust this index:
        # v1/root/ up to this version, and this index-keys list.
        "root_version": root_version,
        "index_keys_version": index_keys_version,
        "series": series,
        "channels": {"stable": stable,
                     "nightly": nightly["version"] if nightly else None},
        "withdrawn": [{"version": w["version"], "reason": w["reason"]}
                      for w in policy.get("withdrawn", [])],
        "releases": releases,
        "nightly": nightly,
    }


def summary(index: dict) -> dict[str, str | None]:
    """What an index offers, as people read it: the stable release, the
    newest beta of any series, the release candidate of the newest series
    that has one (the prerelease), and the nightly. An older index that
    says the beta and the prerelease for all itself is read as it says."""
    channels = index.get("channels") or {}
    series = index.get("series") or {}

    def version(entry) -> str | None:
        return entry.get("version") if isinstance(entry, dict) else None
    betas = [version(s.get("beta")) for s in series.values()
             if isinstance(s, dict) and version(s.get("beta"))]
    rcs = [version(series[k].get("rc"))
           for k in sorted(series, key=policy_mod.series_order)
           if isinstance(series[k], dict) and version(series[k].get("rc"))]
    return {
        "stable": channels.get("stable"),
        "beta": channels.get("beta", max(betas, key=parse_version,
                                         default=None)),
        "prerelease": channels.get("prerelease", rcs[-1] if rcs else None),
        "nightly": channels.get("nightly"),
    }


def dated(content: dict, serial: int, now: dt.datetime) -> dict:
    """@p content as published: its @p serial, made @p now and valid for
    LIFETIME."""
    return dict(content, serial=serial, generated_at=iso(now),
                expires=iso(now + LIFETIME))
