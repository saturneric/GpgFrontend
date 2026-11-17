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
import contextlib
import datetime as dt
import difflib
import getpass
import io
import os
import re
import sys
import textwrap
from pathlib import Path

from ..adapters.windows import authenticode  # noqa: E402
from ..app import doctor as doctor_mod  # noqa: E402
from ..app import host as host_mod  # noqa: E402
from ..app import imported as imported_mod  # noqa: E402
from ..domain import layout  # noqa: E402
from ..adapters import minisig  # noqa: E402
from . import settings as settings_mod  # noqa: E402
from ..app import stable as stable_mod  # noqa: E402
from ..app import stage as stage_mod  # noqa: E402
from . import topics  # noqa: E402
from ..app import trust  # noqa: E402
from . import ui  # noqa: E402
from ..app import verify as verify_mod  # noqa: E402
from ..app import candidate as candidate_mod  # noqa: E402
from ..app import check_order as check_order_mod  # noqa: E402
from ..adapters import git as gitversion_mod  # noqa: E402
from ..app import views  # noqa: E402
from ..common import (MINISIG, ROOT, WINDOWS, Product,  # noqa: E402
                    ReleaseError, atomic_write, load_json, locked, parse_iso, policy_mod,
                    refs, run, utcnow)
from ..adapters.store import Store  # noqa: E402

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
# What each command cannot do without, checked before it starts.
REQUIRED = {
    "stage": ("candidate", "staging", "ref", "commit", "version", "filesafe",
              "built_at"),
    "stable": ("work",),
    "import": ("work", "git_dir"),
    "promote": ("builds", "state_dir", "staging", "git_dir"),
    "index": ("builds", "state_dir", "update", "index_key", "index_pub"),
    "resign": ("builds", "state_dir", "update", "index_key", "index_pub"),
    "trust update": ("git_dir", "state_dir", "root_dir", "index_keys_dir"),
}
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
# Commands with a command line of their own, given everything after their
# name: they need no product, settings or store.
FORWARDED = {
    "policy": lambda argv: policy_mod.main(argv),
    "candidate": lambda argv: candidate_mod.main(argv),
    "ci check-order": lambda argv: check_order_mod.main(argv),
    "ci version": lambda argv: gitversion_mod.main(argv),
}
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


# --- building the command line ---------------------------------------------

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
         handler=cmd_root_new)
    p = leaf(rsub, "sign", settings, "add one root key's signature",
             example="trust root sign release/root/4.root.json -s root1.key",
             key="trust root sign", handler=cmd_root_sign)
    p.add_argument("file", metavar="FILE", help="<N>.root.json")
    p.add_argument("-s", dest="secret", required=True, metavar="SECRET",
                   help="that root key's minisign secret key")
    leaf(rsub, "status", settings, "which root keys signed the newest root, "
         "against both thresholds, and who is missing",
         example="trust root status", key="trust root status",
         handler=cmd_root_status)
    p = leaf(rsub, "verify", settings, "walk release/root/, check "
             "release/index-keys/, say when each expires",
             example="trust root verify", key="trust root verify",
             handler=cmd_root_verify)
    p.add_argument("--from", dest="first", type=int, default=1, metavar="N",
                   help="trust this root on its own signatures, walk on")


