# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Microsoft cabinet files, as an MSI embeds its payload: read and written
here, so signing what is inside an installer needs no Windows tool.

Only what a single-cabinet MSI uses: the stored and MSZIP compressions (an
LZX cabinet is refused: Package.wxs builds with CompressionLevel="mszip"),
no spanning across cabinets. Writing keeps every entry's name, attributes
and time stamp, in the order given, which for an MSI is the File table's
Sequence order Windows Installer reads the cabinet in.
"""

from __future__ import annotations

import struct
import zlib
from dataclasses import dataclass

from ...common import ReleaseError

SIGNATURE = b"MSCF"
BLOCK = 0x8000              # the uncompressed size of a CFDATA block
FOLDER_LIMIT = 0x7FFF8000   # the most one folder holds (cCFData is 16 bits)
COMPRESS_NONE, COMPRESS_MSZIP = 0, 1
FLAG_PREV, FLAG_NEXT, FLAG_RESERVE = 0x1, 0x2, 0x4
ATTR_NAME_UTF8 = 0x80


@dataclass
class Entry:
    name: str
    data: bytes
    attribs: int = 0x20     # archive
    date: int = 0x5821      # 2024-01-01, in the FAT encoding
    time: int = 0


def checksum(data: bytes, seed: int = 0) -> int:
    """The CFDATA checksum: 32-bit little-endian words XORed together, the
    last partial word taken most significant byte first."""
    n = len(data) & ~3
    words = struct.unpack_from(f"<{n // 4}I", data)
    c = seed
    for w in words:
        c ^= w
    tail = 0
    for b in data[n:]:
        tail = (tail << 8) | b
    return c ^ tail


def _block_sum(packed: bytes, uncompressed: int) -> int:
    return checksum(struct.pack("<HH", len(packed), uncompressed),
                    checksum(packed))


def read(raw: bytes, where: str = "cabinet") -> list[Entry]:
    """Every file in @p raw, in the order the cabinet lists them. One that
    is cut short or malformed anywhere is a ReleaseError, never another
    exception: what is read comes from a package others may have made."""
    try:
        return _read(raw, where)
    except (struct.error, ValueError, IndexError, UnicodeDecodeError) as e:
        raise ReleaseError(f"{where}: cut short or malformed ({e})") from e


def _read(raw: bytes, where: str) -> list[Entry]:
    def bad(why: str) -> ReleaseError:
        return ReleaseError(f"{where}: {why}")

    if len(raw) < 36 or raw[:4] != SIGNATURE:
        raise bad("not a cabinet file")
    (size, files_at, minor, major, n_folders, n_files, flags, _set,
     _index) = struct.unpack_from("<8xI4xI4xBBHHHHH", raw)
    if size != len(raw):
        raise bad(f"says it is {size} bytes, holds {len(raw)}")
    if (major, minor) != (1, 3):
        raise bad(f"cabinet format {major}.{minor}, not 1.3")
    if flags & (FLAG_PREV | FLAG_NEXT):
        raise bad("spans several cabinets")
    pos = 36
    folder_reserve = data_reserve = 0
    if flags & FLAG_RESERVE:
        header_reserve, folder_reserve, data_reserve = \
            struct.unpack_from("<HBB", raw, pos)
        pos += 4 + header_reserve

    folders = []
    for _ in range(n_folders):
        start, blocks, kind = struct.unpack_from("<IHH", raw, pos)
        pos += 8 + folder_reserve
        folders.append((start, blocks, kind & 0xF))

    streams = []
    for i, (start, blocks, kind) in enumerate(folders):
        if kind not in (COMPRESS_NONE, COMPRESS_MSZIP):
            raise bad(f"folder {i} uses compression {kind} (only stored and "
                      "MSZIP are read: build the MSI with "
                      "CompressionLevel=\"mszip\")")
        out = bytearray()
        at = start
        for b in range(blocks):
            csum, packed_len, plain_len = struct.unpack_from("<IHH", raw, at)
            at += 8 + data_reserve
            packed = raw[at:at + packed_len]
            at += packed_len
            if len(packed) != packed_len:
                raise bad(f"folder {i} block {b} is cut short")
            if csum and csum != _block_sum(packed, plain_len):
                raise bad(f"folder {i} block {b} fails its checksum")
            if kind == COMPRESS_NONE:
                plain = packed
            else:
                if packed[:2] != b"CK":
                    raise bad(f"folder {i} block {b} is not MSZIP")
                # Each block is one deflate stream; its window may reach
                # back into the blocks before it.
                # Never more than a block holds: a block that would unpack
                # past it is refused before it is held in memory.
                d = zlib.decompressobj(-15, zdict=bytes(out[-BLOCK:]))
                try:
                    plain = d.decompress(packed[2:], BLOCK)
                    if d.unconsumed_tail or not d.eof:
                        raise bad(f"folder {i} block {b} unpacks to more "
                                  f"than {BLOCK} bytes, or not at all")
                except zlib.error as e:
                    raise bad(f"folder {i} block {b}: {e}") from e
            if len(plain) != plain_len:
                raise bad(f"folder {i} block {b} unpacks to {len(plain)} "
                          f"bytes, not {plain_len}")
            out += plain
        streams.append(bytes(out))

    entries = []
    pos = files_at
    for _ in range(n_files):
        length, offset, folder, date, time, attribs = \
            struct.unpack_from("<IIHHHH", raw, pos)
        pos += 16
        end = raw.index(b"\0", pos)
        raw_name = raw[pos:end]
        pos = end + 1
        name = raw_name.decode("utf-8" if attribs & ATTR_NAME_UTF8
                               else "latin-1")
        if folder >= len(streams):
            raise bad(f"{name} is in folder {folder}, which is not there")
        stream = streams[folder]
        if offset + length > len(stream):
            raise bad(f"{name} reaches past its folder")
        # The UTF-8 flag says how the name is stored, not what the file is:
        # write() sets it again from the name.
        entries.append(Entry(name, stream[offset:offset + length],
                             attribs & ~ATTR_NAME_UTF8, date, time))
    return entries


def _pack_folder(data: bytes) -> list[bytes]:
    """@p data as MSZIP CFDATA blocks, each with its header."""
    blocks = []
    for at in range(0, len(data), BLOCK) or [0]:
        plain = data[at:at + BLOCK]
        c = zlib.compressobj(9, zlib.DEFLATED, -15,
                             zdict=data[max(0, at - BLOCK):at])
        packed = b"CK" + c.compress(plain) + c.flush()
        blocks.append(struct.pack("<IHH", _block_sum(packed, len(plain)),
                                  len(packed), len(plain)) + packed)
    return blocks


def write(entries: list[Entry]) -> bytes:
    """A cabinet of @p entries, MSZIP-compressed, in their order; a new
    folder starts only where one would outgrow what a folder can hold."""
    if len(entries) > 0xFFFF:
        raise ReleaseError(f"{len(entries)} files are more than a cabinet "
                           "holds")
    folders: list[list[Entry]] = [[]]
    size = 0
    for e in entries:
        if len(e.data) > FOLDER_LIMIT:
            raise ReleaseError(f"{e.name} is too large for a cabinet")
        if folders[-1] and size + len(e.data) > FOLDER_LIMIT:
            folders.append([])
            size = 0
        folders[-1].append(e)
        size += len(e.data)

    names = [e.name.encode("utf-8") for e in entries]
    utf8 = [any(b > 0x7F for b in n) for n in names]
    files_at = 36 + 8 * len(folders)
    data_at = files_at + sum(16 + len(n) + 1 for n in names)

    folder_table, file_table, data = bytearray(), bytearray(), bytearray()
    i = 0
    for f, members in enumerate(folders):
        stream = b"".join(e.data for e in members)
        blocks = _pack_folder(stream)
        folder_table += struct.pack("<IHH", data_at + len(data), len(blocks),
                                    COMPRESS_MSZIP)
        offset = 0
        for e in members:
            attribs = (e.attribs | ATTR_NAME_UTF8) if utf8[i] else \
                (e.attribs & ~ATTR_NAME_UTF8)
            file_table += struct.pack("<IIHHHH", len(e.data), offset, f,
                                      e.date, e.time, attribs) + names[i] + b"\0"
            offset += len(e.data)
            i += 1
        data += b"".join(blocks)

    total = data_at + len(data)
    header = struct.pack("<4sIIIIIBBHHHHH", SIGNATURE, 0, total, 0, files_at,
                         0, 3, 1, len(folders), len(entries), 0, 0, 0)
    return bytes(header + folder_table + file_table + data)
