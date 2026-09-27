#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build and verify the nightly release candidate.

The release is one invariant:

  final verified artifacts -> signed canonical manifest
                           -> immutable release candidate
                           -> verify-only publication

`stage` runs in the provenance job. It takes the final artifacts of every leg
and the build info that describes them, and writes a candidate directory:

  <deliverables>        every file of every `gpgfrontend-<leg>` artifact
  build-records.json    which leg produced which file, and from what build
  SHA256SUMS.txt        the canonical release set: every file above, hashed

The provenance job then signs SHA256SUMS.txt, once, and uploads the directory
as the `release-candidate` artifact. Nothing writes to it after that.

`verify` runs in the release job, over the downloaded candidate, and is the
whole publication gate: the Sigstore identity, then the manifest, then exact
set equality between the manifest and the directory, then every hash, then
the records. It prints the verified file list for the publish step, so what
is published is what was verified and not whatever a glob finds.

## One authority per fact

  the bytes          SHA256SUMS.txt, and only there
  which leg made it  build-records.json (names, never hashes)
  the pairing        the artifact name: `gpgfrontend-<leg>` and
                     `buildinfo-<leg>` share <leg> exactly

Nothing here knows a platform, an architecture or a package format. A new
build leg only has to upload those two artifacts.

`build-info.json` is an internal CI interface. It is read here and never
published; the name is reserved so it cannot be, even by accident.

Stdlib only: the provenance and release runners install nothing for this.

usage: release_candidate.py stage --artifacts DIR --out DIR
       release_candidate.py verify --dir DIR --identity ID --sha SHA
                                   [--cosign PATH] [--github-output FILE]
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

MANIFEST = "SHA256SUMS.txt"
BUNDLE = "SHA256SUMS.txt.sigstore.json"
RECORDS = "build-records.json"
BUILD_INFO = "build-info.json"

# Provenance names. A deliverable may never take one: it would shadow, or be
# shadowed by, the file that describes it.
RESERVED = frozenset({MANIFEST, BUNDLE, RECORDS, BUILD_INFO})

SAFE_NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._+-]*$")
MANIFEST_LINE = re.compile(r"^([0-9a-f]{64})  (\S+)$")

DELIVERABLE_PREFIX = "gpgfrontend-"
BUILD_INFO_PREFIX = "buildinfo-"

ISSUER = "https://token.actions.githubusercontent.com"

# The aggregate is a container with its own version. Each record inside keeps
# its own `schema`: 4 is the first without per-file hashes, which live in the
# manifest alone.
RECORDS_FORMAT = "gpgfrontend-build-records"
RECORDS_FORMAT_VERSION = 1
RECORD_SCHEMA = 4


class CandidateError(Exception):
    pass


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def dump_json(value):
    """Deterministic: sorted keys, fixed separators, one trailing newline."""
    return json.dumps(value, indent=2, sort_keys=True) + "\n"


def regular_files(directory):
    """Every entry of `directory`, which must all be plain regular files."""
    files = {}
    for entry in directory.iterdir():
        if entry.is_symlink() or not entry.is_file():
            raise CandidateError(f"{entry}: not a regular file")
        files[entry.name] = entry
    return files


def check_name(name, where):
    if not SAFE_NAME.match(name):
        raise CandidateError(f"{where}: unsafe file name {name!r}")
    if name in RESERVED:
        raise CandidateError(f"{where}: {name!r} is a reserved provenance name")


# --------------------------------------------------------------------- stage


def collect_legs(artifacts_root):
    """{leg: (deliverable dir, build info dir)}, paired by exact name."""
    deliverables, infos = {}, {}
    for entry in artifacts_root.iterdir():
        if entry.is_symlink() or not entry.is_dir():
            raise CandidateError(f"{entry}: expected one directory per artifact")
        if entry.name.startswith(DELIVERABLE_PREFIX):
            deliverables[entry.name[len(DELIVERABLE_PREFIX):]] = entry
        elif entry.name.startswith(BUILD_INFO_PREFIX):
            infos[entry.name[len(BUILD_INFO_PREFIX):]] = entry
        else:
            raise CandidateError(f"{entry.name}: not a release artifact")

    if not deliverables:
        raise CandidateError("no deliverable artifacts")

    unpaired = sorted(
        [f"{DELIVERABLE_PREFIX}{leg} (no build info)"
         for leg in deliverables.keys() - infos.keys()] +
        [f"{BUILD_INFO_PREFIX}{leg} (no deliverables)"
         for leg in infos.keys() - deliverables.keys()])
    if unpaired:
        raise CandidateError("unpaired artifacts: " + ", ".join(unpaired))

    return {leg: (deliverables[leg], infos[leg]) for leg in deliverables}


