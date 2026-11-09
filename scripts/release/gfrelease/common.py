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
import refs  # noqa: E402
import release_candidate as rc  # noqa: E402
import semver  # noqa: E402

SCHEMA = 1
DESCRIPTOR = "release.json"
SIG = ".sig"        # an OpenPGP detached signature, binary
STAGED = "staged.json"
SUMS = "SHA256SUMS.txt"
BUNDLE = "SHA256SUMS.txt.sigstore.json"
MINISIG = ".minisig"  # a minisign signature, beside what it signs
INDEX = "v1/index.json"

# The longest any one tool (rclone, git, gpg, minisign, cosign, cmake) may
# run: a transfer that hangs ends as a failed step, and the next round tries
# again, rather than holding the host's lock for ever.
TOOL_TIMEOUT = 3600
# The most a descriptor, an index, a manifest, a signature or release notes
# may weigh. They are read into memory whole, and come from buckets and
# mirrors others can write to.
SMALL_OBJECT_LIMIT = 8 << 20

# What a path inside a descriptor or the index may look like: relative to the
# product root, no scheme, no "..", no empty or dot segments, nothing a URL
# join or a filesystem could read as escaping it.
_SEGMENT = re.compile(r"[A-Za-z0-9][A-Za-z0-9._+-]*")


class ReleaseError(Exception):
    pass


class ToolFailed(ReleaseError):
    """A tool could not do its job (git, cmake, rclone, gpg importing keys,
    minisign signing): nothing was decided about what it worked on, and the
    next run may well succeed. Unlike a verification that ran and said no,
    which is a ReleaseError."""

    passing = True  # is_passing


class ToolTimeout(ToolFailed):
    """A tool did not finish within its time limit."""


class Problems(list):
    """What a run could not do, one line each, and whether trying again may
    help: a problem is passing when it came from a store that did not
    answer or a tool that did not finish, lasting when the thing itself was
    refused (a forged candidate, a bad signature), which the next run would
    only refuse again."""

    passing = False

    def note(self, where: str, error: Exception) -> None:
        self.append(f"{where}: {error}")
        if is_passing(error):
            self.passing = True


def is_passing(error: Exception) -> bool:
    """Not a judgement of what was asked about, only a failure to ask:
    worth trying again as it is (ToolFailed, store.StoreUnavailable)."""
    return getattr(error, "passing", False)


def candidate_error(where: str, error: Exception) -> ReleaseError:
    """release_candidate's verdict on a candidate, as a ReleaseError; a
    cosign that did not finish is no verdict (ToolTimeout)."""
    if isinstance(error, rc.CandidateTimeout):
        return ToolTimeout(f"{where}: {error}")
    return ReleaseError(f"{where}: {error}")


def check_relpath(path: str) -> str:
    # fullmatch: "$" would also match before a trailing newline, and let
    # "a/b\n/c" through.
    if not isinstance(path, str) or \
            any(not _SEGMENT.fullmatch(p) for p in path.split("/")):
        raise ReleaseError(f"not a product-relative path: {path!r}")
    return path


def sha256_file(path: Path) -> str:
    return rc.digest(Path(path))


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_json(data: bytes | str, what: str) -> dict:
    """A JSON object read from a store or the state directory; anything else
    is a ReleaseError naming @p what, never a traceback."""
    def unique(pairs):
        # A key given twice reads differently in different parsers: the
        # checks here would pass on one value and a client act on the other.
        keys = [k for k, _ in pairs]
        if len(keys) != len(set(keys)):
            raise ValueError("a key is given twice")
        return dict(pairs)
    if len(data) > SMALL_OBJECT_LIMIT:
        raise ReleaseError(f"{what}: larger than {SMALL_OBJECT_LIMIT} bytes")
    try:
        value = json.loads(data, object_pairs_hook=unique)
    except (ValueError, UnicodeDecodeError, RecursionError) as e:
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
    """A tool, run to its end: a failure, a missing tool or one that does
    not finish (ToolTimeout) is a ReleaseError, never a traceback."""
    kw.setdefault("check", True)
    kw.setdefault("timeout", TOOL_TIMEOUT)
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
        # check=True: the caller needed the tool to work, not to judge.
        detail = (e.stderr or e.stdout or "").strip()
        raise ToolFailed(f"{Path(str(cmd[0])).name} failed: {detail}") from e
    except subprocess.TimeoutExpired as e:
        raise ToolTimeout(f"{Path(str(cmd[0])).name} did not finish within "
                          f"{kw['timeout']} s") from e
    except FileNotFoundError as e:
        raise ToolFailed(f"{cmd[0]} is not installed") from e
    except OSError as e:
        raise ToolFailed(f"{cmd[0]} cannot be run: {e}") from e


