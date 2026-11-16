# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The payload of an MSI, read and replaced: its one embedded cabinet, by
the File table's keys, and where each file installs to.

Replacing a file's bytes means three things agree again afterwards: the
cabinet, the File table's FileSize, and, for a file without a version
resource, its MsiFileHash row (the MD5 Windows Installer compares before
it overwrites an installed copy). Neither way into the database can say
much about a stream it failed to store, so everything written is read back
and compared.

The database itself is opened by what the platform has (backend()):

  Windows    Windows Installer's own msi.dll, through ctypes: no SDK tool
             (msidb, WiStream.vbs) need be installed
  elsewhere  msitools (msiinfo, msibuild)

Both read the same tables and streams and write the same three things.
"""

from __future__ import annotations

import ctypes
import hashlib
import struct
import tempfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

import cab
from common import WINDOWS, ReleaseError, run

# MSI stores a NULL integer as this value, so a hash part can never be it.
_NULL_INT = -0x80000000


@dataclass(frozen=True)
class Update:
    """Integer columns of the one row of @p table whose @p key_column is
    @p key, set to @p values."""
    table: str
    values: tuple[tuple[str, int], ...]
    key_column: str
    key: str


def _quote(value: str) -> str:
    if "'" in value:
        raise ReleaseError(f"MSI key {value!r} cannot be quoted")
    return f"'{value}'"


class _Msitools:
    """msiinfo and msibuild, from msitools."""

    def table(self, msi: Path, name: str) -> list[dict]:
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

    def stream(self, msi: Path, name: str) -> bytes:
        return run(["msiinfo", "extract", msi, name], text=False).stdout

    def rewrite(self, msi: Path, stream: str, data: bytes,
                updates: list[Update]) -> None:
        with tempfile.TemporaryDirectory(prefix="gfr-msi-") as tmp:
            cab_file = Path(tmp) / "payload.cab"
            cab_file.write_bytes(data)
            run(["msibuild", msi, "-a", stream, cab_file])
        for u in updates:
            run(["msibuild", msi, "-q",
                 f"UPDATE `{u.table}` SET " + ", ".join(
                     f"`{column}` = {value}" for column, value in u.values) +
                 f" WHERE `{u.key_column}` = {_quote(u.key)}"])


class _Handle:
    """A database, a view on it or a record of msi.dll: closed when left."""

    def __init__(self, dll, handle: int):
        self.dll, self.handle = dll, handle

    def __enter__(self) -> int:
        return self.handle

    def __exit__(self, *exc) -> None:
        self.dll.MsiCloseHandle(self.handle)


class _WindowsInstaller:
    """msi.dll, the database functions of Windows Installer."""

    _NO_MORE_ITEMS, _MORE_DATA = 259, 234
    _READONLY, _TRANSACT = 0, 1      # MSIDBOPEN_*, passed in place of a path
    _ASSIGN = 3                      # MSIMODIFY_ASSIGN: insert or replace

    def __init__(self):
        self.dll = dll = ctypes.WinDLL("msi.dll")
        handle, out = ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong)
        wstr, uint = ctypes.c_wchar_p, ctypes.c_uint
        for name, args in {
                "MsiOpenDatabaseW": (wstr, ctypes.c_void_p, out),
                "MsiDatabaseOpenViewW": (handle, wstr, out),
                "MsiViewExecute": (handle, handle),
                "MsiViewFetch": (handle, out),
                "MsiViewGetColumnInfo": (handle, ctypes.c_int, out),
                "MsiViewModify": (handle, ctypes.c_int, handle),
                "MsiDatabaseCommit": (handle,),
                "MsiRecordGetStringW": (handle, uint, wstr, out),
                "MsiRecordReadStream": (handle, uint, ctypes.c_char_p, out),
                "MsiRecordSetStringW": (handle, uint, wstr),
                "MsiRecordSetInteger": (handle, uint, ctypes.c_int),
                "MsiRecordSetStreamW": (handle, uint, wstr),
                "MsiFormatRecordW": (handle, handle, wstr, out),
                "MsiCloseHandle": (handle,),
                "MsiCreateRecord": (uint,),
                "MsiRecordGetFieldCount": (handle,),
                "MsiRecordDataSize": (handle, uint),
                "MsiGetLastErrorRecord": ()}.items():
            getattr(dll, name).argtypes = args
            getattr(dll, name).restype = uint

    # --- handles -----------------------------------------------------------

    def _failed(self, msi: Path, doing: str, code: int) -> ReleaseError:
        detail = ""
        record = self.dll.MsiGetLastErrorRecord()
        if record:
            try:
                detail = self._text(
                    lambda buf, size: self.dll.MsiFormatRecordW(
                        0, record, buf, size))
            except ReleaseError:
                pass
            self.dll.MsiCloseHandle(record)
        detail = detail or ctypes.FormatError(code).strip()
        return ReleaseError(f"{msi.name}: {doing}: {detail} (error {code})")

    def _text(self, call) -> str:
        """What a function that fills a buffer returns, asked for its
        length first."""
        size = ctypes.c_ulong(0)
        code = call(ctypes.create_unicode_buffer(1), ctypes.byref(size))
        if code not in (0, self._MORE_DATA):
            raise ReleaseError(f"Windows Installer error {code}")
        size = ctypes.c_ulong(size.value + 1)
        buf = ctypes.create_unicode_buffer(size.value)
        code = call(buf, ctypes.byref(size))
        if code:
            raise ReleaseError(f"Windows Installer error {code}")
        return buf.value

    def _database(self, msi: Path, mode: int) -> _Handle:
        h = ctypes.c_ulong()
        code = self.dll.MsiOpenDatabaseW(str(msi), ctypes.c_void_p(mode),
                                         ctypes.byref(h))
        if code:
            raise self._failed(msi, "cannot be opened as an MSI", code)
        return _Handle(self.dll, h.value)

    def _record(self, msi: Path, values) -> _Handle:
        record = self.dll.MsiCreateRecord(len(values))
        if not record:
            raise ReleaseError(f"{msi.name}: Windows Installer made no record")
        for n, value in enumerate(values, 1):
            if isinstance(value, int):
                code = self.dll.MsiRecordSetInteger(record, n, value)
            else:
                code = self.dll.MsiRecordSetStringW(record, n, value)
            if code:
                self.dll.MsiCloseHandle(record)
                raise self._failed(msi, "setting a query's values", code)
        return _Handle(self.dll, record)

    def _view(self, msi: Path, db: int, sql: str, *params) -> _Handle:
        """@p sql executed, its "?" markers bound to @p params in order."""
        h = ctypes.c_ulong()
        code = self.dll.MsiDatabaseOpenViewW(db, sql, ctypes.byref(h))
        if code:
            raise self._failed(msi, f"query {sql!r}", code)
        with self._record(msi, params) as record:
            code = self.dll.MsiViewExecute(h.value, record)
        if code:
            failed = self._failed(msi, f"query {sql!r}", code)
            self.dll.MsiCloseHandle(h.value)
            raise failed
        return _Handle(self.dll, h.value)

    def _fetch(self, msi: Path, view: int):
        """The view's rows in turn, each a record closed before the next."""
        while True:
            h = ctypes.c_ulong()
            code = self.dll.MsiViewFetch(view, ctypes.byref(h))
            if code == self._NO_MORE_ITEMS:
                return
            if code:
                raise self._failed(msi, "reading a row", code)
            with _Handle(self.dll, h.value) as record:
                yield record

    def _strings(self, record: int) -> list[str]:
        return [self._text(lambda buf, size, n=n:
                           self.dll.MsiRecordGetStringW(record, n, buf, size))
                for n in range(1, self.dll.MsiRecordGetFieldCount(record) + 1)]

    # --- what Payload asks -------------------------------------------------

    def table(self, msi: Path, name: str) -> list[dict]:
        with self._database(msi, self._READONLY) as db:
            with self._view(msi, db, "SELECT `Name` FROM `_Tables` WHERE "
                            "`Name` = ?", name) as view:
                if not list(self._fetch(msi, view)):
                    return []
            with self._view(msi, db, f"SELECT * FROM `{name}`") as view:
                h = ctypes.c_ulong()
                code = self.dll.MsiViewGetColumnInfo(view, 0,
                                                     ctypes.byref(h))
                if code:
                    raise self._failed(msi, f"table {name} is unreadable",
                                       code)
                with _Handle(self.dll, h.value) as names:
                    columns = self._strings(names)
                return [dict(zip(columns, self._strings(record)))
                        for record in self._fetch(msi, view)]

    def stream(self, msi: Path, name: str) -> bytes:
        with self._database(msi, self._READONLY) as db, \
                self._view(msi, db, "SELECT `Data` FROM `_Streams` WHERE "
                           "`Name` = ?", name) as view:
            for record in self._fetch(msi, view):
                size = self.dll.MsiRecordDataSize(record, 1)
                buf = ctypes.create_string_buffer(size)
                got = ctypes.c_ulong(size)
                code = self.dll.MsiRecordReadStream(record, 1, buf,
                                                    ctypes.byref(got))
                if code or got.value != size:
                    raise self._failed(msi, f"reading its stream {name}",
                                       code)
                return buf.raw
        raise ReleaseError(f"{msi.name}: holds no stream {name}")

    def rewrite(self, msi: Path, stream: str, data: bytes,
                updates: list[Update]) -> None:
        with tempfile.TemporaryDirectory(prefix="gfr-msi-") as tmp:
            cab_file = Path(tmp) / "payload.cab"
            cab_file.write_bytes(data)
            # The database reads the file when it commits, so all of it
            # happens while the directory is there, as one transaction.
            with self._database(msi, self._TRANSACT) as db:
                with self._view(msi, db, "SELECT `Name`, `Data` FROM "
                                "`_Streams`") as view, \
                        self._record(msi, (stream, "")) as record:
                    code = self.dll.MsiRecordSetStreamW(record, 2,
                                                        str(cab_file))
                    code = code or self.dll.MsiViewModify(
                        view, self._ASSIGN, record)
                    if code:
                        raise self._failed(msi, "replacing its stream "
                                           f"{stream}", code)
                for u in updates:
                    # Bound, not written into the statement: MSI's SQL has
                    # no negative literal, and a hash part is negative half
                    # the time.
                    sql = f"UPDATE `{u.table}` SET " + ", ".join(
                        f"`{column}` = ?" for column, _ in u.values) + \
                        f" WHERE `{u.key_column}` = ?"
                    with self._view(msi, db, sql,
                                    *(v for _, v in u.values), u.key):
                        pass
                code = self.dll.MsiDatabaseCommit(db)
                if code:
                    raise self._failed(msi, "saving it", code)


