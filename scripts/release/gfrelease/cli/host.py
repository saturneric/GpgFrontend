# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The signing host's two steps, each a command: promote, and index
(`index --force` re-signs). promote is everything that brings something
from the staging store to the builds store, which only the host writes:
the branch builds CI staged, and the releases and notes a maintainer
handed over. The steps are app/promote.py's and app/index.py's; what they
say to the host is this module's.

Which step runs when, how often, and what is done about the stores besides
(pruning, mirroring) is for whoever runs the host: the Release-Server
container. Each step ends in one of three ways, its exit status:

  DONE     everything asked for was done.
  PARTIAL  it ran to its end, but something was refused for what it is (a
           candidate that does not verify, a release with a bad
           signature). Running again changes nothing until it does.
  FAILED   something could not be done or asked (a store that did not
           answer, a tool that did not finish, a signature that could not
           be made). Worth running again as it is.

On stdout each step prints one JSON document: promoted and received (each
{version, files}), published ({serial, channels, expires}, or null),
unchanged, problems and status ("done", "partial" or "failed"). Every
problem also goes to stderr, each starting "error:", the first naming the
first one.
"""

from __future__ import annotations

import sys
from dataclasses import dataclass, field
from pathlib import Path

from ..adapters.files import locked
from ..adapters.store import Store
from ..app import index as index_mod
from ..app import promote as promote_mod
from ..app.product import Product
from ..domain import index_model
from ..errors import ReleaseError

DONE, FAILED, PARTIAL = 0, 1, 3
STATUS = {DONE: "done", PARTIAL: "partial", FAILED: "failed"}


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
    # What the last step did: promoted, received, published (or
    # unchanged) and the problems.
    report: dict = field(default_factory=dict)

    def promote(self) -> int:
        """Every branch's newest staged build that verifies, promoted."""
        return self._step(self._promote)

    def index(self, force: bool = False) -> int:
        """The index built, and published when it says something new (or
        with @p force: a re-sign)."""
        return self._step(lambda problem: self._index(force, problem))

    def _step(self, step) -> int:
        """@p step under the host's lock, every problem printed on stderr.
        Returns DONE, PARTIAL or FAILED."""
        problems: list[str] = []
        status = DONE
        self.report = {"promoted": [], "received": [], "published": None,
                       "unchanged": None, "problems": problems}

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
                print(f"error: {text}", file=sys.stderr)
        self.report["status"] = STATUS[status]
        return status

    def _promote(self, problem) -> None:
        # The branch builds, then what a maintainer handed over: received
        # whatever became of the builds.
        for name, what, step in (
                ("promote", "promoted", lambda: promote_mod.promote(
                    self.product, self.staging, self.builds,
                    git_dir=self.git_dir, cosign=self.cosign)),
                ("receive", "received", lambda: promote_mod.receive(
                    self.product, self.staging, self.builds,
                    git_dir=self.git_dir, state_dir=self.state_dir,
                    cosign=self.cosign,
                    authenticode_ca=self.authenticode_ca))):
            try:
                done = step()
            except promote_mod.PromoteIncomplete as e:  # ReceiveIncomplete too
                done = e.done
                problem(str(e), e.passing)
            except (ReleaseError, OSError) as e:
                done = []
                problem(f"{name}: {e}", True)
            self.report[what] += [
                {"version": d["version"], "files": len(d["artifacts"])}
                for d in done]

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
        self.report["unchanged"] = index is None
        if index is not None:
            self.report["published"] = {
                "serial": index["serial"],
                "channels": index_model.summary(index),
                "expires": index["expires"]}
