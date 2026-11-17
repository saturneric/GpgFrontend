# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Setting up and finding one's way: help, config, completion."""

from __future__ import annotations

from pathlib import Path

from ...adapters.files import atomic_write
from ...errors import ReleaseError
from .. import settings as settings_mod
from .. import topics, ui
from ..completion import completion
from ..forwarded import forwarded


def cmd_help(ctx) -> int:
    topic = ctx.args.topic
    if not topic:
        for line in topics.listing():
            ui.info(line)
        ui.info("\nCommands: gfrelease --help; one command: gfrelease "
                "COMMAND --help.")
        return 0
    if topic in topics.TOPICS:
        ui.info(topics.text(topic))
        return 0
    # A command: its own --help, or argparse's error (exit 2) for none.
    tool, _ = forwarded(topic.split())
    if tool is not None:
        try:
            return tool(["--help"])
        except SystemExit as e:
            return e.code or 0
    try:
        ctx.build_parser(ctx.settings).parse_args([*topic.split(), "--help"])
    except SystemExit as e:
        return e.code or 0
    return 0


def cmd_config(ctx) -> int:
    if ctx.args.action == "init":
        path = Path.cwd() / "gfrelease.toml" if ctx.args.here else \
            settings_mod.default_config_path()
        if path.exists():
            raise ReleaseError(f"{path} exists; edit it, or remove it first")
        path.parent.mkdir(parents=True, exist_ok=True)
        atomic_write(path, settings_mod.TEMPLATE.encode())
        ctx.result = {"written": str(path)}
        ui.info(f"{path}: written")
        ui.next_step("fill it in, then `gfrelease config show`")
        return 0
    s = ctx.settings
    rows = {}
    for name in sorted(settings_mod.KNOWN):
        value, source = s.lookup(name)
        rows[name] = {"value": value, "from": source}
    ctx.result = {"config": str(s.path) if s.path else None,
                  "settings": rows}
    ui.info(f"config file: {s.path or 'none'}")
    width = max(map(len, rows))
    for name, row in rows.items():
        if row["value"] is None:
            ui.info(f"  {name:<{width}}  -")
        else:
            ui.info(f"  {name:<{width}}  {row['value']}  ({row['from']})")
    return 0


def cmd_completion(ctx) -> int:
    print(completion(ctx.build_parser(), ctx.args.shell))
    return 0
