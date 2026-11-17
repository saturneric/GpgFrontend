# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""CI's stage, and the signing host's promote, index and resign."""

from __future__ import annotations

from pathlib import Path

from ...adapters import minisig
from ...app import stage as stage_mod
from ...app import views
from ...domain import layout
from .. import host as host_mod
from .. import ui


def cmd_stage(ctx) -> int:
    args = ctx.args
    prefix = stage_mod.stage(
        ctx.product, Path(args.candidate), ctx.store(args.staging),
        ref=args.ref, commit=args.commit, version=args.version,
        filesafe=args.filesafe, built_at=args.built_at)
    ctx.result = {"staged": prefix}
    ui.info(f"staged {prefix}")
    if prefix.startswith(f"{layout.TAGS}/"):
        ui.next_step(f"the maintainer runs `gfrelease stable {args.version}`")
    else:
        ui.next_step("the signing host promotes it on its next round")
    return 0


def cmd_host(ctx) -> int:
    """promote, index and resign: the signing host's commands."""
    args, product = ctx.args, ctx.product
    command = args.key
    dry = getattr(args, "dry_run", False)
    if command != "promote":
        product.check_host_keys(minisig.public_key(args.index_pub))

    def path(value):
        return Path(value) if value else None

    host = host_mod.Host(
        product, ctx.store(args.builds), Path(args.state_dir),
        staging=ctx.store(args.staging), update=ctx.store(args.update),
        git_dir=path(args.git_dir), index_key=path(args.index_key),
        # Every listed index key: what a client accepts.
        index_pub=product.index_keys,
        cosign=args.cosign or "cosign", authenticode_ca=args.authenticode_ca)
    if dry:
        report = host.dry_run()
        ctx.result = report
        render_dry_run(report)
        # Ran to its end and would refuse something: as an index run that
        # refuses something ends.
        return host_mod.PARTIAL if report["refused"] else host_mod.DONE
    if command == "promote":
        status = host.promote()
    else:
        status = host.index(force=command == "resign" or args.force)
    ctx.result = dict(host.report, status={
        host_mod.DONE: "done", host_mod.PARTIAL: "partial",
        host_mod.FAILED: "failed"}[status])
    return status


def render_dry_run(report: dict) -> None:
    ui.info("would list:")
    for line in views.channel_lines(report["channels"]):
        ui.info(line)
    for key, version in sorted(report.get("betas", {}).items()):
        ui.info(f"  beta of {key}: {version}")
    ui.info(f"  releases {', '.join(report['releases']) or '-'}")
    if report["added"] or report["removed"]:
        ui.info(f"against the published index: added "
                f"{', '.join(report['added']) or '-'}; removed "
                f"{', '.join(report['removed']) or '-'}")
    ui.info(f"changes: {', '.join(report['changes']) or 'none'}")
    for refused in report["refused"]:
        ui.warn(f"would refuse: {refused}")
