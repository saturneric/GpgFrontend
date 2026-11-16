# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What every gfrelease role shares: errors, hashing, configuration, the lock."""

from __future__ import annotations

import contextlib
import datetime as dt
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path

# The maintainer's stage also runs on Windows, where the Authenticode token's
# own software is; the signing host and CI are Linux.
WINDOWS = os.name == "nt"
if WINDOWS:
    import msvcrt
else:
    import fcntl

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
import ui  # noqa: E402

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
# Release notes: what the update check reads of them at most
# (UpdateChecker::kMaxNotesBytes); longer ones it would drop.
NOTES_LIMIT = 256 << 10
# How far a clock may be off before something has expired: the update
# check allows as much (kExpirySkewSeconds).
EXPIRY_SKEW = dt.timedelta(days=1)

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
        text = str(error)
        # Most errors name their place already: never twice.
        head, sep, rest = text.partition(": ")
        if sep and (head == where or where.endswith("/" + head)):
            text = rest
        self.append(f"{where}: {text}")
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


def verify_candidate(where: str, work: Path, staged: dict, identity: str,
                     cosign: str) -> list[str]:
    """The staged candidate in @p work (staged.json taken out), as CI
    signed it for @p identity and the commit @p staged names; returns its
    files, which must be the ones @p staged lists."""
    try:
        verified = rc.verify(work, identity, staged["commit"], cosign)
    except (rc.CandidateError, rc.CandidateTimeout) as e:
        raise candidate_error(where, e) from e
    files = sorted(p.name for p in verified if p.name not in (SUMS, BUNDLE))
    if files != sorted(staged["files"]):
        raise ReleaseError(f"{where}: staged.json lists other files than the "
                           "signed manifest")
    return files


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
    if not isinstance(data, (bytes, str)):
        raise ReleaseError(f"{what}: missing")
    if len(data) > SMALL_OBJECT_LIMIT:
        raise ReleaseError(f"{what}: larger than {SMALL_OBJECT_LIMIT} bytes")
    try:
        value = json.loads(data, object_pairs_hook=policy_mod.unique_keys)
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


_UTC_TIME = re.compile(r"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:"
                       r"[0-9]{2}Z", re.ASCII)


