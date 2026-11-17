# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Shell completion for bash and zsh, from the parser itself."""

from __future__ import annotations

import argparse


def _commands(parser, path=()):
    """(path, options) of every command of @p parser, deepest last."""
    found = []
    options = [s for a in parser._actions for s in a.option_strings
               if a.help != argparse.SUPPRESS]
    subs = [a for a in parser._actions
            if isinstance(a, argparse._SubParsersAction)]
    words = []
    for action in subs:
        for name, p in action.choices.items():
            words.append(name)
            found += _commands(p, (*path, name))
    found.append((path, words + options))
    return found


def completion(parser, shell: str) -> str:
    cases = []
    for path, words in sorted(_commands(parser), key=lambda c: -len(c[0])):
        key = " ".join(path)
        pattern = f'"{key}"|"{key} "*' if key else '""'
        cases.append(f'    {pattern}) opts="{" ".join(words)}" ;;')
    script = "\n".join([
        "_gfrelease() {",
        '  local cur="${COMP_WORDS[COMP_CWORD]}" path="" w opts',
        '  for w in "${COMP_WORDS[@]:1:COMP_CWORD-1}"; do',
        '    case "$w" in -*) ;; *) path="${path:+$path }$w" ;; esac',
        "  done",
        '  case "$path" in',
        *cases,
        '    *) opts="" ;;',
        "  esac",
        '  COMPREPLY=($(compgen -W "$opts" -- "$cur"))',
        "}",
        "complete -o default -F _gfrelease gfrelease",
    ])
    if shell == "zsh":
        script = "autoload -U +X bashcompinit && bashcompinit\n" + script
    return script
