# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Every stable release of the signed index, mirrored to GitHub Releases.

Runs anywhere that can read the published stores and holds a token that
writes the repository's releases (GitHub Actions' GITHUB_TOKEN): nothing
it uploads rests on that machine or on GitHub. Per release the index lists:

  check     as verify does it (check_release): the index's signature
            through the trust root, the descriptor by its hash, the
            manifest; the notes by their hash and maintainer signature
  tag       GitHub's tag v<version> points at the descriptor's commit;
            a tag is never made here
  plan      github_release.plan, from what GitHub has
  carry out one file at a time, each fetched and checked (its sha256 and
            its maintainer signature, as the index records it) before it
            is uploaded and removed again: never more than one installer
            on disk. A new release is published only once GitHub states
            every file's sha256 as the index's.

A release refused (a file that does not check, a tag elsewhere, a
published file with other bytes) does not hold up the others.
"""

from __future__ import annotations

import contextlib
import tempfile
from pathlib import Path

from ..adapters import openpgp
from ..adapters.files import sha256_file
from ..adapters.github import GitHub
from ..domain import github_release as gh
from ..domain.formats import check_relpath, parse_version
from ..domain.layout import SIG
from ..errors import Problems, ReleaseError
from . import verify
from .product import Product


def sync(product: Product, update, builds, github: GitHub, *,
         builds_base: str, versions: list[str] | None = None,
         dry_run: bool = False) -> dict:
    """GitHub's releases made what the index published by @p update says,
    of the files in @p builds (whose public URL is @p builds_base), only
    for @p versions when given; with @p dry_run only the plans. Returns a
    report; its problems are what was refused, or could not be done."""
    index = verify.read_index(product, update)
    problems = Problems()
    report = {"product": product.slug, "serial": index["serial"],
              "repository": github.repository, "dry_run": dry_run,
              "releases": [], "problems": problems}
    listed = index["releases"]
    for version in versions or []:
        if version not in listed:
            problems.note(version, ReleaseError("not a release the index "
                                                "lists"))
    with tempfile.TemporaryDirectory(prefix="gfr-github-") as tmp, \
            contextlib.ExitStack() as stack:
        ring = None

        def keyring():
            nonlocal ring
            if ring is None:
                ring = stack.enter_context(
                    openpgp.Keyring.of(product, retired=True))
            return ring

        def fetch(rel: str, name: str) -> Path:
            return builds.fetch_to(check_relpath(rel),
                                   Path(tmp) / Path(name).name)

        latest = []  # GitHub's latest release's tag, asked once

        def latest_tag():
            if not latest:
                latest.append(github.latest_tag())
            return latest[0]

        for version in sorted(listed, key=parse_version):
            if versions and version not in versions:
                continue
            one = {"version": version, "steps": [], "state": None}
            try:
                _one(product, index, version, github, builds, builds_base,
                     fetch, keyring, latest_tag, dry_run, one)
            except ReleaseError as e:
                one["state"] = "refused"
                problems.note(version, e)
            if one["steps"] and not dry_run:
                # Steps may have made another release the latest.
                latest.clear()
            if one["state"] != "skipped" or versions:
                report["releases"].append(one)
    return report


def _one(product, index, version, github, builds, builds_base, fetch,
         keyring, latest_tag, dry_run, one) -> None:
    entry = index["releases"][version]
    d = verify.check_release(product, builds, version, entry)
    if not gh.mirrored(d):
        one["state"] = "skipped"
        one["detail"] = f"{d['kind']}, not mirrored"
        return
    notes = verify.checked_notes(entry, fetch, keyring)
    if notes is None:
        raise ReleaseError("listed without release notes")
    small = {}
    for a in d["artifacts"]:
        local = fetch(a["sig"], Path(a["sig"]).name)
        small[a["sig"]] = (sha256_file(local), local.stat().st_size)
    want = gh.desired(product.repository, builds_base, index, d, entry,
                      notes.read_text("utf-8"), small)

    commit = github.tag_commit(want.tag)
    if commit is None:
        raise ReleaseError(f"GitHub has no tag {want.tag}")
    if commit != want.commit:
        raise ReleaseError(f"GitHub's tag {want.tag} points at {commit}, "
                           f"the release was built from {want.commit}")
    have = github.find(want.tag)
    steps = gh.plan(want, have, latest_tag())
    one["steps"] = [_said(s) for s in steps]
    conflicts = [s for s in steps if s.kind == "conflict"]
    if conflicts:
        raise ReleaseError("; ".join(
            f"{s.asset.name} on GitHub is not the published file "
            f"({s.detail})" for s in conflicts))
    if not steps:
        one["state"] = "unchanged"
        return
    if dry_run:
        one["state"] = "planned"
        return
    _carry_out(want, have, steps, github, d, entry, fetch, keyring)
    one["state"] = "synced"
    one["url"] = github.page(want.tag)


def _said(step: gh.Step) -> str:
    return step.kind + (f" {step.asset.name}" if step.asset else "") + \
        (f" ({step.detail})" if step.detail else "")


def _carry_out(want: gh.Release, release: dict | None, steps: list,
               github: GitHub, d: dict, entry: dict, fetch,
               keyring) -> None:
    latest = "true" if want.latest else "false"
    for step in steps:
        if step.kind == "create":
            release = github.create(want.tag, want.name, want.body)
        elif step.kind == "edit":
            fields = {"name": want.name, "body": want.body}
            if not release.get("draft"):
                fields["make_latest"] = latest
            release = github.update(release, **fields)
    # The files, each checked before it leaves; a file and its signature
    # are checked together.
    by_name = {a["name"]: a for a in d["artifacts"]}
    files = [s for s in steps if s.kind in ("upload", "replace")]
    present = {a["name"]: a for a in release.get("assets", [])}
    checked: dict[str, Path] = {}
    for step in files:
        name = step.asset.name
        if name not in checked:
            checked.update(_checked(name, by_name, d, entry, fetch,
                                    keyring))
        if step.kind == "replace":
            github.delete_asset(present[name])
        github.upload(release, name, checked[name])
        # An installer leaves the disk once it is up.
        if name in by_name:
            checked.pop(name).unlink()
    if any(s.kind == "publish" for s in steps):
        # What GitHub now has, file by file, before anyone sees it.
        fresh = github.find(want.tag)
        left = gh.plan(want, fresh, None)
        if [s.kind for s in left] != ["publish"]:
            raise ReleaseError("GitHub's draft is not the release after "
                               "the upload: " +
                               ", ".join(_said(s) for s in left))
        github.update(fresh, draft=False, make_latest=latest)
    elif any(s.kind == "latest" for s in steps):
        github.update(release, make_latest="true")


def _checked(name: str, by_name: dict, d: dict, entry: dict, fetch,
             keyring) -> dict[str, Path]:
    """File @p name of the release and what goes with it, fetched and
    checked: an installer and its signature (verify.checked_artifact)."""
    base = name[:-len(SIG)] if name.endswith(SIG) else name
    if base in by_name:
        local = verify.checked_artifact(d, by_name[base], entry, fetch,
                                        keyring)
        return {base: local, base + SIG: local.with_name(base + SIG)}
    raise ReleaseError(f"{name}: not a file of the release")