def load_build_info(info_dir):
    files = regular_files(info_dir)
    if set(files) != {BUILD_INFO}:
        raise CandidateError(
            f"{info_dir.name}: expected exactly {BUILD_INFO}, got {sorted(files)}")
    try:
        with files[BUILD_INFO].open(encoding="utf-8") as f:
            info = json.load(f)
    except ValueError as e:
        raise CandidateError(f"{info_dir.name}: malformed {BUILD_INFO} ({e})")
    if not isinstance(info, dict):
        raise CandidateError(f"{info_dir.name}: malformed {BUILD_INFO}")
    return info


def make_record(leg, info, names):
    missing = sorted({"project_version", "build_id", "host_os", "modules"}
                     - set(info))
    if missing:
        raise CandidateError(f"{BUILD_INFO_PREFIX}{leg}: missing {missing}")
    return {
        "schema": RECORD_SCHEMA,
        "leg": leg,
        "product": {
            "version": info["project_version"],
            "build_id": info["build_id"],
            "os": info["host_os"],
        },
        "modules": info["modules"],
        "artifacts": sorted(names),
    }


def build_aggregate(records):
    """The build-records.json document. Order of `records` does not matter."""
    legs = [r["leg"] for r in records]
    duplicates = sorted({leg for leg in legs if legs.count(leg) > 1})
    if duplicates:
        raise CandidateError(f"duplicate build records for {duplicates}")

    ordered = []
    for record in sorted(records, key=lambda r: r["leg"]):
        record = dict(record)
        record["artifacts"] = sorted(record["artifacts"])
        ordered.append(record)

    return {
        "format": RECORDS_FORMAT,
        "format_version": RECORDS_FORMAT_VERSION,
        "records": ordered,
    }


def render_manifest(hashes):
    """SHA256SUMS.txt text for {name: sha256}, in `sha256sum -c` format."""
    return "".join(f"{hashes[name]}  {name}\n" for name in sorted(hashes))


def stage(artifacts_root, out):
    # Everything is checked before anything is written: a refused input
    # leaves no half-built candidate behind.
    legs = collect_legs(artifacts_root)

    records, sources = [], {}
    owner = {}
    for leg in sorted(legs):
        deliverable_dir, info_dir = legs[leg]
        info = load_build_info(info_dir)

        files = regular_files(deliverable_dir)
        if not files:
            raise CandidateError(f"{deliverable_dir.name}: no deliverables")

        for name in sorted(files):
            check_name(name, deliverable_dir.name)
            if name in owner:
                raise CandidateError(
                    f"{name!r} is produced by both {owner[name]} and {leg}")
            owner[name] = leg
            sources[name] = files[name]

        records.append(make_record(leg, info, files))
    aggregate = dump_json(build_aggregate(records))

    try:
        out.mkdir(parents=True)
    except FileExistsError:
        raise CandidateError(f"{out}: already exists")

    # Hashed after the copy, so the manifest describes the bytes that ship.
    hashes = {}
    for name in sorted(sources):
        shutil.copyfile(sources[name], out / name)
        hashes[name] = digest(out / name)

    (out / RECORDS).write_text(aggregate, encoding="utf-8", newline="\n")
    hashes[RECORDS] = digest(out / RECORDS)

    (out / MANIFEST).write_text(render_manifest(hashes),
                                encoding="utf-8", newline="\n")

    print(f"staged {len(sources)} deliverable(s) from {len(legs)} leg(s)")
    for name in sorted(hashes):
        print(f"  {hashes[name]}  {name}")


# -------------------------------------------------------------------- verify


def cosign_verify(directory, identity, sha, cosign):
    """The Sigstore identity: this workflow, this commit, GitHub's issuer."""
    if not identity or not sha:
        raise CandidateError("an exact identity and workflow sha are required")
    for name in (MANIFEST, BUNDLE):
        path = directory / name
        if path.is_symlink() or not path.is_file():
            raise CandidateError(f"missing {name}")

    cmd = [
        cosign, "verify-blob",
        "--bundle", str(directory / BUNDLE),
        "--certificate-identity", identity,
        "--certificate-oidc-issuer", ISSUER,
        "--certificate-github-workflow-sha", sha,
        str(directory / MANIFEST),
    ]
    if subprocess.run(cmd).returncode != 0:
        raise CandidateError(f"{BUNDLE} does not verify {MANIFEST} "
                             f"as {identity} at {sha}")