def add_keys_commands(sub, settings):
    """trust keys renew|rotate."""
    ksub = sub.add_subparsers(dest="action", required=True, metavar="ACTION",
                              parser_class=Parser)
    kr = leaf(ksub, "renew", settings, "the same keys, one version up, "
              "expiring in a year, signed with G",
              example="trust keys renew -s <(bw get notes G)",
              key="trust keys renew", handler=cmd_keys)
    ko = leaf(ksub, "rotate", settings, "a new index key, listed in the next "
              "version, signed with G",
              example="trust keys rotate --secret-out ~/index.key --keep-old "
              "-s <(bw get notes G)", key="trust keys rotate",
              handler=cmd_keys)
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
             "verify --deep", handler=cmd_verify)
    opt(p, "update", settings, "the index store (default: the HTTPS bases)")
    opt(p, "builds", settings, "the artifact store (default: the HTTPS "
        "bases)")
    opt(p, "cafile", settings)
    p.add_argument("--deep", action="store_true",
                   help="download every artifact")
    opt(p, "min_days_left", settings)

    p = leaf(sub, "show", settings, "the published index: channels, series, "
             "releases", views.__doc__.split("\n\n")[1], "show",
             handler=cmd_show)
    opt(p, "update", settings, "the index store (default: the HTTPS bases)")
    opt(p, "cafile", settings)

    p = leaf(sub, "status", settings, "where a release (or everything) "
             "stands, and what to do next", views.__doc__.split("\n\n")[1],
             "status 2.3.0", handler=cmd_status)
    p.add_argument("release", nargs="?", metavar="VERSION",
                   help="X.Y.Z, or a nightly's or beta's version")
    for name in ("update", "builds", "staging", "work", "git_dir", "cafile"):
        opt(p, name, settings)

    p = leaf(sub, "doctor", settings, "check this machine can do its part",
             doctor_mod.__doc__.split("\n\n")[1], "doctor --for maintainer",
             handler=cmd_doctor)
    p.add_argument("--for", dest="role", choices=doctor_mod.ROLES,
                   default="maintainer", help="whose part (default: "
                   "maintainer)")
    for name in ("staging", "builds", "update", "git_dir", "cosign", "key",
                 "gnupghome", "authenticode_cert", "authenticode_key",
                 "pkcs11_module", "authenticode_thumbprint", "signtool",
                 "index_key", "index_pub"):
        opt(p, name, settings)

    p = leaf(sub, "help", settings, "walkthroughs: stable, root-ceremony, "
             "config, ...", example="help stable", handler=cmd_help,
             product=False)
    p.add_argument("topic", nargs="?", metavar="TOPIC",
                   help="a topic, or a command")

    # The maintainer's.
    p = leaf(sub, "stable", settings, "sign and publish a stable release",
             "The offline stable stage, on the maintainer's machine "
             "(gfrelease help stable): fetch, verify, authenticode, pgp, "
             "upload. Each step is recorded and skipped once done, so a "
             "stage stopped anywhere resumes where it stopped.",
             "stable 2.3.0", handler=cmd_stable)
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
             "import 2.1.4", handler=cmd_import)
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
             handler=cmd_trust_update)
    opt(p, "git_dir", settings, "this product's git mirror (may be bare)")
    opt(p, "state_dir", settings, "where the lock lives")

    # CI's.
    p = leaf(sub, "stage", settings, "upload a release candidate (CI)",
             example="stage --candidate out/ --staging r2:staging --ref "
             "refs/heads/main --commit <sha> --version <v> --filesafe <v> "
             "--built-at <time>", handler=cmd_stage)
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
                 handler=cmd_host)
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
             "config show", handler=cmd_config, product=False)
    p.add_argument("action", choices=("show", "init"))
    p.add_argument("--here", action="store_true",
                   help="init: ./gfrelease.toml instead of the user's")
    p = leaf(sub, "completion", settings, "bash | zsh: shell completion",
             "Prints a completion script: add `source <(scripts/gfrelease "
             "completion bash)` to ~/.bashrc, or the zsh one to ~/.zshrc.",
             "completion bash", handler=cmd_completion, product=False)
    p.add_argument("shell", choices=("bash", "zsh"))

    for p in LEAVES:
        short_usage(p)
    LEAVES.clear()
    return ap


# --- running it --------------------------------------------------------------

class Context:
    def __init__(self, args, product, settings):
        self.args, self.product, self.settings = args, product, settings
        self.rclone = args.rclone or "rclone"
        self.result = None  # what --json prints

    def store(self, root):
        return Store(root, self.rclone) if root else None


def product_of(args) -> Product:
    if args.product:
        product = Path(args.product)
        policy = Path(args.policy) if args.policy else \
            product.with_name("policy.toml")
    else:
        product = ROOT / "release" / "product.json"
        policy = product.with_name("policy.toml")
    return Product(product, policy, root_dir=args.root_dir,
                   index_keys_dir=args.index_keys_dir,
                   root_sha256=args.root_sha256)


