# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What git says: which version a commit is, asked the one way the build
itself asks (cmake/GitVersion.cmake), and what the tags and the release
notes of a repository are.

  gfrelease ci version [--repo DIR] [--rev COMMIT] [--ref REF] [--field FIELD]
"""

from __future__ import annotations

import subprocess
from pathlib import Path

from ..domain import layout, refs
from ..domain.limits import NOTES_LIMIT
from ..errors import ReleaseError, ToolFailed
from ..paths import ROOT
from .process import run

SCRIPT = ROOT / "cmake" / "GitVersion.cmake"
# A few git commands on a local clone: far longer than they take.
TIMEOUT = 300


def command(repo: Path, ref: str = "", rev: str = "", field: str = "version",
            exclude: tuple[str, ...] = ()) -> list[str]:
    """The cmake command that prints @p field of the version git gives
    @p rev (the work tree's HEAD when empty) on @p ref, the tags in
    @p exclude left out."""
    cmd = ["cmake", f"-DGF_SOURCE_DIR={repo}", f"-DGF_FIELD={field}"]
    if rev:
        cmd.append(f"-DGPGFRONTEND_VERSION_REV={rev}")
    if ref:
        cmd.append(f"-DGPGFRONTEND_VERSION_REF={ref}")
    if exclude:
        cmd.append(f"-DGPGFRONTEND_VERSION_EXCLUDE={';'.join(exclude)}")
    return cmd + ["-P", str(SCRIPT)]


def derive(repo: Path, ref: str = "", rev: str = "",
           field: str = "version") -> str:
    """command()'s answer. Raises subprocess.CalledProcessError (cmake
    failed), subprocess.TimeoutExpired, or FileNotFoundError without
    cmake."""
    return subprocess.run(command(repo, ref, rev, field), check=True,
                          capture_output=True, text=True,
                          timeout=TIMEOUT).stdout.strip()


def current_branch(repo: Path) -> str:
    """The branch @p repo has checked out, or "" (a detached HEAD)."""
    out = run(["git", "-C", repo, "symbolic-ref", "--short", "-q", "HEAD"],
              check=False)
    return out.stdout.strip() if out.returncode == 0 else ""


def git_version(git_dir: Path, commit: str, ref: str,
                exclude: tuple[str, ...] = ()) -> str:
    """Which version a commit is, asked of git through the one derivation the
    build itself uses, never taken from what the build claims; with
    @p exclude, as git called it before those tags were made."""
    return run(command(git_dir, ref=ref, rev=commit, exclude=exclude),
               timeout=TIMEOUT).stdout.strip()


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


def tree_reader(git_dir: Path, ref: str, directory: str):
    """read(name) of the files in @p directory as @p ref has it, None for
    one it does not hold, and their names. A repository that cannot be
    read is an error, never "not there"."""
    prefix = f"{directory}/"
    names = run(["git", "-C", git_dir, "ls-tree", "--name-only", ref,
                 "--", prefix]).stdout.split()

    def read(name: str) -> bytes | None:
        if prefix + name not in names:
            return None
        return run(["git", "-C", git_dir, "show", f"{ref}:{prefix}{name}"],
                   text=False).stdout
    return read, [n[len(prefix):] for n in names]


def notes_at(git_dir: Path, ref: str, version: str) -> bytes | None:
    """release/notes/<version>.md as @p ref has it, or None when it has none
    (or an empty one, or there is no such ref). A repository that cannot be
    read is an error: "no notes" would list a release without them, or
    refuse one that has them."""
    commit = resolve_commit(git_dir, ref)
    if commit is None:
        return None
    path = layout.notes_source(version)
    if not run(["git", "-C", git_dir, "ls-tree", "--name-only", commit, "--",
                path]).stdout.strip():
        return None
    notes = run(["git", "-C", git_dir, "show", f"{commit}:{path}"],
                text=False).stdout
    if len(notes) > NOTES_LIMIT:
        raise ReleaseError(f"{ref}:{path}: larger than {NOTES_LIMIT} bytes, "
                           "which no client reads")
    return notes if notes.strip() else None