def parse_iso(text: str) -> dt.datetime:
    """A time as iso() writes it, every field zero-padded: strptime alone
    would take 2028-1-5T1:2:3Z too, which the update check refuses."""
    try:
        if not _UTC_TIME.fullmatch(text):
            raise ValueError("not YYYY-MM-DDTHH:MM:SSZ")
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
    ui.tool_started(cmd)
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
    """release/product.json, the policy of the tree it sits in, and the
    trust it is published under: the root and the index-keys list
    (trust.py), from @p root_dir and @p index_keys_dir (by default this
    tree's release/root/ and release/index-keys/), root 1 being
    @p root_sha256 when given."""

    def __init__(self, product_path: Path, policy_path: Path,
                 root_dir: Path | None = None,
                 index_keys_dir: Path | None = None,
                 root_sha256: str | None = None):
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
        # The maintainers' public keys sit beside the product file; only
        # those whose fingerprint the root pins are ever used.
        self.keyring_path = Path(product_path).with_name("maintainers.asc")
        self.root_dir = Path(root_dir or ROOT / "release" / "root")
        self.index_keys_dir = Path(index_keys_dir or
                                   ROOT / "release" / "index-keys")
        self.root_sha256 = root_sha256
        self._trust = None

    @property
    def trust(self) -> tuple[dict, dict]:
        """The root and the index-keys list, checked and unexpired; read
        when first asked for (staging needs neither)."""
        if self._trust is None:
            import trust  # trust.py builds on this module
            root, keys = trust.load_trust(self.root_dir, self.index_keys_dir,
                                          self.slug, self.root_sha256)
            trust.check_unexpired(root, f"root {root['version']}")
            trust.check_unexpired(keys, f"index-keys {keys['version']}")
            self._trust = root, keys
        return self._trust

    @property
    def host_policy(self) -> dict:
        """What the root says the signing host admits."""
        return self.trust[0]["host_policy"]

    @property
    def repository(self) -> str:
        return self.host_policy["repository"]

    @property
    def maintainers(self) -> list[str]:
        """Who may sign a stable release: their primary fingerprints."""
        return [f.upper() for f in
                self.host_policy["stable_pgp"]["fingerprints"]]

    @property
    def retired_maintainers(self) -> list[str]:
        """A maintainer key replaced: what it signed is still accepted when
        a release admitted before is checked again, never for a new one."""
        return [f.upper() for f in self.host_policy["stable_pgp"]
                .get("fingerprints_retired", []) if f not in self.maintainers]

    @property
    def trusted_maintainers(self) -> list[str]:
        return self.maintainers + self.retired_maintainers

    @property
    def authenticode_subject(self) -> str:
        return self.host_policy.get("authenticode_subject", "")

    @property
    def authenticode_subjects(self) -> list[str]:
        """The current certificate's subject and the replaced ones (accepted
        as the retired maintainers are)."""
        return [s for s in [self.authenticode_subject] +
                self.host_policy.get("authenticode_subjects_retired", [])
                if s]

    @property
    def authenticode_timestamp_required(self) -> bool:
        # A signature without an RFC 3161 time stamp stops verifying when
        # the certificate expires; required unless the root says otherwise.
        return self.host_policy.get("authenticode_timestamp",
                                    "required") == "required"

    @property
    def index_keys(self) -> list[str]:
        """The keys a client accepts on the index: the index-keys list's."""
        return list(self.trust[1]["keys"].values())

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

    def check_host_keys(self, index_pub: str) -> None:
        """What this host signs with must be what clients accept, or
        everything it publishes is refused by every client while its own
        read-back, done with the same wrong key, passes."""
        if index_pub not in self.index_keys:
            raise ReleaseError(f"--index-pub is not in index-keys "
                               f"{self.trust[1]['version']}: clients would "
                               "refuse what this host signs")

    def identity(self, ref: str) -> str:
        """The Sigstore identity of this product's build workflow at a ref."""
        return (f"https://github.com/{self.repository}/"
                f"{self.host_policy['workflow']}@{ref}")

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