def check_required(args) -> None:
    names = REQUIRED.get(args.key, ())
    if args.key == "index" and getattr(args, "dry_run", False):
        names = tuple(n for n in names if n != "index_key")
    missing = [n for n in names if not getattr(args, n, None)]
    if missing:
        group = {n: g for g, ns in settings_mod.GROUPS.items() for n in ns}
        raise ReleaseError(
            "missing " + ", ".join(f"--{n.replace('_', '-')}"
                                   for n in missing) + "\n" +
            "\n".join(f"--{n.replace('_', '-')}, GFR_{n.upper()}" +
                      (f", or [{group[n]}] {n} in gfrelease.toml"
                       if n in group else "") for n in missing) +
            "\n" + args.parser.format_usage().strip())


def forwarded(argv: list[str]):
    """The forwarded command @p argv starts with, and the rest of it."""
    for words in (2, 1):
        name = " ".join(argv[:words])
        if len(argv) >= words and name in FORWARDED:
            return FORWARDED[name], argv[words:]
    if argv[:1] == ["ci"]:
        print("usage: gfrelease ci {check-order,version} ...",
              file=sys.stderr)
        return None, None
    return None, argv


def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else list(argv)
    tool, rest = forwarded(argv)
    if tool is not None:
        return tool(rest)
    if rest is None:
        return 2
    try:
        settings = settings_mod.Settings()
    except ReleaseError as e:
        ui.error(str(e))
        return host_mod.FAILED
    args = build_parser(settings).parse_args(argv)
    ui.configure(json_out=args.json, quiet=args.quiet, verbose=args.verbose)
    settings.fill(args, sorted(n for n in OPT_NAMES if hasattr(args, n)))
    out = contextlib.ExitStack()
    try:
        product = product_of(args) if args.needs_product else None
        check_required(args)
        ctx = Context(args, product, settings)
        with out:
            if ui.JSON:
                # One JSON document on stdout: whatever else would print
                # there is left out.
                out.enter_context(contextlib.redirect_stdout(io.StringIO()))
            elif ui.QUIET:
                devnull = out.enter_context(open(os.devnull, "w"))
                out.enter_context(contextlib.redirect_stdout(devnull))
            status = args.handler(ctx)
        if ctx.result is not None:
            ui.emit(ctx.result)
        return status
    except ReleaseError as e:
        ui.error(str(e))
        return host_mod.FAILED
    except (KeyError, TypeError, ValueError, AttributeError, IndexError,
            UnicodeError, OSError) as e:
        # A safety net: input that slipped past the checks, or a file that
        # could not be read or written, is reported, not dumped as a
        # traceback.
        ui.error(f"malformed input ({type(e).__name__}: {e})")
        return host_mod.FAILED


# --- the commands ------------------------------------------------------------

def sources(ctx):
    """The update and builds stores, or the HTTPS bases a client reads."""
    args, product = ctx.args, ctx.product
    bases = product.data["bases"]
    update = getattr(args, "update", None)
    builds = getattr(args, "builds", None)
    cafile = getattr(args, "cafile", None)
    return (Store(update, ctx.rclone) if update else
            verify_mod.HttpSource(bases["update"], product.slug, cafile),
            Store(builds, ctx.rclone) if builds else
            verify_mod.HttpSource(bases["builds"], product.slug, cafile))


def cmd_verify(ctx) -> int:
    args = ctx.args
    if bool(getattr(args, "update", None)) != \
            bool(getattr(args, "builds", None)):
        raise ReleaseError("give both --update and --builds, or neither "
                           "(the HTTPS bases)")
    update, builds = sources(ctx)
    report = verify_mod.verify(ctx.product, update, builds, deep=args.deep)
    ctx.result = report
    left = parse_iso(report["expires"]) - utcnow()
    ui.info(f"index serial {report['serial']} of {report['product']} "
            f"verifies; expires {report['expires']} (in "
            f"{left.total_seconds() / 86400:.1f} days)")
    for line in views.channel_lines(report["channels"]):
        ui.info(line)
    ui.info(f"checked {len(report['checked'])}"
            f"{' deeply' if args.deep else ''}:")
    for c in report["checked"]:
        ui.info(f"  ✓ {c.get('version')} ({c.get('kind')}, "
                f"{c.get('artifacts')} files)")
    if getattr(args, "min_days_left", None) and \
            left < dt.timedelta(days=float(args.min_days_left)):
        raise ReleaseError(f"the index expires at {report['expires']}, in "
                           f"{left}: the signing host has not re-signed it")
    return 0


