# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What every gfrelease role shares: errors, hashing, configuration, the lock."""

from __future__ import annotations

import contextlib
import datetime as dt
import fcntl
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
RELEASE_DIR = HERE.parent
ROOT = RELEASE_DIR.parent.parent
sys.path.insert(0, str(RELEASE_DIR))
sys.path.insert(0, str(ROOT / "scripts"))

import gitversion  # noqa: E402
import policy as policy_mod  # noqa: E402
import release_candidate as rc  # noqa: E402
import semver  # noqa: E402

SCHEMA = 1
DESCRIPTOR = "release.json"
SIG = ".sig"        # an OpenPGP detached signature, binary
STAGED = "staged.json"
SUMS = "SHA256SUMS.txt"
BUNDLE = "SHA256SUMS.txt.sigstore.json"
MINISIG = ".minisig"
INDEX = "v1/index.json"

# What a path inside a descriptor or the index may look like: relative to the
# product root, no scheme, no "..", no empty or dot segments, nothing a URL
# join or a filesystem could read as escaping it.
_SEGMENT = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._+-]*$")


class ReleaseError(Exception):
    pass


def check_relpath(path: str) -> str:
    parts = path.split("/")
    if not path or path.startswith("/") or "%" in path or "\\" in path or \
            any(not _SEGMENT.match(p) or p in (".", "..") for p in parts):
        raise ReleaseError(f"not a product-relative path: {path!r}")
    return path


def sha256_file(path: Path) -> str:
    return rc.digest(Path(path))


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_json(data: bytes | str, what: str) -> dict:
    """A JSON object read from a store or the state directory; anything else
    is a ReleaseError naming @p what, never a traceback."""
    try:
        value = json.loads(data)
    except (ValueError, UnicodeDecodeError) as e:
        raise ReleaseError(f"{what}: not JSON ({e})") from e
    if not isinstance(value, dict):
        raise ReleaseError(f"{what}: not a JSON object")
    return value


def parse_version(text, what: str = "") -> semver.Version:
    """semver.parse, failing as a ReleaseError."""
    try:
        return semver.parse(text if isinstance(text, str) else "")
    except ValueError as e:
        raise ReleaseError(f"{what + ': ' if what else ''}{e}") from e


def atomic_write(path: Path, data: bytes | str) -> None:
    """Replaces @p path whole or not at all: a crash leaves the old file or
    the new one, never a truncated one."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".new")
    with open(tmp, "wb") as f:
        f.write(data.encode("utf-8") if isinstance(data, str) else data)
        f.flush()
        os.fsync(f.fileno())
    tmp.replace(path)


def parse_manifest(data: bytes, what: str) -> dict[str, str]:
    """{name: sha256} of a SHA256SUMS.txt, read as strictly as the
    candidate's own (release_candidate.parse_manifest)."""
    try:
        return rc.parse_manifest(data)
    except rc.CandidateError as e:
        raise ReleaseError(f"{what}: {e}") from e


def render_manifest(hashes: dict[str, str]) -> bytes:
    """The SHA256SUMS.txt bytes for {name: sha256}."""
    return rc.render_manifest(hashes).encode("ascii")


def canonical_json(value) -> bytes:
    """One byte form per value: what is hashed and signed is reproducible."""
    return (json.dumps(value, indent=1, sort_keys=True, ensure_ascii=False)
            + "\n").encode("utf-8")


def utcnow() -> dt.datetime:
    override = os.environ.get("GFR_NOW")  # tests and rehearsals only
    if override:
        t = dt.datetime.fromisoformat(override)
        # Without a zone it is UTC, never this machine's local time.
        if t.tzinfo is None:
            t = t.replace(tzinfo=dt.timezone.utc)
        return t.astimezone(dt.timezone.utc)
    return dt.datetime.now(dt.timezone.utc).replace(microsecond=0)


def iso(t: dt.datetime) -> str:
    return t.strftime("%Y-%m-%dT%H:%M:%SZ")


def parse_iso(text: str) -> dt.datetime:
    try:
        return dt.datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ").replace(
            tzinfo=dt.timezone.utc)
    except (TypeError, ValueError) as e:
        raise ReleaseError(f"not a UTC time: {text!r}") from e


def run(cmd, **kw) -> subprocess.CompletedProcess:
    kw.setdefault("check", True)
    kw.setdefault("capture_output", True)
    kw.setdefault("text", True)
    if kw["text"]:
        # What a tool prints can carry bytes from untrusted input (a user ID
        # in a forged signature, a file name): never a UnicodeDecodeError.
        kw.setdefault("encoding", "utf-8")
        kw.setdefault("errors", "replace")
    try:
        return subprocess.run([str(c) for c in cmd], **kw)
    except subprocess.CalledProcessError as e:
        detail = (e.stderr or e.stdout or "").strip()
        raise ReleaseError(f"{Path(str(cmd[0])).name} failed: {detail}") from e
    except FileNotFoundError as e:
        raise ReleaseError(f"{cmd[0]} is not installed") from e


