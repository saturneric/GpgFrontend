# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""CI's stage, and the signing host's promote and index."""

from __future__ import annotations

import json
from pathlib import Path

from ...adapters import minisig
from ...app import stage as stage_mod
from .. import host as host_mod
from .. import ui


def cmd_stage(ctx) -> int:
    args = ctx.args
    prefix = stage_mod.stage(
        ctx.product, Path(args.candidate), ctx.store(args.staging),
        ref=args.ref, commit=args.commit, version=args.version,
        filesafe=args.filesafe, built_at=args.built_at)
    ui.info(f"staged {prefix}")
    return 0


def cmd_host(ctx) -> int:
    """promote and index: the signing host's commands."""
    args, product = ctx.args, ctx.product
    if args.name == "index":
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
    if args.name == "promote":
        status = host.promote()
    else:
        status = host.index(force=args.force)
    ui.info(json.dumps(host.report, indent=2, sort_keys=True))
    return status