def cmd_show(ctx) -> int:
    update, _ = sources(ctx)
    view = views.show(ctx.product, update)
    ctx.result = view
    for line in views.render_show(view):
        ui.info(line)
    nxt = views.show_next(view)
    if nxt:
        ui.next_step(nxt)
    return 0


def cmd_status(ctx) -> int:
    args = ctx.args
    update, builds = sources(ctx)
    staging = ctx.store(getattr(args, "staging", None))
    if args.release:
        view = views.status(ctx.product, args.release, update=update,
                            builds=builds, staging=staging, work=args.work,
                            git_dir=args.git_dir)
        lines = views.render_status(view)
    else:
        view = views.overview(ctx.product, update=update, builds=builds,
                              staging=staging)
        lines = views.render_overview(view)
    ctx.result = view
    for line in lines:
        ui.info(line)
    if view.get("next"):
        ui.next_step(view["next"])
    return 0


def cmd_doctor(ctx) -> int:
    checks = doctor_mod.Doctor(ctx.args, ctx.product, ctx.args.role).run()
    ctx.result = {"role": ctx.args.role, "checks": checks}
    for line in doctor_mod.render(checks):
        ui.info(line)
    return 0 if all(c["ok"] for c in checks) else host_mod.FAILED


def cmd_help(ctx) -> int:
    topic = ctx.args.topic
    if not topic:
        for line in topics.listing():
            ui.info(line)
        ui.info("\nCommands: gfrelease --help; one command: gfrelease "
                "COMMAND --help.")
        return 0
    if topic in topics.TOPICS:
        ui.info(topics.text(topic))
        return 0
    # A command: its own --help, or argparse's error (exit 2) for none.
    tool, _ = forwarded(topic.split())
    if tool is not None:
        try:
            return tool(["--help"])
        except SystemExit as e:
            return e.code or 0
    try:
        build_parser(ctx.settings).parse_args([*topic.split(), "--help"])
    except SystemExit as e:
        return e.code or 0
    return 0


def cmd_config(ctx) -> int:
    if ctx.args.action == "init":
        path = Path.cwd() / "gfrelease.toml" if ctx.args.here else \
            settings_mod.default_config_path()
        if path.exists():
            raise ReleaseError(f"{path} exists; edit it, or remove it first")
        path.parent.mkdir(parents=True, exist_ok=True)
        atomic_write(path, settings_mod.TEMPLATE.encode())
        ctx.result = {"written": str(path)}
        ui.info(f"{path}: written")
        ui.next_step("fill it in, then `gfrelease config show`")
        return 0
    s = ctx.settings
    rows = {}
    for name in sorted(settings_mod.KNOWN):
        value, source = s.lookup(name)
        rows[name] = {"value": value, "from": source}
    ctx.result = {"config": str(s.path) if s.path else None,
                  "settings": rows}
    ui.info(f"config file: {s.path or 'none'}")
    width = max(map(len, rows))
    for name, row in rows.items():
        if row["value"] is None:
            ui.info(f"  {name:<{width}}  -")
        else:
            ui.info(f"  {name:<{width}}  {row['value']}  ({row['from']})")
    return 0


def cmd_completion(ctx) -> int:
    print(completion(build_parser(), ctx.args.shell))
    return 0


