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

import policy as policy_mod  # noqa: E402
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
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def canonical_json(value) -> bytes:
    """One byte form per value: what is hashed and signed is reproducible."""
    return (json.dumps(value, indent=1, sort_keys=True, ensure_ascii=False)
            + "\n").encode("utf-8")


def utcnow() -> dt.datetime:
    override = os.environ.get("GFR_NOW")  # tests and rehearsals only
    if override:
        return dt.datetime.fromisoformat(override).astimezone(dt.timezone.utc)
    return dt.datetime.now(dt.timezone.utc).replace(microsecond=0)


def iso(t: dt.datetime) -> str:
    return t.strftime("%Y-%m-%dT%H:%M:%SZ")


def parse_iso(text: str) -> dt.datetime:
    return dt.datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ").replace(
        tzinfo=dt.timezone.utc)


def run(cmd, **kw) -> subprocess.CompletedProcess:
    kw.setdefault("check", True)
    kw.setdefault("capture_output", True)
    kw.setdefault("text", True)
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
        self.keyring_path = Path(product_path).with_name("maintainers.asc")
        self.authenticode_subject = signing.get("authenticode_subject", "")
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
    out = run(["cmake", f"-DGF_SOURCE_DIR={git_dir}",
               f"-DGPGFRONTEND_VERSION_REV={commit}",
               f"-DGPGFRONTEND_VERSION_REF={ref}", "-DGF_FIELD=version",
               "-P", ROOT / "cmake" / "GitVersion.cmake"])
    return out.stdout.strip()
