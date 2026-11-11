# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease: stage, promote, index and verify releases.

  python3 scripts/release/gfrelease stage   --candidate DIR --ref REF ...  (CI)
  python3 scripts/release/gfrelease promote                               (host)
  python3 scripts/release/gfrelease index [--force]                       (host)
  python3 scripts/release/gfrelease run                 promote, then index (host)
  python3 scripts/release/gfrelease stable 2.3.0   the offline stable stage (you)
  python3 scripts/release/gfrelease import 2.2.4   a release from GitHub Releases
  python3 scripts/release/gfrelease resign              a new serial and expiry
  python3 scripts/release/gfrelease verify [--deep]    over HTTPS (anyone)
  python3 scripts/release/gfrelease root new|sign|verify     the trust root
  python3 scripts/release/gfrelease index-keys renew|rotate  the index keys

The product (slug, mirror bases, artifact rules) comes from
release/product.json, the series and withdrawals from release/policy.toml,
and what is trusted (the build workflow, the maintainers, the index keys)
from the signed root and index-keys list in release/root/ and
release/index-keys/ (trust.py).
Every option can also be set as GFR_<OPTION> in the environment, which is how
the signing host's container configures it (the Release-Server repository).
Stores are rclone paths that already include the product root.

Exit status: 0 when everything was done; 3 from the host commands when the
round ran to its end but something was refused for what it is (named on
stderr, and refused again until it changes); 1 for any other failure, which
is worth running again.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import imported as imported_mod  # noqa: E402
import layout  # noqa: E402
import minisig  # noqa: E402
import authenticode  # noqa: E402
import host as host_mod  # noqa: E402
import stable as stable_mod  # noqa: E402
import stage as stage_mod  # noqa: E402
import trust  # noqa: E402
import verify as verify_mod  # noqa: E402
from common import (MINISIG, ROOT, Product, ReleaseError,  # noqa: E402
                    atomic_write, load_json, parse_iso, run, utcnow)
from store import Store  # noqa: E402


def env(name: str, default=None):
    return os.environ.get(f"GFR_{name.upper()}", default)


def env_flag(name: str) -> bool:
    return (env(name) or "").lower() in ("1", "true", "yes")


def flag(p, name, help):
    p.add_argument(f"--{name.replace('_', '-')}", action="store_true",
                   default=env_flag(name), help=help)


def opt(p, name, help, **kw):
    p.add_argument(f"--{name.replace('_', '-')}", default=env(name), help=help, **kw)


def product_of(args) -> Product:
    if args.product:
        product, policy = Path(args.product), Path(args.policy)
    else:
        product = ROOT / "release" / "product.json"
        policy = product.with_name("policy.toml")
    return Product(product, policy, root_dir=args.root_dir,
                   index_keys_dir=args.index_keys_dir,
                   root_sha256=args.root_sha256)


def need(args, *names):
    missing = [n for n in names if not getattr(args, n)]
    if missing:
        raise ReleaseError("missing " + ", ".join(
            f"--{n.replace('_', '-')} (or GFR_{n.upper()})" for n in missing))


