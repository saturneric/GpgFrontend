# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The signed update index: v1/index.json, the one thing clients trust.

Built from the release policy and the promoted descriptors, signed with the
product's index key (trusted comment "product=<slug> serial=<n>"), and
published to the update store, then the mirrors. A run whose index would say
nothing new, and is not near expiry, publishes nothing; `resign` publishes
regardless, to push `expires` out.

The serial only grows: it is one more than both the host's own record and the
published index, so a client that has seen serial n never accepts an older
index again (its anti-rollback check), even after the host lost its state.
"""

from __future__ import annotations

import datetime as dt
import json
import tempfile
from pathlib import Path

import authenticode
import imported
import minisig
import openpgp
from common import (BUNDLE, DESCRIPTOR, INDEX, SCHEMA, SIG, SUMS, Product,
                    ReleaseError, git_version, run,
                    canonical_json, check_relpath, iso, parse_iso, semver,
                    sha256_bytes, utcnow)
from store import Store
from verify import parse_sums

import release_candidate as rc  # noqa: E402  (scripts/, on the path via common)

LIFETIME = dt.timedelta(days=14)
RESIGN_BEFORE = dt.timedelta(days=7)
VOLATILE = ("serial", "generated_at", "expires")


def admit_nightly(product: Product, builds: Store, path: str, raw: bytes,
                  artifact_pub: str, tmp: Path) -> dict:
    """A nightly descriptor is listed only if it is this product's, says what
    it is consistently, and its manifest carries the artifact key's signature
    over exactly the hashes it states."""
    d = json.loads(raw)
    prefix = path.rsplit("/", 1)[0]
    if d.get("schema") != SCHEMA or d.get("product") != product.slug:
        raise ReleaseError(f"{path}: not a {product.slug} descriptor")
    if d.get("kind") != "nightly" or not d.get("ref", "").startswith("refs/heads/"):
        raise ReleaseError(f"{path}: not a nightly")
    if d["signing"] != {"scheme": "minisign",
                        "key_id": minisig.key_id(artifact_pub)}:
        raise ReleaseError(f"{path}: not signed with the artifact key")
    if prefix != f"nightly/{str(semver.parse(d['version'])).replace('+', '-')}":
        raise ReleaseError(f"{path}: published under another version's name")
    for p in [d["sums"], d["sums_sig"]] + [a[k] for a in d["artifacts"]
                                           for k in ("path", "sig")]:
        if not check_relpath(p).startswith(prefix + "/"):
            raise ReleaseError(f"{path}: {p} is outside its own release")

    sums, sig = tmp / "SHA256SUMS.txt", tmp / "SHA256SUMS.txt.minisig"
    sums.write_bytes(builds.read(d["sums"]))
    sig.write_bytes(builds.read(d["sums_sig"]))
    minisig.verify([artifact_pub], sums, sig)
    listed = parse_sums(sums.read_bytes())
    if listed != {a["name"]: a["sha256"] for a in d["artifacts"]}:
        raise ReleaseError(f"{path}: artifacts differ from the signed manifest")
    if any(a["sha256"] != a["build_sha256"] or a["transform"] != "none"
           for a in d["artifacts"]):
        raise ReleaseError(f"{path}: a nightly is published as built")
    return d


def release_notes(git_dir: Path, version: str) -> bytes | None:
    """A release's notes as reviewed: main's copy (where a typo is fixed after
    the release) or, without one, the tagged tree's."""
    for ref in ("refs/heads/main", f"v{version}"):
        out = run(["git", "-C", git_dir, "show",
                   f"{ref}:release/notes/{version}.md"], check=False, text=False)
        if out.returncode == 0 and out.stdout.strip():
            return out.stdout
    return None


