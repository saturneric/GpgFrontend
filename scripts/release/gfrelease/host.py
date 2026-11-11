# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""A round of the signing host: promote, then index, then read back.

The steps are independent on purpose. Whatever stops promotion, the index
is still built (and kept from expiring) from what is already published; and
whatever stops the index, every candidate that could not be promoted is
still named. A round ends in one of three ways, which is all its caller
(the Release-Server container) needs to know:

  DONE     everything asked for was done.
  PARTIAL  the round ran to its end, but something was refused for what it
           is (a candidate that does not verify, a release with a bad
           signature). Running again changes nothing until it does.
  FAILED   something could not be done or asked (a store that did not
           answer, a tool that did not finish, a signature that could not
           be made). Worth running again as it is.

What a round prints on stdout is read by that caller too: "index unchanged",
or a line starting "published serial".
"""

from __future__ import annotations

import sys
from dataclasses import dataclass, field
from pathlib import Path

import index as index_mod
import promote as promote_mod
import verify as verify_mod
from common import Product, ReleaseError, locked, run
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
    mirrors: list[Store] = field(default_factory=list)
    git_dir: Path | None = None
    index_key: Path | None = None
    index_pub: list[str] | None = None
    cosign: str = "cosign"
    authenticode_ca: str | None = None

    def fetch(self) -> None:
        """The mirror brought up to date: the version promote derives and
        the notes and tags index admits are all read from it."""
        run(["git", "-C", self.git_dir, "fetch", "--quiet", "--prune",
             "--tags", "origin"])

    def dry_run(self) -> dict:
        return index_mod.dry_run(
            self.product, self.builds, self.update,
            state_dir=self.state_dir,
            git_dir=self.git_dir, cosign=self.cosign,
            authenticode_ca=self.authenticode_ca, index_pub=self.index_pub)

    def round(self, *, promote: bool, index: bool, force: bool = False,
              fetch: bool = False) -> int:
        """Runs the steps asked for under the host's lock; prints what was
        done on stdout and every problem on stderr. Returns DONE, PARTIAL
        or FAILED."""
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
                if fetch:
                    self.fetch()
                if promote:
                    self._promote(problem)
                if index:
                    self._index(force, problem)
        except ReleaseError as e:
            # Not one thing refused among others: the step did not happen.
            problem(str(e), True)
        finally:
            # Whatever ends the round, even an error no step expected: what
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
        except ReleaseError as e:
            # Whatever stopped promotion, the index is still built (and
            # kept from expiring) from what is already there.
            done = []
            problem(f"promote: {e}", True)
        for d in done:
            print(f"promoted {d['version']} ({len(d['artifacts'])} files)")

    def _index(self, force: bool, problem) -> None:
        try:
            index = index_mod.publish(
                self.product, self.builds, self.update,
                index_key=self.index_key, state_dir=self.state_dir,
                force=force, mirrors=self.mirrors, git_dir=self.git_dir,
                cosign=self.cosign, authenticode_ca=self.authenticode_ca,
                index_pub=self.index_pub)
        except index_mod.IndexIncomplete as e:
            index = e.index
            problem(str(e), e.passing)
        if index is None:
            print(UNCHANGED)
        else:
            # Read back what was published, as a client would see it.
            verify_mod.verify(self.product, self.update, self.builds,
                              index_keys=self.index_pub)
            channels = index["channels"]
            print(f"{PUBLISHED} {index['serial']}, stable "
                  f"{channels['stable']}, beta {channels['beta']}, nightly "
                  f"{channels['nightly']}, expires {index['expires']}")
