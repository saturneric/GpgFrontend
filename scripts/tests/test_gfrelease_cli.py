#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease's command line itself: help, settings, mistakes, output.

What the commands do is test_gfrelease's and test_trust's; this is how
they are asked for, and how they answer.

Run: python3 -m unittest discover -s scripts/tests
"""

from __future__ import annotations

import argparse
import contextlib
import io
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]

import _paths  # noqa: E402,F401
from gfrelease.cli import settings as settings_mod  # noqa: E402
from gfrelease.cli import topics  # noqa: E402
from gfrelease.cli import ui  # noqa: E402


def load_cli():
    import importlib
    cli = importlib.reload(
        importlib.import_module("gfrelease.cli.main"))
    return cli


class CliTest(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-cli-test-"))
        # No setting from the machine running the tests.
        clean = {k: v for k, v in os.environ.items()
                 if not k.startswith("GFR_")}
        clean.update(XDG_CONFIG_HOME=str(self.tmp / "config"),
                     XDG_CACHE_HOME=str(self.tmp / "cache"),
                     XDG_STATE_HOME=str(self.tmp / "state"))
        patcher = mock.patch.dict(os.environ, clean, clear=True)
        patcher.start()
        self.addCleanup(patcher.stop)
        cwd = os.getcwd()
        os.chdir(self.tmp)
        self.addCleanup(os.chdir, cwd)
        self.addCleanup(ui.configure)
        self.cli = load_cli()

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def run_cli(self, *args):
        out, err = io.StringIO(), io.StringIO()
        status = None
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            try:
                status = self.cli.main(list(args))
            except SystemExit as e:
                status = e.code
        return status, out.getvalue(), err.getvalue()

    def leaves(self, parser, path=()):
        for action in parser._actions:
            if isinstance(action, argparse._SubParsersAction):
                for name, p in action.choices.items():
                    yield from self.leaves(p, (*path, name))
                return
        yield path, parser

    # --- help -----------------------------------------------------------

    def test_every_command_explains_itself(self):
        parser = self.cli.build_parser()
        found = list(self.leaves(parser))
        self.assertGreaterEqual(len(found), 20)
        for path, p in found:
            with self.subTest(" ".join(path)):
                self.assertTrue(p.description, path)
                self.assertTrue(p.usage.startswith("gfrelease "), p.usage)
                text = p.format_help()
                for action in p._actions:
                    if action.option_strings and action.help != \
                            argparse.SUPPRESS:
                        self.assertTrue(action.help, action.option_strings)
                self.assertIn("--json", text)

    def test_the_command_list_is_by_who_runs_what(self):
        status, out, _ = self.run_cli("--help")
        self.assertEqual(status, 0)
        for group in ("Anyone", "Maintainer (offline)", "CI",
                      "Signing host", "Setup"):
            self.assertIn(group, out)
        # The trust commands are under trust only.
        parser = self.cli.build_parser()
        args = parser.parse_args(["trust", "keys", "renew", "-s", "g.key"])
        self.assertEqual(args.key, "trust keys renew")
        for old in (["root", "verify"], ["index-keys", "renew"]):
            with self.assertRaises(SystemExit), \
                    contextlib.redirect_stderr(io.StringIO()):
                parser.parse_args(old)

    def test_each_option_names_its_variable_and_where_it_is_set(self):
        os.environ["GFR_BUILDS"] = "r2:fromenv"
        cli = load_cli()
        text = cli.build_parser(settings_mod.Settings()).parse_known_args(
            ["promote"])[0].parser.format_help()
        self.assertIn("env GFR_STAGING", text)
        self.assertIn("now r2:fromenv, from GFR_BUILDS", " ".join(
            text.split()))

    def test_global_options_go_before_or_after_the_command(self):
        parser = self.cli.build_parser()
        before = parser.parse_args(["--product", "p.json", "verify"])
        after = parser.parse_args(["verify", "--product", "p.json"])
        self.assertEqual((before.product, after.product),
                         ("p.json", "p.json"))
        self.assertTrue(parser.parse_args(["show", "--json"]).json)
        self.assertTrue(parser.parse_args(["--json", "show"]).json)

    def test_help_topics_and_commands(self):
        for topic in topics.TOPICS:
            with self.subTest(topic):
                self.assertTrue(topics.text(topic).strip())
                status, out, _ = self.run_cli("help", topic)
                self.assertEqual(status, 0)
                self.assertTrue(out.strip())
        status, out, _ = self.run_cli("help")
        self.assertIn("root-ceremony", out)
        status, out, _ = self.run_cli("help", "verify")
        self.assertIn("usage: gfrelease verify", out)

    def test_the_version_is_the_tooling_commit(self):
        status, out, _ = self.run_cli("--version")
        self.assertEqual(status, 0)
        self.assertRegex(out, r"^gfrelease \S+")

    # --- mistakes -------------------------------------------------------

    def test_a_mistyped_command_gets_a_suggestion(self):
        status, _, err = self.run_cli("verfy")
        self.assertEqual(status, 2)
        self.assertIn("did you mean `verify`", err)
        status, _, err = self.run_cli("trust", "root", "stauts")
        self.assertIn("did you mean `status`", err)

    def test_everything_missing_is_named_at_once(self):
        status, _, err = self.run_cli("index", "--builds", "b",
                                      "--state-dir", "s")
        self.assertEqual(status, 1)
        first = err.splitlines()[0]
        self.assertEqual(first, "error: missing --update, --index-key, "
                         "--index-pub")
        self.assertIn("GFR_INDEX_KEY, or [host] index_key in gfrelease.toml",
                      err)
        self.assertIn("usage: gfrelease index [options]", err)
        self.assertIn("hint: set them once in gfrelease.toml", err)
        # A dry run signs nothing: no key needed.
        _, _, err = self.run_cli("index", "--dry-run", "--builds", "b",
                                 "--state-dir", "s")
        self.assertNotIn("--index-key", err.splitlines()[0])

    def test_hints_say_how_to_fix_it(self):
        self.assertIn("winget install jedisct1.minisign" if os.name == "nt"
                      else "apt install minisign",
                      ui.hint("minisign is not installed"))
        self.assertIn("cosign", ui.hint("cosign is not installed"))
        self.assertIn("another gfrelease command" if os.name == "nt"
                      else "fuser", ui.hint("another run holds /s/lock"))
        self.assertIn("rclone listremotes", ui.hint(
            "listing r2:x: didn't find section in config file"))
        self.assertIsNone(ui.hint("forged"))

    # --- settings -------------------------------------------------------

    def write_config(self, text, name="gfrelease.toml"):
        path = self.tmp / name
        path.write_text(text)
        return path

    def test_settings_come_from_flag_env_config_default_in_that_order(self):
        self.write_config('[stores]\nbuilds = "r2:config"\n'
                          'staging = "r2:config"\n'
                          '[host]\nstate_dir = "~/somewhere"\n')
        os.environ["GFR_STAGING"] = "r2:env"
        parser = self.cli.build_parser()
        args = parser.parse_args(["promote", "--update", "r2:flag"])
        sources = settings_mod.Settings().fill(
            args, ["builds", "staging", "update", "state_dir", "git_dir",
                   "index_key"])
        self.assertEqual((args.update, sources["update"]),
                         ("r2:flag", "command line"))
        self.assertEqual((args.staging, sources["staging"]),
                         ("r2:env", "GFR_STAGING"))
        self.assertEqual(args.builds, "r2:config")
        self.assertTrue(sources["builds"].endswith("gfrelease.toml"))
        self.assertEqual(args.state_dir, str(Path.home() / "somewhere"))
        self.assertEqual(sources["git_dir"], "default")
        self.assertIsNone(args.index_key)

        status, out, _ = self.run_cli("config", "show", "--json")
        rows = json.loads(out)["settings"]
        self.assertEqual(rows["staging"], {"value": "r2:env",
                                           "from": "GFR_STAGING"})
        self.assertEqual(rows["update"]["value"], None)

    def test_a_config_file_says_what_it_does_not_know(self):
        self.write_config('[stores]\nbuild = "x"\n[nope]\na = "b"\n')
        status, _, err = self.run_cli("config", "show")
        self.assertEqual(status, 1)
        self.assertIn("[stores] build is not a setting (did you mean "
                      "builds?)", err)
        self.assertIn("[nope] is not a group", err)
        os.environ["GFR_CONFIG"] = str(self.tmp / "missing.toml")
        status, _, err = self.run_cli("config", "show")
        self.assertIn("GFR_CONFIG names", err)

    def test_config_init_writes_a_template_once(self):
        status, out, _ = self.run_cli("config", "init")
        self.assertEqual(status, 0)
        path = self.tmp / "config" / "gfrelease" / "config.toml"
        self.assertTrue(path.is_file())
        self.assertIn("next: fill it in", out)
        settings_mod.load_config(path)  # a template that loads as it is
        status, _, err = self.run_cli("config", "init")
        self.assertEqual(status, 1)
        self.assertIn("exists", err)
        self.assertEqual(self.run_cli("config", "init", "--here")[0], 0)
        self.assertTrue((self.tmp / "gfrelease.toml").is_file())

    # --- output ---------------------------------------------------------

    def test_quiet_leaves_only_errors(self):
        status, out, err = self.run_cli("-q", "help")
        self.assertEqual((status, out, err), (0, "", ""))

    def test_verbose_shows_each_tool_run(self):
        if not shutil.which("git"):
            self.skipTest("needs git")
        status, _, err = self.run_cli("-v", "trust", "root", "status")
        self.assertEqual(status, 0, err)
        self.assertRegex(err, r"(?m)^\+ git -C ")

    def test_json_is_one_document(self):
        status, out, _ = self.run_cli("--json", "trust", "root", "verify")
        self.assertEqual(status, 0)
        doc = json.loads(out)
        self.assertEqual(set(doc), {"root", "index_keys"})
        self.assertIn("days_left", doc["root"])

    def test_completion_offers_commands_and_options(self):
        status, out, _ = self.run_cli("completion", "bash")
        self.assertEqual(status, 0)
        # (On Windows a "bash" may be WSL's, which is another machine.)
        if shutil.which("bash") and os.name != "nt":
            words = subprocess.run(
                ["bash", "-c", out + '\nCOMP_WORDS=(gfrelease trust root '
                 's); COMP_CWORD=3; _gfrelease; echo "${COMPREPLY[@]}"'],
                capture_output=True, text=True, check=True).stdout.split()
            self.assertEqual(words, ["sign", "status"])
        self.assertNotIn(" index-keys", out.split('""')[1].split(";;")[0])
        self.assertIn("bashcompinit", self.run_cli("completion", "zsh")[1])


if __name__ == "__main__":
    unittest.main()
