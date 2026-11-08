# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The offline stage of a stable release, on the maintainer's own machine.

CI only stages a tag build (gfrelease stage); a stable release reaches users
only through this, run by hand:

  fetch        the staged build of tag v<version>              (network)
  verify       before anything is signed: its Sigstore bundle names this
               product's workflow at exactly that tag and commit, every file
               matches the signed manifest, the tag is signed by a maintainer
               key and points at that commit, and git calls it that version
  authenticode the Windows installers, with the certificate on its token;
               the time stamp is the one network call               (TSA)
  pgp          a detached .sig for every final file, a final SHA256SUMS.txt
               and its .sig, and the release notes the tag carries, signed
  upload       everything to releases/<version>/ and notes/, the descriptor
               last                                                 (network)

After the release, `--notes-only` signs and uploads main's copy of the notes
(a typo fixed there); the host then publishes it in place of the old one.

Each step records itself in <work>/<version>/state.json and is skipped once
done, so a run interrupted anywhere (a token unplugged, a connection lost)
resumes where it stopped; `--redo STEP` runs that step and every one after it
again. The signing host then admits the release on its next round.

Authenticode changes the bytes it signs. The descriptor keeps both hashes:
`build_sha256`, which the CI's signed manifest covers, and `sha256`, of the
file as published, with `transform: authenticode` saying why they differ.
"""

from __future__ import annotations

import json
import shutil
from pathlib import Path

import authenticode
import openpgp
from common import (BUNDLE, DESCRIPTOR, SCHEMA, STAGED, SUMS, Product,
                    ReleaseError, canonical_json, iso, load_json, notes_at,
                    parse_version, render_manifest, sha256_bytes, sha256_file,
                    tagged_commit, utcnow)
from stage import read_staged
from store import Store

import release_candidate as rc  # noqa: E402  (scripts/, on the path via common)

STEPS = ("fetch", "verify", "authenticode", "pgp", "upload")


class Stage:
    def __init__(self, product: Product, version: str, work: Path, *,
                 staging: Store | None = None, builds: Store | None = None,
                 git_dir: Path | None = None, cosign: str = "cosign",
                 key: str | None = None, gnupghome: str | None = None,
                 signer: authenticode.Signer | None = None,
                 authenticode_ca: str | None = None):
        v = parse_version(version)
        if v.prerelease or v.build:
            raise ReleaseError(f"{version}: the offline stage is for final "
                               "releases (vX.Y.Z)")
        self.product = product
        self.version = str(v)
        self.tag = f"v{self.version}"
        self.dir = Path(work) / self.version
        self.staging, self.builds = staging, builds
        self.git_dir, self.cosign = git_dir, cosign
        self.key = (key or (product.maintainers[0] if product.maintainers
                            else "")).upper()
        self.gnupghome = gnupghome
        self.signer = signer
        self.authenticode_ca = authenticode_ca
        self.dir.mkdir(parents=True, exist_ok=True)
        self.state_path = self.dir / "state.json"
        self.state = (load_json(self.state_path.read_text(),
                                str(self.state_path))
                      if self.state_path.exists() else
                      {"version": self.version, "done": []})

    # --- bookkeeping ------------------------------------------------------

    def _save(self) -> None:
        tmp = self.state_path.with_suffix(".new")
        tmp.write_text(json.dumps(self.state, indent=1, sort_keys=True))
        tmp.replace(self.state_path)

    def run(self, redo: str | None = None) -> dict | None:
        if redo:
            if redo not in STEPS:
                raise ReleaseError(f"no step {redo!r}; steps are {STEPS}")
            keep = STEPS[:STEPS.index(redo)]
            self.state["done"] = [s for s in self.state["done"] if s in keep]
        for step in STEPS:
            if step in self.state["done"]:
                continue
            getattr(self, f"_{step}")()
            self.state["done"].append(step)
            self._save()
        return self.state.get("descriptor")

    def _sign_notes(self, ref: str) -> tuple[Path, str]:
        """The notes @p ref has for this release, written content-addressed
        under <work>/notes/ and signed; returns the file and its hash."""
        data = notes_at(self.git_dir, ref, self.version)
        if data is None:
            raise ReleaseError(f"{ref} carries no "
                               f"release/notes/{self.version}.md")
        digest = sha256_bytes(data)
        local = self.dir / "notes" / f"{digest}.md"
        local.parent.mkdir(parents=True, exist_ok=True)
        local.write_bytes(data)
        openpgp.sign(local, self.key, self.gnupghome)
        return local, digest

    def publish_notes(self, ref: str = "refs/heads/main") -> str:
        """Signs and uploads the notes @p ref has for this release; returns
        their hash. Content-addressed, so the old notes stay where they are
        and nothing published is ever replaced."""
        self._need("key", "git_dir", "builds")
        local, digest = self._sign_notes(ref)
        with openpgp.Keyring(self.product.keyring_path,
                             self.product.maintainers) as ring:
            ring.verify(local, local.with_name(local.name + ".sig"))
        for suffix in ("", ".sig"):
            self.builds.put_file(local.with_name(local.name + suffix),
                                 f"notes/{self.version}/{digest}.md{suffix}")
        return digest

    @property
    def staged_dir(self) -> Path:
        return self.dir / "staged"

    @property
    def final_dir(self) -> Path:
        return self.dir / "final"

    @property
    def prefix(self) -> str:
        return f"releases/{self.version}"

    def _need(self, *names: str) -> None:
        missing = [n for n in names if not getattr(self, n)]
        if missing:
            raise ReleaseError(f"this step needs {', '.join(missing)}")

    # --- steps --------------------------------------------------------------

    def _fetch(self) -> None:
        self._need("staging")
        if self.staged_dir.exists():
            shutil.rmtree(self.staged_dir)
        self.staging.download(f"tags/{self.version}", self.staged_dir)
        if not (self.staged_dir / STAGED).is_file():
            raise ReleaseError(f"nothing complete is staged for {self.tag}")

    def _verify(self) -> None:
        self._need("git_dir")
        staged = read_staged((self.staged_dir / STAGED).read_bytes())
        ref = f"refs/tags/{self.tag}"
        if staged["product"] != self.product.slug or staged["ref"] != ref or \
                staged["version"] != self.version:
            raise ReleaseError(f"staging holds {staged['product']} "
                               f"{staged['ref']} {staged['version']}, not {ref}")

        build = self.dir / "build"
        if build.exists():
            shutil.rmtree(build)
        shutil.copytree(self.staged_dir, build)
        (build / STAGED).unlink()
        identity = self.product.identity(ref)
        try:
            verified = rc.verify(build, identity, staged["commit"], self.cosign)
        except rc.CandidateError as e:
            raise ReleaseError(f"{self.tag}: {e}") from e
        files = sorted(p.name for p in verified if p.name not in (SUMS, BUNDLE))
        if files != sorted(staged["files"]):
            raise ReleaseError(f"{self.tag}: staged.json lists other files "
                               "than the signed manifest")

        with openpgp.Keyring(self.product.keyring_path,
                             self.product.maintainers) as ring:
            ring.verify_tag(self.git_dir, self.tag)
        tagged = tagged_commit(self.git_dir, self.tag, expect=staged["commit"],
                               version=self.version)

        self.state["build"] = {
            "commit": tagged, "built_at": staged["built_at"],
            "identity": identity,
            "files": {n: {**self.product.classify(n, self.version),
                          "build_sha256": sha256_file(build / n)}
                      for n in files}}

    def _authenticode(self) -> None:
        build = self.dir / "build"
        if self.final_dir.exists():
            shutil.rmtree(self.final_dir)
        self.final_dir.mkdir()
        files = self.state["build"]["files"]
        for name, info in sorted(files.items()):
            if info["kind"] in authenticode.SIGNED_KINDS:
                if self.signer is None:
                    raise ReleaseError(f"{name} needs an Authenticode signer "
                                       "(--authenticode-cert/-key)")
                self.signer.sign(build / name, self.final_dir / name)
                authenticode.verify(self.final_dir / name,
                                    self.product.authenticode_subject,
                                    self.authenticode_ca)
                info["transform"] = "authenticode"
            else:
                shutil.copy2(build / name, self.final_dir / name)
                info["transform"] = "none"
            info["sha256"] = sha256_file(self.final_dir / name)

    def _pgp(self) -> None:
        self._need("key", "git_dir")
        files = self.state["build"]["files"]
        for stale in self.final_dir.glob("*.sig"):
            stale.unlink()
        for name in sorted(files):
            openpgp.sign(self.final_dir / name, self.key, self.gnupghome)
        (self.final_dir / SUMS).write_bytes(render_manifest(
            {n: files[n]["sha256"] for n in files}))
        openpgp.sign(self.final_dir / SUMS, self.key, self.gnupghome)

        # The notes the tag carries, reviewed before it was made.
        notes_dir = self.dir / "notes"
        if notes_dir.exists():
            shutil.rmtree(notes_dir)
        _, digest = self._sign_notes(f"refs/tags/{self.tag}")
        self.state["notes_sha256"] = digest

        # Everything signed is checked against the pinned keys before it
        # leaves this machine.
        with openpgp.Keyring(self.product.keyring_path,
                             self.product.maintainers) as ring:
            for name in [*sorted(files), SUMS]:
                ring.verify(self.final_dir / name,
                            self.final_dir / f"{name}.sig")
            ring.verify(notes_dir / f"{digest}.md",
                        notes_dir / f"{digest}.md.sig")

    def _upload(self) -> None:
        self._need("builds")
        files, p = self.state["build"]["files"], self.prefix
        artifacts = []
        for name in sorted(files):
            info = files[name]
            self.builds.put_file(self.final_dir / name, f"{p}/{name}")
            self.builds.put_file(self.final_dir / f"{name}.sig",
                                 f"{p}/{name}.sig")
            artifacts.append({
                k: info[k] for k in ("os", "arch", "kind", "flavor", "min_os",
                                     "sha256", "build_sha256", "transform")
                if k in info} | {
                "name": name, "size": (self.final_dir / name).stat().st_size,
                "path": f"{p}/{name}", "sig": f"{p}/{name}.sig"})
        self.builds.put_file(self.final_dir / SUMS, f"{p}/{SUMS}")
        self.builds.put_file(self.final_dir / f"{SUMS}.sig", f"{p}/{SUMS}.sig")
        build = self.dir / "build"
        self.builds.put_file(build / SUMS, f"{p}/build/{SUMS}")
        self.builds.put_file(build / BUNDLE, f"{p}/build/{BUNDLE}")

        digest = self.state["notes_sha256"]
        for suffix in ("", ".sig"):
            self.builds.put_file(self.dir / "notes" / f"{digest}.md{suffix}",
                                 f"notes/{self.version}/{digest}.md{suffix}")

        b = self.state["build"]
        descriptor = {
            "schema": SCHEMA, "product": self.product.slug, "kind": "stable",
            "version": self.version, "series": parse_version(self.version).series,
            "ref": f"refs/tags/{self.tag}", "commit": b["commit"],
            "built_at": b["built_at"], "released_at": iso(utcnow()),
            "signing": {"scheme": "openpgp", "fingerprint": self.key},
            "build": {"sums": f"{p}/build/{SUMS}",
                      "bundle": f"{p}/build/{BUNDLE}",
                      "identity": b["identity"]},
            "sums": f"{p}/{SUMS}", "sums_sig": f"{p}/{SUMS}.sig",
            "artifacts": artifacts,
        }
        # Last: a descriptor only ever names objects that are already there.
        self.builds.put_bytes(canonical_json(descriptor), f"{p}/{DESCRIPTOR}")
        self.state["descriptor"] = descriptor