def pub(path_or_key: str) -> str:
    p = Path(path_or_key)
    return minisig.public_key_of(p) if p.is_file() else path_or_key


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gfrelease", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    opt(ap, "product", "release/product.json of another product")
    opt(ap, "policy", "its release/policy.toml")
    opt(ap, "rclone", "the rclone binary")
    opt(ap, "root_dir", "the trust roots (default: release/root/)")
    opt(ap, "index_keys_dir", "the index-keys lists (default: "
        "release/index-keys/)")
    opt(ap, "root_sha256", "the sha256 root 1 must have (the signing "
        "host's anchor)")
    sub = ap.add_subparsers(dest="command", required=True)

    r = sub.add_parser("root", help="the trust root (offline)")
    rsub = r.add_subparsers(dest="action", required=True)
    rsub.add_parser("new", help="write the next root: the newest one, one "
                    "version up, expiring in two years; edit it by hand "
                    "for anything else")
    rs = rsub.add_parser("sign", help="add one root key's signature")
    rs.add_argument("file", help="<N>.root.json")
    rs.add_argument("-s", dest="secret", required=True,
                    help="that root key's minisign secret key")
    rv = rsub.add_parser("verify", help="walk release/root/, check "
                         "release/index-keys/, say when each expires")
    rv.add_argument("--from", dest="first", type=int, default=1,
                    help="trust this root on its own signatures, walk on")

    k = sub.add_parser("index-keys", help="the keys that may sign the index")
    ksub = k.add_subparsers(dest="action", required=True)
    kr = ksub.add_parser("renew", help="the same keys, one version up, "
                         "expiring in a year, signed with G")
    ko = ksub.add_parser("rotate", help="a new index key, listed in the "
                         "next version, signed with G")
    ko.add_argument("--secret-out", required=True,
                    help="where the new index key's secret goes (never "
                    "inside the repository)")
    ko.add_argument("--keep-old", action="store_true",
                    help="list the current keys too, for a swap at leisure")
    for p in (kr, ko):
        p.add_argument("-s", dest="secret", required=True,
                       help="G's minisign secret key; a path read once, so "
                       "<(bw get notes ...) works")

    s = sub.add_parser("stage", help="upload a release candidate (CI)")
    for name, h in (("candidate", "the verified candidate directory"),
                    ("staging", "the staging store"), ("ref", "full git ref"),
                    ("commit", "full commit sha"), ("version", "SemVer"),
                    ("filesafe", "file-safe version"), ("built_at", "UTC time")):
        opt(s, name, h)

    st = sub.add_parser("stable", help="sign and publish a stable release "
                        "(the maintainer's machine)")
    st.add_argument("version", help="X.Y.Z, its tag vX.Y.Z already pushed")
    opt(st, "work", "where the stage keeps its files and progress")
    opt(st, "staging", "the staging store (read)")
    opt(st, "builds", "the final artifact store (write)")
    opt(st, "git_dir", "a clone with the release's signed tag")
    opt(st, "cosign", "the cosign binary")
    opt(st, "key", "your signing key's fingerprint (default: the first "
        "pinned maintainer)")
    opt(st, "gnupghome", "the GnuPG home holding that key (default: yours)")
    opt(st, "authenticode_cert", "certificate file, or PKCS#11 URI")
    opt(st, "authenticode_key", "key file, or PKCS#11 URI")
    opt(st, "pkcs11_module", "the token's PKCS#11 module")
    opt(st, "timestamp_url", "RFC 3161 time-stamping service")
    opt(st, "authenticode_ca", "CA file for checking the signature (a "
        "rehearsal's own CA)")
    st.add_argument("--redo", choices=stable_mod.STEPS,
                    help="run this step, and every one after it, again")
    st.add_argument("--notes-only", action="store_true",
                    help="after the release: sign and upload main's notes")

    host = []
    for name in ("promote", "index", "run", "resign"):
        p = sub.add_parser(name)
        host.append(p)
        opt(p, "staging", "the staging store")
        opt(p, "builds", "the final artifact store")
        opt(p, "update", "the index store")
        opt(p, "mirror_update", "the mirror's index store", )
        opt(p, "git_dir", "this product's git mirror (may be bare)")
        opt(p, "index_key", "minisign secret key for the index")
        opt(p, "index_pub", "its public key (file or base64)")
        opt(p, "state_dir", "where the serial and the lock live")
        opt(p, "cosign", "the cosign binary")
        opt(p, "authenticode_ca", "CA file for Authenticode (a rehearsal's)")
        flag(p, "fetch", "git fetch the mirror first")
        flag(p, "force", "publish even if nothing changed")
        if name == "index":
            flag(p, "dry_run", "decide everything, write nothing: what "
                 "would be listed and refused, against the published index")

    im = sub.add_parser("import", help="bring a release made before this "
                        "pipeline over from GitHub Releases")
    im.add_argument("versions", nargs="+", metavar="version",
                    help="X.Y.Z, each a published GitHub release")
    opt(im, "work", "where the downloads are kept")
    opt(im, "builds", "the final artifact store (write); without it, only "
        "download and check")
    opt(im, "git_dir", "a clone with the release tags")

    v = sub.add_parser("verify", help="check a published index")
    opt(v, "update", "the index store (default: the HTTPS bases)")
    opt(v, "builds", "the artifact store (default: the HTTPS bases)")
    opt(v, "cafile", "CA bundle for the HTTPS bases")
    v.add_argument("--deep", action="store_true", help="download every artifact")
    opt(v, "min_days_left", "fail when the index expires sooner than this "
        "(a monitor's early warning that the signing host stopped)")

    args = ap.parse_args(argv)
    if args.product and not args.policy:
        args.policy = str(Path(args.product).with_name("policy.toml"))
    rclone = args.rclone or "rclone"

    try:
        product = product_of(args)
        if args.command == "root":
            return root_command(args, product.slug, product.root_dir,
                                product.index_keys_dir)
        if args.command == "index-keys":
            return index_keys_command(args, product.slug, product.root_dir,
                                      product.index_keys_dir)

        if args.command == "stage":
            need(args, "candidate", "staging", "ref", "commit", "version",
                 "filesafe", "built_at")
            prefix = stage_mod.stage(
                product, Path(args.candidate), Store(args.staging, rclone),
                ref=args.ref, commit=args.commit, version=args.version,
                filesafe=args.filesafe, built_at=args.built_at)
            print(f"staged {prefix}")
            return 0

        if args.command == "stable":
            need(args, "work")
            signer = None
            if args.authenticode_cert and args.authenticode_key:
                signer = authenticode.Signer(
                    args.authenticode_cert, args.authenticode_key,
                    args.pkcs11_module or "", args.timestamp_url or "")
            stage = stable_mod.Stage(
                product, args.version, Path(args.work),
                staging=Store(args.staging, rclone) if args.staging else None,
                builds=Store(args.builds, rclone) if args.builds else None,
                git_dir=Path(args.git_dir) if args.git_dir else None,
                cosign=args.cosign or "cosign", key=args.key,
                gnupghome=args.gnupghome, signer=signer,
                authenticode_ca=args.authenticode_ca)
            if args.notes_only:
                print(f"{args.version}: notes {stage.publish_notes()} "
                      "uploaded; the host publishes them on its next round")
                return 0
            descriptor = stage.run(args.redo)
            print(f"{args.version}: published to {descriptor['sums'].rsplit('/', 1)[0]}"
                  if descriptor else f"{args.version}: done")
            return 0

        if args.command == "import":
            need(args, "work", "git_dir")
            builds = Store(args.builds, rclone) if args.builds else None
            for version in args.versions:
                job = imported_mod.Import(product, version, Path(args.work),
                                          builds=builds,
                                          git_dir=Path(args.git_dir))
                d = job.run()
                print(f"{version}: {len(d['artifacts'])} files, signed by "
                      f"{d['signing']['fingerprint']}"
                      + (f", uploaded to releases/{version}/" if builds
                         else " (checked only, nothing uploaded)"))
                for skipped in job.skipped:
                    print(f"  left out: {skipped}")
            return 0

        if args.command == "verify":
            if bool(args.update) != bool(args.builds):
                raise ReleaseError("give both --update and --builds, or "
                                   "neither (the HTTPS bases)")
            if args.update and args.builds:
                update, builds = Store(args.update, rclone), Store(args.builds, rclone)
            else:
                bases = product.data["bases"]
                update = verify_mod.HttpSource(bases["update"], product.slug, args.cafile)
                builds = verify_mod.HttpSource(bases["builds"], product.slug, args.cafile)
            report = verify_mod.verify(product, update, builds,
                                       index_keys=product.index_keys,
                                       deep=args.deep)
            print(json.dumps(report, indent=2))
            if args.min_days_left:
                left = parse_iso(report["expires"]) - utcnow()
                if left < dt.timedelta(days=float(args.min_days_left)):
                    raise ReleaseError(
                        f"the index expires at {report['expires']}, in "
                        f"{left}: the signing host has not re-signed it")
            return 0

        return host_round(args, product, rclone)
    except ReleaseError as e:
        print(f"gfrelease: {e}", file=sys.stderr)
        return host_mod.FAILED
    except (KeyError, TypeError, ValueError, AttributeError, IndexError,
            UnicodeError, OSError) as e:
        # A safety net: input that slipped past the checks, or a file that
        # could not be read or written, is reported, not dumped as a
        # traceback.
        print(f"gfrelease: malformed input ({type(e).__name__}: {e})",
              file=sys.stderr)
        return host_mod.FAILED


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