def parse_manifest(data):
    """{name: sha256} from the signed bytes, strictly."""
    try:
        text = data.decode("ascii")
    except UnicodeDecodeError:
        raise CandidateError(f"{MANIFEST}: not ASCII")
    if "\r" in text:
        raise CandidateError(f"{MANIFEST}: carriage return")
    if not text.endswith("\n"):
        raise CandidateError(f"{MANIFEST}: no trailing newline")

    entries = {}
    for number, line in enumerate(text[:-1].split("\n"), 1):
        match = MANIFEST_LINE.match(line)
        if not match:
            raise CandidateError(f"{MANIFEST}:{number}: malformed line")
        sha, name = match.groups()
        if not SAFE_NAME.match(name):
            raise CandidateError(f"{MANIFEST}:{number}: unsafe name {name!r}")
        if name in RESERVED and name != RECORDS:
            raise CandidateError(f"{MANIFEST}:{number}: lists {name!r}")
        if name in entries:
            raise CandidateError(f"{MANIFEST}:{number}: duplicate {name!r}")
        entries[name] = sha

    if RECORDS not in entries:
        raise CandidateError(f"{MANIFEST}: does not cover {RECORDS}")
    return entries


def check_records(document, deliverables):
    """The records say which leg produced each deliverable, exactly once."""
    if not isinstance(document, dict) or \
       document.get("format") != RECORDS_FORMAT or \
       document.get("format_version") != RECORDS_FORMAT_VERSION:
        raise CandidateError(f"{RECORDS}: unexpected format")

    records = document.get("records")
    if not isinstance(records, list) or not records:
        raise CandidateError(f"{RECORDS}: no records")

    legs, claimed = set(), {}
    for record in records:
        if record.get("schema") != RECORD_SCHEMA:
            raise CandidateError(f"{RECORDS}: unexpected record schema")
        leg = record.get("leg")
        if leg in legs:
            raise CandidateError(f"{RECORDS}: duplicate leg {leg!r}")
        legs.add(leg)
        for name in record.get("artifacts", []):
            if name in claimed:
                raise CandidateError(
                    f"{RECORDS}: {name!r} claimed by {claimed[name]} and {leg}")
            claimed[name] = leg

    if set(claimed) != deliverables:
        missing = sorted(deliverables - set(claimed))
        extra = sorted(set(claimed) - deliverables)
        raise CandidateError(
            f"{RECORDS} disagrees with {MANIFEST}: "
            f"unrecorded {missing}, not in manifest {extra}")


def verify(directory, identity, sha, cosign="cosign"):
    """Every check publication depends on. Returns the files to publish."""
    # 1. The signature, before a single byte of the manifest is trusted.
    cosign_verify(directory, identity, sha, cosign)

    # 2. The manifest, strictly: anything sha256sum -c would read loosely is
    #    refused rather than interpreted.
    entries = parse_manifest((directory / MANIFEST).read_bytes())

    # 3. Exact set equality. Nothing unlisted is published, nothing listed is
    #    missing.
    present = set(regular_files(directory)) - {MANIFEST, BUNDLE}
    if present != set(entries):
        missing = sorted(set(entries) - present)
        extra = sorted(present - set(entries))
        raise CandidateError(
            f"candidate does not match {MANIFEST}: "
            f"missing {missing}, unlisted {extra}")

    # 4. Every byte.
    for name in sorted(entries):
        if digest(directory / name) != entries[name]:
            raise CandidateError(f"{name}: sha256 does not match {MANIFEST}")

    # 5. The records, now that their bytes are the signed ones.
    try:
        with (directory / RECORDS).open(encoding="utf-8") as f:
            check_records(json.load(f), set(entries) - {RECORDS})
    except (ValueError, AttributeError, TypeError) as e:
        raise CandidateError(f"{RECORDS}: malformed ({e})")

    return [directory / name for name in sorted(entries)] + \
        [directory / MANIFEST, directory / BUNDLE]


# ---------------------------------------------------------------------- main


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("stage", help="write the release candidate")
    p.add_argument("--artifacts", required=True, type=Path)
    p.add_argument("--out", required=True, type=Path)

    p = sub.add_parser("verify", help="verify the release candidate")
    p.add_argument("--dir", required=True, type=Path)
    p.add_argument("--identity", required=True)
    p.add_argument("--sha", required=True)
    p.add_argument("--cosign", default="cosign")
    p.add_argument("--github-output", type=Path,
                   help="append the verified file list as output `files`")

    args = parser.parse_args(argv)
    try:
        if args.command == "stage":
            stage(args.artifacts, args.out)
            return 0

        files = verify(args.dir, args.identity, args.sha, args.cosign)
    except CandidateError as e:
        print(f"release_candidate: {e}", file=sys.stderr)
        return 1

    print(f"verified {len(files)} file(s):")
    for path in files:
        print(f"  {path.name}")
    if args.github_output:
        with args.github_output.open("a", encoding="utf-8") as f:
            f.write("files<<EOF_RELEASE_FILES\n")
            for path in files:
                f.write(f"{os.path.abspath(path)}\n")
            f.write("EOF_RELEASE_FILES\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
