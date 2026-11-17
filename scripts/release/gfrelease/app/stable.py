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
  authenticode the Windows packages, inside and out, with the certificate
               on its token: first every program in every MSI and
               portable zip, then the packages themselves, so a token
               that asks its PIN for each run of the signing tool asks
               twice; the time stamps are the one network call      (TSA).
               Between the two, on Windows, the MSI's sideload MSIX is
               made from it (msix.py), and signed with the packages
  pgp          a detached .sig for every final file, a final SHA256SUMS.txt
               and its .sig, and the release notes the tag carries, signed
  upload       everything to the staging store, under signed/ as it is to
               lie in the builds store (releases/<version>/ and notes/),
               the descriptor last                                  (network)

That is where this machine's part ends, and all it may write: the signing
host receives what was handed over (promote.py), checks every signature
again as it does for the index, and only then copies it to the builds
store, which it alone writes.

After the release, `--notes-only` signs and hands over main's copy of the
notes (a typo fixed there); the host then publishes it in place of the old
one. And `--msix` makes the MSIX of a release that was handed over without
one, and hands over only that: the package, its signature, the manifest
that now lists it and the descriptor. The host takes everything else from
what it has published, and adds the package to the release.

Each step records itself in <work>/<version>/state.json and is skipped once
done, so a run interrupted anywhere (a token unplugged, a connection lost)
resumes where it stopped; `--redo STEP` runs that step and every one after it
again (a signing step only until the upload has begun). The signing host
receives it on its next round, checks every signature again and lists it: the
application relies on the signed index, which names the descriptor by its
hash, and the descriptor every file's; the `.sig` files are for checking by
hand.