def cmd_stage(ctx) -> int:
    args = ctx.args
    prefix = stage_mod.stage(
        ctx.product, Path(args.candidate), ctx.store(args.staging),
        ref=args.ref, commit=args.commit, version=args.version,
        filesafe=args.filesafe, built_at=args.built_at)
    ctx.result = {"staged": prefix}
    ui.info(f"staged {prefix}")
    if prefix.startswith(f"{layout.TAGS}/"):
        ui.next_step(f"the maintainer runs `gfrelease stable {args.version}`")
    else:
        ui.next_step("the signing host promotes it on its next round")
    return 0


def cmd_stable(ctx) -> int:
    args, product = ctx.args, ctx.product
    signer = None
    if args.authenticode_thumbprint:
        if not WINDOWS:
            raise ReleaseError("--authenticode-thumbprint names a "
                               "certificate of the Windows store; here it "
                               "is --authenticode-cert and -key")
        # No PIN is asked here: the card's own software does, once for the
        # programs and once for the packages.
        signer = authenticode.SigntoolSigner(
            args.authenticode_thumbprint, args.signtool or "",
            args.timestamp_url or "")
    elif args.authenticode_cert and args.authenticode_key:
        token = bool(args.pkcs11_module)
        signer = authenticode.Signer(
            args.authenticode_cert, args.authenticode_key,
            args.pkcs11_module or "", args.timestamp_url or "",
            # One PIN for every program signed, asked at the first.
            ask_pin=(lambda: getpass.getpass(
                "PIN of the Authenticode token (empty: the token asks): "))
            if token and sys.stdin.isatty() else None)
    stage = stable_mod.Stage(
        product, args.version, Path(args.work),
        staging=ctx.store(args.staging),
        git_dir=Path(args.git_dir) if args.git_dir else None,
        cosign=args.cosign or "cosign", key=args.key,
        gnupghome=args.gnupghome, signer=signer,
        authenticode_ca=args.authenticode_ca,
        on_signed=lambda name, n, total, path:
        ui.info(f"  {name}: [{n}/{total}] {path or 'the package itself'}"))
    if args.notes_only:
        digest = stage.publish_notes()
        ctx.result = {"version": args.version, "notes": digest}
        ui.info(f"{args.version}: notes {digest} handed over")
        ui.next_step(f"the signing host publishes them on its next round "
                     f"(`gfrelease status {args.version}`)")
        return 0
    if args.msix:
        descriptor = stage.add_msix()
        added = [a["name"] for a in descriptor["artifacts"]
                 if a["transform"] == "msix"]
        ctx.result = {"version": args.version, "added": added}
        ui.info(f"{args.version}: {', '.join(added)} handed over")
        ui.next_step(f"the signing host adds it to the release on its next "
                     f"round (`gfrelease status {args.version}`)")
        return 0

    total = len(stable_mod.STEPS)
    ask = not args.yes and not ui.JSON and sys.stdin.isatty() and \
        not args.dry_run
    ui.info(f"{args.version}: {' → '.join(stable_mod.STEPS)}"
            f"{' (dry run: until verify)' if args.dry_run else ''}")

    def on_step(step, number, done_before):
        ui.info(f"[{number}/{total}] {step}"
                f"{' (done before)' if done_before else ''}")
        if step == "upload" and not done_before and ask:
            try:
                answer = input(f"Hand {args.version} to the signing host "
                               f"(upload to {args.staging})? Once it has "
                               "published it, it is never replaced. [y/N] ")
            except EOFError:
                # Windows calls NUL a terminal too: nobody there is a No.
                answer = ""
            if answer.strip().lower() not in ("y", "yes"):
                raise ReleaseError(f"stopped before the upload: nothing of "
                                   f"{args.version} was published; run "
                                   "again to go on")

    descriptor = stage.run(args.redo, until="verify" if args.dry_run
                           else None, on_step=on_step)
    ctx.result = {"version": args.version,
                  "done": stage.state.get("done", []),
                  "published": descriptor["sums"].rsplit("/", 1)[0]
                  if descriptor else None}
    if args.dry_run:
        ui.info(f"{args.version}: fetched and verified; nothing signed or "
                "uploaded")
        ui.next_step(f"gfrelease stable {args.version}")
    elif descriptor:
        ui.info(f"{args.version}: handed to the signing host as "
                f"{descriptor['sums'].rsplit('/', 1)[0]}")
        kinds = {a["kind"] for a in descriptor["artifacts"]}
        if "msi" in kinds and "msix" not in kinds:
            ui.info(f"{args.version}: without an MSIX, which only Windows "
                    f"packs (there: gfrelease stable {args.version} --msix)")
        ui.next_step(f"the signing host receives and lists it on its next "
                     f"round: `gfrelease status {args.version}`")
    else:
        ui.info(f"{args.version}: done")
    return 0