def admit_stable(product: Product, builds: Store, path: str, raw: bytes, *,
                 git_dir: Path, state_dir: Path, cosign: str, tmp: Path,
                 authenticode_ca: str | None = None) -> tuple[dict, dict]:
    """A stable release is listed only with everything the offline stage
    promises (scripts/release/gfrelease/stable.py), checked again here.

    Returns the descriptor and its index entry. What was checked is recorded
    under the state directory, keyed by the descriptor's and the notes'
    hashes: a release is checked again only when either changes, not on every
    round (checking an installer means downloading it).
    """
    d = json.loads(raw)
    version = d.get("version", "")
    prefix = f"releases/{version}"
    if d.get("schema") != SCHEMA or d.get("product") != product.slug or \
            d.get("kind") != "stable" or path != f"{prefix}/{DESCRIPTOR}":
        raise ReleaseError(f"{path}: not a {product.slug} stable release")
    v = semver.parse(version)
    if v.prerelease or v.build:
        raise ReleaseError(f"{path}: {version} is not a final release")
    tag = f"v{version}"
    if d.get("ref") != f"refs/tags/{tag}":
        raise ReleaseError(f"{path}: not built from {tag}")
    signing = d.get("signing", {})
    if signing.get("scheme") != "openpgp" or \
            signing.get("fingerprint", "").upper() not in product.maintainers:
        raise ReleaseError(f"{path}: not signed with a pinned maintainer key")
    for p in [d["sums"], d["sums_sig"], d["build"]["sums"],
              d["build"]["bundle"]] + [a[k] for a in d["artifacts"]
                                       for k in ("path", "sig")]:
        if not check_relpath(p).startswith(prefix + "/"):
            raise ReleaseError(f"{path}: {p} is outside its own release")

    notes = release_notes(git_dir, version)
    if notes is None:
        raise ReleaseError(f"{tag}: no release notes to publish with it")
    notes_sha = sha256_bytes(notes)
    notes_path = f"notes/{version}/{notes_sha}.md"
    entry = {"descriptor": path, "sha256": sha256_bytes(raw),
             "notes": notes_path, "notes_sha256": notes_sha,
             "notes_sig": notes_path + SIG}

    record = state_dir / "admitted" / f"{version}.json"
    if record.exists() and json.loads(record.read_text()) == entry:
        return d, entry

    work = tmp / version
    work.mkdir(parents=True, exist_ok=True)
    with openpgp.Keyring(product.keyring_path, product.maintainers) as ring:
        # The tag: a maintainer's, and the commit that was built.
        ring.verify_tag(git_dir, tag)
        tagged = run(["git", "-C", git_dir, "rev-parse",
                      f"{tag}^{{commit}}"]).stdout.strip()
        if tagged != d["commit"]:
            raise ReleaseError(f"{path}: {tag} is {tagged}, not {d['commit']}")

        # The final manifest, signed by a maintainer, and exactly the
        # descriptor's hashes.
        for name, rel in ((SUMS, d["sums"]), (SUMS + SIG, d["sums_sig"])):
            (work / name).write_bytes(builds.read(rel))
        ring.verify(work / SUMS, work / (SUMS + SIG))
        if parse_sums((work / SUMS).read_bytes()) != \
                {a["name"]: a["sha256"] for a in d["artifacts"]}:
            raise ReleaseError(f"{path}: artifacts differ from the signed "
                               "manifest")

        # The notes, signed by a maintainer.
        if not builds.exists(notes_path) or not builds.exists(notes_path + SIG):
            raise ReleaseError(
                f"{tag}: the notes main has for it are not signed and uploaded "
                f"yet (gfrelease stable {version} --notes-only)")
        (work / "notes.md").write_bytes(builds.read(notes_path))
        (work / "notes.md.sig").write_bytes(builds.read(notes_path + SIG))
        if sha256_bytes((work / "notes.md").read_bytes()) != notes_sha:
            raise ReleaseError(f"{notes_path}: not the reviewed notes")
        ring.verify(work / "notes.md", work / "notes.md.sig")

    # What CI built, as Sigstore says: every file's build hash.
    build = work / "build"
    build.mkdir(exist_ok=True)
    (build / SUMS).write_bytes(builds.read(d["build"]["sums"]))
    (build / BUNDLE).write_bytes(builds.read(d["build"]["bundle"]))
    try:
        rc.cosign_verify(build, product.identity(d["ref"]), d["commit"], cosign)
        built = rc.parse_manifest((build / SUMS).read_bytes())
    except rc.CandidateError as e:
        raise ReleaseError(f"{path}: {e}") from e
    for a in d["artifacts"]:
        if built.get(a["name"]) != a["build_sha256"]:
            raise ReleaseError(f"{a['name']}: not what CI built")
        if a["transform"] == "none":
            if a["sha256"] != a["build_sha256"]:
                raise ReleaseError(f"{a['name']}: changed without a transform")
        elif a["transform"] == "authenticode":
            local = work / a["name"]
            local.write_bytes(builds.read(a["path"]))
            if sha256_bytes(local.read_bytes()) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the published bytes")
            authenticode.verify(local, product.authenticode_subject,
                                authenticode_ca)
            local.unlink()
        else:
            raise ReleaseError(f"{a['name']}: unknown transform "
                               f"{a['transform']!r}")

    record.parent.mkdir(parents=True, exist_ok=True)
    record.write_text(json.dumps(entry, sort_keys=True))
    return d, entry


