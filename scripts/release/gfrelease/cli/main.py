# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease's command line run: a forwarded command handed its arguments,
anything else parsed (parser.py), its settings filled in, and its
handler (commands/) run with the product.
"""

from __future__ import annotations

import contextlib
import io
import os
import sys
from pathlib import Path

from ..adapters import process
from ..adapters.store import Store
from ..app.product import Product
from ..errors import ReleaseError
from ..paths import PRODUCT_FILE
from . import host as host_mod
from . import settings as settings_mod
from . import ui
from .forwarded import forwarded
from .parser import OPT_NAMES, build_parser

# What each command cannot do without, checked before it starts.
REQUIRED = {
    "stage": ("candidate", "staging", "ref", "commit", "version", "filesafe",
              "built_at"),
    "stable": ("work",),
    "import": ("work", "git_dir"),
    "promote": ("builds", "state_dir", "staging", "git_dir"),
    "index": ("builds", "state_dir", "update", "index_key", "index_pub"),
    "resign": ("builds", "state_dir", "update", "index_key", "index_pub"),
    "trust update": ("git_dir", "state_dir", "root_dir", "index_keys_dir"),
}


class Context:
    def __init__(self, args, product, settings):
        self.args, self.product, self.settings = args, product, settings
        self.rclone = args.rclone or "rclone"
        self.result = None  # what --json prints
        # The command line itself, for the commands that read it
        # (help, completion).
        self.build_parser = build_parser

    def store(self, root):
        return Store(root, self.rclone) if root else None


def product_of(args) -> Product:
    if args.product:
        product = Path(args.product)
        policy = Path(args.policy) if args.policy else \
            product.with_name("policy.toml")
    else:
        product = PRODUCT_FILE
        policy = product.with_name("policy.toml")
    return Product(product, policy, root_dir=args.root_dir,
                   index_keys_dir=args.index_keys_dir,
                   root_sha256=args.root_sha256)


def check_required(args) -> None:
    names = REQUIRED.get(args.key, ())
    if args.key == "index" and getattr(args, "dry_run", False):
        names = tuple(n for n in names if n != "index_key")
    missing = [n for n in names if not getattr(args, n, None)]
    if missing:
        group = {n: g for g, ns in settings_mod.GROUPS.items() for n in ns}
        raise ReleaseError(
            "missing " + ", ".join(f"--{n.replace('_', '-')}"
                                   for n in missing) + "\n" +
            "\n".join(f"--{n.replace('_', '-')}, GFR_{n.upper()}" +
                      (f", or [{group[n]}] {n} in gfrelease.toml"
                       if n in group else "") for n in missing) +
            "\n" + args.parser.format_usage().strip())


def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else list(argv)
    tool, rest = forwarded(argv)
    if tool is not None:
        return tool(rest)
    if rest is None:
        return 2
    try:
        settings = settings_mod.Settings()
    except ReleaseError as e:
        ui.error(str(e))
        return host_mod.FAILED
    args = build_parser(settings).parse_args(argv)
    ui.configure(json_out=args.json, quiet=args.quiet, verbose=args.verbose)
    process.on_start = ui.tool_started
    settings.fill(args, sorted(n for n in OPT_NAMES if hasattr(args, n)))
    out = contextlib.ExitStack()
    try:
        product = product_of(args) if args.needs_product else None
        check_required(args)
        ctx = Context(args, product, settings)
        with out:
            if ui.JSON:
                # One JSON document on stdout: whatever else would print
                # there is left out.
                out.enter_context(contextlib.redirect_stdout(io.StringIO()))
            elif ui.QUIET:
                devnull = out.enter_context(open(os.devnull, "w"))
                out.enter_context(contextlib.redirect_stdout(devnull))
            status = args.handler(ctx)
        if ctx.result is not None:
            ui.emit(ctx.result)
        return status
    except ReleaseError as e:
        ui.error(str(e))
        return host_mod.FAILED
    except (KeyError, TypeError, ValueError, AttributeError, IndexError,
            UnicodeError, OSError) as e:
        # A safety net: input that slipped past the checks, or a file that
        # could not be read or written, is reported, not dumped as a
        # traceback.
        ui.error(f"malformed input ({type(e).__name__}: {e})")
        return host_mod.FAILED
