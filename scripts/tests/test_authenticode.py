#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authenticode inside Windows packages: the cabinet codec, an MSI's
payload rewritten, and every program in an MSI or a portable zip signed
and checked (gfrelease's cab.py, msi.py and authenticode.py).

Run: python3 -m unittest discover -s scripts/tests
"""

import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "scripts" / "release" / "gfrelease"))
sys.path.insert(0, str(ROOT / "scripts" / "tests"))
sys.path.insert(0, str(ROOT / "scripts" / "release"))

import authenticode  # noqa: E402
import cab  # noqa: E402
import common  # noqa: E402
import msi  # noqa: E402
import policy  # noqa: E402
from trustutil import tools  # noqa: E402


def pe_bytes(tag: str = "") -> bytes:
    """A minimal x64 Windows program (one section, returns at once):
    enough for osslsigncode to sign and verify. @p tag makes it unique."""
    dos = b"MZ" + b"\0" * 58 + struct.pack("<I", 0x40)
    coff = struct.pack("<HHIIIHH", 0x8664, 1, 0, 0, 0, 240, 0x22)
    opt = struct.pack("<HBBIIIII", 0x20B, 14, 0, 0x200, 0, 0, 0x1000, 0x1000)
    opt += struct.pack("<QIIHHHHHHIIIIHHQQQQII", 0x140000000, 0x1000, 0x200,
                       6, 0, 0, 0, 6, 0, 0, 0x2000, 0x200, 0, 3, 0x8160,
                       0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    opt += b"\0" * (16 * 8)
    section = b".text\0\0\0" + struct.pack("<IIIIIIHHI", 0x1000, 0x1000,
                                           0x200, 0x200, 0, 0, 0, 0,
                                           0x60000020)
    header = (dos + b"PE\0\0" + coff + opt + section).ljust(0x200, b"\0")
    return header + (b"\xc3" + tag.encode()).ljust(0x200, b"\0")


WXS = """<?xml version="1.0" encoding="utf-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" Name="Test" Language="1033" Version="2.3.0" Manufacturer="T"
           UpgradeCode="12345678-1234-1234-1234-123456789012">
    <Package InstallerVersion="200" Compressed="yes"/>
    <Media Id="1" Cabinet="t.cab" EmbedCab="yes"/>
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFilesFolder"><Directory Id="D" Name="T">
        <Component Id="C" Guid="12345678-1234-1234-1234-123456789013">
          <File Id="F" Source="payload.txt"/>
          <File Id="E" Source="app.exe"/></Component>
        <Directory Id="G" Name="GnuPG"><Directory Id="GB" Name="bin">
          <Component Id="C2" Guid="12345678-1234-1234-1234-123456789014">
            <File Id="B" Source="gpg.exe"/></Component></Directory>
        </Directory>
      </Directory></Directory></Directory>
    <Feature Id="M" Level="1"><ComponentRef Id="C"/><ComponentRef Id="C2"/>
    </Feature>
  </Product>
</Wix>
"""


def write_payload(directory: Path) -> None:
    """What the test packages hold: a text file, a program to sign and one
    of GnuPG's, which stays as it is."""
    (directory / "payload.txt").write_text("payload\n")
    (directory / "app.exe").write_bytes(pe_bytes("app"))
    (directory / "gpg.exe").write_bytes(pe_bytes("gpg"))


def build_msi(directory: Path, out: Path) -> Path:
    write_payload(directory)
    (directory / "t.wxs").write_text(WXS)
    subprocess.run(["wixl", "-o", out, directory / "t.wxs"], cwd=directory,
                   check=True, capture_output=True)
    return out


def build_zip(directory: Path, out: Path) -> Path:
    write_payload(directory)
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("GpgFrontend/", "")
        z.write(directory / "payload.txt", "GpgFrontend/payload.txt")
        z.write(directory / "app.exe", "GpgFrontend/bin/app.exe")
        z.write(directory / "gpg.exe", "GpgFrontend/gnupg/bin/gpg.exe")
    return out


RULES = authenticode.Rules((".exe", ".dll"), ("gnupg",))
MSI = {"kind": "msi", "os": "windows"}
ZIP = {"kind": "zip", "os": "windows"}


class CabTest(unittest.TestCase):

    def test_a_written_cabinet_reads_back(self):
        entries = [cab.Entry("small", b"x"),
                   cab.Entry("empty", b""),
                   # Several blocks, some reaching back into the one before.
                   cab.Entry("big", os.urandom(70000) + b"abc" * 50000,
                             attribs=0x21, date=0x5A6E, time=0x8C20),
                   cab.Entry("naïve", b"utf-8 name")]
        back = cab.read(cab.write(entries))
        self.assertEqual(back, entries)

    def test_a_damaged_block_is_refused(self):
        raw = bytearray(cab.write([cab.Entry("a", b"hello" * 1000)]))
        raw[-5] ^= 0xFF
        with self.assertRaisesRegex(common.ReleaseError, "checksum"):
            cab.read(bytes(raw))

    def test_what_is_not_read_is_refused(self):
        raw = bytearray(cab.write([cab.Entry("a", b"hello")]))
        lzx = bytearray(raw)
        struct.pack_into("<H", lzx, 36 + 6, 3)  # the folder's compression
        with self.assertRaisesRegex(common.ReleaseError, "mszip"):
            cab.read(bytes(lzx))
        spanning = bytearray(raw)
        struct.pack_into("<H", spanning, 30, cab.FLAG_NEXT)
        with self.assertRaisesRegex(common.ReleaseError, "spans"):
            cab.read(bytes(spanning))
        with self.assertRaisesRegex(common.ReleaseError, "not a cabinet"):
            cab.read(b"PK\3\4" + bytes(40))

    def test_the_checksum_is_the_cabinet_one(self):
        # Words XORed, the tail's first byte most significant.
        self.assertEqual(cab.checksum(b"\1\0\0\0\2\0\0\0"), 3)
        self.assertEqual(cab.checksum(b"\1\2\3"), 0x010203)


class RulesTest(unittest.TestCase):

    def test_what_is_signed_inside(self):
        self.assertTrue(RULES.wants("T/app.exe"))
        self.assertTrue(RULES.wants("T/Qt6Core.DLL"))
        self.assertFalse(RULES.wants("T/GnuPG/bin/gpg.exe"))
        self.assertFalse(RULES.wants("T/readme.txt"))
        # A folder name, not a file's: gnupg.dll is ours.
        self.assertTrue(RULES.wants("T/gnupg.dll"))

    def test_which_packages_are_signed_where(self):
        self.assertEqual(authenticode.plan(MSI), (True, True))
        self.assertEqual(authenticode.plan(ZIP), (True, False))
        self.assertEqual(authenticode.plan({"kind": "msix"}), (False, True))
        self.assertEqual(authenticode.plan({"kind": "zip", "os": "macos"}),
                         (False, False))
        self.assertEqual(authenticode.plan({"kind": "dmg"}), (False, False))

    def test_the_product_rule_is_checked(self):
        product = json.loads((ROOT / "release" / "product.json").read_text())
        policy.check_product(product)
        for bad in ({"sign_inside": []}, {"sign_inside": ["exe"]},
                    {"sign_inside": [".exe"], "leave_unsigned": ["a/b"]},
                    {"sign_inside": [".exe"], "other": 1}):
            with self.assertRaises(policy.PolicyError):
                policy.check_product(dict(product, authenticode=bad))


PACKAGE_TOOLS = tools("osslsigncode", "openssl", "wixl", "msiinfo",
                      "msibuild")


@unittest.skipUnless(PACKAGE_TOOLS,
                     "needs osslsigncode, openssl, wixl and msitools")
class PackageTest(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-authenticode-"))
        self.cert, self.key = self.tmp / "cert.pem", self.tmp / "key.pem"
        self.subject = "/CN=GpgFrontend Test Signing/O=Rehearsal/C=DE"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048",
                        "-nodes", "-days", "2", "-subj", self.subject,
                        "-keyout", self.key, "-out", self.cert],
                       check=True, capture_output=True)
        self.signer = authenticode.Signer(str(self.cert), str(self.key))

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def verify(self, file, info):
        return authenticode.verify_package(file, info, self.subject,
                                           str(self.cert), RULES)

    def signed(self, program: bytes) -> bool:
        local = self.tmp / "check.exe"
        local.write_bytes(program)
        try:
            authenticode.verify(local, self.subject, str(self.cert))
            return True
        except common.ReleaseError:
            return False

    def test_an_msi_is_signed_inside_and_out(self):
        src = build_msi(self.tmp, self.tmp / "unsigned.msi")
        dst = self.tmp / "signed.msi"
        seen = []
        signed = authenticode.sign_package(
            src, dst, MSI, self.signer, RULES,
            progress=lambda n, total, path: seen.append((n, total, path)))
        self.assertEqual(signed, ["T/app.exe"])
        self.assertEqual(seen, [(1, 1, "T/app.exe")])

        files = authenticode.contents(dst, MSI)
        self.assertEqual(files["T/payload.txt"], b"payload\n")
        self.assertTrue(self.signed(files["T/app.exe"]))
        self.assertEqual(files["T/GnuPG/bin/gpg.exe"], pe_bytes("gpg"))
        authenticode.unchanged_apart_from(src, dst, MSI, signed)
        self.assertEqual(self.verify(dst, MSI), 1)

        # Windows Installer's view agrees: the table's size and hash are the
        # signed program's, and msiextract (libgcab) unpacks it.
        program = files["T/app.exe"]
        row = next(f for f in msi.table(dst, "File") if f["File"] == "E")
        self.assertEqual(int(row["FileSize"]), len(program))
        hashed = next(h for h in msi.table(dst, "MsiFileHash")
                      if h["File_"] == "E")
        self.assertEqual(tuple(int(hashed[f"HashPart{i}"])
                               for i in range(1, 5)),
                         msi.hash_parts(program))
        out = self.tmp / "extracted"
        out.mkdir()
        subprocess.run(["msiextract", "-C", out, dst], check=True,
                       capture_output=True)
        found = next(out.rglob("app.exe"))
        self.assertEqual(found.read_bytes(), program)

    def test_a_portable_zip_is_signed_inside(self):
        src = build_zip(self.tmp, self.tmp / "portable.zip")
        dst = self.tmp / "signed.zip"
        signed = authenticode.sign_package(src, dst, ZIP, self.signer, RULES)
        self.assertEqual(signed, ["GpgFrontend/bin/app.exe"])
        authenticode.unchanged_apart_from(src, dst, ZIP, signed)
        self.assertEqual(self.verify(dst, ZIP), 1)
        with zipfile.ZipFile(src) as a, zipfile.ZipFile(dst) as b:
            # The same entries, in order, with their times and modes.
            self.assertEqual(
                [(i.filename, i.date_time, i.external_attr, i.compress_type)
                 for i in a.infolist()],
                [(i.filename, i.date_time, i.external_attr, i.compress_type)
                 for i in b.infolist()])

    def test_an_unsigned_program_inside_is_refused(self):
        src = build_msi(self.tmp, self.tmp / "unsigned.msi")
        # The package signed, but not what is in it.
        outer_only = self.tmp / "outer.msi"
        self.signer.sign(src, outer_only)
        with self.assertRaisesRegex(common.ReleaseError, "T/app.exe"):
            self.verify(outer_only, MSI)
        with self.assertRaisesRegex(common.ReleaseError, "app.exe"):
            self.verify(build_zip(self.tmp, self.tmp / "p.zip"), ZIP)

    def test_a_change_beyond_the_signed_programs_is_refused(self):
        src = build_zip(self.tmp, self.tmp / "portable.zip")
        dst = self.tmp / "signed.zip"
        signed = authenticode.sign_package(src, dst, ZIP, self.signer, RULES)
        tampered = self.tmp / "tampered.zip"
        authenticode._rezip(dst, tampered,
                            {"GpgFrontend/payload.txt": b"changed\n"})
        with self.assertRaisesRegex(common.ReleaseError, "payload.txt"):
            authenticode.unchanged_apart_from(src, tampered, ZIP, signed)

    def test_the_pin_reaches_osslsigncode_through_a_pipe(self):
        locked = self.tmp / "locked.pem"
        subprocess.run(["openssl", "pkey", "-in", self.key, "-aes256",
                        "-passout", "pass:1234", "-out", locked],
                       check=True, capture_output=True)
        asked = []
        signer = authenticode.Signer(str(self.cert), str(locked),
                                     ask_pin=lambda: asked.append(1) or "1234")
        src = build_zip(self.tmp, self.tmp / "portable.zip")
        authenticode.sign_package(src, self.tmp / "signed.zip", ZIP, signer,
                                  RULES)
        self.assertEqual(asked, [1])  # once, however many are signed
        self.assertEqual(self.verify(self.tmp / "signed.zip", ZIP), 1)

    def test_time_stamping_services_are_tried_in_turn(self):
        tried = []

        def once(src, dst, ts):
            tried.append(ts)
            if len(tried) < 3:
                raise common.ToolFailed(f"osslsigncode failed: connect error "
                                        f"Connecting to {ts} Failed")
            shutil.copy(src, dst)

        signer = authenticode.Signer("c", "k", timestamp_url="http://a, http://b")
        signer._once = once
        old = authenticode.RETRY_DELAY
        authenticode.RETRY_DELAY = 0
        try:
            signer.sign(self.tmp / "cert.pem", self.tmp / "out")
        finally:
            authenticode.RETRY_DELAY = old
        self.assertEqual(tried, ["http://a", "http://b", "http://a"])

    def test_a_login_failure_is_never_tried_again(self):
        # A wrong PIN tried six times could lock the token.
        tried = []

        def once(src, dst, ts):
            tried.append(ts)
            raise common.ToolFailed("osslsigncode failed: PKCS11 login "
                                    "failed, then no timestamp")

        signer = authenticode.Signer("c", "k", timestamp_url="http://a")
        signer._once = once
        with self.assertRaisesRegex(common.ReleaseError, "login"):
            signer.sign(self.tmp / "cert.pem", self.tmp / "out")
        self.assertEqual(tried, ["http://a"])


if __name__ == "__main__":
    unittest.main()
