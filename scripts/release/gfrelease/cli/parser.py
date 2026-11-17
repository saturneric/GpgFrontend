# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease: sign, verify and publish the product's releases.

Run it as scripts/gfrelease (or python3 scripts/release/gfrelease);
`gfrelease COMMAND --help` explains a command. The product comes from
release/product.json and release/policy.toml, what is trusted from the
signed root and index-keys list in release/root/ and release/index-keys/.
A setting comes from its option, else GFR_<NAME>, else gfrelease.toml
(cli/settings.py).
"""

from __future__ import annotations

import argparse
from pathlib import Path

from ..adapters.process import run
from ..app import stable as stable_mod
from ..domain import policy as policy_mod
from ..errors import ReleaseError
from ..paths import ROOT
from .commands import ci, publish, release, trust, verify
from .commands import github as github_cmd

# What each setting is, and what its value looks like.
SETTINGS = {
    "product": ("release/product.json of another product", "FILE"),
    "policy": ("its release/policy.toml (default: beside it)", "FILE"),
    "root_dir": ("the trust roots (default: release/root/)", "DIR"),
    "index_keys_dir": ("the index-keys lists (default: release/index-keys/)",
                       "DIR"),
    "root_sha256": ("the sha256 root 1 must have", "SHA256"),
    "rclone": ("the rclone binary", "PATH"),
    "staging": ("the staging store", "REMOTE"),
    "builds": ("the final artifact store (default: the HTTPS bases)",
               "REMOTE"),
    "update": ("the index store (default: the HTTPS bases)", "REMOTE"),
    "git_dir": ("a clone of the product (default: this checkout)", "DIR"),
    "cosign": ("the cosign binary", "PATH"),
    "cafile": ("CA bundle for the HTTPS bases", "FILE"),
    "work": ("where files and progress are kept", "DIR"),
    "key": ("your signing key's fingerprint (default: the first pinned "
            "maintainer)", "FINGERPRINT"),
    "gnupghome": ("the GnuPG home holding that key", "DIR"),
    "authenticode_thumbprint": ("SHA-1 thumbprint of the certificate in "
                                "your Windows store", "SHA1"),
    "signtool": ("signtool.exe (default: PATH, else the newest Windows "
                 "SDK's)", "PATH"),
    "timestamp_url": ("RFC 3161 time-stamping services, comma-separated",
                      "URL[,URL...]"),
    "authenticode_cert": ("certificate file (a rehearsal: osslsigncode)",
                          "CERT"),
    "authenticode_key": ("its key file", "KEY"),
    "authenticode_ca": ("CA file for checking Authenticode (a rehearsal's)",
                        "FILE"),
    "state_dir": ("where the serial and the lock live", "DIR"),
    "index_key": ("minisign secret key for the index", "FILE"),
    "index_pub": ("its public key (file or base64)", "KEY"),
}
GLOBAL = ("product", "policy", "root_dir", "index_keys_dir", "root_sha256",
          "rclone")
HOST = ("staging", "builds", "update", "git_dir", "index_key", "index_pub",
        "state_dir", "cosign", "authenticode_ca")


class Version(argparse.Action):
    """--version: the product commit the tooling comes from, COMMIT beside
    it (the signing host's image), else git describe of this checkout."""

    def __init__(self, option_strings, dest, **kw):
        super().__init__(option_strings, dest, nargs=0,
                         default=argparse.SUPPRESS, **kw)

    def __call__(self, parser, namespace, values, option_string=None):
        commit = ROOT / "COMMIT"
        if commit.is_file():
            version = commit.read_text().strip()
        else:
            try:
                version = run(["git", "-C", ROOT, "describe", "--always",
                               "--dirty", "--tags"], timeout=10).stdout.strip()
            except ReleaseError:
                version = "unknown"
        print(f"gfrelease {version}")
        parser.exit()


def settings(p, *names) -> None:
    """--name for each setting, unset unless given (settings.fill)."""
    for name in names:
        text, metavar = SETTINGS[name]
        p.add_argument(f"--{name.replace('_', '-')}", metavar=metavar,
                       help=f"{text} [GFR_{name.upper()}]")


def command(sub, name, help, handler, *, key=None, product=True):
    p = sub.add_parser(name, help=help, description=help)
    p.set_defaults(name=key or name, handler=handler, needs_product=product)
    return p


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        prog="gfrelease", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--version", action=Version,
                    help="the product commit this tooling comes from")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="print each tool run (rclone, git, gpg, ...)")
    settings(ap, *GLOBAL)
    sub = ap.add_subparsers(dest="command", required=True, metavar="COMMAND")

    # The maintainer's.
    p = command(sub, "stable", "sign and hand over a stable release "
                "(fetch, verify, authenticode, pgp, upload; resumes where "
                "it stopped)", release.cmd_stable)
    p.add_argument("version", metavar="VERSION",
                   help="X.Y.Z, its tag vX.Y.Z already pushed")
    settings(p, "work", "staging", "git_dir", "cosign", "key", "gnupghome",
             "authenticode_thumbprint", "signtool", "timestamp_url",
             "authenticode_cert", "authenticode_key", "authenticode_ca")
    p.add_argument("--redo", choices=stable_mod.STEPS,
                   help="run this step, and every one after it, again")
    p.add_argument("--notes-only", action="store_true",
                   help="after the release: sign and hand over main's notes")
    p.add_argument("--dry-run", action="store_true",
                   help="fetch and verify only: nothing signed or uploaded")
    p.add_argument("-y", "--yes", action="store_true",
                   help="upload without asking (asked only on a terminal)")

    t = sub.add_parser("trust", help="the trust root and the index keys")
    tsub = t.add_subparsers(dest="group", required=True, metavar="GROUP")
    r = tsub.add_parser("root", help="new | sign | verify")
    rsub = r.add_subparsers(dest="action", required=True, metavar="ACTION")
    command(rsub, "new", "write the next root: the newest one, one version "
            "up, expiring in two years", trust.cmd_root_new,
            key="trust root new")
    p = command(rsub, "sign", "add one root key's signature, and say who "
                "has signed", trust.cmd_root_sign, key="trust root sign")
    p.add_argument("file", metavar="FILE", help="<N>.root.json")
    p.add_argument("-s", dest="secret", required=True, metavar="SECRET",
                   help="that root key's minisign secret key")
    command(rsub, "verify", "walk release/root/, check release/index-keys/, "
            "say when each expires", trust.cmd_root_verify,
            key="trust root verify")
    k = tsub.add_parser("keys", help="renew")
    ksub = k.add_subparsers(dest="action", required=True, metavar="ACTION")
    p = command(ksub, "renew", "the index-keys list one version up, expiring "
                "in a year, signed with G", trust.cmd_keys,
                key="trust keys renew")
    p.add_argument("-s", dest="secret", required=True, metavar="SECRET",
                   help="G's minisign secret key; a path read once, so "
                   "<(bw get notes ...) works")
    p.add_argument("--rotate", action="store_true",
                   help="list a new index key in place of the current ones")
    p.add_argument("--secret-out", metavar="FILE",
                   help="with --rotate: where the new key's secret goes "
                   "(never inside the repository)")
    p.add_argument("--keep-old", action="store_true",
                   help="with --rotate: list the current keys too")
    p = command(tsub, "update", "take main's next roots and index-keys list "
                "as far as they hold (the signing host)",
                trust.cmd_trust_update, key="trust update")
    settings(p, "git_dir", "state_dir")

    # CI's.
    p = command(sub, "policy", "check the release policy, or render "
                "SECURITY.md from it", ci.cmd_policy, product=False)
    p.add_argument("action", choices=("check", "render-security"))
    p.add_argument("--security", type=Path, default=policy_mod.SECURITY,
                   help="the SECURITY.md to check or write")

    c = sub.add_parser("candidate", help="stage: gather and check a release "
                       "candidate")
    csub = c.add_subparsers(dest="action", required=True, metavar="ACTION")
    p = command(csub, "stage", "write the release candidate",
                ci.cmd_candidate_stage, key="candidate stage", product=False)
    p.add_argument("--artifacts", required=True, type=Path,
                   help="the build legs' downloaded artifacts")
    p.add_argument("--out", required=True, type=Path,
                   help="where the candidate is written")
    p.add_argument("--version", dest="filesafe_version", metavar="FILESAFE",
                   help="the file-safe version every leg must have built")

    c = sub.add_parser("ci", help="build-mode | check-order: what CI builds")
    csub = c.add_subparsers(dest="action", required=True, metavar="ACTION")
    p = command(csub, "build-mode", "key=value lines for $GITHUB_OUTPUT",
                ci.cmd_build_mode, key="ci build-mode", product=False)
    p.add_argument("--repo", type=Path, default=ROOT,
                   help="the checkout (default: this one)")
    p = command(csub, "check-order", "this build's version sorts right "
                "against the release tags", ci.cmd_check_order,
                key="ci check-order", product=False)
    p.add_argument("--repo", type=Path, default=ROOT,
                   help="the checkout (default: this one)")
    p.add_argument("--ref", help="the ref built (default: GITHUB_REF, "
                   "else the branch checked out)")
    p.add_argument("--version", dest="build_version", metavar="VERSION",
                   help="the version built (default: derived from git)")

    p = command(sub, "stage", "upload a verified release candidate",
                publish.cmd_stage)
    settings(p, "staging")
    p.add_argument("--candidate", required=True, metavar="DIR",
                   help="the verified candidate directory")
    for name, help in (("ref", "full git ref"), ("commit", "full commit sha"),
                       ("version", "SemVer"),
                       ("filesafe", "file-safe version"),
                       ("built_at", "UTC time")):
        p.add_argument(f"--{name.replace('_', '-')}", required=True,
                       help=help)

    g = sub.add_parser("github", help="sync: the stable releases on GitHub")
    gsub = g.add_subparsers(dest="action", required=True, metavar="ACTION")
    p = command(gsub, "sync", "mirror the stable releases the signed index "
                "lists to GitHub Releases, each file checked first; writes "
                "with GITHUB_TOKEN", github_cmd.cmd_sync, key="github sync")
    p.add_argument("versions", nargs="*", metavar="VERSION",
                   help="only these releases (default: every stable one)")
    settings(p, "update", "builds", "cafile")
    p.add_argument("--repository", metavar="OWNER/NAME",
                   help="another repository than the trust root's")
    p.add_argument("--dry-run", action="store_true",
                   help="check and plan only: nothing is written")

    # The signing host's, and anyone's verify.
    p = command(sub, "promote", "promote each branch's newest staged build, "
                "receive what maintainers handed over", publish.cmd_host)
    settings(p, *HOST)
    p = command(sub, "index", "build the index, and publish it signed when "
                "it says something new", publish.cmd_host)
    settings(p, *HOST)
    p.add_argument("--force", action="store_true",
                   help="publish even if nothing changed: a re-sign")

    p = command(sub, "verify", "check the published index as a client does",
                verify.cmd_verify)
    settings(p, "update", "builds", "cafile")
    p.add_argument("--deep", action="store_true",
                   help="download every artifact")
    p.add_argument("--min-days-left", type=float, metavar="DAYS",
                   help="fail when the index expires sooner than this")
    return ap
