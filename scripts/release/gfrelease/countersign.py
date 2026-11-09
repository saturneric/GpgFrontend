# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The release server's minisign countersignature on a stable or imported
release, made only after it was admitted.

A stable release reaches the server signed by a maintainer: an OpenPGP
`.sig` on every file, on the manifest and on the notes. The server checks
every one (index.admit_stable, admit_imported); a release with any of them
missing or invalid gets nothing from here and no index entry. One that
passes is signed again, with the product's artifact key, the way a nightly
is:

  releases/<v>/<file>.minisig           every file
  releases/<v>/SHA256SUMS.txt.minisig   the manifest (an imported release
                                        has none: the server writes one
                                        from the descriptor first, at
                                        releases/<v>/manifests/<sha256>/)
  notes/<v>/<sha256>.md.minisig         the notes, once their .sig verified
  commits/<sha>                         the commit marker

The `.sig` files are for people who check by hand; the application relies
on minisign alone, through the index (index -> descriptor -> file hashes),
the same chain as for a nightly.

Every object is immutable. One already there is kept when it verifies with
the current artifact key or one it replaced (a rotation re-signs nothing);
one there with other bytes is an error, never overwritten.
"""

from __future__ import annotations

from pathlib import Path

import minisig
import layout
from common import (MINISIG, SUMS, Product, ReleaseError, parse_manifest,
                    render_manifest, sha256_bytes, sha256_file)
from store import Store

NOTES_NAME = "notes.md"  # what a notes' countersignature calls its file


def manifest_path(d: dict) -> str:
    """The manifest the countersignature covers: the maintainer's signed
    SHA256SUMS.txt, or for an imported release the one the server writes,
    named after its own hash. A published object is never replaced, and a
    hotfix (a file signed again, later) is a new manifest: it gets its own
    path beside the old one."""
    if d.get("sums"):
        return d["sums"]
    return layout.server_manifest(
        d["version"], sha256_bytes(render_manifest(hashes_of(d))))


def hashes_of(d: dict) -> dict[str, str]:
    """{name: sha256} of a descriptor's files: what its manifest lists."""
    return {a["name"]: a["sha256"] for a in d["artifacts"]}


def entry_fields(d: dict, notes: dict | None) -> dict:
    """The index entry's paths to the countersignatures. They follow from
    the descriptor alone, so a dry run lists exactly what a real run does."""
    fields = {"sums_minisig": manifest_path(d) + MINISIG}
    if notes:
        fields["notes_minisig"] = notes["notes"] + MINISIG
    return fields


class Countersigner:
    """The artifact key, as the release server holds it."""

    def __init__(self, product: Product, secret_key: Path | None,
                 public_key: str, dry: bool = False):
        """With @p dry (a dry run, no secret key) nothing is signed or
        written; a countersignature already there that does not verify is
        reported exactly as the real run would."""
        self.product = product
        self.dry = dry
        self.secret_key = Path(secret_key) if secret_key else None
        self.public_key = public_key
        self.accepted = product.artifact_keys(public_key)
        self.key_id = minisig.key_id(public_key)

    def _sign(self, builds: Store, path: str, local: Path, comment: str) -> None:
        """@p local (the bytes at @p path) countersigned at path.minisig,
        unless a valid countersignature is there already."""
        target = path + MINISIG
        if builds.exists(target):
            existing = builds.fetch_to(target, local.with_name(local.name +
                                                               ".existing"))
            try:
                minisig.verify(self.accepted, local, existing)
                return
            except ReleaseError as e:
                raise ReleaseError(f"{target} exists and does not verify "
                                   f"{path}; a published object is never "
                                   f"replaced ({e})") from e
        if self.dry:
            return
        sig = minisig.sign(self.secret_key, local, comment)
        minisig.verify([self.public_key], local, sig)
        builds.put_file(sig, target)

    def release(self, builds: Store, d: dict, work: Path) -> None:
        """Every file, the manifest and the commit marker of an admitted
        release. A file admission already downloaded into @p work (and
        whose hash still holds) is not fetched again."""
        version, slug = d["version"], self.product.slug
        work.mkdir(parents=True, exist_ok=True)
        for a in d["artifacts"]:
            if self.dry and not builds.exists(a["path"] + MINISIG):
                continue  # nothing to sign in a dry run, nothing to clash
            local = work / a["name"]
            if not (local.is_file() and sha256_file(local) == a["sha256"]):
                builds.fetch_to(a["path"], local)
            if sha256_file(local) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the descriptor's bytes")
            self._sign(builds, a["path"], local,
                       layout.file_comment(slug, a["name"], version))
            local.unlink()

        sums = manifest_path(d)
        local = work / SUMS
        hashes = hashes_of(d)
        if d.get("sums"):
            builds.fetch_to(sums, local)
        else:
            local.write_bytes(render_manifest(hashes))
            if not self.dry:
                builds.put_file(local, sums)
        # Signed only as exactly the hashes admission checked, never as
        # whatever the bucket holds by now: the artifact key's signature on
        # a manifest is what a nightly is trusted on too.
        if parse_manifest(local.read_bytes(), sums) != hashes:
            raise ReleaseError(f"{sums}: not the admitted release's hashes; "
                               "nothing is countersigned")
        self._sign(builds, sums, local,
                   layout.file_comment(slug, SUMS, version))

        marker = layout.commit_marker(d["commit"])
        if not self.dry and not builds.exists(marker):
            builds.put_bytes(b"", marker)

    def notes(self, builds: Store, version: str, notes: dict,
              work: Path) -> None:
        """Notes whose maintainer signature verified: the bytes signed are
        the ones whose hash was admitted, whatever the bucket holds now."""
        work.mkdir(parents=True, exist_ok=True)
        local = builds.fetch_to(notes["notes"], work / NOTES_NAME)
        if sha256_file(local) != notes["notes_sha256"]:
            raise ReleaseError(f"{notes['notes']}: not the admitted notes; "
                               "nothing is countersigned")
        self._sign(builds, notes["notes"], local, layout.file_comment(
            self.product.slug, NOTES_NAME, version))
