# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease's command line itself: the commands, settings, output.

What the commands do is test_gfrelease's and test_trust's; this is how
they are asked for, and how they answer.

Run: python3 -m unittest discover -s scripts/tests
"""

from __future__ import annotations

import argparse
import contextlib
import io
import os
import shutil
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import _paths  # noqa: E402,F401
from gfrelease import report  # noqa: E402
from gfrelease.cli import parser as parser_mod  # noqa: E402
from gfrelease.cli import settings as settings_mod  # noqa: E402
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

    # --- the commands ---------------------------------------------------

    def test_the_commands_are_these(self):
        found = {" ".join(path): p for path, p in
                 self.leaves(self.cli.build_parser())}
        self.assertEqual(set(found), {
            "stable", "trust root new", "trust root sign",
            "trust root verify", "trust keys renew", "trust update",
            "policy", "candidate stage", "ci build-mode", "ci check-order",
            "stage", "github sync", "promote", "index", "verify"})
        for name, p in found.items():
            with self.subTest(name):
                self.assertEqual(p.get_default("name"), name)
                self.assertTrue(p.description)
                for action in p._actions:
                    if action.option_strings:
                        self.assertTrue(action.help, action.option_strings)

    def test_each_setting_names_its_variable(self):
        parser = self.cli.build_parser()
        args = parser.parse_args(["promote"])
        self.assertIsNone(args.staging)
        help_text = " ".join(parser.format_help().split())
        self.assertIn("[GFR_ROOT_SHA256]", help_text)
        # Every setting has its option, and every option of a setting is
        # filled from the environment and the config file.
        self.assertEqual(set(settings_mod.KNOWN), set(parser_mod.SETTINGS))

    def test_the_version_is_the_tooling_commit(self):
        status, out, _ = self.run_cli("--version")
        self.assertEqual(status, 0)
        self.assertRegex(out, r"^gfrelease \S+")

    def test_everything_missing_is_named_at_once(self):
        status, _, err = self.run_cli("index", "--builds", "b",
                                      "--state-dir", "s")
        self.assertEqual(status, 1)
        self.assertEqual(err.splitlines()[0],
                         "error: missing --update (or GFR_UPDATE), "
                         "--index-key (or GFR_INDEX_KEY), --index-pub (or "
                         "GFR_INDEX_PUB)")

    def test_github_sync_writes_only_with_a_token_from_the_environment(self):
        status, _, err = self.run_cli("github", "sync")
        self.assertEqual(status, 1)
        self.assertIn("GITHUB_TOKEN is not set", err)
        parser = self.cli.build_parser()
        args = parser.parse_args(["github", "sync", "2.2.5", "--dry-run"])
        self.assertEqual((args.name, args.versions, args.dry_run),
                         ("github sync", ["2.2.5"], True))
        with self.assertRaises(SystemExit), \
                contextlib.redirect_stderr(io.StringIO()):
            parser.parse_args(["github", "sync", "--token", "t"])

    def test_rotating_needs_somewhere_for_the_secret(self):
        status, _, err = self.run_cli("trust", "keys", "renew", "-s", "g",
                                      "--rotate")
        self.assertEqual(status, 1)
        self.assertIn("--rotate needs --secret-out", err)
        status, _, err = self.run_cli("trust", "keys", "renew", "-s", "g",
                                      "--keep-old")
        self.assertIn("go with --rotate", err)

    # --- settings -------------------------------------------------------

    def write_config(self, text, name="gfrelease.toml"):
        path = self.tmp / name
        path.write_text(text)
        return path

    def test_settings_come_from_flag_env_config_default_in_that_order(self):
        self.write_config('builds = "r2:config"\nstaging = "r2:config"\n'
                          '[host]\nstate_dir = "~/somewhere"\n')
        os.environ["GFR_STAGING"] = "r2:env"
        args = self.cli.build_parser().parse_args(
            ["promote", "--update", "r2:flag"])
        settings_mod.fill(args)
        self.assertEqual(args.update, "r2:flag")
        self.assertEqual(args.staging, "r2:env")
        self.assertEqual(args.builds, "r2:config")
        self.assertEqual(args.state_dir, str(Path.home() / "somewhere"))
        self.assertTrue(args.git_dir)
        self.assertIsNone(args.index_key)

    def test_the_user_config_is_read_when_the_directory_has_none(self):
        path = self.tmp / "config" / "gfrelease" / "config.toml"
        path.parent.mkdir(parents=True)
        path.write_text('key = "ABCD"\n')
        args = self.cli.build_parser().parse_args(["stable", "2.3.0"])
        settings_mod.fill(args)
        self.assertEqual(args.key, "ABCD")

    def test_a_config_file_says_what_it_does_not_know(self):
        self.write_config('build = "x"\n')
        status, _, err = self.run_cli("verify")
        self.assertEqual(status, 1)
        self.assertIn("build is not a setting", err)
        self.write_config('builds = true\n')
        status, _, err = self.run_cli("verify")
        self.assertIn("builds must be a string", err)

    # --- output ---------------------------------------------------------

    def test_verbose_shows_each_tool_run(self):
        if not shutil.which("git"):
            self.skipTest("needs git")
        status, _, err = self.run_cli("-v", "--version")
        self.assertEqual(status, 0, err)
        ui.configure(verbose=True)
        out = io.StringIO()
        with contextlib.redirect_stderr(out):
            ui.tool_started(["git", "-C", "x", "status"])
        self.assertEqual(out.getvalue(), "+ git -C x status\n")

    def test_what_a_role_reports_is_shown_as_the_command_line_shows_it(self):
        # The roles report through gfrelease.report, never printing
        # themselves; once the command line is configured, a warning is a
        # "warning:" line on stderr and progress a line on stdout.
        ui.configure()
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            report.warn("two lines\nof it")
            report.info("progress")
        self.assertEqual(err.getvalue(), "warning: two lines\n  of it\n")
        self.assertEqual(out.getvalue(), "progress\n")


if __name__ == "__main__":
    unittest.main()
