# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The payload of an MSI, read and replaced through msitools (msiinfo,
msibuild): its one embedded cabinet, by the File table's keys, and where
each file installs to.

Replacing a file's bytes means three things agree again afterwards: the
cabinet, the File table's FileSize, and, for a file without a version
resource, its MsiFileHash row (the MD5 Windows Installer compares before
it overwrites an installed copy). msibuild can say nothing about a stream
it failed to load, so everything written is read back and compared.
"""

from __future__ import annotations

import hashlib
import struct
import tempfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

import cab
from common import ReleaseError, run

# MSI stores a NULL integer as this value, so a hash part can never be it.
_NULL_INT = -0x80000000


def table(msi: Path, name: str) -> list[dict]:
    """The rows of table @p name (none when the MSI has no such table)."""
    if name not in run(["msiinfo", "tables", msi]).stdout.split():
        return []
    lines = run(["msiinfo", "export", msi, name]).stdout.split("\n")
    if len(lines) < 3:
        raise ReleaseError(f"{msi.name}: table {name} is unreadable")
    columns = lines[0].split("\t")
    rows = []
    for line in lines[3:]:
        if line:
            cells = line.split("\t")
            rows.append(dict(zip(columns, cells + [""] * len(columns))))
    return rows


def hash_parts(data: bytes) -> tuple[int, int, int, int]:
    """MsiFileHash's four columns: the MD5, as signed little-endian words."""
    return struct.unpack("<4i", hashlib.md5(data).digest())


def _quote(value: str) -> str:
    if "'" in value:
        raise ReleaseError(f"MSI key {value!r} cannot be quoted")
    return f"'{value}'"


@dataclass
class Payload:
    """The files of an MSI's cabinet, by File key, with where each one
    installs (a relative path in the target tree, "/"-separated)."""
    msi: Path
    stream: str
    entries: list[cab.Entry]
    paths: dict[str, str]
    hashed: set[str]    # File keys with an MsiFileHash row

    @classmethod
    def read(cls, msi: Path) -> "Payload":
        media = [m for m in table(msi, "Media") if m.get("Cabinet")]
        if len(media) != 1:
            raise ReleaseError(f"{msi.name}: {len(media)} cabinets in its "
                               "Media table, not one")
        stream = media[0]["Cabinet"]
        if not stream.startswith("#"):
            raise ReleaseError(f"{msi.name}: its cabinet {stream} is not "
                               "embedded")
        stream = stream[1:]
        raw = run(["msiinfo", "extract", msi, stream], text=False).stdout
        entries = cab.read(raw, f"{msi.name}:{stream}")

        directories = {d["Directory"]: d for d in table(msi, "Directory")}
        components = {c["Component"]: c for c in table(msi, "Component")}

        def target(name: str) -> str:
            # "short|long", and "target:source" for a directory.
            name = name.split(":", 1)[0]
            return name.split("|", 1)[-1]

        def directory_path(key: str) -> PurePosixPath:
            parts, seen = [], set()
            while key and key not in seen:
                seen.add(key)
                d = directories.get(key)
                if d is None:
                    break
                name = target(d["DefaultDir"])
                # The roots (TARGETDIR = "SourceDir", ".") are no folder.
                if d.get("Directory_Parent") and name not in (".", ""):
                    parts.append(name)
                key = d.get("Directory_Parent", "")
            return PurePosixPath(*reversed(parts))

        paths = {}
        for f in table(msi, "File"):
            component = components.get(f["Component_"])
            if component is None:
                raise ReleaseError(f"{msi.name}: file {f['File']} has no "
                                   "component")
            paths[f["File"]] = str(directory_path(component["Directory_"]) /
                                   target(f["FileName"]))
        listed = {e.name for e in entries}
        if listed != set(paths):
            raise ReleaseError(f"{msi.name}: its cabinet and File table "
                               "list different files")
        hashed = {h["File_"] for h in table(msi, "MsiFileHash")}
        return cls(msi, stream, entries, paths, hashed)

    def files(self) -> dict[str, bytes]:
        """Every file's bytes, by the path it installs to."""
        return {self.paths[e.name]: e.data for e in self.entries}

    def replace(self, new: dict[str, bytes], out: Path) -> None:
        """Writes the MSI to @p out with the files at the paths in @p new
        replaced, and checks the result reads back as exactly that."""
        by_path = {p: k for k, p in self.paths.items()}
        unknown = set(new) - set(by_path)
        if unknown:
            raise ReleaseError(f"{self.msi.name}: holds no {sorted(unknown)}")
        changed = {by_path[p]: data for p, data in new.items()}
        entries = [cab.Entry(e.name, changed.get(e.name, e.data), e.attribs,
                             e.date, e.time) for e in self.entries]
        queries = []
        for key, data in sorted(changed.items()):
            queries.append(f"UPDATE `File` SET `FileSize` = {len(data)} "
                           f"WHERE `File` = {_quote(key)}")
            if key in self.hashed:
                parts = hash_parts(data)
                if _NULL_INT in parts:
                    raise ReleaseError(f"{self.paths[key]}: its MD5 cannot "
                                       "be stored in an MsiFileHash row")
                queries.append(
                    "UPDATE `MsiFileHash` SET " + ", ".join(
                        f"`HashPart{i}` = {v}" for i, v in
                        enumerate(parts, 1)) +
                    f" WHERE `File_` = {_quote(key)}")

        packed = cab.write(entries)
        out.write_bytes(self.msi.read_bytes())
        with tempfile.TemporaryDirectory(prefix="gfr-msi-") as tmp:
            cab_file = Path(tmp) / "payload.cab"
            cab_file.write_bytes(packed)
            run(["msibuild", out, "-a", self.stream, cab_file])
        for q in queries:
            run(["msibuild", out, "-q", q])

        again = Payload.read(out)
        expected = self.files() | new
        if again.files() != expected:
            raise ReleaseError(f"{out.name}: the rewritten payload does not "
                               "read back as written")
        for key in changed.keys() & self.hashed:
            row = next(h for h in table(out, "MsiFileHash")
                       if h["File_"] == key)
            if tuple(int(row[f"HashPart{i}"]) for i in range(1, 5)) != \
                    hash_parts(changed[key]):
                raise ReleaseError(f"{out.name}: {self.paths[key]}'s "
                                   "MsiFileHash did not take")
        sizes = {f["File"]: int(f["FileSize"]) for f in table(out, "File")}
        if any(sizes[k] != len(d) for k, d in changed.items()):
            raise ReleaseError(f"{out.name}: a FileSize did not take")
