# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The maintainer's release command: stable."""

from __future__ import annotations

import sys
from pathlib import Path

from ...adapters.process import WINDOWS
from ...adapters.windows import authenticode
from ...app import stable as stable_mod
from ...errors import ReleaseError
from .. import ui


def signer_of(args):
    """signtool with a certificate of the Windows store; or, for a
    rehearsal or a test, osslsigncode with a certificate and key file."""
    if args.authenticode_thumbprint:
        if not WINDOWS:
            raise ReleaseError("--authenticode-thumbprint names a "
                               "certificate of the Windows store; sign on "
                               "Windows")
        # No PIN is asked here: the card's own software does, once for the
        # programs and once for the packages.
        return authenticode.SigntoolSigner(
            args.authenticode_thumbprint, args.signtool or "",
            args.timestamp_url or "")
    if args.authenticode_cert and args.authenticode_key:
        return authenticode.Signer(args.authenticode_cert,
                                   args.authenticode_key,
                                   timestamp_url=args.timestamp_url or "")
    return None


def cmd_stable(ctx) -> int:
    args, product = ctx.args, ctx.product
    stage = stable_mod.Stage(
        product, args.version, Path(args.work),
        staging=ctx.store(args.staging),
        git_dir=Path(args.git_dir) if args.git_dir else None,
        cosign=args.cosign or "cosign", key=args.key,
        gnupghome=args.gnupghome, signer=signer_of(args),
        authenticode_ca=args.authenticode_ca,
        on_signed=lambda name, n, total, path:
        ui.info(f"  {name}: [{n}/{total}] {path or 'the package itself'}"))
    if args.notes_only:
        digest = stage.publish_notes()
        ui.info(f"{args.version}: notes {digest} handed over")
        return 0

    total = len(stable_mod.STEPS)
    ask = not args.yes and sys.stdin.isatty() and not args.dry_run
    ui.info(f"{args.version}: {' → '.join(stable_mod.STEPS)}"
            f"{' (dry run: until verify)' if args.dry_run else ''}")

    def on_step(step, number, done_before):
        ui.info(f"[{number}/{total}] {step}"
                f"{' (done before)' if done_before else ''}")
        if step == "upload" and not done_before and ask:
            try:
                answer = input(f"Hand {args.version} to the signing host "
                               f"(upload to {args.staging})? Once it has "
                               "published it, it is never replaced. [y/N] ")
            except EOFError:
                # Windows calls NUL a terminal too: nobody there is a No.
                answer = ""
            if answer.strip().lower() not in ("y", "yes"):
                raise ReleaseError(f"stopped before the upload: nothing of "
                                   f"{args.version} was published; run "
                                   "again to go on")

    descriptor = stage.run(args.redo, until="verify" if args.dry_run
                           else None, on_step=on_step)
    if args.dry_run:
        ui.info(f"{args.version}: fetched and verified; nothing signed or "
                "uploaded")
    elif descriptor:
        ui.info(f"{args.version}: handed to the signing host as "
                f"{descriptor['sums'].rsplit('/', 1)[0]}")
    else:
        ui.info(f"{args.version}: done")
    return 0