class Product:
    """release/product.json, and the policy of the tree it sits in."""

    def __init__(self, product_path: Path, policy_path: Path):
        try:
            self.data = policy_mod.load_product(product_path)
            policy_mod.check_product(self.data)
            self.policy = policy_mod.load_policy(policy_path)
            policy_mod.check_policy(self.policy)
        except policy_mod.PolicyError as e:
            raise ReleaseError(f"{product_path}, {policy_path}:\n{e}") from e
        except (OSError, ValueError) as e:
            # Unreadable, or not JSON or TOML at all.
            raise ReleaseError(f"{product_path}, {policy_path}: {e}") from e
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

    @property
    def signing_key(self) -> str:
        """The key that signs anything new by default: the first pinned
        maintainer's, or "" when none is pinned."""
        return self.maintainers[0] if self.maintainers else ""

    @property
    def legacy_floor(self) -> semver.Version:
        """The first release signed with a pinned key: nothing older is
        imported or listed."""
        return parse_version(self.policy["signing"]["legacy_unsigned_before"])

    def check_host_keys(self, artifact_pub: str,
                        index_pub: str | None = None) -> None:
        """What this host signs with must be what clients pin, or everything
        it publishes is refused by every client while its own read-back,
        done with the same wrong key, passes."""
        signing = self.data.get("signing", {})
        if artifact_pub != signing.get("artifact_key"):
            raise ReleaseError("--artifact-pub is not signing.artifact_key in "
                               "release/product.json")
        if index_pub is not None and \
                index_pub not in signing.get("index_keys", []):
            raise ReleaseError("--index-pub is not one of signing.index_keys "
                               "in release/product.json")

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
            m = re.fullmatch(entry["match"], rest, re.ASCII)
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
def locked(state_dir: Path, shared: bool = False):
    """One run at a time per host: promote, index and resign all write the
    same state directory, and the stores through it. A second run is
    refused at once. With @p shared (a reader of the state directory: a dry
    run copying it) it waits for the run in progress instead.

    The lock is this host's only. The maintainer's `stable` and `import`
    and CI's `stage` write the stores from other machines, each under paths
    of its own (releases/<version>/, branches/, tags/)."""
    state_dir = Path(state_dir)
    state_dir.mkdir(parents=True, exist_ok=True)
    with open(state_dir / "lock", "a") as f:
        try:
            fcntl.flock(f, fcntl.LOCK_SH if shared
                        else fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise ReleaseError(f"another run holds {state_dir / 'lock'}")
        yield


def git_version(git_dir: Path, commit: str, ref: str) -> str:
    """Which version a commit is, asked of git through the one derivation the
    build itself uses, never taken from what the build claims."""
    try:
        return gitversion.derive(git_dir, ref=ref, rev=commit)
    except subprocess.CalledProcessError as e:
        raise ToolFailed(f"cmake failed: {(e.stderr or '').strip()}") from e
    except subprocess.TimeoutExpired as e:
        raise ToolTimeout(f"cmake did not finish within {e.timeout} s") from e
    except FileNotFoundError as e:
        raise ToolFailed("cmake is not installed") from e


def resolve_commit(git_dir: Path, rev: str) -> str | None:
    """The commit @p rev names in @p git_dir, or None when it names
    nothing. A repository that cannot be asked is a ToolFailed, never
    "nothing": a missing tag unlists a release, while a release that could
    not be checked keeps its listing for the round (index.build)."""
    out = run(["git", "-C", git_dir, "rev-parse", "-q", "--verify",
               f"{rev}^{{commit}}"], check=False)
    commit = out.stdout.strip()
    if out.returncode == 0 and refs.is_commit(commit):
        return commit
    if out.returncode == 1 and not commit:
        return None
    raise ToolFailed(f"{git_dir}: cannot resolve {rev}: "
                     f"{(out.stderr or out.stdout).strip()}")


def tagged_commit(git_dir: Path, tag: str, expect: str | None = None,
                  version: str | None = None) -> str:
    """The commit @p tag names, checked: it is @p expect when given, and git
    calls it @p version when given."""
    commit = resolve_commit(git_dir, refs.TAGS + tag)
    if commit is None:
        raise ReleaseError(f"{git_dir} has no tag {tag}")
    if expect is not None and commit != expect:
        raise ReleaseError(f"{tag} is {commit}, not {expect}")
    if version is not None and \
            git_version(git_dir, commit, refs.TAGS + tag) != version:
        raise ReleaseError(f"git does not call {commit} {version}")
    return commit


def tag_time(git_dir: Path, tag: str) -> int:
    """When @p tag was made (Unix seconds): an annotated tag's tagger date,
    or for a lightweight one its commit's committer date. A maintainer's
    signature on a release's file is never older than the release's tag."""
    out = run(["git", "-C", git_dir, "for-each-ref",
               "--format=%(taggerdate:unix) %(committerdate:unix) "
               "%(*committerdate:unix)", refs.TAGS + tag]).stdout.split()
    if not out or not out[0].isdigit():
        raise ReleaseError(f"{git_dir} has no tag {tag} with a date")
    return int(out[0])


def notes_at(git_dir: Path, ref: str, version: str) -> bytes | None:
    """release/notes/<version>.md as @p ref has it, or None when it has none
    (or an empty one, or there is no such ref). A repository that cannot be
    read is an error: "no notes" would list a release without them, or
    refuse one that has them."""
    commit = resolve_commit(git_dir, ref)
    if commit is None:
        return None
    path = f"release/notes/{version}.md"
    if not run(["git", "-C", git_dir, "ls-tree", "--name-only", commit, "--",
                path]).stdout.strip():
        return None
    notes = run(["git", "-C", git_dir, "show", f"{commit}:{path}"],
                text=False).stdout
    if len(notes) > SMALL_OBJECT_LIMIT:
        raise ReleaseError(f"{ref}:{path}: larger than {SMALL_OBJECT_LIMIT} "
                           "bytes")
    return notes if notes.strip() else None