def admit_imported(product: Product, builds: Store, path: str, raw: bytes, *,
                   git_dir: Path, state_dir: Path,
                   tmp: Path) -> tuple[dict, dict]:
    """A release from before this pipeline (gfrelease import): listed when
    every file verifies against its own maintainer signature, made when it
    was released, and its tag is the commit git calls that version.

    There is no signed manifest to check instead, so every file is
    downloaded and checked, once: the result is recorded under the state
    directory by the descriptor's hash, like a stable release's.
    """
    d = json.loads(raw)
    version = d.get("version", "")
    prefix = f"releases/{version}"
    if d.get("schema") != SCHEMA or d.get("product") != product.slug or \
            d.get("kind") != imported.KIND or \
            path != f"{prefix}/{DESCRIPTOR}":
        raise ReleaseError(f"{path}: not a {product.slug} imported release")
    v = semver.parse(version)
    if v.prerelease or v.build:
        raise ReleaseError(f"{path}: {version} is not a final release")
    floor = semver.parse(product.policy["signing"]["legacy_unsigned_before"])
    if v < floor:
        raise ReleaseError(f"{path}: {version} is older than {floor}")
    tag = f"v{version}"
    if d.get("ref") != f"refs/tags/{tag}":
        raise ReleaseError(f"{path}: not the release tagged {tag}")
    if d.get("signing", {}).get("scheme") != "openpgp" or \
            d["signing"].get("fingerprint", "").upper() not in product.maintainers:
        raise ReleaseError(f"{path}: not signed with a pinned maintainer key")
    if not d.get("artifacts"):
        raise ReleaseError(f"{path}: lists no files")
    for a in d["artifacts"]:
        for k in ("path", "sig"):
            if not check_relpath(a[k]).startswith(prefix + "/"):
                raise ReleaseError(f"{path}: {a[k]} is outside its own release")
        if product.classify(a["name"], version) != \
                {k: a[k] for k in ("os", "arch", "kind", "flavor", "min_os")
                 if k in a}:
            raise ReleaseError(f"{a['name']}: not what the artifact rules "
                               "call it")

    entry = {"descriptor": path, "sha256": sha256_bytes(raw)}
    record = state_dir / "admitted" / f"{version}.json"
    if record.exists() and json.loads(record.read_text()) == entry:
        return d, entry

    tagged = run(["git", "-C", git_dir, "rev-parse", f"{tag}^{{commit}}"],
                 check=False).stdout.strip()
    if tagged != d.get("commit"):
        raise ReleaseError(f"{path}: {tag} is {tagged or 'missing'}, not "
                           f"{d.get('commit')}")
    if git_version(git_dir, tagged, f"refs/tags/{tag}") != version:
        raise ReleaseError(f"{path}: git does not call {tagged} {version}")

    work = tmp / version
    work.mkdir(parents=True, exist_ok=True)
    with openpgp.Keyring(product.keyring_path, product.maintainers) as ring:
        for a in d["artifacts"]:
            local, sig = work / a["name"], work / (a["name"] + SIG)
            local.write_bytes(builds.read(a["path"]))
            sig.write_bytes(builds.read(a["sig"]))
            if sha256_bytes(local.read_bytes()) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the descriptor's bytes")
            ring.verify(local, sig)
            local.unlink()
            sig.unlink()

    record.parent.mkdir(parents=True, exist_ok=True)
    record.write_text(json.dumps(entry, sort_keys=True))
    return d, entry