@contextlib.contextmanager
def locked(state_dir: Path, shared: bool = False):
    """One run at a time per host: promote, index and resign all write the
    same state directory, and the stores through it. A second run is
    refused at once. With @p shared (a reader of the state directory: a dry
    run copying it) it waits for the run in progress instead.

    The lock is this host's only. The maintainer's `stable` and `import`
    and CI's `stage` write the stores from other machines, each under paths
    of its own (releases/<version>/, branches/, tags/).

    Windows has no shared lock on a byte range through msvcrt: a reader
    there is refused like a second writer, rather than made to wait."""
    state_dir = Path(state_dir)
    state_dir.mkdir(parents=True, exist_ok=True)
    with open(state_dir / "lock", "a") as f:
        try:
            if WINDOWS:
                # A range is locked from the file's position: always byte 0,
                # which need not exist. Released when the file closes.
                f.seek(0)
                msvcrt.locking(f.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                fcntl.flock(f, fcntl.LOCK_SH if shared
                            else fcntl.LOCK_EX | fcntl.LOCK_NB)
        except (BlockingIOError, PermissionError):
            raise ReleaseError(f"another run holds {state_dir / 'lock'}")
        yield


def git_version(git_dir: Path, commit: str, ref: str,
                exclude: tuple[str, ...] = ()) -> str:
    """Which version a commit is, asked of git through the one derivation the
    build itself uses, never taken from what the build claims; with
    @p exclude, as git called it before those tags were made."""
    return run(gitversion.command(git_dir, ref=ref, rev=commit,
                                  exclude=exclude),
               timeout=gitversion.TIMEOUT).stdout.strip()


# How many tags made after a build check_git_version() looks past.
_LATER_TAGS = 4


def check_git_version(git_dir: Path, commit: str, ref: str, branch: str,
                      claimed: str, where: str) -> str:
    """@p claimed, once git calls @p commit, built from @p ref, on @p branch
    that: now, or before tags made after it was built. A build is named
    when it is made; tagging its commit (or one before it) later renames it
    in git's eyes, never what was built. Its own tag (a candidate's) is
    never looked past. Anything else is refused with what git says it is."""
    own = refs.tag_of(ref)
    derived = git_version(git_dir, commit, branch)
    found, excluded = derived, []
    for _ in range(_LATER_TAGS):
        if found == claimed:
            return claimed
        out = run(["git", "-C", git_dir, "describe", "--tags", "--abbrev=0",
                   "--match=v[0-9]*.[0-9]*.[0-9]*",
                   *(f"--exclude={t}" for t in excluded), commit],
                  check=False)
        tag = out.stdout.strip()
        if out.returncode != 0 or not tag or tag == own:
            break
        excluded.append(tag)
        found = git_version(git_dir, commit, branch, tuple(excluded))
    raise ReleaseError(f"{where}: git says {commit[:12]} on {branch} is "
                       f"{derived}, not {claimed}")


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


def is_ancestor(git_dir: Path, commit: str, rev: str) -> bool:
    """Whether @p commit is @p rev or reachable from it. A repository that
    cannot be asked is a ToolFailed, never "no"."""
    out = run(["git", "-C", git_dir, "merge-base", "--is-ancestor", commit,
               rev], check=False)
    if out.returncode in (0, 1):
        return out.returncode == 0
    raise ToolFailed(f"{git_dir}: cannot tell whether {commit[:12]} is on "
                     f"{rev}: {(out.stderr or out.stdout).strip()}")


def release_tags(git_dir: Path) -> list[str]:
    """The versions of every release tag (vX.Y.Z) in @p git_dir."""
    out = run(["git", "-C", git_dir, "tag", "--list", "v*"])
    return [t[1:] for t in out.stdout.split() if refs.is_release_tag(t)]


def check_candidate(git_dir: Path, ref: str, commit: str, version: str, *,
                    unreleased: bool = True) -> None:
    """A release candidate is its tag's build, of a commit on its series'
    release branch, and comes before that series' first release: never
    after it, never of a patch (refs.RC_TAG holds the shape). Without
    @p unreleased, a series released since is no error: the index then
    simply no longer lists the candidate (index._choose_rcs)."""
    tag = refs.tag_of(ref)
    if version != tag[1:]:
        raise ReleaseError(f"{ref}: git says {commit[:12]} is {version}, "
                           f"not {tag[1:]}")
    if resolve_commit(git_dir, ref) != commit:
        raise ReleaseError(f"{ref} does not point to {commit[:12]}")
    series = refs.rc_series(ref)
    branch = refs.HEADS + refs.release_branch(series)
    if resolve_commit(git_dir, branch) is None or not is_ancestor(
            git_dir, commit, branch):
        raise ReleaseError(f"{ref}: {commit[:12]} is not on "
                           f"{refs.release_branch(series)}")
    if not unreleased:
        return
    released = [t for t in release_tags(git_dir)
                if parse_version(t).series == series]
    if released:
        raise ReleaseError(f"{ref}: {series} has been released already "
                           f"({max(released, key=parse_version)}); a "
                           "candidate only comes before its first release")


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
    # creatordate: the tagger date, or the commit's for a lightweight tag.
    out = run(["git", "-C", git_dir, "for-each-ref",
               "--format=%(creatordate:unix)", refs.TAGS + tag]).stdout.strip()
    if not out.isdigit():
        raise ReleaseError(f"{git_dir} has no tag {tag} with a date")
    return int(out)


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
    if len(notes) > NOTES_LIMIT:
        raise ReleaseError(f"{ref}:{path}: larger than {NOTES_LIMIT} bytes, "
                           "which no client reads")
    return notes if notes.strip() else None
