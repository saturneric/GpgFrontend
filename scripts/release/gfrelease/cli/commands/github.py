# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""github sync: the stable releases mirrored to GitHub Releases."""

from __future__ import annotations

import os

from ...adapters.github import GitHub
from ...app import github_sync
from ...errors import ReleaseError
from .. import host as host_mod
from .. import ui
from .verify import sources

# The token, from the environment only (never a command line, which others
# on the machine can read): GitHub Actions' own, with contents: write.
TOKEN = "GITHUB_TOKEN"


def cmd_sync(ctx) -> int:
    args, product = ctx.args, ctx.product
    token = os.environ.get(TOKEN) or None
    if token is None and not args.dry_run:
        raise ReleaseError(f"{TOKEN} is not set: a token that writes the "
                           "repository's releases (or --dry-run)")
    update, builds = sources(ctx)
    github = GitHub(args.repository or product.repository, token)
    report = github_sync.sync(product, update, builds, github,
                              versions=args.versions or None,
                              dry_run=args.dry_run)
    problems = report["problems"]

    ui.info(f"index serial {report['serial']}, GitHub {report['repository']}"
            f"{' (dry run)' if args.dry_run else ''}:")
    if not report["releases"]:
        ui.info("  no stable release to mirror")
    for one in report["releases"]:
        ui.info(f"  {one['version']}: {one['state']}" +
                (f", {one['detail']}" if one.get("detail") else "") +
                (f" {one['url']}" if one.get("url") else ""))
        steps = one["steps"]
        if args.dry_run or args.verbose:
            for step in steps:
                ui.info(f"    {step}")
        elif steps:
            uploads = sum(s.startswith(("upload ", "replace ")) for s in steps)
            others = [s for s in steps
                      if not s.startswith(("upload ", "replace "))]
            ui.info("    " + ", ".join(
                others + ([f"{uploads} files uploaded"] if uploads else [])))
    for problem in problems:
        ui.warn(problem)
    if problems:
        return host_mod.FAILED if problems.passing else host_mod.PARTIAL
    return host_mod.DONE
