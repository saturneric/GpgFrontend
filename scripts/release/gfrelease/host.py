# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The signing host's two steps, each a command: promote, and index
(resign is index --force).

Which step runs when, in what order, how often, and what is done about
the stores besides (pruning, mirroring) is for whoever runs the host: the
Release-Server container. It runs promote, then index, then a read-back
`gfrelease verify` of anything published, and builds the index even when
promotion failed. Each step ends in one of three ways, its exit status:

  DONE     everything asked for was done.
  PARTIAL  it ran to its end, but something was refused for what it is (a
           candidate that does not verify, a release with a bad
           signature). Running again changes nothing until it does.
  FAILED   something could not be done or asked (a store that did not
           answer, a tool that did not finish, a signature that could not
           be made). Worth running again as it is.

What it prints on stdout is read by that caller too: "promoted <version>
(<n> files)" for each build promoted, and "index unchanged" or a line
starting "published serial". Every problem goes to stderr, the first line
naming the first one.
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

import index as index_mod
import promote as promote_mod
from common import Product, ReleaseError, locked
from store import Store

DONE, FAILED, PARTIAL = 0, 1, 3

UNCHANGED = "index unchanged"
PUBLISHED = "published serial"


@dataclass
class Host:
    """What the signing host is configured with (the GFR_* settings)."""
    product: Product
    builds: Store
    state_dir: Path
    staging: Store | None = None
    update: Store | None = None
    git_dir: Path | None = None
    index_key: Path | None = None
    index_pub: list[str] | None = None
    cosign: str = "cosign"
    authenticode_ca: str | None = None

    def dry_run(self) -> dict:
        return index_mod.dry_run(
            self.product, self.builds, self.update,
            state_dir=self.state_dir,
            git_dir=self.git_dir, cosign=self.cosign,
            authenticode_ca=self.authenticode_ca, index_pub=self.index_pub)

    def promote(self) -> int:
        """Every branch's newest staged build that verifies, promoted."""
        return self._step(self._promote)

    def index(self, force: bool = False) -> int:
        """The index built, and published when it says something new (or
        with @p force: a re-sign)."""
        return self._step(lambda problem: self._index(force, problem))

    def _step(self, step) -> int:
        """@p step under the host's lock; prints what was done on stdout
        and every problem on stderr. Returns DONE, PARTIAL or FAILED."""
        problems: list[str] = []
        status = DONE

        def problem(text: str, again: bool) -> None:
            """@p again: running again as things are may help."""
            nonlocal status
            problems.append(text)
            if status != FAILED:
                status = FAILED if again else PARTIAL

        try:
            with locked(self.state_dir):
                step(problem)
        except ReleaseError as e:
            # Not one thing refused among others: the step did not happen.
            problem(str(e), True)
        finally:
            # Whatever ends the step, even an error no step expected: what
            # was refused before it is named.
            for text in problems:
                print(f"gfrelease: {text}", file=sys.stderr)
        return status

    def _promote(self, problem) -> None:
        try:
            done = promote_mod.promote(
                self.product, self.staging, self.builds,
                git_dir=self.git_dir, cosign=self.cosign)
        except promote_mod.PromoteIncomplete as e:
            done = e.done
            problem(str(e), e.passing)
        except (ReleaseError, OSError) as e:
            done = []
            problem(f"promote: {e}", True)
        for d in done:
            print(f"promoted {d['version']} ({len(d['artifacts'])} files)")

    def _index(self, force: bool, problem) -> None:
        try:
            index = index_mod.publish(
                self.product, self.builds, self.update,
                index_key=self.index_key, state_dir=self.state_dir,
                force=force, git_dir=self.git_dir,
                cosign=self.cosign, authenticode_ca=self.authenticode_ca,
                index_pub=self.index_pub)
        except index_mod.IndexIncomplete as e:
            index = e.index
            problem(str(e), e.passing)
        if index is None:
            print(UNCHANGED)
        else:
            channels = index["channels"]
            print(f"{PUBLISHED} {index['serial']}, stable "
                  f"{channels['stable']}, beta {channels['beta']}, nightly "
                  f"{channels['nightly']}, expires {index['expires']}")