def sign_index_keys(file: Path, secret: str, slug: str, version: int) -> None:
    minisig.sign(Path(secret), file, layout.index_keys_comment(slug, version))


def report_trust(root: dict, keys: dict) -> None:
    for doc, what in ((root, f"root {root['version']}"),
                      (keys, f"index-keys {keys['version']}")):
        print(f"{what}: expires {doc['expires']}, in "
              f"{trust.days_left(doc):.0f} days")


def root_command(args, slug: str, root_dir: Path, keys_dir: Path) -> int:
    if args.action == "sign":
        key_id = sign_root(Path(args.file), args.secret)
        print(f"{args.file}: signed by {key_id}")
        return 0
    if args.action == "new":
        root = trust.walk(trust.read_dir(root_dir), slug, steps=None)
        if root is None:
            raise ReleaseError(f"{root_dir}: no root to follow")
        nxt = trust.renewed(root, trust.ROOT_LIFETIME)
        path = root_dir / layout.root_name(nxt["version"])
        write_new(path, trust.dump(nxt))
        print(f"{path}: written; edit it by hand if anything else changes, "
              f"then have {root['roles']['root']['threshold']} of root "
              f"{root['version']}'s keys and as many of its own sign it "
              "(gfrelease root sign)")
        return 0
    root, keys = trust.load_trust(root_dir, keys_dir, slug, first=args.first)
    report_trust(root, keys)
    trust.check_unexpired(root, f"root {root['version']}")
    trust.check_unexpired(keys, f"index-keys {keys['version']}")
    return 0


