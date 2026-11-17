#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The contract fixtures (contract.py) are what gfrelease publishes today,
byte for byte, and read back as a client reads them. The update check's
ContractTest (modules/src/m_ver_check/test) reads the same files: a change
to what is published fails here first, and the fixtures written again
(python3 scripts/tests/contract.py) carry it to the other side.

Run: python3 -m unittest discover -s scripts/tests
"""

import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import _paths  # noqa: F401
import contract
from gfrelease.adapters import minisig
from gfrelease.app import verify as verify_mod
from gfrelease.domain import descriptor, layout, limits
from gfrelease.domain.formats import canonical_json, iso, sha256_bytes
from trustutil import WINDOWS, tools

TOOLS = tools("minisign")


@unittest.skipUnless(TOOLS, "needs minisign")
class ContractTest(unittest.TestCase):

    def case(self, name: str) -> Path:
        return contract.FIXTURE / name

    @unittest.skipIf(WINDOWS, "minisign takes the key through a pipe")
    def test_the_fixtures_are_what_gfrelease_publishes(self):
        with tempfile.TemporaryDirectory(prefix="gfr-contract-") as tmp:
            contract.write(Path(tmp))
            made = contract.files(Path(tmp))
        have = contract.files(contract.FIXTURE)
        self.assertEqual(sorted(have), sorted(made),
                         "run python3 scripts/tests/contract.py")
        for rel in made:
            self.assertEqual(have[rel], made[rel],
                             f"{rel}: run python3 scripts/tests/contract.py")

    def test_the_limits_are_the_tools_own(self):
        self.assertEqual(
            json.loads((contract.FIXTURE / "limits.json").read_bytes()),
            limits.as_json())

    def test_each_index_reads_as_a_client_reads_it(self):
        keys = json.loads((contract.TRUST / "index-keys" /
                           layout.index_keys_name(contract.INDEX_KEYS_VERSION)
                           ).read_bytes())["keys"]
        product = contract.product()
        later = iso(contract.NOW + limits.INDEX_LIFETIME / 2)
        for name in contract.CASES:
            with self.subTest(name):
                update = self.case(name) / "update"
                data = (update / layout.INDEX).read_bytes()
                sig = (update / (layout.INDEX + layout.MINISIG)).read_bytes()
                comment = minisig.verify_bytes(list(keys.values()), data, sig)
                index = json.loads(data)
                with mock.patch.dict(os.environ, {"GFR_NOW": later}):
                    verify_mod.check_index(product, index, comment)
                self.assertEqual(index["root_version"],
                                 contract.ROOT_VERSION)

    def test_every_descriptor_and_notes_the_index_names_is_there(self):
        for name in contract.CASES:
            builds = self.case(name) / "builds"
            index = json.loads(
                (self.case(name) / "update" / layout.INDEX).read_bytes())
            named = [b for b in (index.get("nightly"), index.get("beta"),
                                 index.get("prerelease")) if b] + \
                [s[k] for s in index["series"].values()
                 for k in ("beta", "rc") if s.get(k)] + \
                list(index["releases"].values())
            for entry in named:
                with self.subTest(name=name, entry=entry["descriptor"]):
                    raw = (builds / entry["descriptor"]).read_bytes()
                    self.assertEqual(sha256_bytes(raw), entry["sha256"])
                    d = json.loads(raw)
                    descriptor.check_shape(d, entry["descriptor"])
                    self.assertEqual(canonical_json(d), raw)
                    if "notes" in entry:
                        notes = (builds / entry["notes"]).read_bytes()
                        self.assertEqual(sha256_bytes(notes),
                                         entry["notes_sha256"])


if __name__ == "__main__":
    unittest.main()
