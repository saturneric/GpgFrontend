# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Anyone's commands: verify, show, status, doctor."""

from __future__ import annotations

import datetime as dt

from ...adapters.clock import utcnow
from ...adapters.store import Store
from ...app import doctor as doctor_mod
from ...app import verify as verify_mod
from ...app import views
from ...domain.formats import parse_iso
from ...errors import ReleaseError
from .. import host as host_mod
from .. import ui


def sources(ctx):
    """The update and builds stores, or the HTTPS bases a client reads."""
    args, product = ctx.args, ctx.product
    bases = product.data["bases"]
    update = getattr(args, "update", None)
    builds = getattr(args, "builds", None)
    cafile = getattr(args, "cafile", None)
    return (Store(update, ctx.rclone) if update else
            verify_mod.HttpSource(bases["update"], product.slug, cafile),
            Store(builds, ctx.rclone) if builds else
            verify_mod.HttpSource(bases["builds"], product.slug, cafile))


def cmd_verify(ctx) -> int:
    args = ctx.args
    if bool(getattr(args, "update", None)) != \
            bool(getattr(args, "builds", None)):
        raise ReleaseError("give both --update and --builds, or neither "
                           "(the HTTPS bases)")
    update, builds = sources(ctx)
    report = verify_mod.verify(ctx.product, update, builds, deep=args.deep)
    ctx.result = report
    left = parse_iso(report["expires"]) - utcnow()
    ui.info(f"index serial {report['serial']} of {report['product']} "
            f"verifies; expires {report['expires']} (in "
            f"{left.total_seconds() / 86400:.1f} days)")
    for line in views.channel_lines(report["channels"]):
        ui.info(line)
    ui.info(f"checked {len(report['checked'])}"
            f"{' deeply' if args.deep else ''}:")
    for c in report["checked"]:
        ui.info(f"  ✓ {c.get('version')} ({c.get('kind')}, "
                f"{c.get('artifacts')} files)")
    if getattr(args, "min_days_left", None) and \
            left < dt.timedelta(days=float(args.min_days_left)):
        raise ReleaseError(f"the index expires at {report['expires']}, in "
                           f"{left}: the signing host has not re-signed it")
    return 0


def cmd_show(ctx) -> int:
    update, _ = sources(ctx)
    view = views.show(ctx.product, update)
    ctx.result = view
    for line in views.render_show(view):
        ui.info(line)
    nxt = views.show_next(view)
    if nxt:
        ui.next_step(nxt)
    return 0


def cmd_status(ctx) -> int:
    args = ctx.args
    update, builds = sources(ctx)
    staging = ctx.store(getattr(args, "staging", None))
    if args.release:
        view = views.status(ctx.product, args.release, update=update,
                            builds=builds, staging=staging, work=args.work,
                            git_dir=args.git_dir)
        lines = views.render_status(view)
    else:
        view = views.overview(ctx.product, update=update, builds=builds,
                              staging=staging)
        lines = views.render_overview(view)
    ctx.result = view
    for line in lines:
        ui.info(line)
    if view.get("next"):
        ui.next_step(view["next"])
    return 0


def cmd_doctor(ctx) -> int:
    checks = doctor_mod.Doctor(ctx.args, ctx.product, ctx.args.role).run()
    ctx.result = {"role": ctx.args.role, "checks": checks}
    for line in doctor_mod.render(checks):
        ui.info(line)
    return 0 if all(c["ok"] for c in checks) else host_mod.FAILED
