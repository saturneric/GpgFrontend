# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""CI's commands: policy, candidate stage, ci build-mode and ci
check-order. They need no product or store, and each prints in the form
its caller reads: GitHub's ::error:: lines, key=value lines for
$GITHUB_OUTPUT."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

from ...adapters import git as gitversion_mod
from ...app import candidate as candidate_mod
from ...app import check_order as check_order_mod
from ...domain import build_mode, refs
from ...domain import policy as policy_mod
from ...domain.manifest import CandidateError
from ...errors import ReleaseError


def cmd_policy(ctx) -> int:
    """check: the policy, the product and SECURITY.md agree with them;
    render-security: SECURITY.md written from them."""
    args = ctx.args
    policy_file = Path(args.policy) if args.policy else policy_mod.POLICY
    product_file = Path(args.product) if args.product else policy_mod.PRODUCT
    root_dir = Path(args.root_dir) if args.root_dir else policy_mod.TRUST_ROOT
    try:
        policy = policy_mod.load_policy(policy_file)
        for warning in policy_mod.check_policy(policy):
            print(f"warning: {warning}", file=sys.stderr)
        policy_mod.check_product(policy_mod.load_product(product_file))
        current = args.security.read_text(encoding="utf-8")
        rendered = policy_mod.render_security(
            current, policy, policy_mod.newest_root(root_dir))
        if args.action == "render-security":
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


def cmd_candidate_stage(ctx) -> int:
    args = ctx.args
    try:
        staged = candidate_mod.stage(args.artifacts, args.out,
                                     args.filesafe_version)
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


def cmd_check_order(ctx) -> int:
    args = ctx.args
    policy_file = Path(args.policy) if args.policy else policy_mod.POLICY
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
        version = args.build_version or \
            gitversion_mod.derive(args.repo, ref=ref)
        mainline = policy_mod.mainline(policy_mod.load_policy(policy_file))
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


def cmd_build_mode(ctx) -> int:
    """Reads GITHUB_EVENT_NAME and GITHUB_REF, and the inputs
    BUILD_MODE_INPUT (workflow_dispatch's build_mode) and SCREENSHOT_INPUT
    ("true" for a screenshot-mode build), passed through the environment
    rather than interpolated into a script. Prints key=value lines for
    $GITHUB_OUTPUT. The version is derived once, from the full history
    (fetch-depth: 0), so every job names its files alike."""
    repo = ctx.args.repo
    env = os.environ
    screenshot = env.get("SCREENSHOT_INPUT", "false") == "true"
    try:
        event, ref = env["GITHUB_EVENT_NAME"], env["GITHUB_REF"]
        fields = build_mode.decide(
            event, ref, build_mode=env.get("BUILD_MODE_INPUT", ""),
            screenshot=screenshot,
            android=policy_mod.load_product()["android"])
        for field, name in (("version", "version"),
                            ("artifact_version", "filesafe"),
                            ("build_number", "build_number")):
            fields[field] = gitversion_mod.derive(repo, field=name)
        build_mode.check_version(fields["version"],
                                 fields["artifact_version"],
                                 fields["build_number"])
    except KeyError as e:
        print(f"::error::{e.args[0]} is not set", file=sys.stderr)
        return 1
    except ReleaseError as e:
        print(f"::error::{e}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as e:
        print(f"::error::cmake failed: {(e.stderr or '').strip()}",
              file=sys.stderr)
        return 1
    except (subprocess.TimeoutExpired, OSError) as e:
        print(f"::error::{e}", file=sys.stderr)
        return 1
    for field in build_mode.FIELDS:
        print(f"{field}={fields[field]}")
    print(f"Version: {fields['version']}\n"
          f"Build mode: {fields['build_mode']}\n"
          f"Publish: {fields['publish_kind']}\n"
          f"Build type: {fields['build_type']}\n"
          f"Screenshot mode: {str(screenshot).lower()}", file=sys.stderr)
    return 0
