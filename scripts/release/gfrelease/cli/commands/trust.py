# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The trust commands: trust root new | sign | verify, trust keys renew,
and the signing host's trust update.
"""

from __future__ import annotations

from pathlib import Path

from ...adapters import minisig
from ...app import trust, trust_admin
from ...domain.layout import MINISIG
from .. import host as host_mod
from .. import ui


def report_trust(root: dict, keys: dict) -> None:
    for doc, what in ((root, f"root {root['version']}"),
                      (keys, f"index-keys {keys['version']}")):
        ui.info(f"{what}: expires {doc['expires']}, in "
                f"{trust.days_left(doc):.0f} days")
    due = trust_admin.renewal_due(root, keys)
    if due:
        ui.warn(f"renew the {'root' if due == 'root' else 'index-keys list'}"
                " soon")


def cmd_root_new(ctx) -> int:
    path, _, _ = trust_admin.new_root(ctx.product)
    ui.info(f"{path}: written; edit it by hand if anything else changes, "
            "then have the root keys sign it (trust root sign)")
    return 0


def cmd_root_sign(ctx) -> int:
    file = Path(ctx.args.file)
    key_id = trust_admin.sign_root(file, ctx.args.secret)
    ui.info(f"{file}: signed by {key_id}")
    for g in trust_admin.signatures(file, ctx.product.slug):
        ui.info(f"  {g['whose']} root keys: {len(g['signed'])} of "
                f"{g['threshold']} needed have signed")
        for key in g["missing"]:
            ui.info(f"    missing {key}")
    return 0


def cmd_root_verify(ctx) -> int:
    product = ctx.product
    root, keys = trust.load_trust(product.root_dir, product.index_keys_dir,
                                  product.slug)
    report_trust(root, keys)
    trust.check_unexpired(root, f"root {root['version']}")
    trust.check_unexpired(keys, f"index-keys {keys['version']}")
    return 0


def cmd_keys(ctx) -> int:
    args = ctx.args
    if (args.secret_out or args.keep_old) and not args.rotate:
        ui.error("--secret-out and --keep-old go with --rotate")
        return host_mod.FAILED
    if args.rotate and not args.secret_out:
        ui.error("--rotate needs --secret-out: where the new key's secret "
                 "goes")
        return host_mod.FAILED
    done = trust_admin.renew_index_keys(
        ctx.product, args.secret,
        secret_out=args.secret_out if args.rotate else None,
        keep_old=args.keep_old)
    path, new_key, secret = done["path"], done["new_key"], done["secret"]
    ui.info(f"{path}: written and signed, expires {done['expires']}; commit "
            f"it and its {MINISIG}")
    if new_key is not None:
        ui.info(f"new index key {minisig.key_id(new_key)}: {new_key}\n"
                f"  its secret is {secret}, made without a password "
                "(minisign -W): install it on the signing host as "
                "index.key. Until the host has it and the list is "
                "committed, the host refuses to sign"
                + ("" if args.keep_old else
                   " (the old index stays valid until it expires)") + ".")
    return 0


def cmd_trust_update(ctx) -> int:
    args, product = ctx.args, ctx.product
    root, keys, problems = trust_admin.update_from_git(
        product, Path(args.git_dir), Path(args.state_dir))
    report_trust(root, keys)
    for problem in problems:
        ui.warn(problem)
    return host_mod.PARTIAL if problems else host_mod.DONE
