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
again (a signing step only until the upload has begun). The signing host
then checks every signature again on its next round and lists it: the
application relies on the signed index, which names the descriptor by its
hash, and the descriptor every file's; the `.sig` files are for checking by
hand.

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
import layout
from common import (BUNDLE, SCHEMA, SIG, STAGED, SUMS, Product, ReleaseError,
                    atomic_write, canonical_json, iso, load_json, notes_at,
                    parse_version, refs, render_manifest, sha256_bytes,
                    sha256_file, tagged_commit, utcnow, verify_candidate)
from stage import read_staged
from store import Store


STEPS = ("fetch", "verify", "authenticode", "pgp", "upload")


class Stage:
    def __init__(self, product: Product, version: str, work: Path, *,
                 staging: Store | None = None, builds: Store | None = None,
                 git_dir: Path | None = None, cosign: str = "cosign",
                 key: str | None = None, gnupghome: str | None = None,
                 signer: authenticode.Signer | None = None,
                 authenticode_ca: str | None = None):
        v = parse_version(version)
        if not v.is_release:
            raise ReleaseError(f"{version}: the offline stage is for final "
                               "releases (vX.Y.Z)")
        self.product = product
        self.version = str(v)
        self.tag = refs.release_tag(self.version)
        self.dir = Path(work) / self.version
        self.staging, self.builds = staging, builds
        self.git_dir, self.cosign = git_dir, cosign
        self.requested_key = (key or "").upper()
        self.gnupghome = gnupghome
        self.signer = signer
        self.authenticode_ca = authenticode_ca
        self.dir.mkdir(parents=True, exist_ok=True)
        self.state_path = self.dir / "state.json"
        self.state = (load_json(self.state_path.read_text(),
                                str(self.state_path))
                      if self.state_path.exists() else
                      {"version": self.version, "done": []})
        if self.state.get("version") != self.version:
            raise ReleaseError(f"{self.state_path} is the stage of "
                               f"{self.state.get('version')!r}, not "
                               f"{self.version}")
        # The key that signed this release, once it has: a resumed stage
        # names it in the descriptor, whatever the default is by now (a
        # maintainer key rotated meanwhile).
        signed_with = self.state.get("key")
        if signed_with and self.requested_key and \
                self.requested_key != signed_with:
            self._conflict = (f"{self.version} is signed with {signed_with}; "
                              f"--key {self.requested_key} cannot finish it")
        else:
            self._conflict = None
        self.key = signed_with or self.requested_key or product.signing_key
        if not signed_with:
            self._check_signing_key()

    # --- bookkeeping ------------------------------------------------------

    def _check_signing_key(self, key: str | None = None) -> None:
        """A key that signs anything new: the descriptor names it, and the
        host admits only a pinned primary fingerprint. A subkey's, or a
        retired key's, would sign everything here and never be listed."""
        key = key or self.key
        if key and key not in self.product.maintainers:
            raise ReleaseError(f"--key {key} is not a current pinned "
                               "maintainer's primary fingerprint "
                               "(the trust root's host_policy)")

    def _save(self) -> None:
        # Whole or not at all, and on disk before the next step starts: a
        # machine that loses power mid-release resumes, it never signs
        # again what it already sent.
        atomic_write(self.state_path,
                     json.dumps(self.state, indent=1, sort_keys=True))

    def run(self, redo: str | None = None) -> dict | None:
        if self._conflict:
            raise ReleaseError(self._conflict)
        if redo:
            if redo not in STEPS:  # the command line offers only these
                raise ReleaseError(f"no step {redo!r}; steps are {STEPS}")
            # Signing again makes other bytes (new time stamps), and what was
            # uploaded is never replaced: once anything left this machine,
            # only the upload itself can be run again.
            if redo != "upload" and self.state.get("upload_started"):
                raise ReleaseError(
                    f"{self.version}: the upload has begun, so {redo} cannot "
                    "run again (it would sign other bytes than the ones "
                    "published); only --redo upload can")
            keep = STEPS[:STEPS.index(redo)]
            self.state["done"] = [s for s in self.state["done"] if s in keep]
            if "pgp" not in keep:
                # Signed again from scratch: by the key asked for now.
                self.state.pop("key", None)
                self.key = self.requested_key or self.product.signing_key
                self._check_signing_key()
        for step in STEPS:
            if step in self.state["done"]:
                continue
            getattr(self, f"_{step}")()
            self.state["done"].append(step)
            self._save()
        return self.state.get("descriptor")

    def _notes(self, ref: str) -> tuple[bytes, str]:
        """The notes @p ref has for this release, and their hash."""
        data = notes_at(self.git_dir, ref, self.version)
        if data is None:
            raise ReleaseError(f"{ref} carries no "
                               f"release/notes/{self.version}.md")
        return data, sha256_bytes(data)

    def _sign_notes(self, data: bytes, digest: str, directory: str = "notes",
                    key: str | None = None) -> Path:
        """Notes written content-addressed under <work>/<directory>/ and
        signed; returns the file."""
        local = self.dir / directory / f"{digest}.md"
        atomic_write(local, data)
        openpgp.sign(local, key or self.key, self.gnupghome)
        return local

    def publish_notes(self, ref: str = "refs/heads/main") -> str:
        """Signs and uploads the notes @p ref has for this release; returns
        their hash. Content-addressed, so the old notes stay where they are
        and nothing published is ever replaced."""
        self._need("key", "git_dir", "builds")
        data, digest = self._notes(ref)
        remote = layout.notes_path(self.version, digest)
        # Signed and uploaded already (the tag's notes, unchanged on main):
        # nothing to do. Signing again would only make a .sig of other
        # bytes, which can never replace the published one.
        if self.builds.exists(remote) and self.builds.exists(remote + SIG) \
                and self.builds.sha256(remote) == digest:
            return digest
        # New notes are signed by a current maintainer, whoever signed the
        # release itself.
        key = self.requested_key or self.product.signing_key
        self._check_signing_key(key)
        # Its own directory: the stage's notes/ holds what an interrupted
        # upload still has to send, and is never signed over.
        local = self._sign_notes(data, digest, "notes-only", key)
        with openpgp.Keyring.of(self.product) as ring:
            ring.verify(local, local.with_name(local.name + SIG))
        for suffix in ("", SIG):
            self.builds.put_file(local.with_name(local.name + suffix),
                                 remote + suffix)
        return digest

    @property
    def staged_dir(self) -> Path:
        return self.dir / "staged"

    @property
    def final_dir(self) -> Path:
        return self.dir / "final"

    @property
    def prefix(self) -> str:
        return layout.release_dir(self.version)

    def _need(self, *names: str) -> None:
        missing = [n for n in names if not getattr(self, n)]
        if missing:
            raise ReleaseError(f"this step needs {', '.join(missing)}")

    # --- steps --------------------------------------------------------------

    def _fetch(self) -> None:
        self._need("staging")
        if self.staged_dir.exists():
            shutil.rmtree(self.staged_dir)
        self.staging.download(layout.staged_tag(self.version),
                              self.staged_dir)
        if not (self.staged_dir / STAGED).is_file():
            raise ReleaseError(f"nothing complete is staged for {self.tag}")

    def _verify(self) -> None:
        self._need("git_dir")
        staged = read_staged((self.staged_dir / STAGED).read_bytes())
        ref = refs.release_tag_ref(self.version)
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
        files = verify_candidate(self.tag, build, staged, identity,
                                 self.cosign)

        with openpgp.Keyring.of(self.product) as ring:
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
        if self.signer is not None and not self.signer.timestamp_url and \
                self.product.authenticode_timestamp_required:
            # Without a time stamp the signature dies with the certificate:
            # every later check of the release would refuse it.
            raise ReleaseError("Authenticode signing needs --timestamp-url "
                               "(the trust root requires a timestamp)")
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
                                    self.authenticode_ca,
                                    require_timestamp=self.product
                                    .authenticode_timestamp_required)
                info["transform"] = "authenticode"
            else:
                shutil.copy2(build / name, self.final_dir / name)
                info["transform"] = "none"
            info["sha256"] = sha256_file(self.final_dir / name)

    def _pgp(self) -> None:
        self._need("key", "git_dir")
        self._check_signing_key()
        self.state["key"] = self.key
        files = self.state["build"]["files"]
        for stale in self.final_dir.glob("*.sig"):
            stale.unlink()
        for name in sorted(files):
            openpgp.sign(self.final_dir / name, self.key, self.gnupghome)
        atomic_write(self.final_dir / SUMS, render_manifest(
            {n: files[n]["sha256"] for n in files}))
        openpgp.sign(self.final_dir / SUMS, self.key, self.gnupghome)

        # The notes the tag carries, reviewed before it was made.
        notes_dir = self.dir / "notes"
        if notes_dir.exists():
            shutil.rmtree(notes_dir)
        data, digest = self._notes(refs.release_tag_ref(self.version))
        self._sign_notes(data, digest)
        self.state["notes_sha256"] = digest

        # Everything signed is checked against the pinned keys before it
        # leaves this machine.
        with openpgp.Keyring.of(self.product) as ring:
            for name in [*sorted(files), SUMS]:
                ring.verify(self.final_dir / name,
                            self.final_dir / (name + SIG))
            ring.verify(notes_dir / f"{digest}.md",
                        notes_dir / f"{digest}.md{SIG}")

    def _upload(self) -> None:
        self._need("builds")
        files, p = self.state["build"]["files"], self.prefix
        # Fixed before anything is sent: a resumed upload makes the same
        # descriptor, byte for byte, as the one it may already have sent.
        self.state["upload_started"] = True
        self.state.setdefault("released_at", iso(utcnow()))
        self._save()
        artifacts = []
        for name in sorted(files):
            info = files[name]
            self.builds.put_file(self.final_dir / name, f"{p}/{name}")
            self.builds.put_file(self.final_dir / (name + SIG),
                                 f"{p}/{name}{SIG}")
            artifacts.append({
                k: info[k] for k in ("os", "arch", "kind", "flavor", "min_os",
                                     "sha256", "build_sha256", "transform")
                if k in info} | {
                "name": name, "size": (self.final_dir / name).stat().st_size,
                "path": f"{p}/{name}", "sig": f"{p}/{name}{SIG}"})
        self.builds.put_file(self.final_dir / SUMS, f"{p}/{SUMS}")
        self.builds.put_file(self.final_dir / (SUMS + SIG),
                             f"{p}/{SUMS}{SIG}")
        build = self.dir / "build"
        self.builds.put_file(build / SUMS, f"{p}/build/{SUMS}")
        self.builds.put_file(build / BUNDLE, f"{p}/build/{BUNDLE}")

        digest = self.state["notes_sha256"]
        for suffix in ("", SIG):
            self.builds.put_file(self.dir / "notes" / f"{digest}.md{suffix}",
                                 layout.notes_path(self.version, digest)
                                 + suffix)

        b = self.state["build"]
        descriptor = {
            "schema": SCHEMA, "product": self.product.slug,
            "kind": layout.KIND_STABLE,
            "version": self.version, "series": parse_version(self.version).series,
            "ref": refs.release_tag_ref(self.version), "commit": b["commit"],
            "built_at": b["built_at"], "released_at": self.state["released_at"],
            "signing": {"scheme": layout.SCHEME_OPENPGP,
                        "fingerprint": self.key},
            "build": {"sums": f"{p}/build/{SUMS}",
                      "bundle": f"{p}/build/{BUNDLE}",
                      "identity": b["identity"]},
            "sums": f"{p}/{SUMS}", "sums_sig": f"{p}/{SUMS}{SIG}",
            "artifacts": artifacts,
        }
        # Last: a descriptor only ever names objects that are already there.
        self.builds.put_bytes(canonical_json(descriptor),
                              layout.descriptor_of(p))
        self.state["descriptor"] = descriptor
