# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease: sign, verify and publish the product's releases.

Run it as scripts/gfrelease (or python3 scripts/release/gfrelease).
`gfrelease help` lists the walkthroughs, `gfrelease COMMAND --help` explains
a command; `gfrelease doctor` checks this machine, `gfrelease status` says
where things stand.

The product (slug, update and build bases, artifact rules) comes from
release/product.json, the series and withdrawals from release/policy.toml,
and what is trusted (the build workflow, the maintainers, the index keys)
from the signed root and index-keys list in release/root/ and
release/index-keys/ (trust.py). Settings come from the command line, then
GFR_<OPTION> variables, then gfrelease.toml (gfrelease help config).
"""

from __future__ import annotations

import argparse
import difflib
import os
import re
import sys
import textwrap

from ..adapters.process import run
from ..app import doctor as doctor_mod
from ..app import stable as stable_mod
from ..app import views
from ..errors import ReleaseError
from ..paths import ROOT
from . import settings as settings_mod
from .commands import inspect, publish, release, setup
from .commands import trust as trust_cmd

# What every option says, and what its value looks like.
OPTIONS = {
    "product": ("release/product.json of another product", "FILE"),
    "policy": ("its release/policy.toml (default: beside it)", "FILE"),
    "rclone": ("the rclone binary", "PATH"),
    "root_dir": ("the trust roots (default: release/root/)", "DIR"),
    "index_keys_dir": ("the index-keys lists (default: release/index-keys/)",
                       "DIR"),
    "root_sha256": ("the sha256 root 1 must have (the signing host's "
                    "anchor)", "SHA256"),
    "staging": ("the staging store", "REMOTE"),
    "builds": ("the final artifact store", "REMOTE"),
    "update": ("the index store", "REMOTE"),
    "git_dir": ("a clone of the product (default: this checkout)", "DIR"),
    "state_dir": ("where the serial and the lock live", "DIR"),
    "index_key": ("minisign secret key for the index", "FILE"),
    "index_pub": ("its public key (file or base64)", "KEY"),
    "cosign": ("the cosign binary", "PATH"),
    "authenticode_ca": ("CA file for checking Authenticode (a rehearsal's "
                        "own CA)", "FILE"),
    "work": ("where files and progress are kept", "DIR"),
    "key": ("your signing key's fingerprint (default: the first pinned "
            "maintainer)", "FINGERPRINT"),
    "gnupghome": ("the GnuPG home holding that key (default: yours)", "DIR"),
    "authenticode_cert": ("certificate file, or PKCS#11 URI", "CERT"),
    "authenticode_key": ("key file, or PKCS#11 URI", "KEY"),
    "pkcs11_module": ("the token's PKCS#11 module", "FILE"),
    "authenticode_thumbprint": ("on Windows: the SHA-1 thumbprint of the "
                                "certificate in your store, signed with by "
                                "signtool", "SHA1"),
    "signtool": ("signtool.exe (default: PATH, else the newest Windows "
                 "SDK's)", "PATH"),
    "timestamp_url": ("RFC 3161 time-stamping services, comma-separated, "
                      "tried in turn", "URL[,URL...]"),
    "cafile": ("CA bundle for the HTTPS bases", "FILE"),
    "candidate": ("the verified candidate directory", "DIR"),
    "ref": ("full git ref", "REF"),
    "commit": ("full commit sha", "SHA"),
    "version": ("SemVer", "VERSION"),
    "filesafe": ("file-safe version", "VERSION"),
    "built_at": ("UTC time", "TIME"),
    "min_days_left": ("fail when the index expires sooner than this (a "
                      "monitor's early warning that the signing host "
                      "stopped)", "DAYS"),
}


GLOBAL = ("product", "policy", "rclone", "root_dir", "index_keys_dir",
          "root_sha256")


# The command list of `gfrelease --help`, by who runs what.
GROUPS = (
    ("Anyone", (
        ("verify", "check the published index as a client does"),
        ("show", "the published index: channels, series, releases"),
        ("status", "where a release (or everything) stands, and what next"),
        ("doctor", "check this machine can do its part"),
        ("help", "walkthroughs: stable, root-ceremony, config, ..."))),
    ("Maintainer (offline)", (
        ("stable", "sign and publish a stable release"),
        ("import", "bring over a release made before this pipeline"),
        ("trust root", "new | sign | status | verify: the trust root"),
        ("trust keys", "renew | rotate: the keys that sign the index"))),
    ("CI", (
        ("stage", "upload a release candidate"),
        ("candidate", "stage: gather and check a release candidate"),
        ("ci", "check-order | version: the version a build may have"),
        ("policy", "check | render-security | check-security | mainline"))),
    ("Signing host", (
        ("promote", "promote each branch's newest staged build"),
        ("index", "build and publish the index when it says something new"),
        ("resign", "publish the index again with a new serial and expiry"),
        ("trust update", "take main's next roots and index-keys list"))),
    ("Setup", (
        ("config", "show | init: settings and where they come from"),
        ("completion", "bash | zsh: shell completion"))),
)


OPT_NAMES: set[str] = set()


LEAVES: list = []


class Parser(argparse.ArgumentParser):
    """argparse, whose mistakes say what was probably meant."""

    def error(self, message):
        # (argparse of Python 3.15 puts a suggestion of its own between.)
        m = re.search(r"invalid choice: '?([^'\s]*)'?(?:, maybe you meant"
                      r"[^(]*)? \(choose from (.*)\)", message)
        if m:
            choices = [c.strip(" '") for c in m.group(2).split(",")]
            close = difflib.get_close_matches(m.group(1), choices, n=1)
            if close:
                message += f"\nhint: did you mean `{close[0]}`?"
        self.print_usage(sys.stderr)
        print(f"error: {message}", file=sys.stderr)
        sys.exit(2)


class Version(argparse.Action):
    """--version, worked out only when asked."""

    def __init__(self, option_strings, dest, **kw):
        super().__init__(option_strings, dest, nargs=0,
                         default=argparse.SUPPRESS, **kw)

    def __call__(self, parser, namespace, values, option_string=None):
        print(f"gfrelease {tool_version()}")
        parser.exit()


def tool_version() -> str:
    """The product commit the tooling comes from: COMMIT beside it (the
    signing host's image), else git describe of this checkout."""
    commit = ROOT / "COMMIT"
    if commit.is_file():
        return commit.read_text().strip()
    try:
        return run(["git", "-C", ROOT, "describe", "--always", "--dirty",
                    "--tags"], timeout=10).stdout.strip()
    except ReleaseError:
        return "unknown"


def opt(p, name, settings=None, help=None, repeated=False):
    """--name, unset unless given: its value comes from the environment, the
    config file or the defaults after parsing (settings.Settings.fill).
    @p repeated: a global option on a command, which must not undo what was
    given before the command's name."""
    OPT_NAMES.add(name)
    text, metavar = OPTIONS[name]
    text = help or text
    notes = [f"env GFR_{name.upper()}"]
    if settings is not None:
        value, source = settings.lookup(name)
        if value is not None:
            notes.append(f"now {value}, from "
                         f"{'the defaults' if source == 'default' else source}")
    p.add_argument(f"--{name.replace('_', '-')}", metavar=metavar,
                   default=argparse.SUPPRESS if repeated else None,
                   help=f"{text} [{'; '.join(notes)}]")


def flag(p, name, help):
    env = os.environ.get(f"GFR_{name.upper()}", "").lower()
    p.add_argument(f"--{name.replace('_', '-')}", action="store_true",
                   default=env in ("1", "true", "yes"), help=help)


def add_globals(p, settings, top=False):
    """The options every command takes, before or after its name."""
    g = p.add_argument_group("global options")
    for name in GLOBAL:
        opt(g, name, settings, repeated=not top)
    default = False if top else argparse.SUPPRESS
    g.add_argument("--json", action="store_true", default=default,
                   help="print the result as one JSON document")
    g.add_argument("-v", "--verbose", action="store_true", default=default,
                   help="print each tool run (rclone, git, gpg, ...)")
    g.add_argument("-q", "--quiet", action="store_true", default=default,
                   help="print only errors")


def leaf(sub, name, settings, help, description="", example="",
         key=None, handler=None, product=True):
    """A command: its help, description and an example; what runs it."""
    epilog = f"example:\n  gfrelease {example}" if example else None
    kw = {"help": help} if help else {}
    text = "\n\n".join(textwrap.fill(" ".join(part.split()), 76)
                        for part in (description or help).split("\n\n"))
    p = sub.add_parser(name, description=text, epilog=epilog,
                       prog=f"gfrelease {key or name}",
                       formatter_class=argparse.RawDescriptionHelpFormatter,
                       **kw)
    add_globals(p, settings)
    p.set_defaults(key=key or name, handler=handler, needs_product=product,
                   parser=p)
    LEAVES.append(p)
    return p


def short_usage(p) -> None:
    """gfrelease COMMAND [options] ARGS: the options are listed below it."""
    args = []
    for a in p._actions:
        if a.option_strings:
            continue
        name = a.metavar or a.dest
        if isinstance(a.choices, (list, tuple)) and not a.metavar:
            name = "{" + ",".join(a.choices) + "}"
        args.append(f"[{name}]" if a.nargs == "?" else
                    f"{name} ..." if a.nargs == "+" else name)
    p.usage = " ".join([p.prog, "[options]", *args])


def command_list() -> str:
    width = max(len(n) for _, cmds in GROUPS for n, _ in cmds)
    lines = ["commands:"]
    for group, cmds in GROUPS:
        lines.append(f"  {group}")
        lines += [f"    {n:<{width}}  {h}" for n, h in cmds]
    return "\n".join(lines)


def add_root_commands(sub, settings):
    """trust root new|sign|status|verify."""
    rsub = sub.add_subparsers(dest="action", required=True, metavar="ACTION",
                              parser_class=Parser)
    leaf(rsub, "new", settings, "write the next root: the newest one, one "
         "version up, expiring in two years; edit it by hand for anything "
         "else", example="trust root new", key="trust root new",
         handler=trust_cmd.cmd_root_new)
    p = leaf(rsub, "sign", settings, "add one root key's signature",
             example="trust root sign release/root/4.root.json -s root1.key",
             key="trust root sign", handler=trust_cmd.cmd_root_sign)
    p.add_argument("file", metavar="FILE", help="<N>.root.json")
    p.add_argument("-s", dest="secret", required=True, metavar="SECRET",
                   help="that root key's minisign secret key")
    leaf(rsub, "status", settings, "which root keys signed the newest root, "
         "against both thresholds, and who is missing",
         example="trust root status", key="trust root status",
         handler=trust_cmd.cmd_root_status)
    p = leaf(rsub, "verify", settings, "walk release/root/, check "
             "release/index-keys/, say when each expires",
             example="trust root verify", key="trust root verify",
             handler=trust_cmd.cmd_root_verify)
    p.add_argument("--from", dest="first", type=int, default=1, metavar="N",
                   help="trust this root on its own signatures, walk on")


def add_keys_commands(sub, settings):
    """trust keys renew|rotate."""
    ksub = sub.add_subparsers(dest="action", required=True, metavar="ACTION",
                              parser_class=Parser)
    kr = leaf(ksub, "renew", settings, "the same keys, one version up, "
              "expiring in a year, signed with G",
              example="trust keys renew -s <(bw get notes G)",
              key="trust keys renew", handler=trust_cmd.cmd_keys)
    ko = leaf(ksub, "rotate", settings, "a new index key, listed in the next "
              "version, signed with G",
              example="trust keys rotate --secret-out ~/index.key --keep-old "
              "-s <(bw get notes G)", key="trust keys rotate",
              handler=trust_cmd.cmd_keys)
    ko.add_argument("--secret-out", required=True, metavar="FILE",
                    help="where the new index key's secret goes (never "
                    "inside the repository)")
    ko.add_argument("--keep-old", action="store_true",
                    help="list the current keys too, for a swap at leisure")
    for p in (kr, ko):
        p.add_argument("-s", dest="secret", required=True, metavar="SECRET",
                       help="G's minisign secret key; a path read once, so "
                       "<(bw get notes ...) works")


def build_parser(settings=None) -> argparse.ArgumentParser:
    ap = Parser(prog="gfrelease", description=__doc__, epilog=command_list(),
                usage="gfrelease [options] COMMAND ...",
                formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--version", action=Version,
                    help="the product commit this tooling comes from")
    add_globals(ap, settings, top=True)
    sub = ap.add_subparsers(dest="command", required=True, metavar="COMMAND",
                            help=argparse.SUPPRESS, parser_class=Parser)

    # Anyone.
    p = leaf(sub, "verify", settings, "check the published index as a "
             "client does",
             "Checks the published index the way a client does, and further: "
             "its signature and trust, every descriptor and manifest it "
             "names, and with --deep every artifact.",
             "verify --deep", handler=inspect.cmd_verify)
    opt(p, "update", settings, "the index store (default: the HTTPS bases)")
    opt(p, "builds", settings, "the artifact store (default: the HTTPS "
        "bases)")
    opt(p, "cafile", settings)
    p.add_argument("--deep", action="store_true",
                   help="download every artifact")
    opt(p, "min_days_left", settings)

    p = leaf(sub, "show", settings, "the published index: channels, series, "
             "releases", views.__doc__.split("\n\n")[1], "show",
             handler=inspect.cmd_show)
    opt(p, "update", settings, "the index store (default: the HTTPS bases)")
    opt(p, "cafile", settings)

    p = leaf(sub, "status", settings, "where a release (or everything) "
             "stands, and what to do next", views.__doc__.split("\n\n")[1],
             "status 2.3.0", handler=inspect.cmd_status)
    p.add_argument("release", nargs="?", metavar="VERSION",
                   help="X.Y.Z, or a nightly's or beta's version")
    for name in ("update", "builds", "staging", "work", "git_dir", "cafile"):
        opt(p, name, settings)

    p = leaf(sub, "doctor", settings, "check this machine can do its part",
             doctor_mod.__doc__.split("\n\n")[1], "doctor --for maintainer",
             handler=inspect.cmd_doctor)
    p.add_argument("--for", dest="role", choices=doctor_mod.ROLES,
                   default="maintainer", help="whose part (default: "
                   "maintainer)")
    for name in ("staging", "builds", "update", "git_dir", "cosign", "key",
                 "gnupghome", "authenticode_cert", "authenticode_key",
                 "pkcs11_module", "authenticode_thumbprint", "signtool",
                 "index_key", "index_pub"):
        opt(p, name, settings)

    p = leaf(sub, "help", settings, "walkthroughs: stable, root-ceremony, "
             "config, ...", example="help stable", handler=setup.cmd_help,
             product=False)
    p.add_argument("topic", nargs="?", metavar="TOPIC",
                   help="a topic, or a command")

    # The maintainer's.
    p = leaf(sub, "stable", settings, "sign and publish a stable release",
             "The offline stable stage, on the maintainer's machine "
             "(gfrelease help stable): fetch, verify, authenticode, pgp, "
             "upload. Each step is recorded and skipped once done, so a "
             "stage stopped anywhere resumes where it stopped.",
             "stable 2.3.0", handler=release.cmd_stable)
    p.add_argument("version", metavar="VERSION",
                   help="X.Y.Z, its tag vX.Y.Z already pushed")
    for name in ("work", "staging", "git_dir", "cosign", "key",
                 "gnupghome", "authenticode_cert", "authenticode_key",
                 "pkcs11_module", "authenticode_thumbprint", "signtool",
                 "timestamp_url", "authenticode_ca"):
        opt(p, name, settings, OPTIONS[name][0] + (
            " (read, and write under signed/)" if name == "staging" else ""))
    p.add_argument("--redo", choices=stable_mod.STEPS,
                   help="run this step, and every one after it, again")
    p.add_argument("--notes-only", action="store_true",
                   help="after the release: sign and upload main's notes")
    p.add_argument("--msix", action="store_true",
                   help="after the release: make the MSIX of one handed "
                   "over without it, from its signed MSI, and hand over "
                   "only that")
    p.add_argument("--dry-run", action="store_true",
                   help="fetch and verify only: nothing signed or uploaded")
    p.add_argument("-y", "--yes", action="store_true",
                   help="upload without asking (asked only on a terminal)")

    p = leaf(sub, "import", settings, "bring over a release made before this "
             "pipeline", "Brings a release published on GitHub Releases "
             "before this pipeline over: downloaded, its maintainer "
             "signatures checked, handed to the signing host through the "
             "staging store.",
             "import 2.1.4", handler=release.cmd_import)
    p.add_argument("versions", nargs="+", metavar="VERSION",
                   help="X.Y.Z, each a published GitHub release")
    opt(p, "work", settings, "where the downloads are kept")
    opt(p, "staging", settings, "the staging store (write under signed/); "
        "without it, only download and check")
    opt(p, "git_dir", settings, "a clone with the release tags")

    t = sub.add_parser("trust", help="the trust root, the index keys, and "
                       "the host's copy of them", description="The trust "
                       "root (offline), the index keys, and the signing "
                       "host's copy of them (gfrelease help root-ceremony).")
    add_globals(t, settings)
    tsub = t.add_subparsers(dest="group", required=True, metavar="GROUP",
                            parser_class=Parser)
    r = tsub.add_parser("root", help="new | sign | status | verify")
    add_globals(r, settings)
    add_root_commands(r, settings)
    k = tsub.add_parser("keys", help="renew | rotate")
    add_globals(k, settings)
    add_keys_commands(k, settings)
    p = leaf(tsub, "update", settings, "take main's next roots and "
             "index-keys list into --root-dir and --index-keys-dir, as far "
             "as they hold", example="trust update", key="trust update",
             handler=trust_cmd.cmd_trust_update)
    opt(p, "git_dir", settings, "this product's git mirror (may be bare)")
    opt(p, "state_dir", settings, "where the lock lives")

    # CI's.
    p = leaf(sub, "stage", settings, "upload a release candidate (CI)",
             example="stage --candidate out/ --staging r2:staging --ref "
             "refs/heads/main --commit <sha> --version <v> --filesafe <v> "
             "--built-at <time>", handler=publish.cmd_stage)
    for name in ("candidate", "staging", "ref", "commit", "version",
                 "filesafe", "built_at"):
        opt(p, name, settings)

    # The signing host's.
    for name, help, description in (
            ("promote", "promote each branch's newest staged build",
             "Promotes each branch's newest staged build that verifies: "
             "main's are the nightly channel, a release branch's the beta "
             "of its series (gfrelease help host)."),
            ("index", "build and publish the index when it says something "
             "new", "Builds the index from what is promoted and released, "
             "and publishes it, signed, when it says something new."),
            ("resign", "publish the index again with a new serial and "
             "expiry", "Publishes the index again whatever changed: a new "
             "serial and expiry.")):
        p = leaf(sub, name, settings, help, description, name,
                 handler=publish.cmd_host)
        for o in ("staging", "builds", "update", "git_dir", "index_key",
                  "index_pub", "state_dir", "cosign", "authenticode_ca"):
            opt(p, o, settings)
        if name == "index":
            # resign is index --force.
            flag(p, "force", "publish even if nothing changed")
            flag(p, "dry_run", "decide everything, write nothing: what "
                 "would be listed and refused, against the published index")

    # Setup.
    p = leaf(sub, "config", settings, "show | init: settings and where they "
             "come from", settings_mod.__doc__.split("\n\n")[0],
             "config show", handler=setup.cmd_config, product=False)
    p.add_argument("action", choices=("show", "init"))
    p.add_argument("--here", action="store_true",
                   help="init: ./gfrelease.toml instead of the user's")
    p = leaf(sub, "completion", settings, "bash | zsh: shell completion",
             "Prints a completion script: add `source <(scripts/gfrelease "
             "completion bash)` to ~/.bashrc, or the zsh one to ~/.zshrc.",
             "completion bash", handler=setup.cmd_completion, product=False)
    p.add_argument("shell", choices=("bash", "zsh"))

    for p in LEAVES:
        short_usage(p)
    LEAVES.clear()
    return ap
