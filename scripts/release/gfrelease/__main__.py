# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease: stage, promote, index and verify releases.

  python3 scripts/release/gfrelease stage   --candidate DIR --ref REF ...  (CI)
  python3 scripts/release/gfrelease promote                               (host)
  python3 scripts/release/gfrelease index [--force]                       (host)
  python3 scripts/release/gfrelease run                 promote, then index (host)
  python3 scripts/release/gfrelease resign              a new serial and expiry
  python3 scripts/release/gfrelease verify [--http] [--deep]           (anyone)

The product (slug, repository, workflow, mirror bases, public keys) comes from
release/product.json, the series and withdrawals from release/policy.toml.
Every option can also be set as GFR_<OPTION> in the environment, which is how
the signing host's container configures it (the Release-Server repository).
Stores are rclone paths that already include the product root.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import index as index_mod  # noqa: E402
import minisig  # noqa: E402
import promote as promote_mod  # noqa: E402
import stage as stage_mod  # noqa: E402
import verify as verify_mod  # noqa: E402
from common import Product, ReleaseError, default_product, locked  # noqa: E402
from store import Store  # noqa: E402


def env(name: str, default=None):
    return os.environ.get(f"GFR_{name.upper()}", default)


def opt(p, name, help, **kw):
    p.add_argument(f"--{name.replace('_', '-')}", default=env(name), help=help, **kw)


def product_of(args) -> Product:
    if args.product:
        return Product(Path(args.product), Path(args.policy))
    return default_product()


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
    sub = ap.add_subparsers(dest="command", required=True)

    s = sub.add_parser("stage", help="upload a release candidate (CI)")
    for name, h in (("candidate", "the verified candidate directory"),
                    ("staging", "the staging store"), ("ref", "full git ref"),
                    ("commit", "full commit sha"), ("version", "SemVer"),
                    ("filesafe", "file-safe version"), ("built_at", "UTC time")):
        opt(s, name, h)

    host = []
    for name in ("promote", "index", "run", "resign"):
        p = sub.add_parser(name)
        host.append(p)
        opt(p, "staging", "the staging store")
        opt(p, "builds", "the final artifact store")
        opt(p, "update", "the index store")
        opt(p, "mirror_update", "the mirror's index store", )
        opt(p, "git_dir", "this product's git mirror (may be bare)")
        opt(p, "artifact_key", "minisign secret key for artifacts")
        opt(p, "artifact_pub", "its public key (file or base64)")
        opt(p, "index_key", "minisign secret key for the index")
        opt(p, "index_pub", "its public key (file or base64)")
        opt(p, "state_dir", "where the serial and the lock live")
        opt(p, "cosign", "the cosign binary")
        p.add_argument("--fetch", action="store_true", default=env("fetch") == "1",
                       help="git fetch the mirror first")
        p.add_argument("--force", action="store_true",
                       help="publish even if nothing changed")

    v = sub.add_parser("verify", help="check a published index")
    opt(v, "update", "the index store (default: the HTTPS bases)")
    opt(v, "builds", "the artifact store (default: the HTTPS bases)")
    opt(v, "index_pub", "pinned index key(s), comma-separated (file or base64)")
    opt(v, "artifact_pub", "the artifact key (file or base64)")
    opt(v, "cafile", "CA bundle for the HTTPS bases")
    v.add_argument("--deep", action="store_true", help="download every artifact")

    args = ap.parse_args(argv)
    if args.product and not args.policy:
        args.policy = str(Path(args.product).with_name("policy.toml"))
    rclone = args.rclone or "rclone"

    try:
        product = product_of(args)
        signing = product.data["signing"]

        if args.command == "stage":
            need(args, "candidate", "staging", "ref", "commit", "version",
                 "filesafe", "built_at")
            prefix = stage_mod.stage(
                product, Path(args.candidate), Store(args.staging, rclone),
                ref=args.ref, commit=args.commit, version=args.version,
                filesafe=args.filesafe, built_at=args.built_at)
            print(f"staged {prefix}")
            return 0

        if args.command == "verify":
            if args.update and args.builds:
                update, builds = Store(args.update, rclone), Store(args.builds, rclone)
            else:
                bases = product.data["bases"]
                update = verify_mod.HttpSource(bases["update"], product.slug, args.cafile)
                builds = verify_mod.HttpSource(bases["builds"], product.slug, args.cafile)
            keys = [pub(k) for k in (args.index_pub.split(",") if args.index_pub
                                     else signing["index_keys"])]
            artifact = pub(args.artifact_pub) if args.artifact_pub else signing["artifact_key"]
            report = verify_mod.verify(product, update, builds, index_keys=keys,
                                       artifact_key=artifact, deep=args.deep)
            print(json.dumps(report, indent=2))
            return 0

        need(args, "builds", "state_dir", "artifact_pub")
        builds = Store(args.builds, rclone)
        artifact_pub = pub(args.artifact_pub)
        state = Path(args.state_dir)
        with locked(state):
            if args.command in ("promote", "run"):
                need(args, "staging", "git_dir", "artifact_key")
                done = promote_mod.promote(
                    product, Store(args.staging, rclone), builds,
                    git_dir=Path(args.git_dir), artifact_key=Path(args.artifact_key),
                    artifact_pub=artifact_pub, cosign=args.cosign or "cosign",
                    fetch=args.fetch)
                for d in done:
                    print(f"promoted {d['version']} ({len(d['artifacts'])} files)")
            if args.command in ("index", "run", "resign"):
                need(args, "update", "index_key", "index_pub")
                update = Store(args.update, rclone)
                mirrors = [Store(args.mirror_update, rclone)] if args.mirror_update else []
                index = index_mod.publish(
                    product, builds, update, index_key=Path(args.index_key),
                    artifact_pub=artifact_pub, state_dir=state,
                    force=args.force or args.command == "resign", mirrors=mirrors)
                if index is None:
                    print("index unchanged")
                else:
                    # Read back what was published, as a client would see it.
                    verify_mod.verify(product, update, builds,
                                      index_keys=[pub(args.index_pub)],
                                      artifact_key=artifact_pub)
                    print(f"published serial {index['serial']}, nightly "
                          f"{index['channels']['nightly']}, expires "
                          f"{index['expires']}")
        return 0
    except ReleaseError as e:
        print(f"gfrelease: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