def cmd_import(ctx) -> int:
    args = ctx.args
    staging = ctx.store(args.staging)
    done = []
    for version in args.versions:
        job = imported_mod.Import(ctx.product, version, Path(args.work),
                                  staging=staging, git_dir=Path(args.git_dir))
        d = job.run()
        done.append({"version": version, "files": len(d["artifacts"]),
                     "signed_by": d["signing"]["fingerprint"],
                     "left_out": list(job.skipped),
                     "uploaded": bool(staging)})
        ui.info(f"{version}: {len(d['artifacts'])} files, signed by "
                f"{d['signing']['fingerprint']}"
                + (f", handed over as releases/{version}/" if staging
                   else " (checked only, nothing uploaded)"))
        for skipped in job.skipped:
            ui.info(f"  left out: {skipped}")
    ctx.result = {"imported": done}
    if staging:
        ui.next_step("the signing host receives and lists them on its next "
                     "round")
    else:
        ui.next_step("run again with --staging to hand them over")
    return 0


def write_new(path: Path, data: bytes) -> None:
    """A trust file is written once: one there already is never replaced."""
    if path.exists():
        raise ReleaseError(f"{path} exists")
    atomic_write(path, data)


def sign_root(file: Path, secret: str) -> str:
    """One root key's signature on @p file, beside it as
    <N>.root.json.<keyid>.minisig; returns the key id."""
    doc = load_json(file.read_bytes(), str(file))
    n, slug = doc.get("version"), doc.get("product")
    if not isinstance(n, int) or not isinstance(slug, str):
        raise ReleaseError(f"{file}: not a root")
    tmp = file.with_name(file.name + ".new" + MINISIG)
    try:
        minisig.sign(Path(secret), file, layout.root_comment(slug, n), tmp)
        key_id = minisig.prehashed_key_id(tmp.read_bytes())
        if key_id is None:
            raise ReleaseError("minisign made a legacy signature")
        write_new(file.with_name(layout.root_sig_name(n, key_id)),
                  tmp.read_bytes())
    finally:
        tmp.unlink(missing_ok=True)
    return key_id


def trust_report(root: dict, keys: dict) -> dict:
    return {name: {"version": doc["version"], "expires": doc["expires"],
                   "days_left": round(trust.days_left(doc), 1)}
            for name, doc in (("root", root), ("index_keys", keys))}


def report_trust(root: dict, keys: dict) -> None:
    for doc, what in ((root, f"root {root['version']}"),
                      (keys, f"index-keys {keys['version']}")):
        ui.info(f"{what}: expires {doc['expires']}, in "
                f"{trust.days_left(doc):.0f} days")


def trust_next(root: dict, keys: dict) -> None:
    """A next: line when the root or the list should be renewed soon."""
    if trust.days_left(root) < trust.ROOT_WARN_DAYS:
        ui.next_step("renew the root: gfrelease trust root new (gfrelease "
                     "help root-ceremony)")
    elif trust.days_left(keys) < trust.INDEX_KEYS_WARN_DAYS:
        ui.next_step("renew the index-keys list: gfrelease trust keys renew")


def cmd_root_new(ctx) -> int:
    product = ctx.product
    root = trust.newest_root(product.root_dir, product.slug)
    nxt = trust.renewed(root, trust.ROOT_LIFETIME)
    path = product.root_dir / layout.root_name(nxt["version"])
    write_new(path, trust.dump(nxt))
    threshold = root["roles"]["root"]["threshold"]
    ctx.result = {"written": str(path), "version": nxt["version"]}
    ui.info(f"{path}: written; edit it by hand if anything else changes")
    ui.next_step(f"{threshold} of root {root['version']}'s keys and as many "
                 f"of its own sign it: gfrelease trust root sign {path} -s "
                 "<root key> (`gfrelease trust root status` shows who has)")
    return 0


