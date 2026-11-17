# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The maintainer's release commands: stable, import."""

from __future__ import annotations

import getpass
import sys
from pathlib import Path

from ...adapters.process import WINDOWS
from ...adapters.windows import authenticode
from ...app import imported as imported_mod
from ...app import stable as stable_mod
from ...errors import ReleaseError
from .. import ui


def cmd_stable(ctx) -> int:
    args, product = ctx.args, ctx.product
    signer = None
    if args.authenticode_thumbprint:
        if not WINDOWS:
            raise ReleaseError("--authenticode-thumbprint names a "
                               "certificate of the Windows store; here it "
                               "is --authenticode-cert and -key")
        # No PIN is asked here: the card's own software does, once for the
        # programs and once for the packages.
        signer = authenticode.SigntoolSigner(
            args.authenticode_thumbprint, args.signtool or "",
            args.timestamp_url or "")
    elif args.authenticode_cert and args.authenticode_key:
        token = bool(args.pkcs11_module)
        signer = authenticode.Signer(
            args.authenticode_cert, args.authenticode_key,
            args.pkcs11_module or "", args.timestamp_url or "",
            # One PIN for every program signed, asked at the first.
            ask_pin=(lambda: getpass.getpass(
                "PIN of the Authenticode token (empty: the token asks): "))
            if token and sys.stdin.isatty() else None)
    stage = stable_mod.Stage(
        product, args.version, Path(args.work),
        staging=ctx.store(args.staging),
        git_dir=Path(args.git_dir) if args.git_dir else None,
        cosign=args.cosign or "cosign", key=args.key,
        gnupghome=args.gnupghome, signer=signer,
        authenticode_ca=args.authenticode_ca,
        on_signed=lambda name, n, total, path:
        ui.info(f"  {name}: [{n}/{total}] {path or 'the package itself'}"))
    if args.notes_only:
        digest = stage.publish_notes()
        ctx.result = {"version": args.version, "notes": digest}
        ui.info(f"{args.version}: notes {digest} handed over")
        ui.next_step(f"the signing host publishes them on its next round "
                     f"(`gfrelease status {args.version}`)")
        return 0
    if args.msix:
        descriptor = stage.add_msix()
        added = [a["name"] for a in descriptor["artifacts"]
                 if a["transform"] == "msix"]
        ctx.result = {"version": args.version, "added": added}
        ui.info(f"{args.version}: {', '.join(added)} handed over")
        ui.next_step(f"the signing host adds it to the release on its next "
                     f"round (`gfrelease status {args.version}`)")
        return 0

    total = len(stable_mod.STEPS)
    ask = not args.yes and not ui.JSON and sys.stdin.isatty() and \
        not args.dry_run
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
    ctx.result = {"version": args.version,
                  "done": stage.state.get("done", []),
                  "published": descriptor["sums"].rsplit("/", 1)[0]
                  if descriptor else None}
    if args.dry_run:
        ui.info(f"{args.version}: fetched and verified; nothing signed or "
                "uploaded")
        ui.next_step(f"gfrelease stable {args.version}")
    elif descriptor:
        ui.info(f"{args.version}: handed to the signing host as "
                f"{descriptor['sums'].rsplit('/', 1)[0]}")
        kinds = {a["kind"] for a in descriptor["artifacts"]}
        if "msi" in kinds and "msix" not in kinds:
            ui.info(f"{args.version}: without an MSIX, which only Windows "
                    f"packs (there: gfrelease stable {args.version} --msix)")
        ui.next_step(f"the signing host receives and lists it on its next "
                     f"round: `gfrelease status {args.version}`")
    else:
        ui.info(f"{args.version}: done")
    return 0


def cmd_import(ctx) -> int:
    args = ctx.args
    staging = ctx.store(args.staging)
    done = []
    for version in args.versions:
        job = imported_mod.Import(ctx.product, version, Path(args.work),
                                  staging=staging, git_dir=Path(args.git_dir))
        d = job.run()
        done.append({"version": version, "files": len(d["artifacts"]),
                     "signed_by": d["signing"]["fingerprint"],
                     "left_out": list(job.skipped),
                     "uploaded": bool(staging)})
        ui.info(f"{version}: {len(d['artifacts'])} files, signed by "
                f"{d['signing']['fingerprint']}"
                + (f", handed over as releases/{version}/" if staging
                   else " (checked only, nothing uploaded)"))
        for skipped in job.skipped:
            ui.info(f"  left out: {skipped}")
    ctx.result = {"imported": done}
    if staging:
        ui.next_step("the signing host receives and lists them on its next "
                     "round")
    else:
        ui.next_step("run again with --staging to hand them over")
    return 0