_BACKEND = None


def backend():
    """How this platform opens an MSI database."""
    global _BACKEND
    if _BACKEND is None:
        _BACKEND = _WindowsInstaller() if WINDOWS else _Msitools()
    return _BACKEND


def table(msi: Path, name: str) -> list[dict]:
    """The rows of table @p name (none when the MSI has no such table),
    every value as text."""
    return backend().table(msi, name)


def hash_parts(data: bytes) -> tuple[int, int, int, int]:
    """MsiFileHash's four columns: the MD5, as signed little-endian words."""
    return struct.unpack("<4i", hashlib.md5(data).digest())


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
        """The payload of @p msi; one whose tables lack what a payload is
        read from is a ReleaseError, never another exception."""
        try:
            return cls._read(msi)
        except (KeyError, ValueError) as e:
            raise ReleaseError(f"{msi.name}: its tables are not an "
                               f"installer's ({type(e).__name__}: {e})") from e

    @classmethod
    def _read(cls, msi: Path) -> "Payload":
        media = [m for m in table(msi, "Media") if m.get("Cabinet")]
        if len(media) != 1:
            raise ReleaseError(f"{msi.name}: {len(media)} cabinets in its "
                               "Media table, not one")
        stream = media[0]["Cabinet"]
        if not stream.startswith("#"):
            raise ReleaseError(f"{msi.name}: its cabinet {stream} is not "
                               "embedded")
        stream = stream[1:]
        raw = backend().stream(msi, stream)
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
        updates = []
        for key, data in sorted(changed.items()):
            updates.append(Update("File", (("FileSize", len(data)),),
                                  "File", key))
            if key in self.hashed:
                parts = hash_parts(data)
                if _NULL_INT in parts:
                    raise ReleaseError(f"{self.paths[key]}: its MD5 cannot "
                                       "be stored in an MsiFileHash row")
                updates.append(Update(
                    "MsiFileHash", tuple((f"HashPart{i}", v) for i, v in
                                         enumerate(parts, 1)), "File_", key))

        out.write_bytes(self.msi.read_bytes())
        backend().rewrite(out, self.stream, cab.write(entries), updates)

        again = Payload.read(out)
        expected = self.files() | new
        if again.files() != expected:
            raise ReleaseError(f"{out.name}: the rewritten payload does not "
                               "read back as written")
        stored = {h["File_"]: tuple(int(h[f"HashPart{i}"])
                                    for i in range(1, 5))
                  for h in table(out, "MsiFileHash")}
        for key in changed.keys() & self.hashed:
            if stored.get(key) != hash_parts(changed[key]):
                raise ReleaseError(f"{out.name}: {self.paths[key]}'s "
                                   "MsiFileHash did not take")
        sizes = {f["File"]: int(f["FileSize"]) for f in table(out, "File")}
        if any(sizes[k] != len(d) for k, d in changed.items()):
            raise ReleaseError(f"{out.name}: a FileSize did not take")
