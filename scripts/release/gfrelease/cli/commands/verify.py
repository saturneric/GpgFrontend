# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""verify: the published index checked as a client checks it."""

from __future__ import annotations

import datetime as dt

from ...adapters.clock import utcnow
from ...adapters.store import Store
from ...app import verify as verify_mod
from ...domain.formats import parse_iso
from ...errors import ReleaseError
from .. import ui

CHANNELS = ("stable", "beta", "prerelease", "nightly")


def sources(ctx):
    """The update and builds stores, or the HTTPS bases a client reads."""
    args, product = ctx.args, ctx.product
    if bool(args.update) != bool(args.builds):
        raise ReleaseError("give both --update and --builds, or neither "
                           "(the HTTPS bases)")
    bases = product.data["bases"]
    return (Store(args.update, ctx.rclone) if args.update else
            verify_mod.HttpSource(bases["update"], product.slug, args.cafile),
            Store(args.builds, ctx.rclone) if args.builds else
            verify_mod.HttpSource(bases["builds"], product.slug, args.cafile))


def cmd_verify(ctx) -> int:
    args = ctx.args
    update, builds = sources(ctx)
    report = verify_mod.verify(ctx.product, update, builds, deep=args.deep)
    left = parse_iso(report["expires"]) - utcnow()
    ui.info(f"index serial {report['serial']} of {report['product']} "
            f"verifies; expires {report['expires']} (in "
            f"{left.total_seconds() / 86400:.1f} days)")
    for name in CHANNELS:
        ui.info(f"  {name:<10} {report['channels'].get(name) or '-'}")
    ui.info(f"checked {len(report['checked'])}"
            f"{' deeply' if args.deep else ''}:")
    for c in report["checked"]:
        ui.info(f"  {c.get('version')} ({c.get('kind')}, "
                f"{c.get('artifacts')} files)")
    if args.min_days_left and \
            left < dt.timedelta(days=float(args.min_days_left)):
        raise ReleaseError(f"the index expires at {report['expires']}, in "
                           f"{left}: the signing host has not re-signed it")
    return 0
