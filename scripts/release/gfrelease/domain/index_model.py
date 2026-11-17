# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What the index says, from what was admitted: the series table, each
channel and each series' beta and release candidate. Decided here, apart
from how anything is admitted or published (app/index.py), and nothing here
reads a store or runs a tool."""

from __future__ import annotations

import datetime as dt

from . import descriptor
from . import policy as policy_mod
from .formats import SCHEMA, parse_version

LIFETIME = dt.timedelta(days=policy_mod.INDEX_LIFETIME_DAYS)
VOLATILE = ("serial", "generated_at", "expires")


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


def choose_rcs(series: dict[str, dict], rcs: list[dict]) -> dict | None:
    """Sets each series' release candidate: its newest X.Y.0-rc.N, while
    the series has no release and users are meant to move to it (stable,
    or mainline before the cut). Such a series is a prerelease: offered to
    those who ask for betas. Its release makes it an ordinary series again,
    and the candidate is no longer listed. Returns the candidate of the
    newest prerelease series: the prerelease channel."""
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
    newest = None
    for key in sorted(series, key=policy_mod.series_order):
        if series[key]["rc"] is not None:
            newest = series[key]["rc"]
    return newest


def choose_betas(series: dict[str, dict], betas: list[dict]) -> dict | None:
    """Sets each series' beta: its release branch's newest build, when users
    are meant to be on that series and the build is newer than its latest
    release. Admitted as a nightly is; a user opts in, and is only ever
    offered the beta of the series they run. Returns the newest of them
    all, where clients older than the per-series entry look for it."""
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
    beta = None
    for entry in series.values():
        if entry["beta"] is not None and newer(entry["beta"], beta):
            beta = entry["beta"]
    return beta


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
    beta = choose_betas(series, betas)
    prerelease = choose_rcs(series, rcs)
    return {
        "schema": SCHEMA, "product": slug,
        # Where a client finds what it needs to trust this index:
        # v1/root/ up to this version, and this index-keys list.
        "root_version": root_version,
        "index_keys_version": index_keys_version,
        "series": series,
        "channels": {"stable": stable,
                     "beta": beta["version"] if beta else None,
                     "prerelease":
                         prerelease["version"] if prerelease else None,
                     "nightly": nightly["version"] if nightly else None},
        "withdrawn": [{"version": w["version"], "reason": w["reason"]}
                      for w in policy.get("withdrawn", [])],
        "releases": releases,
        "nightly": nightly,
        "beta": beta,
        "prerelease": prerelease,
    }
