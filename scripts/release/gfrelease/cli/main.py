# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease's command line run: parsed (parser.py), its settings filled
in (settings.py), and its handler (commands/) run with the product."""

from __future__ import annotations

import sys
from pathlib import Path

from ..adapters import process
from ..adapters.store import Store
from ..app.product import Product
from ..errors import ReleaseError
from ..paths import PRODUCT_FILE
from . import settings as settings_mod
from . import ui
from .host import FAILED
from .parser import build_parser

# What each command cannot do without, checked before it starts.
REQUIRED = {
    "stable": ("work",),
    "promote": ("builds", "state_dir", "staging", "git_dir"),
    "index": ("builds", "state_dir", "update", "index_key", "index_pub"),
    "trust update": ("git_dir", "state_dir", "root_dir", "index_keys_dir"),
}


class Context:
    def __init__(self, args, product):
        self.args, self.product = args, product
        self.rclone = args.rclone or "rclone"

    def store(self, root):
        return Store(root, self.rclone) if root else None


def product_of(args) -> Product:
    product = Path(args.product) if args.product else PRODUCT_FILE
    policy = Path(args.policy) if args.policy else \
        product.with_name("policy.toml")
    return Product(product, policy, root_dir=args.root_dir,
                   index_keys_dir=args.index_keys_dir,
                   root_sha256=args.root_sha256)


def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else list(argv)
    args = build_parser().parse_args(argv)
    ui.configure(verbose=args.verbose)
    process.on_start = ui.tool_started
    try:
        settings_mod.fill(args)
        missing = [n for n in REQUIRED.get(args.name, ())
                   if not getattr(args, n, None)]
        if missing:
            raise ReleaseError("missing " + ", ".join(
                f"--{n.replace('_', '-')} (or GFR_{n.upper()})"
                for n in missing))
        product = product_of(args) if args.needs_product else None
        return args.handler(Context(args, product))
    except ReleaseError as e:
        ui.error(str(e))
        return FAILED
    except (KeyError, TypeError, ValueError, AttributeError, IndexError,
            UnicodeError, OSError) as e:
        # A safety net: input that slipped past the checks, or a file that
        # could not be read or written, is reported, not dumped as a
        # traceback.
        ui.error(f"malformed input ({type(e).__name__}: {e})")
        return FAILED