class Product:
    """release/product.json, and the policy of the tree it sits in."""

    def __init__(self, product_path: Path, policy_path: Path):
        self.data = policy_mod.load_product(product_path)
        policy_mod.check_product(self.data)
        self.policy = policy_mod.load_policy(policy_path)
        policy_mod.check_policy(self.policy)
        self.slug = self.data["slug"]
        self.repository = self.data["repository"]
        self.workflow = self.data["workflow"]
        signing = self.data.get("signing", {})
        # Who may sign a stable release: their public keys sit beside the
        # product file (release/maintainers.asc), their primary fingerprints
        # are pinned in it.
        self.maintainers = [f.upper() for f in
                            signing.get("maintainer_fingerprints", [])]
        # A maintainer key replaced: what it signed still verifies (the
        # releases it made stay listed), it is never used to sign again.
        self.retired_maintainers = [
            f.upper() for f in signing.get("maintainer_fingerprints_retired",
                                           [])]
        self.trusted_maintainers = self.maintainers + [
            f for f in self.retired_maintainers if f not in self.maintainers]
        self.keyring_path = Path(product_path).with_name("maintainers.asc")
        self.authenticode_subject = signing.get("authenticode_subject", "")
        # A signature without an RFC 3161 time stamp stops verifying when the
        # certificate expires; required unless the product says otherwise.
        self.authenticode_timestamp_required = \
            signing.get("authenticode_timestamp", "required") == "required"
        # Replaced keys and certificates: accepted on what they signed before,
        # never used to sign anything new. The host and the offline stage sign
        # only with the current ones.
        self.retired_artifact_keys = list(signing.get("artifact_keys_retired",
                                                      []))
        self.authenticode_subjects = [s for s in [self.authenticode_subject]
                                      + list(signing.get(
                                          "authenticode_subjects_retired", []))
                                      if s]

    def artifact_keys(self, current: str) -> list[str]:
        """The artifact keys a published file may carry: @p current (the one
        the host signs with) and the retired ones."""
        return [current] + [k for k in self.retired_artifact_keys
                            if k != current]

    def identity(self, ref: str) -> str:
        """The Sigstore identity of this product's build workflow at a ref."""
        return f"https://github.com/{self.repository}/{self.workflow}@{ref}"

    def classify(self, name: str, filesafe: str) -> dict:
        """os, arch, kind, flavor (and min_os) of a published file, from the
        product's own table; a file it does not know is refused, so nothing
        reaches a descriptor undescribed."""
        prefix = f"{self.data['artifact_prefix']}{filesafe}-"
        if not name.startswith(prefix):
            raise ReleaseError(f"{name}: not named {prefix}...")
        rest = name[len(prefix):]
        for entry in self.data["artifacts"]:
            m = re.match(entry["match"], rest)
            if not m:
                continue
            groups = m.groupdict()
            out = {k: entry[k].format(**groups)
                   for k in ("os", "arch", "kind", "flavor")}
            if "min_os" in entry:
                out["min_os"] = {k: v.format(**groups)
                                 for k, v in entry["min_os"].items()}
            return out
        raise ReleaseError(f"{name}: no artifact rule in release/product.json")


def default_product() -> Product:
    return Product(ROOT / "release" / "product.json",
                   ROOT / "release" / "policy.toml")


@contextlib.contextmanager
def locked(state_dir: Path):
    """One run at a time per host: promote, index and resign all write the
    same shared stores."""
    state_dir.mkdir(parents=True, exist_ok=True)
    with open(state_dir / "lock", "w") as f:
        try:
            fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise ReleaseError(f"another run holds {state_dir / 'lock'}")
        yield


def git_version(git_dir: Path, commit: str, ref: str) -> str:
    """Which version a commit is, asked of git through the one derivation the
    build itself uses, never taken from what the build claims."""
    try:
        return gitversion.derive(git_dir, ref=ref, rev=commit)
    except subprocess.CalledProcessError as e:
        raise ReleaseError(f"cmake failed: {(e.stderr or '').strip()}") from e
    except FileNotFoundError as e:
        raise ReleaseError("cmake is not installed") from e


def tagged_commit(git_dir: Path, tag: str, expect: str | None = None,
                  version: str | None = None) -> str:
    """The commit @p tag names, checked: it is @p expect when given, and git
    calls it @p version when given."""
    commit = run(["git", "-C", git_dir, "rev-parse", "-q", "--verify",
                  f"refs/tags/{tag}^{{commit}}"], check=False).stdout.strip()
    if not commit:
        raise ReleaseError(f"{git_dir} has no tag {tag}")
    if expect is not None and commit != expect:
        raise ReleaseError(f"{tag} is {commit}, not {expect}")
    if version is not None and \
            git_version(git_dir, commit, f"refs/tags/{tag}") != version:
        raise ReleaseError(f"git does not call {commit} {version}")
    return commit


def tag_time(git_dir: Path, tag: str) -> int:
    """When @p tag was made (Unix seconds): an annotated tag's tagger date,
    or for a lightweight one its commit's committer date. A maintainer's
    signature on a release's file is never older than the release's tag."""
    out = run(["git", "-C", git_dir, "for-each-ref",
               "--format=%(taggerdate:unix) %(committerdate:unix) "
               "%(*committerdate:unix)", f"refs/tags/{tag}"],
              check=False).stdout.split()
    if not out or not out[0].isdigit():
        raise ReleaseError(f"{git_dir} has no tag {tag} with a date")
    return int(out[0])


def notes_at(git_dir: Path, ref: str, version: str) -> bytes | None:
    """release/notes/<version>.md as @p ref has it, or None when it has none
    (or an empty one)."""
    out = run(["git", "-C", git_dir, "show",
               f"{ref}:release/notes/{version}.md"], check=False, text=False)
    if out.returncode != 0 or not out.stdout.strip():
        return None
    return out.stdout