def index_keys_command(args, slug: str, root_dir: Path,
                       keys_dir: Path) -> int:
    root = trust.walk(trust.read_dir(root_dir), slug, steps=None)
    if root is None:
        raise ReleaseError(f"{root_dir}: no root")
    trust.check_unexpired(root, f"root {root['version']}")
    found = trust.versions(keys_dir)
    if not found:
        raise ReleaseError(f"{keys_dir}: no index-keys list to follow")
    name = layout.index_keys_name(found[-1])
    read = trust.read_dir(keys_dir)
    # Expired or not: renewing one that lapsed is what this is for.
    current = trust.accept_index_keys(root, read(name),
                                      read(name + MINISIG), slug, name)
    nxt = trust.renewed(current, trust.INDEX_KEYS_LIFETIME)
    new_key = None
    if args.action == "rotate":
        secret = Path(args.secret_out).resolve()
        if secret == ROOT or ROOT in secret.parents:
            raise ReleaseError(f"{secret} is inside the repository")
        if secret.exists():
            raise ReleaseError(f"{secret} exists")
        run(["minisign", "-G", "-W", "-s", secret, "-p",
             secret.with_name(secret.name + ".pub")])
        new_key = minisig.public_key_of(secret.with_name(secret.name + ".pub"))
        nxt["keys"] = ({**current["keys"]} if args.keep_old else {}) |             {minisig.key_id(new_key): new_key}
    path = keys_dir / layout.index_keys_name(nxt["version"])
    write_new(path, trust.dump(nxt))
    try:
        sign_index_keys(path, args.secret, slug, nxt["version"])
        trust.accept_index_keys(root, path.read_bytes(),
                                path.with_name(path.name + MINISIG)
                                .read_bytes(), slug, path.name)
    except ReleaseError:
        path.unlink(missing_ok=True)
        path.with_name(path.name + MINISIG).unlink(missing_ok=True)
        raise
    print(f"{path}: written and signed, expires {nxt['expires']}")
    if new_key is None:
        print(f"next: commit {path} and its {MINISIG}")
        return 0
    print(f"new index key {minisig.key_id(new_key)}: {new_key}\n"
          f"  its secret is {secret}, made without a password (minisign -W):"
          "\n  the signing host uses it unattended. Keep it as a secret, "
          "nowhere else.")
    if args.keep_old:
        print("next: commit the list; then, in any order, install the new "
              "secret on the host as index.key, and later list only the "
              "new key (rotate again without --keep-old, or edit and sign)")
    else:
        print("next: the host signs with a listed key only. Install the new "
              "secret on the host as index.key and commit the list together:"
              " until both are done the host refuses to sign (the old "
              "index stays valid until it expires)")
    return 0


def host_round(args, product: Product, rclone: str) -> int:
    """promote, index, run and resign: the signing host's commands."""
    need(args, "builds", "state_dir")
    command = args.command
    promotes = command in ("promote", "run")
    indexes = command in ("index", "run", "resign")
    dry = getattr(args, "dry_run", False)
    if indexes:
        need(args, "update", *(() if dry else ("index_key",)), "index_pub")
    if promotes:
        need(args, "staging", "git_dir")
    if args.fetch:
        need(args, "git_dir")
    if indexes:
        product.check_host_keys(pub(args.index_pub))

    def store(root):
        return Store(root, rclone) if root else None

    def path(value):
        return Path(value) if value else None

    host = host_mod.Host(
        product, Store(args.builds, rclone), Path(args.state_dir),
        staging=store(args.staging), update=store(args.update),
        mirrors=[Store(args.mirror_update, rclone)]
        if args.mirror_update else [],
        git_dir=path(args.git_dir), index_key=path(args.index_key),
        # Every listed index key: what a client accepts.
        index_pub=product.index_keys,
        cosign=args.cosign or "cosign", authenticode_ca=args.authenticode_ca)
    if dry:
        if args.fetch:
            host.fetch()
        report = host.dry_run()
        print(json.dumps(report, indent=2))
        return host_mod.FAILED if report["refused"] else host_mod.DONE
    return host.round(promote=promotes, index=indexes, fetch=args.fetch,
                      force=args.force or command == "resign")


if __name__ == "__main__":
    sys.exit(main())
