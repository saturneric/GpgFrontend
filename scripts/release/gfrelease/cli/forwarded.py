# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The commands with a command line of their own, handed everything after
their name: policy, candidate, ci check-order and ci version. They need no
product, settings or store, and are what CI and the maintainer's scripts
(cut.sh, notes.sh, rehearse.sh) run. Each prints in the form its caller
reads: GitHub's ::error:: lines for check-order, a bare version for
version."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

from ..adapters import git as gitversion_mod
from ..app import candidate as candidate_mod
from ..app import check_order as check_order_mod
from ..domain import policy as policy_mod
from ..domain import refs
from ..domain.manifest import CandidateError
from ..errors import ReleaseError
from ..paths import ROOT


def policy_main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(
        prog="gfrelease policy", description=policy_mod.__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", choices=["check", "render-security",
                                        "check-security", "mainline"])
    ap.add_argument("--policy", type=Path, default=policy_mod.POLICY)
    ap.add_argument("--product", type=Path, default=policy_mod.PRODUCT)
    ap.add_argument("--security", type=Path, default=policy_mod.SECURITY)
    ap.add_argument("--root-dir", type=Path, default=policy_mod.TRUST_ROOT)
    args = ap.parse_args(argv)

    try:
        policy = policy_mod.load_policy(args.policy)
        if args.command == "mainline":
            print(policy_mod.mainline(policy))
            return 0
        for warning in policy_mod.check_policy(policy):
            print(f"warning: {warning}", file=sys.stderr)
        product = policy_mod.load_product(args.product)
        policy_mod.check_product(product)
        if args.command == "check":
            return 0

        current = args.security.read_text(encoding="utf-8")
        rendered = policy_mod.render_security(
            current, policy, policy_mod.newest_root(args.root_dir))
        if args.command == "render-security":
            if rendered != current:
                args.security.write_text(rendered, encoding="utf-8")
            return 0
        if rendered != current:
            print(f"{args.security} is out of date; run "
                  "scripts/gfrelease policy render-security", file=sys.stderr)
            return 1
        return 0
    except policy_mod.PolicyError as e:
        print(f"policy: {e}", file=sys.stderr)
        return 1
    except (OSError, ValueError) as e:
        # A file that is missing, or not TOML or JSON at all.
        print(f"policy: {e}", file=sys.stderr)
        return 1


def candidate_main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        prog="gfrelease candidate",
        description=candidate_mod.__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("stage", help="write the release candidate")
    p.add_argument("--artifacts", required=True, type=Path)
    p.add_argument("--out", required=True, type=Path)

    args = parser.parse_args(argv)
    try:
        staged = candidate_mod.stage(args.artifacts, args.out)
    except CandidateError as e:
        print(f"candidate: {e}", file=sys.stderr)
        return 1
    hashes = staged["hashes"]
    print(f"staged {len(hashes)} deliverable(s) from {staged['legs']} "
          "leg(s)")
    for name in sorted(hashes):
        print(f"  {hashes[name]}  {name}")
    return 0


def current_ref(repo: Path) -> str:
    """The ref being built: GitHub's (the branch, or a tag as its full
    ref; "" for anything else, a pull request's merge ref), else the
    branch @p repo has checked out."""
    ref = os.environ.get("GITHUB_REF", "")
    if refs.branch_of(ref):
        return refs.branch_of(ref)
    if refs.tag_of(ref):
        return ref
    if ref:
        return ""
    return gitversion_mod.current_branch(repo)


def check_order_main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(
        prog="gfrelease ci check-order", description=check_order_mod.__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", type=Path, default=ROOT)
    ap.add_argument("--ref")
    ap.add_argument("--version")
    ap.add_argument("--policy", type=Path, default=policy_mod.POLICY)
    args = ap.parse_args(argv)

    try:
        ref = args.ref if args.ref is not None else current_ref(args.repo)
        if refs.tag_of(ref):
            problems = check_order_mod.check_tag(refs.tag_of(ref), args.repo)
            for p in problems:
                print(f"::error::{p}", file=sys.stderr)
            return 1 if problems else 0
        # Only the branches whose builds are published have an order to
        # keep (release/2.2-fix is no release branch, and publishes
        # nothing).
        if ref != refs.MAIN_BRANCH and not refs.branch_series(ref):
            print(f"check_order: nothing to check for ref {ref or '(none)'}")
            return 0
        version = args.version or gitversion_mod.derive(args.repo, ref=ref)
        mainline = policy_mod.mainline(policy_mod.load_policy(args.policy))
        problems = check_order_mod.check(
            version, ref, check_order_mod.release_tags(args.repo),
            mainline)
    except subprocess.CalledProcessError as e:
        print(f"::error::check_order: {' '.join(map(str, e.cmd[:2]))} "
              f"failed: {(e.stderr or '').strip()}", file=sys.stderr)
        return 1
    except (ReleaseError, subprocess.TimeoutExpired, OSError,
            ValueError) as e:
        print(f"::error::check_order: {e}", file=sys.stderr)
        return 1
    for p in problems:
        print(f"::error::{p}", file=sys.stderr)
    if not problems:
        print(f"check_order: {ref} builds {version}; order holds")
    return 1 if problems else 0


def version_main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(
        prog="gfrelease ci version", description=gitversion_mod.__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", type=Path, default=ROOT)
    ap.add_argument("--rev", default="")
    ap.add_argument("--ref", default="")
    ap.add_argument("--field", default="version")
    args = ap.parse_args(argv)
    try:
        print(gitversion_mod.derive(args.repo, args.ref, args.rev,
                                    args.field))
    except subprocess.CalledProcessError as e:
        print(f"gitversion: {(e.stderr or '').strip()}", file=sys.stderr)
        return 1
    except (subprocess.TimeoutExpired, OSError) as e:
        print(f"gitversion: {e}", file=sys.stderr)
        return 1
    return 0


FORWARDED = {
    "policy": policy_main,
    "candidate": candidate_main,
    "ci check-order": check_order_main,
    "ci version": version_main,
}


def forwarded(argv: list[str]):
    """The forwarded command @p argv starts with, and the rest of it."""
    for words in (2, 1):
        name = " ".join(argv[:words])
        if len(argv) >= words and name in FORWARDED:
            return FORWARDED[name], argv[words:]
    if argv[:1] == ["ci"]:
        print("usage: gfrelease ci {check-order,version} ...",
              file=sys.stderr)
        return None, None
    return None, argv