def cmd_root_sign(ctx) -> int:
    key_id = sign_root(Path(ctx.args.file), ctx.args.secret)
    ui.info(f"{ctx.args.file}: signed by {key_id}")
    ctx.result = {"file": ctx.args.file, "signed_by": key_id}
    try:
        view = views.root_status(Path(ctx.args.file).parent,
                                 ctx.product.slug)
    except ReleaseError:
        return 0
    nxt = views.root_status_next(view)
    if view["file"] == str(Path(ctx.args.file)) and nxt:
        ui.next_step(nxt)
    return 0


def cmd_root_status(ctx) -> int:
    view = views.root_status(ctx.product.root_dir, ctx.product.slug)
    ctx.result = view
    for line in views.render_root_status(view):
        ui.info(line)
    nxt = views.root_status_next(view)
    if nxt:
        ui.next_step(nxt)
    return 0


def cmd_root_verify(ctx) -> int:
    product = ctx.product
    root, keys = trust.load_trust(product.root_dir, product.index_keys_dir,
                                  product.slug, first=ctx.args.first)
    ctx.result = trust_report(root, keys)
    report_trust(root, keys)
    trust.check_unexpired(root, f"root {root['version']}")
    trust.check_unexpired(keys, f"index-keys {keys['version']}")
    trust_next(root, keys)
    return 0


def outside_repository(path: str) -> Path:
    p = Path(path).resolve()
    if p == ROOT or ROOT in p.parents:
        raise ReleaseError(f"{p} is inside the repository")
    return p


def cmd_keys(ctx) -> int:
    args, product = ctx.args, ctx.product
    slug, root_dir, keys_dir = product.slug, product.root_dir, \
        product.index_keys_dir
    root = trust.newest_root(root_dir, slug)
    trust.check_unexpired(root, f"root {root['version']}")
    found = trust.versions(keys_dir)
    if not found:
        raise ReleaseError(f"{keys_dir}: no index-keys list to follow")
    name = layout.index_keys_name(found[-1])
    read = trust.read_dir(keys_dir)
    # Expired or not: renewing one that lapsed is what this is for. Signed
    # by the trusted root's G, or after G was replaced by an earlier root's:
    # the new G takes over by signing the next one.
    for n in range(root["version"], 0, -1):
        signer = trust.walk(trust.read_dir(root_dir), slug, upto=n,
                            steps=None)
        try:
            current = trust.accept_index_keys(signer, read(name),
                                              read(name + MINISIG), slug,
                                              name)
            break
        except ReleaseError:
            if n == 1:
                raise
    nxt = trust.renewed(current, trust.INDEX_KEYS_LIFETIME)
    new_key = None
    if args.action == "rotate":
        secret = outside_repository(args.secret_out)
        new_key = minisig.generate(secret)
        nxt["keys"] = (dict(current["keys"]) if args.keep_old else {}) | \
            {minisig.key_id(new_key): new_key}
    path = keys_dir / layout.index_keys_name(nxt["version"])
    write_new(path, trust.dump(nxt))
    try:
        minisig.sign(Path(args.secret), path,
                     layout.index_keys_comment(slug, nxt["version"]))
        trust.accept_index_keys(root, path.read_bytes(),
                                path.with_name(path.name + MINISIG)
                                .read_bytes(), slug, path.name)
    except ReleaseError:
        path.unlink(missing_ok=True)
        path.with_name(path.name + MINISIG).unlink(missing_ok=True)
        raise
    ctx.result = {"written": str(path), "expires": nxt["expires"],
                  "new_key": new_key}
    ui.info(f"{path}: written and signed, expires {nxt['expires']}")
    if new_key is None:
        ui.next_step(f"commit {path} and its {MINISIG}")
        return 0
    ui.info(f"new index key {minisig.key_id(new_key)}: {new_key}\n"
            f"  its secret is {secret}, made without a password (minisign "
            "-W):\n  the signing host uses it unattended. Keep it as a "
            "secret, nowhere else.")
    if args.keep_old:
        ui.next_step("commit the list; then, in any order, install the new "
                     "secret on the host as index.key, and later list only "
                     "the new key (rotate again without --keep-old, or edit "
                     "and sign)")
    else:
        ui.next_step("the host signs with a listed key only. Install the new "
                     "secret on the host as index.key and commit the list "
                     "together: until both are done the host refuses to sign "
                     "(the old index stays valid until it expires)")
    return 0


