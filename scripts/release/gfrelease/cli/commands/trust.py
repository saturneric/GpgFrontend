# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The trust commands: trust root new | sign | status | verify, trust keys
renew | rotate, and the signing host's trust update.
"""

from __future__ import annotations

from pathlib import Path

from ...adapters import minisig
from ...app import trust, trust_admin, views
from ...domain.layout import MINISIG
from ...errors import ReleaseError
from .. import host as host_mod
from .. import ui


def report_trust(root: dict, keys: dict) -> None:
    for doc, what in ((root, f"root {root['version']}"),
                      (keys, f"index-keys {keys['version']}")):
        ui.info(f"{what}: expires {doc['expires']}, in "
                f"{trust.days_left(doc):.0f} days")


def trust_next(root: dict, keys: dict) -> None:
    """A next: line when the root or the list should be renewed soon."""
    due = trust_admin.renewal_due(root, keys)
    if due == "root":
        ui.next_step("renew the root: gfrelease trust root new (gfrelease "
                     "help root-ceremony)")
    elif due == "index_keys":
        ui.next_step("renew the index-keys list: gfrelease trust keys renew")


def cmd_root_new(ctx) -> int:
    path, nxt, root = trust_admin.new_root(ctx.product)
    threshold = root["roles"]["root"]["threshold"]
    ctx.result = {"written": str(path), "version": nxt["version"]}
    ui.info(f"{path}: written; edit it by hand if anything else changes")
    ui.next_step(f"{threshold} of root {root['version']}'s keys and as many "
                 f"of its own sign it: gfrelease trust root sign {path} -s "
                 "<root key> (`gfrelease trust root status` shows who has)")
    return 0


def cmd_root_sign(ctx) -> int:
    key_id = trust_admin.sign_root(Path(ctx.args.file), ctx.args.secret)
    ui.info(f"{ctx.args.file}: signed by {key_id}")
    ctx.result = {"file": ctx.args.file, "signed_by": key_id}
    try:
        view = views.root_status(Path(ctx.args.file).parent,
                                 ctx.product.slug)
    except ReleaseError:
        return 0
    nxt = views.root_status_next(view)
    if view["file"] == str(Path(ctx.args.file)) and nxt:
        ui.next_step(nxt)
    return 0


def cmd_root_status(ctx) -> int:
    view = views.root_status(ctx.product.root_dir, ctx.product.slug)
    ctx.result = view
    for line in views.render_root_status(view):
        ui.info(line)
    nxt = views.root_status_next(view)
    if nxt:
        ui.next_step(nxt)
    return 0


def cmd_root_verify(ctx) -> int:
    product = ctx.product
    root, keys = trust.load_trust(product.root_dir, product.index_keys_dir,
                                  product.slug, first=ctx.args.first)
    ctx.result = trust_admin.summary(root, keys)
    report_trust(root, keys)
    trust.check_unexpired(root, f"root {root['version']}")
    trust.check_unexpired(keys, f"index-keys {keys['version']}")
    trust_next(root, keys)
    return 0


def cmd_keys(ctx) -> int:
    args = ctx.args
    done = trust_admin.renew_index_keys(
        ctx.product, args.secret,
        secret_out=args.secret_out if args.action == "rotate" else None,
        keep_old=getattr(args, "keep_old", False))
    path, new_key, secret = done["path"], done["new_key"], done["secret"]
    ctx.result = {"written": str(path), "expires": done["expires"],
                  "new_key": new_key}
    ui.info(f"{path}: written and signed, expires {done['expires']}")
    if new_key is None:
        ui.next_step(f"commit {path} and its {MINISIG}")
        return 0
    ui.info(f"new index key {minisig.key_id(new_key)}: {new_key}\n"
            f"  its secret is {secret}, made without a password (minisign "
            "-W):\n  the signing host uses it unattended. Keep it as a "
            "secret, nowhere else.")
    if args.keep_old:
        ui.next_step("commit the list; then, in any order, install the new "
                     "secret on the host as index.key, and later list only "
                     "the new key (rotate again without --keep-old, or edit "
                     "and sign)")
    else:
        ui.next_step("the host signs with a listed key only. Install the new "
                     "secret on the host as index.key and commit the list "
                     "together: until both are done the host refuses to sign "
                     "(the old index stays valid until it expires)")
    return 0


def cmd_trust_update(ctx) -> int:
    args, product = ctx.args, ctx.product
    root, keys, problems = trust_admin.update_from_git(
        product, Path(args.git_dir), Path(args.state_dir))
    ctx.result = dict(trust_admin.summary(root, keys),
                      problems=list(problems))
    report_trust(root, keys)
    for problem in problems:
        ui.warn(problem)
    trust_next(root, keys)
    return host_mod.PARTIAL if problems else host_mod.DONE