Authenticode changes the bytes it signs. The descriptor keeps both hashes:
`build_sha256`, which the CI's signed manifest covers, and `sha256`, of the
file as published, with `transform: authenticode` saying why they differ.
An MSIX made here has no build hash: its `transform` is `msix` and `from`
names the MSI it was made from, whose files are all it holds.
Which programs inside a package are signed is product.json's
`authenticode` rule (authenticode.py); everything else in it stays as CI
built it, and the stage checks that before the upload.
"""

from __future__ import annotations

import shutil
import tempfile
from pathlib import Path

from ..adapters import openpgp
from ..adapters.clock import utcnow
from ..adapters.files import atomic_write, sha256_file
from ..adapters.git import notes_at, tagged_commit
from ..adapters.store import Store
from ..adapters.windows import authenticode, msix
from ..cli import ui
from ..domain import descriptor as descriptors
from ..domain import layout, refs
from ..domain.formats import (canonical_json, iso, load_json, parse_version,
                              sha256_bytes)
from ..domain.layout import BUNDLE, SIG, STAGED, SUMS
from ..domain.manifest import render_manifest
from ..errors import ReleaseError
from . import checks
from .candidate import verify_candidate
from .product import Product
from .stage import read_staged

STEPS = ("fetch", "verify", "authenticode", "pgp", "upload")


class Stage:
    def __init__(self, product: Product, version: str, work: Path, *,
                 staging: Store | None = None,
                 git_dir: Path | None = None, cosign: str = "cosign",
                 key: str | None = None, gnupghome: str | None = None,
                 signer: authenticode.Signer | None = None,
                 authenticode_ca: str | None = None, on_signed=None):
        v = parse_version(version)
        if not v.is_release:
            raise ReleaseError(f"{version}: the offline stage is for final "
                               "releases (vX.Y.Z)")
        self.product = product
        self.version = str(v)
        self.tag = refs.release_tag(self.version)
        self.dir = Path(work) / self.version
        self.staging = staging
        # Where everything signed here is handed to the host.
        self.outbox = staging.sub(layout.SIGNED) if staging else None
        self.git_dir, self.cosign = git_dir, cosign
        self.requested_key = (key or "").upper()
        self.gnupghome = gnupghome
        self.signer = signer
        self.authenticode_ca = authenticode_ca
        # on_signed(file, done, total, path): each program signed inside a
        # package, and then the package itself (path None), for whoever
        # shows the progress.
        self.on_signed = on_signed
        self.dir.mkdir(parents=True, exist_ok=True)
        self.state_path = self.dir / layout.STAGE_STATE
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
        atomic_write(self.state_path, canonical_json(self.state))

    def run(self, redo: str | None = None, *, until: str | None = None,
            on_step=None) -> dict | None:
        """Every step not done yet, in order; returns the descriptor once
        published. @p until: stop after that step (a dry run stops after
        verify, before anything is signed). @p on_step(step, number,
        done_before): called before each step, e.g. to show progress or
        ask before the upload; raising stops the stage there, resumably."""
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
        for number, step in enumerate(STEPS, 1):
            done_before = step in self.state["done"]
            if on_step:
                on_step(step, number, done_before)
            if not done_before:
                getattr(self, f"_{step}")()
                self.state["done"].append(step)
                self._save()
            if step == until:
                return None
        return self.state.get("descriptor")

    def _notes(self, ref: str) -> tuple[bytes, str]:
        """The notes @p ref has for this release, and their hash."""
        data = notes_at(self.git_dir, ref, self.version)
        if data is None:
            raise ReleaseError(f"{ref} carries no "
                               f"{layout.notes_source(self.version)}")
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
        """Signs and hands over the notes @p ref has for this release;
        returns their hash. Content-addressed, so the old notes stay where
        they are and nothing published is ever replaced."""
        self._need("key", "git_dir", "staging")
        data, digest = self._notes(ref)
        remote = layout.notes_path(self.version, digest)
        # Signed and handed over already (the tag's notes, unchanged on
        # main, still waiting for the host): nothing to do. Once the host
        # has them, another .sig of the same notes is one it leaves aside:
        # the published one is never replaced.
        if self.outbox.exists(remote) and self.outbox.exists(remote + SIG) \
                and self.outbox.sha256(remote) == digest:
            return digest
        # New notes are signed by a current maintainer, whoever signed the
        # release itself.
        key = self.requested_key or self.product.signing_key
        self._check_signing_key(key)
        # Its own directory: the stage's notes/ holds what an interrupted
        # upload still has to send, and is never signed over.
        local = self._sign_notes(data, digest, "notes-only", key)
        with openpgp.Keyring.of(self.product) as ring:
            ring.verify_beside(local.parent, [local.name])
        for suffix in (SIG, ""):  # the signature first, as _upload
            self.outbox.put_file(local.with_name(local.name + suffix),
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
        if self.signer is not None and not self.signer.timestamps and \
                self.product.authenticode_timestamp_required:
            # Without a time stamp the signature dies with the certificate:
            # every later check of the release would refuse it.
            raise ReleaseError("Authenticode signing needs --timestamp-url "
                               "(the trust root requires a timestamp)")
        build = self.dir / "build"
        if self.final_dir.exists():
            shutil.rmtree(self.final_dir)
        self.final_dir.mkdir()
        rules = authenticode.Rules.of(self.product)
        files = self.state["build"]["files"]
        # What this step made before, it makes again.
        for name in [n for n, info in files.items() if not _built(info)]:
            del files[name]
        packages = {name: info for name, info in sorted(files.items())
                    if any(authenticode.plan(info))}
        if packages and self.signer is None:
            raise ReleaseError(f"{next(iter(packages))} needs an "
                               "Authenticode signer (--authenticode-cert/"
                               "-key, or --authenticode-thumbprint)")
        for name, info in sorted(files.items()):
            if name not in packages:
                shutil.copy2(build / name, self.final_dir / name)
                info["transform"] = "none"

        # In two rounds, each one call of the signer: the programs of every
        # package, then the packages closed around them.
        signed = {}
        with tempfile.TemporaryDirectory(prefix="gfr-sign-") as tmp:
            opened = {name: authenticode.open_package(
                build / name, info, rules, Path(tmp) / str(n))
                for n, (name, info) in enumerate(packages.items())}
            self._sign({file: (name, path) for name, o in opened.items()
                        for path, file in o.programs.items()})
            for name, o in opened.items():
                signed[name] = authenticode.close_package(
                    o, self.final_dir / name)
        made = self._make_msix()
        self._sign({self.final_dir / name: (name, None)
                    for name in sorted([n for n, o in opened.items()
                                        if o.outside] + list(made))})

        for name, info in packages.items():
            # Checked as the signing host will, before anything leaves.
            authenticode.unchanged_apart_from(build / name,
                                              self.final_dir / name,
                                              info, signed[name])
            checks.windows_signed(self.product, self.final_dir / name, info,
                                  self.authenticode_ca)
            info["transform"] = "authenticode"
        for name, source in made.items():
            self._check_msix(name, source)
        for name, info in files.items():
            info["sha256"] = sha256_file(self.final_dir / name)

    def _make_msix(self) -> dict[str, str]:
        """The MSIX of every MSI CI built none for, written to final/ from
        the MSI there (the programs inside it signed already), not signed
        itself yet; returns {its name: the MSI's}. Where no package can be
        packed, none is made."""
        files, made = self.state["build"]["files"], {}
        if not msix.available():
            return made
        for name, info in sorted(files.items()):
            target = msix.name_for(name)
            if info["kind"] != "msi" or not _built(info) or target in files:
                continue
            try:
                self.product.classify(target, self.version)
            except ReleaseError:
                continue  # an MSI of a flavour that has no MSIX
            if self.signer is None:
                raise ReleaseError(f"{target} needs an Authenticode signer "
                                   "(--authenticode-cert/-key, or "
                                   "--authenticode-thumbprint)")
            msix.build(self.final_dir / name, self.final_dir / target,
                       version=self.version,
                       subject=self.product.authenticode_subject)
            made[target] = name
        for target, name in made.items():
            files[target] = {**self.product.classify(target, self.version),
                             "transform": "msix", "from": name}
        return made

    def _check_msix(self, name: str, source: str) -> None:
        """The signed package @p name, checked as the signing host will:
        its signature, and that it holds @p source's files and no others."""
        checks.msix_of(self.product, self.final_dir / name,
                       self.state["build"]["files"][name],
                       self.final_dir / source, self.authenticode_ca)

    def _sign(self, what: dict[Path, tuple[str, str | None]]) -> None:
        """Signs the files of @p what in one call, each a (package, path
        inside it, or None for the package itself), and tells on_signed
        about each as it is done, counted within its package."""
        if not what:
            return
        totals, done = {}, {}
        for name, _ in what.values():
            totals[name] = totals.get(name, 0) + 1

        def on_done(file: Path) -> None:
            name, path = what[file]
            done[name] = done.get(name, 0) + 1
            if self.on_signed is not None:
                self.on_signed(name, done[name], totals[name], path)

        self.signer.sign_files(list(what), on_done)

    def _sign_files(self, names: list[str]) -> None:
        """A detached signature for each of @p names in final/, and the
        manifest of everything there written and signed again."""
        files = self.state["build"]["files"]
        for name in [*names, SUMS]:
            (self.final_dir / (name + SIG)).unlink(missing_ok=True)
        for name in names:
            openpgp.sign(self.final_dir / name, self.key, self.gnupghome)
        atomic_write(self.final_dir / SUMS, render_manifest(
            {n: files[n]["sha256"] for n in files}))
        openpgp.sign(self.final_dir / SUMS, self.key, self.gnupghome)

    def _pgp(self) -> None:
        self._need("key", "git_dir")
        self._check_signing_key()
        self.state["key"] = self.key
        files = self.state["build"]["files"]
        for stale in self.final_dir.glob(f"*{SIG}"):
            stale.unlink()
        self._sign_files(sorted(files))

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
            ring.verify_beside(self.final_dir, [*sorted(files), SUMS])
            ring.verify_beside(notes_dir, [f"{digest}.md"])

    def _upload(self) -> None:
        self._need("staging")
        files, p = self.state["build"]["files"], self.prefix
        # Fixed before anything is sent: a resumed upload makes the same
        # descriptor, byte for byte, as the one it may already have sent.
        self.state["upload_started"] = True
        self.state.setdefault("released_at", iso(utcnow()))
        self._save()
        staged = layout.staged_tag(self.version)
        left_out = 0
        for name in sorted(files):
            info = files[name]
            # Its signature first: what the host takes the file by.
            self.outbox.put_file(self.final_dir / (name + SIG),
                                 layout.sig_of(f"{p}/{name}"))
            # A file signing did not change is CI's, byte for byte, and
            # still staged: the host takes it from there (promote.py), and
            # checks it as any other.
            if info["transform"] == "none" and \
                    self.staging.exists(f"{staged}/{name}"):
                left_out += (self.final_dir / name).stat().st_size
            else:
                self.outbox.put_file(self.final_dir / name, f"{p}/{name}")
        self.outbox.put_file(self.final_dir / SUMS, f"{p}/{SUMS}")
        self.outbox.put_file(self.final_dir / (SUMS + SIG),
                             layout.sig_of(f"{p}/{SUMS}"))
        build = self.dir / "build"
        self.outbox.put_file(build / SUMS, layout.build_sums(p))
        self.outbox.put_file(build / BUNDLE, layout.build_bundle(p))

        if left_out >= 1e6:
            ui.info(f"  {left_out / 1e6:.1f} MB not uploaded: the host takes "
                    f"the files signing left as they were from {staged}/")

        digest = self.state["notes_sha256"]
        # The signature first: notes are taken by their .md, so a host round
        # in between never finds them unsigned.
        for suffix in (SIG, ""):
            self.outbox.put_file(self.dir / "notes" / f"{digest}.md{suffix}",
                                 layout.notes_path(self.version, digest)
                                 + suffix)

        descriptor = self._descriptor()
        # Last: a descriptor only ever names objects that are already there.
        self.outbox.put_bytes(canonical_json(descriptor),
                              layout.descriptor_of(p))
        self.state["descriptor"] = descriptor

    def _descriptor(self) -> dict:
        """The release as the stage's record has it: every file in final/,
        and when it was first handed over."""
        files, p = self.state["build"]["files"], self.prefix
        artifacts = [descriptors.classified(info) | {
            k: info[k] for k in ("sha256", "build_sha256", "transform",
                                 "from") if k in info} | {
            "name": name, "size": (self.final_dir / name).stat().st_size,
            "path": f"{p}/{name}", "sig": layout.sig_of(f"{p}/{name}")}
            for name, info in sorted(files.items())]
        b = self.state["build"]
        return descriptors.make(
            layout.KIND_STABLE, self.product.slug, self.version,
            refs.release_tag_ref(self.version), b["commit"],
            {"scheme": layout.SCHEME_OPENPGP, "fingerprint": self.key},
            artifacts,
            built_at=b["built_at"], released_at=self.state["released_at"],
            build={"sums": layout.build_sums(p),
                   "bundle": layout.build_bundle(p),
                   "identity": b["identity"]},
            sums=f"{p}/{SUMS}", sums_sig=layout.sig_of(f"{p}/{SUMS}"))

    def add_msix(self) -> dict:
        """For a release handed over without its MSIX (made before the
        stage made one, or where none can be packed): makes it from the
        signed MSI in final/, signs it, lists it in a manifest signed
        anew, and hands over only what that changes. The host adds it to
        the release it has published (promote.py); one it has not received
        yet, it receives with it. Returns the descriptor."""
        if self._conflict:
            raise ReleaseError(self._conflict)
        if "upload" not in self.state["done"]:
            raise ReleaseError(f"{self.version} is not handed over yet: the "
                               "stage itself makes its MSIX (gfrelease "
                               f"stable {self.version})")
        self._need("key", "staging")
        files, p = self.state["build"]["files"], self.prefix
        if not msix.available() and all(_built(i) for i in files.values()):
            raise ReleaseError("an MSIX is packed by the Windows SDK's "
                               "makeappx: only on Windows")
        made = self._make_msix()
        if made:
            self._sign({self.final_dir / name: (name, None) for name in made})
            for name, source in made.items():
                self._check_msix(name, source)
                files[name]["sha256"] = sha256_file(self.final_dir / name)
            self._sign_files(sorted(made))
            with openpgp.Keyring.of(self.product) as ring:
                ring.verify_beside(self.final_dir, [*sorted(made), SUMS])
            # On disk before anything is sent: a run stopped here sends
            # these files, it never makes others.
            self._save()
        added = sorted(n for n, info in files.items() if not _built(info))
        if not added:
            raise ReleaseError(f"{self.version} has no MSI an MSIX is made "
                               "of")
        # What was handed over before may still lie there, and is replaced:
        # nothing in the staging store counts until the host has checked it.
        for name in [*added, SUMS]:
            for suffix in (SIG, ""):
                self.outbox.put_file(self.final_dir / (name + suffix),
                                     f"{p}/{name}{suffix}", immutable=False)
        descriptor = self._descriptor()
        with tempfile.TemporaryDirectory(prefix="gfr-msix-") as tmp:
            local = Path(tmp) / "release.json"
            local.write_bytes(canonical_json(descriptor))
            self.outbox.put_file(local, layout.descriptor_of(p),
                                 immutable=False)
        self.state["descriptor"] = descriptor
        self._save()
        return descriptor


def _built(info: dict) -> bool:
    """Whether a file of the stage's record is one CI built, rather than
    one made here from another."""
    return "build_sha256" in info