def build(product: Product, builds: Store, artifact_pub: str, *,
          git_dir: Path | None = None, state_dir: Path | None = None,
          cosign: str = "cosign", authenticode_ca: str | None = None) -> dict:
    """The index's content, without serial and times."""
    nightly = None
    releases: dict[str, dict] = {}
    with tempfile.TemporaryDirectory(prefix="gfr-index-") as tmp:
        for rel in builds.files("nightly", include=f"*/{DESCRIPTOR}"):
            path = f"nightly/{rel}"
            raw = builds.read(path)
            d = admit_nightly(product, builds, path, raw, artifact_pub, Path(tmp))
            # Only main's nightlies make the channel; release/X.Y builds are
            # published for testers who fetch them by hand.
            if d["ref"] != "refs/heads/main":
                continue
            if nightly is None or semver.parse(d["version"]) > \
                    semver.parse(nightly["version"]):
                nightly = {"version": d["version"], "descriptor": path,
                           "sha256": sha256_bytes(raw)}

        for rel in builds.files("releases", include=f"*/{DESCRIPTOR}"):
            path = f"releases/{rel}"
            if git_dir is None or state_dir is None:
                raise ReleaseError("stable releases need --git-dir and "
                                   "--state-dir to be admitted")
            raw = builds.read(path)
            if json.loads(raw).get("kind") == imported.KIND:
                d, entry = admit_imported(product, builds, path, raw,
                                          git_dir=git_dir, state_dir=state_dir,
                                          tmp=Path(tmp))
            else:
                d, entry = admit_stable(product, builds, path, raw,
                                        git_dir=git_dir, state_dir=state_dir,
                                        cosign=cosign, tmp=Path(tmp),
                                        authenticode_ca=authenticode_ca)
            releases[d["version"]] = entry

    series = {}
    for key, entry in product.policy["series"].items():
        series[key] = {k: entry.get(k) for k in ("status", "eol", "successor")}
        series[key]["platforms"] = entry.get("platforms", {})
        series[key]["latest"] = None
        if series[key]["eol"] is not None:
            series[key]["eol"] = str(series[key]["eol"])

    # Each series' newest release; the stable channel is the newest release
    # of the series users are meant to be on (stable, or mainline once its .0
    # shipped and before the policy moved on). Betas join with their own path.
    withdrawn = {w["version"] for w in product.policy.get("withdrawn", [])}
    for version in sorted(releases, key=semver.parse):
        if version in withdrawn:
            continue
        key = semver.parse(version).series
        if key in series:
            series[key]["latest"] = version
    current = [s["latest"] for s in series.values()
               if s["status"] in ("stable", "mainline") and s["latest"]]
    stable = max(current, key=semver.parse) if current else None

    return {
        "schema": SCHEMA, "product": product.slug,
        "series": series,
        "channels": {"stable": stable, "beta": stable,
                     "nightly": nightly["version"] if nightly else None},
        "withdrawn": [{"version": w["version"], "reason": w["reason"]}
                      for w in product.policy.get("withdrawn", [])],
        "releases": releases,
        "nightly": nightly,
    }


def published(update: Store) -> dict | None:
    if not update.exists(INDEX):
        return None
    return json.loads(update.read(INDEX))


def read_serial(state_dir: Path) -> int:
    path = state_dir / "serial"
    return int(path.read_text()) if path.exists() else 0


def publish(product: Product, builds: Store, update: Store, *,
            index_key: Path, artifact_pub: str, state_dir: Path,
            force: bool = False, mirrors: list[Store] = (),
            git_dir: Path | None = None, cosign: str = "cosign",
            authenticode_ca: str | None = None) -> dict | None:
    content = build(product, builds, artifact_pub, git_dir=git_dir,
                    state_dir=state_dir, cosign=cosign,
                    authenticode_ca=authenticode_ca)
    previous = published(update)
    now = utcnow()
    if previous is not None and not force:
        same = {k: v for k, v in previous.items() if k not in VOLATILE} == content
        if same and parse_iso(previous["expires"]) - now > RESIGN_BEFORE:
            return None

    state_dir.mkdir(parents=True, exist_ok=True)
    serial = max(read_serial(state_dir),
                 previous["serial"] if previous else 0) + 1
    index = dict(content, serial=serial, generated_at=iso(now),
                 expires=iso(now + LIFETIME))

    with tempfile.TemporaryDirectory(prefix="gfr-sign-") as tmp:
        local = Path(tmp) / "index.json"
        local.write_bytes(canonical_json(index))
        sig = minisig.sign(index_key, local,
                           f"product={product.slug} serial={serial}")
        # Recorded before the upload: a crash after it can only skip a
        # serial, never reuse one.
        (state_dir / "serial").write_text(f"{serial}\n")
        for store in [update, *mirrors]:
            store.put_file(local, INDEX, immutable=False)
            store.put_file(sig, INDEX + ".minisig", immutable=False)
    return index
