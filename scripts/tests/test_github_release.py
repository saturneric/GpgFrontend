#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""A stable release as GitHub shows it: the release notes made into a
release body (release_notes.for_github), and the release the signed index
says there should be and the steps to it (github_release).

Run: python3 -m unittest discover -s scripts/tests
"""

import copy
import json
import unittest

import _paths
from gfrelease.domain import github_release as gh
from gfrelease.domain.release_notes import for_github, unwrap
from gfrelease.errors import ReleaseError

FIXTURES = _paths.TESTS / "fixtures"
CONTRACT = FIXTURES / "contract" / "full"


class ReleaseNotesTest(unittest.TestCase):

    def test_the_2_2_5_notes_make_the_body_held_in_the_fixtures(self):
        notes = (FIXTURES / "github" / "2.2.5.notes.md").read_text("utf-8")
        body = (FIXTURES / "github" / "2.2.5.body.md").read_text("utf-8")
        self.assertEqual(for_github(notes, "2.2.5"), body)

    def test_the_heading_naming_the_version_is_left_out(self):
        self.assertEqual(for_github("## 2.2.5 (2026-11-16)\n\nText.\n",
                                    "2.2.5"), "Text.\n")
        self.assertEqual(for_github("# v2.2.5\nText.\n", "2.2.5"), "Text.\n")
        # Another heading is the notes' own.
        self.assertEqual(for_github("## Security\n\nText.\n", "2.2.5"),
                         "## Security\n\nText.\n")
        self.assertEqual(for_github("Text.\n", "2.2.5"), "Text.\n")

    def test_line_endings_become_lf(self):
        self.assertEqual(for_github("## 1.0.0\r\n\r\nOne\r\ntwo.\r\n",
                                    "1.0.0"), "One two.\n")

    def test_paragraphs_and_list_items_are_put_on_one_line(self):
        self.assertEqual(unwrap("One\ntwo.\n\n- a\n  b\n- c\n  d\n"),
                         "One two.\n\n- a b\n- c d\n")
        self.assertEqual(unwrap("1. a\n   b\n2) c\n"), "1. a b\n2) c\n")
        self.assertEqual(unwrap("- a\n  - nested\n    more\n"),
                         "- a\n  - nested more\n")

    def test_what_is_not_running_text_is_kept(self):
        for text in ("```\nkeep\nthese\n```\n",
                     "~~~sh\nkeep\n```\nthese\n~~~\n",
                     "| a | b |\n|---|---|\n| 1 | 2 |\n",
                     "> quoted\n> lines\n",
                     "<details>\n<summary>x</summary>\n</details>\n",
                     "Text\n\n---\n\nmore\n",
                     "    indented\n    code\n"):
            self.assertEqual(unwrap(text), text)

    def test_a_heading_ends_a_paragraph(self):
        self.assertEqual(unwrap("Text\n### Heading\nmore\nlines\n"),
                         "Text\n### Heading\nmore lines\n")

    def test_a_hard_break_is_kept(self):
        self.assertEqual(unwrap("One  \ntwo\nthree\n"), "One  \ntwo three\n")
        self.assertEqual(unwrap("One\\\ntwo\n"), "One\\\ntwo\n")


def fixture():
    index = json.loads((CONTRACT / "update/v1/index.json").read_text())
    entry = index["releases"]["2.2.5"]
    descriptor = json.loads(
        (CONTRACT / "builds/releases/2.2.5/release.json").read_text())
    notes = (CONTRACT / "builds" / entry["notes"]).read_text("utf-8")
    small = {a["sig"]: ("1" * 64, 147) for a in descriptor["artifacts"]}
    return index, entry, descriptor, notes, small


def want(index=None, descriptor=None):
    i, entry, d, notes, small = fixture()
    return gh.desired(index or i, descriptor or d, entry, notes, small)


def github_has(release, *, draft=False, skip=(), body=None):
    """What GitHub would answer for @p release, fully uploaded."""
    return {"name": release.name, "draft": draft,
            "body": release.body if body is None else body,
            "assets": [{"name": a.name, "size": a.size, "state": "uploaded",
                        "digest": f"sha256:{a.sha256}"}
                       for a in release.assets if a.name not in skip]}


class DesiredTest(unittest.TestCase):

    def test_only_what_the_pipeline_released_is_mirrored(self):
        self.assertTrue(gh.mirrored({"kind": "stable"}))
        self.assertFalse(gh.mirrored({"kind": "imported"}))
        self.assertFalse(gh.mirrored({"kind": "nightly"}))
        for version in ("2.1.9", "2.2.5"):
            d = json.loads((CONTRACT / "builds/releases" / version /
                            "release.json").read_text())
            self.assertTrue(gh.mirrored(d))

    def test_every_file_with_its_signature_and_nothing_else(self):
        index, entry, d, notes, small = fixture()
        r = want()
        self.assertEqual(r.tag, "v2.2.5")
        self.assertEqual(r.name, "v2.2.5")
        self.assertEqual(r.commit, d["commit"])
        names = [a.name for a in r.assets]
        for a in d["artifacts"]:
            self.assertIn(a["name"], names)
            self.assertIn(a["name"] + ".sig", names)
        self.assertNotIn("SHA256SUMS.txt", names)
        self.assertEqual(len(names), 2 * len(d["artifacts"]))
        by_name = {a.name: a for a in r.assets}
        first = d["artifacts"][0]
        self.assertEqual(by_name[first["name"]],
                         gh.Asset(first["name"], first["path"],
                                  first["sha256"], first["size"]))
        self.assertEqual(by_name[first["name"] + ".sig"].sha256, "1" * 64)

    def test_the_body_is_the_notes_alone(self):
        index, entry, d, notes, small = fixture()
        self.assertEqual(want().body, for_github(notes, "2.2.5"))

    def test_only_the_stable_channels_release_is_the_latest(self):
        index, *_ = fixture()
        self.assertTrue(want(index).latest)
        older = copy.deepcopy(index)
        older["channels"]["stable"] = "2.2.6"
        self.assertFalse(want(older).latest)

    def test_an_older_series_release_is_never_the_latest(self):
        # 2.1.9 of the fixtures: an older series' release, whenever it is
        # published, never takes the badge from the stable channel's.
        index, entry, d, notes, small = fixture()
        old = json.loads((CONTRACT / "builds/releases/2.1.9/release.json")
                         .read_text())
        old_entry = dict(index["releases"]["2.1.9"], **{
            k: entry[k] for k in ("notes", "notes_sha256", "notes_sig")})
        small = {a["sig"]: ("1" * 64, 147) for a in old["artifacts"]}
        r = gh.desired(index, old, old_entry, notes, small)
        self.assertEqual(index["channels"]["stable"], "2.2.5")
        self.assertFalse(r.latest)
        # Published as not the latest, and never made it later.
        self.assertEqual(gh.plan(r, None, "v2.2.5")[-1].kind, "publish")
        self.assertEqual(gh.plan(r, github_has(r), "v2.2.5"), [])
        self.assertEqual(gh.plan(r, github_has(r), None), [])

    def test_a_withdrawn_release_says_so_first(self):
        index, *_ = fixture()
        index = copy.deepcopy(index)
        index["withdrawn"] = [{"version": "2.2.5",
                               "reason": "its installer did not start"}]
        index["channels"]["stable"] = "2.1.9"
        r = want(index)
        self.assertTrue(r.body.startswith(
            "> **Withdrawn:** its installer did not start."))
        self.assertFalse(r.latest)

    def test_a_body_github_would_not_take_is_refused(self):
        index, entry, d, notes, small = fixture()
        with self.assertRaisesRegex(ReleaseError, "more than GitHub's"):
            gh.desired(index, d, entry, "x " * gh.BODY_LIMIT, small)


class PlanTest(unittest.TestCase):

    def kinds(self, steps):
        return [(s.kind, s.asset.name if s.asset else None) for s in steps]

    def test_a_new_release_is_made_as_a_draft_filled_and_published(self):
        r = want()
        steps = gh.plan(r, None, "v2.2.4")
        self.assertEqual(self.kinds(steps),
                         [("create", None)] +
                         [("upload", a.name) for a in r.assets] +
                         [("publish", None)])

    def test_a_draft_left_half_uploaded_is_finished(self):
        r = want()
        have = github_has(r, draft=True, skip={r.assets[0].name})
        have["assets"][0]["state"] = "starter"
        self.assertEqual(self.kinds(gh.plan(r, have, "v2.2.4")),
                         [("upload", r.assets[0].name),
                          ("replace", r.assets[1].name),
                          ("publish", None)])

    def test_a_published_release_as_the_index_has_it_needs_nothing(self):
        r = want()
        self.assertEqual(gh.plan(r, github_has(r), "v2.2.5"), [])
        # GitHub's body has CRLF line ends when edited on the web page.
        crlf = github_has(r, body=r.body.replace("\n", "\r\n"))
        self.assertEqual(gh.plan(r, crlf, "v2.2.5"), [])

    def test_a_file_added_after_the_release_is_uploaded(self):
        r = want()
        late = next(a for a in r.assets if a.name.endswith(".msi"))
        self.assertEqual(
            self.kinds(gh.plan(r, github_has(r, skip={late.name}), "v2.2.5")),
            [("upload", late.name)])

    def test_other_files_on_github_are_left_alone(self):
        r = want()
        have = github_has(r)
        have["assets"].append({"name": "extra.txt", "size": 1,
                               "state": "uploaded", "digest": None})
        self.assertEqual(gh.plan(r, have, "v2.2.5"), [])

    def test_a_published_file_with_other_bytes_is_a_conflict(self):
        r = want()
        have = github_has(r)
        have["assets"][0]["digest"] = "sha256:" + "f" * 64
        steps = gh.plan(r, have, "v2.2.4")
        self.assertEqual(self.kinds(steps), [("conflict", r.assets[0].name)])
        self.assertIn("f" * 64, steps[0].detail)
        have = github_has(r)
        have["assets"][1]["size"] += 1
        self.assertEqual(self.kinds(gh.plan(r, have, "v2.2.5")),
                         [("conflict", r.assets[1].name)])

    def test_a_draft_file_with_other_bytes_is_replaced(self):
        r = want()
        have = github_has(r, draft=True)
        have["assets"][0]["digest"] = "sha256:" + "f" * 64
        self.assertEqual(self.kinds(gh.plan(r, have, "v2.2.4")),
                         [("replace", r.assets[0].name), ("publish", None)])

    def test_new_notes_or_a_withdrawal_rewrite_the_body(self):
        r = want()
        have = github_has(r, body="old notes")
        self.assertEqual(self.kinds(gh.plan(r, have, "v2.2.5")),
                         [("edit", None)])

    def test_the_stable_channels_release_is_made_the_latest(self):
        r = want()
        self.assertEqual(self.kinds(gh.plan(r, github_has(r), "v2.1.13")),
                         [("latest", None)])
        index, *_ = fixture()
        index = copy.deepcopy(index)
        index["channels"]["stable"] = "2.2.6"
        self.assertEqual(gh.plan(want(index), github_has(r), "v2.2.6"), [])


if __name__ == "__main__":
    unittest.main()