def git_reader(git_dir: Path, directory: str):
    """read() for trust.walk(): main's release/<directory>/, and the names
    it holds. A mirror that cannot be read is an error, never "not
    there"."""
    prefix = f"release/{directory}/"
    names = run(["git", "-C", git_dir, "ls-tree", "--name-only", refs.MAIN,
                 "--", prefix]).stdout.split()

    def read(name: str) -> bytes | None:
        if prefix + name not in names:
            return None
        return run(["git", "-C", git_dir, "show",
                    f"{refs.MAIN}:{prefix}{name}"], text=False).stdout
    return read, [n[len(prefix):] for n in names]


def cmd_trust_update(ctx) -> int:
    args, product = ctx.args, ctx.product
    with locked(Path(args.state_dir)):
        read_root, _ = git_reader(Path(args.git_dir), "root")
        read_keys, names = git_reader(Path(args.git_dir), "index-keys")
        root, keys, problems = trust.update(
            read_root, read_keys,
            policy_mod.numbered(names, policy_mod.INDEX_KEYS_FILE),
            product.root_dir, product.index_keys_dir, product.slug,
            product.root_sha256)
    ctx.result = dict(trust_report(root, keys), problems=list(problems))
    report_trust(root, keys)
    for problem in problems:
        ui.warn(problem)
    trust_next(root, keys)
    return host_mod.PARTIAL if problems else host_mod.DONE


def cmd_host(ctx) -> int:
    """promote, index and resign: the signing host's commands."""
    args, product = ctx.args, ctx.product
    command = args.key
    dry = getattr(args, "dry_run", False)
    if command != "promote":
        product.check_host_keys(minisig.public_key(args.index_pub))

    def path(value):
        return Path(value) if value else None

    host = host_mod.Host(
        product, ctx.store(args.builds), Path(args.state_dir),
        staging=ctx.store(args.staging), update=ctx.store(args.update),
        git_dir=path(args.git_dir), index_key=path(args.index_key),
        # Every listed index key: what a client accepts.
        index_pub=product.index_keys,
        cosign=args.cosign or "cosign", authenticode_ca=args.authenticode_ca)
    if dry:
        report = host.dry_run()
        ctx.result = report
        render_dry_run(report)
        # Ran to its end and would refuse something: as an index run that
        # refuses something ends.
        return host_mod.PARTIAL if report["refused"] else host_mod.DONE
    if command == "promote":
        status = host.promote()
    else:
        status = host.index(force=command == "resign" or args.force)
    ctx.result = dict(host.report, status={
        host_mod.DONE: "done", host_mod.PARTIAL: "partial",
        host_mod.FAILED: "failed"}[status])
    return status


def render_dry_run(report: dict) -> None:
    ui.info("would list:")
    for line in views.channel_lines(report["channels"]):
        ui.info(line)
    for key, version in sorted(report.get("betas", {}).items()):
        ui.info(f"  beta of {key}: {version}")
    ui.info(f"  releases {', '.join(report['releases']) or '-'}")
    if report["added"] or report["removed"]:
        ui.info(f"against the published index: added "
                f"{', '.join(report['added']) or '-'}; removed "
                f"{', '.join(report['removed']) or '-'}")
    ui.info(f"changes: {', '.join(report['changes']) or 'none'}")
    for refused in report["refused"]:
        ui.warn(f"would refuse: {refused}")


# --- shell completion --------------------------------------------------------

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
