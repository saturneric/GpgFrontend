# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease help TOPIC: the walkthroughs, from the code they describe.

Most topics are the docstring of the module that does the work, so what
`help` says and what the code does cannot drift apart; the ceremonies,
which span several commands and people, are written out here.
"""

from __future__ import annotations

from .. import __doc__ as ARCHITECTURE
from ..app import doctor, stable, trust, views
from ..domain import layout
from . import host, settings, ui

ROOT_CEREMONY = """\
A new root: renewing one before it expires, or changing the root keys, the
index_list key (G) or the host policy. Offline, with the root keys.

  1. gfrelease trust root new
       writes release/root/<N+1>.root.json: the newest root, one version
       up, expiring in two years. Edit it by hand for anything else that
       changes (keys, thresholds, host_policy).
  2. gfrelease trust root sign release/root/<N+1>.root.json -s <root key>
       once per root key holder: as many of root N's keys as its
       threshold, and as many of root N+1's own.
  3. gfrelease trust root status
       who has signed, against both thresholds, and who is missing.
  4. gfrelease trust root verify
       walks the chain to the new root and checks the index-keys list.
  5. commit release/root/ and push to main: the signing host takes the
       new root on its next round (gfrelease trust update), and publishes
       it beside the next index.

""" + trust.__doc__

ROTATE_INDEX_KEY = """\
The index keys: the minisign keys that may sign v1/index.json, listed in
release/index-keys/<V>.json and signed by the root's G.

  Renew (same keys, a year longer; before the list expires):
    gfrelease trust keys renew -s <G's secret key>
    commit release/index-keys/<V+1>.json and its .minisig

  Rotate (a new index key):
    gfrelease trust keys rotate --secret-out <path outside the checkout> \\
      -s <G's secret key> [--keep-old]
    With --keep-old the list holds the old and the new key: commit it,
    install the new secret on the host as index.key whenever convenient,
    and later rotate again without --keep-old. Without it, commit the list
    and install the new secret together: until both are done the host
    refuses to sign (the old index stays valid until it expires).

G's secret key is read once from the path given, so -s <(bw get notes ...)
works and it never touches the disk.
"""

EXIT_CODES = """\
  0  everything was done
  1  something could not be done or asked (a store that did not answer, a
     tool that did not finish, a setting missing): worth running again as
     it is, once fixed; also index --dry-run when it would refuse
     something, and doctor when a check failed
  2  the command line itself is wrong (an unknown command or option)
  3  promote, index and resign: the step ran to its end but refused
     something for what it is (named on stderr; refused again until it
     changes). trust update: something to look at (a next root or list
     that does not hold, an expiry near)
"""

TOPICS = {
    "stable": ("the offline stable release, step by step", stable.__doc__),
    "root-ceremony": ("a new trust root, signed by the root key holders",
                      ROOT_CEREMONY),
    "rotate-index-key": ("renewing or replacing the index keys",
                         ROTATE_INDEX_KEY),
    "host": ("what the signing host runs, and how it reads the results",
             host.__doc__),
    "layout": ("where everything lives in the stores", layout.__doc__),
    "config": ("settings: options, GFR_* variables, gfrelease.toml",
               settings.__doc__),
    "output": ("what goes to stdout and stderr; --json, -v, -q", ui.__doc__),
    "views": ("show and status", views.__doc__),
    "doctor": ("checking a machine before it does its part",
               doctor.__doc__),
    "exit-codes": ("what each exit status means", EXIT_CODES),
    "architecture": ("how the tooling is built: its layers and modules",
                     ARCHITECTURE),
}


def listing() -> list[str]:
    width = max(map(len, TOPICS))
    return ["Topics (gfrelease help TOPIC):"] + [
        f"  {name:<{width}}  {summary}"
        for name, (summary, _) in TOPICS.items()]


def text(topic: str) -> str:
    return TOPICS[topic][1].strip("\n")
