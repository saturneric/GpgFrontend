#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authenticode inside Windows packages: the cabinet codec, an MSI's
payload rewritten, and every program in an MSI or a portable zip signed
and checked (gfrelease's cab.py, msi.py and authenticode.py).

Run: python3 -m unittest discover -s scripts/tests
"""

import base64
import json
import os
import shutil
import struct
import subprocess
import tempfile
import unittest
import zipfile
import zlib
from pathlib import Path
from unittest import mock
from xml.etree import ElementTree

ROOT = Path(__file__).resolve().parent.parent.parent

import _paths  # noqa: E402,F401
from gfrelease.adapters.windows import authenticode  # noqa: E402
from gfrelease.adapters.windows import x509  # noqa: E402
from gfrelease.adapters.windows import cab  # noqa: E402
from gfrelease import errors  # noqa: E402
from gfrelease.adapters.windows import msi  # noqa: E402
from gfrelease.adapters.windows import msix  # noqa: E402
from gfrelease.domain import policy  # noqa: E402
from trustutil import (SigningCertificate, pack_as_a_zip,  # noqa: E402
                       signing_tools)


def pe_bytes(tag: str = "") -> bytes:
    """A minimal x64 Windows program (one section, returns at once):
    enough for osslsigncode and signtool to sign, and to verify. @p tag makes it unique."""
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


# The MSI of these tests, fixtures/authenticode/test.msi, was built once by
# wixl 0.106 (`wixl -o test.msi t.wxs`, beside what write_payload() writes)
# from this t.wxs, so that no test needs a tool that builds one:
#
#   <?xml version="1.0" encoding="utf-8"?>
#   <Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
#     <Product Id="*" Name="Test" Language="1033" Version="2.3.0"
#              Manufacturer="T"
#              UpgradeCode="12345678-1234-1234-1234-123456789012">
#       <Package InstallerVersion="200" Compressed="yes"/>
#       <Media Id="1" Cabinet="t.cab" EmbedCab="yes"/>
#       <Directory Id="TARGETDIR" Name="SourceDir">
#         <Directory Id="ProgramFilesFolder"><Directory Id="D" Name="T">
#           <Component Id="C" Guid="12345678-1234-1234-1234-123456789013">
#             <File Id="F" Source="payload.txt"/>
#             <File Id="E" Source="app.exe"/></Component>
#           <Directory Id="G" Name="GnuPG"><Directory Id="GB" Name="bin">
#             <Component Id="C2" Guid="12345678-1234-1234-1234-123456789014">
#               <File Id="B" Source="gpg.exe"/></Component></Directory>
#           </Directory>
#         </Directory></Directory></Directory>
#       <Feature Id="M" Level="1"><ComponentRef Id="C"/>
#         <ComponentRef Id="C2"/></Feature>
#     </Product>
#   </Wix>
FIXTURE_MSI = Path(__file__).resolve().parent / "fixtures" / "authenticode" \
    / "test.msi"


def write_payload(directory: Path) -> None:
    """What the test packages hold: a text file, a program to sign and one
    of GnuPG's, which stays as it is."""
    (directory / "payload.txt").write_bytes(b"payload\n")
    (directory / "app.exe").write_bytes(pe_bytes("app"))
    (directory / "gpg.exe").write_bytes(pe_bytes("gpg"))


def build_msi(directory: Path, out: Path) -> Path:
    """The fixture MSI, which holds what write_payload() writes as
    T/payload.txt, T/app.exe and T/GnuPG/bin/gpg.exe."""
    shutil.copyfile(FIXTURE_MSI, out)
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
def sign_package(src: Path, dst: Path, info: dict, signer,
                 rules: authenticode.Rules, progress=None) -> list[str]:
    """@p src signed as plan() says, written to @p dst, as the stable
    stage signs it (open, sign the programs, close, sign the package);
    returns the paths signed inside. @p progress(done, total, path)
    follows the programs."""
    with tempfile.TemporaryDirectory(prefix="gfr-sign-") as tmp:
        opened = authenticode.open_package(src, info, rules, Path(tmp))
        paths = {file: path for path, file in opened.programs.items()}
        done = []

        def on_done(file: Path) -> None:
            done.append(file)
            if progress:
                progress(len(done), len(paths), paths[file])

        signer.sign_files(list(paths), on_done)
        signed = authenticode.close_package(opened, dst)
    if opened.outside:
        signer.sign_files([dst])
    return signed


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
        with self.assertRaisesRegex(errors.ReleaseError, "checksum"):
            cab.read(bytes(raw))

    def test_what_is_not_read_is_refused(self):
        raw = bytearray(cab.write([cab.Entry("a", b"hello")]))
        lzx = bytearray(raw)
        struct.pack_into("<H", lzx, 36 + 6, 3)  # the folder's compression
        with self.assertRaisesRegex(errors.ReleaseError, "mszip"):
            cab.read(bytes(lzx))
        spanning = bytearray(raw)
        struct.pack_into("<H", spanning, 30, cab.FLAG_NEXT)
        with self.assertRaisesRegex(errors.ReleaseError, "spans"):
            cab.read(bytes(spanning))
        with self.assertRaisesRegex(errors.ReleaseError, "not a cabinet"):
            cab.read(b"PK\3\4" + bytes(40))

    def test_a_cabinet_cut_anywhere_is_a_refusal(self):
        raw = cab.write([cab.Entry("a", b"hello" * 100),
                         cab.Entry("b", b"world")])
        for end in range(len(raw)):
            # The size in the header is what is held, so every cut reaches
            # the parser rather than stopping at the size check.
            cut = bytearray(raw[:end])
            if len(cut) >= 12:
                struct.pack_into("<I", cut, 8, len(cut))
            with self.subTest(end=end), \
                    self.assertRaises(errors.ReleaseError):
                cab.read(bytes(cut))

    def test_a_block_that_unpacks_past_its_size_is_refused(self):
        plain = bytes(cab.BLOCK + 1000)
        c = zlib.compressobj(9, zlib.DEFLATED, -15)
        packed = b"CK" + c.compress(plain) + c.flush()
        block = struct.pack("<IHH", 0, len(packed), cab.BLOCK) + packed
        name = b"a\0"
        files_at, data_at = 36 + 8, 36 + 8 + 16 + len(name)
        raw = struct.pack("<4sIIIIIBBHHHHH", cab.SIGNATURE, 0,
                          data_at + len(block), 0, files_at, 0, 3, 1, 1, 1,
                          0, 0, 0) + \
            struct.pack("<IHH", data_at, 1, cab.COMPRESS_MSZIP) + \
            struct.pack("<IIHHHH", cab.BLOCK, 0, 0, 0, 0, 0x20) + name + block
        with self.assertRaisesRegex(errors.ReleaseError, "more than"):
            cab.read(raw)

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
        product = json.loads((ROOT / "release" / "product.json")
                             .read_text("utf-8"))
        policy.check_product(product)
        for bad in ({"sign_inside": []}, {"sign_inside": ["exe"]},
                    {"sign_inside": [".exe"], "leave_unsigned": ["a/b"]},
                    {"sign_inside": [".exe"], "other": 1}):
            with self.assertRaises(policy.PolicyError):
                policy.check_product(dict(product, authenticode=bad))


class CertificateTest(unittest.TestCase):
    """A subject read from the certificate itself, as Windows' check has it."""

    @staticmethod
    def der(tag: int, *parts: bytes) -> bytes:
        body = b"".join(parts)
        size = bytes([len(body)]) if len(body) < 128 else \
            b"\x82" + len(body).to_bytes(2, "big")
        return bytes([tag]) + size + body

    def certificate(self, *rdns) -> bytes:
        oids = {"C": b"\x55\x04\x06", "ST": b"\x55\x04\x08",
                "O": b"\x55\x04\x0a", "CN": b"\x55\x04\x03",
                "private": b"\x2b\x06\x01\x04\x01\x82\x37\x63"}
        name = self.der(0x30, *(
            self.der(0x31, self.der(0x30, self.der(6, oids[attr]),
                                    self.der(kind, value)))
            for attr, kind, value in rdns))
        filler = self.der(0x30)
        tbs = self.der(0x30, self.der(0xA0, self.der(2, b"\2")),
                       self.der(2, b"\1"), filler, filler, filler, name,
                       filler)
        return self.der(0x30, tbs, filler, self.der(3, b"\0"))

    def test_the_subject_is_read_in_certificate_order(self):
        der = self.certificate(
            ("C", 19, b"DE"),
            ("ST", 12, "Baden-Württemberg".encode()),
            ("O", 30, "Open Source".encode("utf-16-be")),
            ("CN", 12, b"Yu Hu, Developer / Signer"),
            ("private", 12, b"x"))
        subject = x509.certificate_subject(der)
        self.assertEqual(subject, [
            ("C", "DE"), ("ST", "Baden-Württemberg"), ("O", "Open Source"),
            ("CN", "Yu Hu, Developer / Signer"),
            ("1.3.6.1.4.1.311.99", "x")])
        # Either way a pin may be written names the same certificate.
        for pin in ("/C=DE/ST=Baden-W\\xC3\\xBCrttemberg/O=Open Source"
                    "/CN=Yu Hu, Developer \\/ Signer/1.3.6.1.4.1.311.99=x",
                    "1.3.6.1.4.1.311.99=x,CN=Yu Hu\\, Developer / Signer,"
                    "O=Open Source,ST=Baden-Württemberg,C=DE",
                    x509.shown(subject)):
            self.assertEqual(x509.subject_rdns(pin), subject)

    def test_what_is_no_certificate_is_refused(self):
        whole = self.certificate(("CN", 12, b"a"))
        # A subject's attribute that is its type alone.
        lone = self.der(0x30, self.der(0x02, b"\x01"), self.der(0x30),
                        self.der(0x30), self.der(0x30),
                        self.der(0x30, self.der(0x31, self.der(
                            0x30, self.der(0x06, b"\x55\x04\x03")))))
        for bad in (b"", b"\x30", whole[:-40], b"\x30\x03\x02\x01\x01",
                    self.certificate(("CN", 4, b"a")),
                    self.der(0x30, lone)):
            with self.assertRaises(errors.ReleaseError):
                x509.certificate_subject(bad)

    def test_a_certificate_file_is_pem_or_der(self):
        tmp = Path(tempfile.mkdtemp(prefix="gfr-authenticode-"))
        self.addCleanup(shutil.rmtree, tmp, ignore_errors=True)
        a, b = (self.certificate(("CN", 12, n)) for n in (b"a", b"b"))
        (tmp / "one.cer").write_bytes(a)
        (tmp / "two.pem").write_text("".join(
            "-----BEGIN CERTIFICATE-----\n" +
            base64.encodebytes(c).decode() +
            "-----END CERTIFICATE-----\n" for c in (a, b)))
        self.assertEqual(x509.certificates_in(tmp / "one.cer"), [a])
        self.assertEqual(x509.certificates_in(tmp / "two.pem"),
                         [a, b])


class SigntoolTest(unittest.TestCase):
    """Which signtool is run, and what a failed run of it leads to."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-authenticode-"))
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.files = [self.tmp / f"{n}.exe" for n in "abc"]
        for file in self.files:
            file.write_bytes(file.name.encode())
        self.addCleanup(setattr, authenticode, "RETRY_DELAY",
                        authenticode.RETRY_DELAY)
        authenticode.RETRY_DELAY = 0

    def signer(self, runs, urls="http://a, http://b"):
        """A signer whose runs of signtool are @p runs, in turn: each how
        many of the files it is given it signs, and what it says."""
        signer = authenticode.SigntoolSigner("AB" * 20, timestamp_url=urls)
        self.tried = []

        def once(files, ts, on_done=None):
            signs, said = runs[len(self.tried)]
            self.tried.append((ts, [f.name for f in files]))
            for file in files[:signs]:
                file.write_bytes(file.read_bytes() + b" signed")
                on_done(file)
            # What it began and did not finish: changed, and not signed.
            for file in files[signs:signs + 1]:
                file.write_bytes(file.read_bytes() + b" half")
            return files[signs:], said
        signer._once = once
        return signer

    def test_the_newest_sdk_is_found_x64_first(self):
        kits = self.tmp / "x86" / "Windows Kits" / "10" / "bin"
        for version, arch in (("10.0.9999.0", "x64"), ("10.0.19041.0", "x86"),
                              ("10.0.26100.0", "x86"),
                              ("10.0.26100.0", "x64"),
                              ("10.0.26100.0", "arm64"), ("x64", "old")):
            (kits / version / arch).mkdir(parents=True)
            (kits / version / arch / "signtool.exe").write_bytes(b"")
        env = {"ProgramFiles(x86)": str(self.tmp / "x86"),
               "ProgramFiles": str(self.tmp / "none"), "PATH": ""}
        with mock.patch.dict(os.environ, env):
            self.assertEqual(
                Path(authenticode.find_signtool()),
                kits / "10.0.26100.0" / "x64" / "signtool.exe")
            # One named is the one run, and has to be there.
            with self.assertRaisesRegex(errors.ReleaseError, "not there"):
                authenticode.find_signtool(str(self.tmp / "signtool.exe"))
        with mock.patch.dict(os.environ, dict(env, **{
                "ProgramFiles(x86)": str(self.tmp / "none")})):
            with self.assertRaisesRegex(errors.ReleaseError,
                                        "signtool is not installed"):
                authenticode.find_signtool()

    def test_a_thumbprint_is_a_sha1(self):
        self.assertEqual(authenticode.SigntoolSigner(
            "f9 cd:dc" + "00" * 17).thumbprint, "F9CDDC" + "00" * 17)
        for bad in ("", "F9CD", "G" * 40, "00" * 32):
            with self.assertRaisesRegex(errors.ReleaseError, "40 hex"):
                authenticode.SigntoolSigner(bad)

    def test_everything_is_signed_by_one_run(self):
        signer = self.signer([(3, "")])
        done = []
        signer.sign_files(self.files, done.append)
        self.assertEqual(self.tried, [("http://a",
                                       ["a.exe", "b.exe", "c.exe"])])
        self.assertEqual(done, self.files)

    def test_a_time_stamp_failure_signs_the_rest_again(self):
        failed = ("SignTool Error: The specified timestamp server either "
                  "could not be reached or\nreturned an invalid response.")
        signer = self.signer([(1, failed), (0, "Error: SignerSign() failed."
                                               " (-2147012889/0x80072ee7)"),
                              (2, "")])
        done = []
        signer.sign_files(self.files, done.append)
        # The next service each time, and only what is still unsigned.
        self.assertEqual(self.tried, [
            ("http://a", ["a.exe", "b.exe", "c.exe"]),
            ("http://b", ["b.exe", "c.exe"]),
            ("http://a", ["b.exe", "c.exe"])])
        self.assertEqual(done, self.files)
        # What a failed run left half done was put back before the next.
        self.assertEqual([f.read_bytes() for f in self.files],
                         [b"a.exe signed", b"b.exe signed", b"c.exe signed"])

    def test_the_services_are_given_up_after_six_runs(self):
        signer = self.signer([(0, "the timestamp server did not answer")] * 6)
        with self.assertRaisesRegex(errors.ToolFailed, "timestamp server"):
            signer.sign_files(self.files)
        self.assertEqual(len(self.tried), authenticode.ATTEMPTS)

    def test_what_the_card_refuses_is_never_tried_again(self):
        # A wrong PIN tried six times locks the card.
        for said in ("Error: SignerSign() failed. (-2146434965/0x8010006b)",
                     "Error: SignerSign() failed. (-1073741275/0xc0000225)",
                     "SignerSign() failed. (-2147023673/0x800704c7), then "
                     "the timestamp server could not be reached",
                     "SignTool Error: No certificates were found that met "
                     "all the given criteria."):
            signer = self.signer([(0, said)] * 6)
            with self.assertRaisesRegex(errors.ToolFailed, "signtool failed"):
                signer.sign_files(self.files)
            self.assertEqual(len(self.tried), 1, said)

    def test_without_a_service_there_is_one_run(self):
        signer = self.signer([(0, "the timestamp server, somehow")], urls="")
        with self.assertRaises(errors.ToolFailed):
            signer.sign_files(self.files)
        self.assertEqual(self.tried, [(None, ["a.exe", "b.exe", "c.exe"])])

    def test_names_that_outgrow_a_command_line_take_more_runs(self):
        signer = self.signer([(2, ""), (1, "")])
        with mock.patch.object(authenticode, "_COMMAND_LINE",
                               2 * (len(str(self.files[0])) + 3)):
            signer.sign_files(self.files)
        self.assertEqual([names for _, names in self.tried],
                         [["a.exe", "b.exe"], ["c.exe"]])


class OsslsigncodeTest(unittest.TestCase):
    """What a failed run of osslsigncode leads to."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-authenticode-"))
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        (self.tmp / "in").write_bytes(b"program")

    def test_time_stamping_services_are_tried_in_turn(self):
        tried = []

        def once(src, dst, ts):
            tried.append(ts)
            if len(tried) < 3:
                raise errors.ToolFailed(f"osslsigncode failed: connect error "
                                        f"Connecting to {ts} Failed")
            shutil.copy(src, dst)

        signer = authenticode.Signer("c", "k", timestamp_url="http://a, http://b")
        signer._once = once
        old = authenticode.RETRY_DELAY
        authenticode.RETRY_DELAY = 0
        try:
            signer.sign(self.tmp / "in", self.tmp / "out")
        finally:
            authenticode.RETRY_DELAY = old
        self.assertEqual(tried, ["http://a", "http://b", "http://a"])

    def test_a_login_failure_is_never_tried_again(self):
        # A wrong PIN tried six times could lock the token.
        tried = []

        def once(src, dst, ts):
            tried.append(ts)
            raise errors.ToolFailed("osslsigncode failed: PKCS11 login "
                                    "failed, then no timestamp")

        signer = authenticode.Signer("c", "k", timestamp_url="http://a")
        signer._once = once
        with self.assertRaisesRegex(errors.ReleaseError, "login"):
            signer.sign(self.tmp / "in", self.tmp / "out")
        self.assertEqual(tried, ["http://a"])

    def test_files_are_signed_where_they_are(self):
        signer = authenticode.Signer("c", "k")
        signer._once = lambda src, dst, ts: dst.write_bytes(
            src.read_bytes() + b" signed")
        done = []
        signer.sign_files([self.tmp / "in"], done.append)
        self.assertEqual(done, [self.tmp / "in"])
        self.assertEqual((self.tmp / "in").read_bytes(), b"program signed")
        self.assertEqual([f.name for f in self.tmp.iterdir()], ["in"])


class MsixTest(unittest.TestCase):
    """What an MSIX is made with (gfrelease's msix.py and resource/msix/);
    one made and signed is among the stable stage's tests."""

    def test_a_publisher_is_the_subject_as_windows_spells_it(self):
        self.assertEqual(
            msix.publisher("/C=DE/ST=Baden-W\\xC3\\xBCrttemberg/L=Karlsruhe"
                           "/O=Open Source Developer/CN=Open Source "
                           "Developer Yu Hu"),
            "CN=Open Source Developer Yu Hu, O=Open Source Developer, "
            "L=Karlsruhe, S=Baden-Württemberg, C=DE")
        # The other way a pin is written, and a value Windows quotes.
        self.assertEqual(msix.publisher("CN=A\\, Inc.,O=B,C=DE"),
                         'CN="A, Inc.", O=B, C=DE')
        with self.assertRaisesRegex(errors.ReleaseError, "UID"):
            msix.publisher("/UID=1/CN=a")

    def test_the_manifest_takes_the_version_and_the_publisher(self):
        text = msix.manifest(msix.RESOURCES, "2.3.0", "/C=DE/O=A & B/CN=T")
        self.assertIn('Version="2.3.0.0"', text)
        self.assertIn('Publisher="CN=T, O=A &amp; B, C=DE"', text)
        self.assertNotIn('"{', text)
        package = ElementTree.fromstring(text)
        # Every logo it names is one make_logos.ps1 draws.
        logos = [value for element in package.iter()
                 for name, value in element.attrib.items()
                 if name.endswith("Logo")]
        logos += [e.text for e in package.iter() if e.tag.endswith("}Logo")]
        self.assertGreaterEqual(len(logos), 7)
        drawn = (msix.RESOURCES / msix.LOGOS).read_text("utf-8")
        for logo in logos:
            name = logo.rsplit("\\", 1)[1][:-len(".png")]
            self.assertIn(f"'{name}'", drawn)
        with self.assertRaisesRegex(errors.ReleaseError, "final release"):
            msix.manifest(msix.RESOURCES, "2.3.0-rc.1", "/CN=T")

    def test_an_msix_is_named_after_its_msi(self):
        self.assertEqual(msix.name_for("GpgFrontend-2.3.0-windows-x86_64.msi"),
                         "GpgFrontend-2.3.0-windows-x86_64.msix")


PACKAGE_TOOLS = signing_tools()


@unittest.skipUnless(PACKAGE_TOOLS, "needs osslsigncode, openssl and "
                     "msitools, or on Windows signtool")
class PackageTest(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-authenticode-"))
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.certificate = SigningCertificate(self.tmp)
        self.addCleanup(self.certificate.close)
        self.subject = self.certificate.subject
        self.cert = self.certificate.ca
        self.signer = self.certificate.signer()

    def verify(self, file, info):
        return authenticode.verify_package(file, info, self.subject,
                                           str(self.cert), RULES)

    def signed(self, program: bytes) -> bool:
        local = self.tmp / "check.exe"
        local.write_bytes(program)
        try:
            authenticode.verify(local, self.subject, str(self.cert))
            return True
        except errors.ReleaseError:
            return False

    def test_the_fixture_holds_the_payload(self):
        write_payload(self.tmp)
        self.assertEqual(
            authenticode.contents(FIXTURE_MSI, MSI),
            {"T/payload.txt": (self.tmp / "payload.txt").read_bytes(),
             "T/app.exe": (self.tmp / "app.exe").read_bytes(),
             "T/GnuPG/bin/gpg.exe": (self.tmp / "gpg.exe").read_bytes()})

    def test_an_msi_is_signed_inside_and_out(self):
        src = build_msi(self.tmp, self.tmp / "unsigned.msi")
        dst = self.tmp / "signed.msi"
        seen = []
        signed = sign_package(
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
        if shutil.which("msiextract"):
            out = self.tmp / "extracted"
            out.mkdir()
            subprocess.run(["msiextract", "-C", out, dst], check=True,
                           capture_output=True)
            found = next(out.rglob("app.exe"))
            self.assertEqual(found.read_bytes(), program)

    def test_a_payload_replaced_reads_back(self):
        # Through whichever way into the database this platform has: the
        # cabinet, both sizes and both hashes, and nothing else.
        self.assertEqual(type(msi.backend()).__name__, "_WindowsInstaller"
                         if os.name == "nt" else "_Msitools")
        src = build_msi(self.tmp, self.tmp / "unsigned.msi")
        dst = self.tmp / "replaced.msi"
        new = {"T/app.exe": os.urandom(70000), "T/payload.txt": b""}
        msi.Payload.read(src).replace(new, dst)
        self.assertEqual(authenticode.contents(dst, MSI),
                         authenticode.contents(src, MSI) | new)
        sizes = {f["File"]: int(f["FileSize"])
                 for f in msi.table(dst, "File")}
        self.assertEqual(sizes, {"F": 0, "E": 70000, "B": 1024})
        hashes = {h["File_"]: tuple(int(h[f"HashPart{i}"])
                                    for i in range(1, 5))
                  for h in msi.table(dst, "MsiFileHash")}
        self.assertEqual(hashes, {"F": msi.hash_parts(b""),
                                  "E": msi.hash_parts(new["T/app.exe"]),
                                  "B": msi.hash_parts(pe_bytes("gpg"))})
        self.assertEqual(msi.table(dst, "NoSuchTable"), [])
        with self.assertRaisesRegex(errors.ReleaseError, "holds no"):
            msi.Payload.read(src).replace({"T/other.exe": b""}, dst)
        (self.tmp / "not.msi").write_bytes(b"not an MSI")
        with self.assertRaises(errors.ReleaseError):
            msi.Payload.read(self.tmp / "not.msi")
        # Tables without the columns a payload is read from.
        table = msi.table
        with mock.patch.object(msi, "table", lambda m, name: [
                {k: v for k, v in row.items() if k != "Component_"}
                for row in table(m, name)]), \
                self.assertRaisesRegex(errors.ReleaseError, "not an "
                                       "installer's"):
            msi.Payload.read(src)

    def test_a_portable_zip_is_signed_inside(self):
        src = build_zip(self.tmp, self.tmp / "portable.zip")
        dst = self.tmp / "signed.zip"
        signed = sign_package(src, dst, ZIP, self.signer, RULES)
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
        with self.assertRaisesRegex(errors.ReleaseError, "T/app.exe"):
            self.verify(outer_only, MSI)
        with self.assertRaisesRegex(errors.ReleaseError, "app.exe"):
            self.verify(build_zip(self.tmp, self.tmp / "p.zip"), ZIP)

    def test_a_signature_is_checked_for_who_made_it_and_what_it_covers(self):
        program = self.tmp / "app.exe"
        program.write_bytes(pe_bytes("app"))
        self.signer.sign_files([program])
        authenticode.verify(program, self.subject, str(self.cert))
        authenticode.verify(program, ["/CN=Another", self.subject],
                            str(self.cert))
        with self.assertRaisesRegex(errors.ReleaseError, "signed by"):
            authenticode.verify(program, "/CN=Not Us", str(self.cert))
        with self.assertRaisesRegex(errors.ReleaseError, "no time-stamp"):
            authenticode.verify(program, self.subject, str(self.cert),
                                require_timestamp=True)
        # A certificate nothing vouches for, however it names itself.
        with self.assertRaisesRegex(errors.ReleaseError, "no valid"):
            authenticode.verify(program, self.subject)
        other = SigningCertificate(self.tmp, "Someone Else")
        self.addCleanup(other.close)
        with self.assertRaisesRegex(errors.ReleaseError, "no valid"):
            authenticode.verify(program, self.subject, str(other.ca))
        changed = bytearray(program.read_bytes())
        changed[0x205] ^= 1
        program.write_bytes(changed)
        with self.assertRaisesRegex(errors.ReleaseError, "no valid"):
            authenticode.verify(program, self.subject, str(self.cert))

    def test_a_change_beyond_the_signed_programs_is_refused(self):
        src = build_zip(self.tmp, self.tmp / "portable.zip")
        dst = self.tmp / "signed.zip"
        signed = sign_package(src, dst, ZIP, self.signer, RULES)
        tampered = self.tmp / "tampered.zip"
        authenticode._rezip(dst, tampered,
                            {"GpgFrontend/payload.txt": b"changed\n"})
        with self.assertRaisesRegex(errors.ReleaseError, "payload.txt"):
            authenticode.unchanged_apart_from(src, tampered, ZIP, signed)

    def test_an_msix_holds_the_files_of_its_msi_and_no_others(self):
        # A package as anything but Windows reads it: a zip. What it is
        # checked for is the same wherever it was packed.
        tree = self.tmp / "tree"
        tree.mkdir()
        (tree / msix.MANIFEST).write_text("<Package/>")
        (tree / "Assets").mkdir()
        (tree / "Assets" / "Logo.scale-100.png").write_bytes(b"png")
        files = authenticode.contents(FIXTURE_MSI, MSI)

        def refused(why, files=files):
            out = self.tmp / "refused.msix"
            pack_as_a_zip(tree, files, out)
            with self.assertRaisesRegex(errors.ReleaseError, why):
                msix.check_derived(out, FIXTURE_MSI)

        package = self.tmp / "test.msix"
        pack_as_a_zip(tree, files, package)
        msix.check_derived(package, FIXTURE_MSI)
        self.assertEqual(msix.contents(package)["VFS/T/app.exe"],
                         files["T/app.exe"])
        refused("T/app.exe is not the one in", files | {"T/app.exe": b"MZ"})
        refused("does not hold the files .*T/more.dll",
                files | {"T/more.dll": b"MZ"})
        refused("does not hold the files .*T/payload.txt",
                {p: d for p, d in files.items() if p != "T/payload.txt"})
        (tree / "run.cmd").write_text("rem")
        refused("holds run.cmd")

    @unittest.skipIf(os.name == "nt", "osslsigncode's PIN, where it signs")
    def test_the_pin_reaches_osslsigncode_through_a_pipe(self):
        locked = self.tmp / "locked.pem"
        subprocess.run(["openssl", "pkey", "-in", self.certificate.key,
                        "-aes256", "-passout", "pass:1234", "-out", locked],
                       check=True, capture_output=True)
        asked = []
        signer = authenticode.Signer(str(self.cert), str(locked),
                                     ask_pin=lambda: asked.append(1) or "1234")
        src = build_zip(self.tmp, self.tmp / "portable.zip")
        sign_package(src, self.tmp / "signed.zip", ZIP, signer,
                                  RULES)
        self.assertEqual(asked, [1])  # once, however many are signed
        self.assertEqual(self.verify(self.tmp / "signed.zip", ZIP), 1)

    @unittest.skipUnless(os.name == "nt", "signtool, where it signs")
    def test_signtool_tells_a_dead_time_stamping_service(self):
        # For real: nothing listens there, so signtool fails as it does
        # when a service is down, which is what is tried again.
        program = self.tmp / "app.exe"
        program.write_bytes(pe_bytes("app"))
        left, said = self.signer._once([program], "http://127.0.0.1:9/")
        self.assertEqual(left, [program])
        self.assertRegex(said, authenticode._SIGNTOOL_TSA)
        self.assertNotRegex(said, authenticode._SIGNTOOL_CARD)

    @unittest.skipUnless(os.name == "nt", "signtool, where it signs")
    def test_signtool_tells_a_certificate_that_is_not_there(self):
        program = self.tmp / "app.exe"
        program.write_bytes(pe_bytes("app"))
        signer = authenticode.SigntoolSigner("00" * 20,
                                             timestamp_url="http://a")
        with self.assertRaisesRegex(errors.ToolFailed, "No certificates"):
            signer.sign_files([program])
        self.assertEqual(program.read_bytes(), pe_bytes("app"))


if __name__ == "__main__":
    unittest.main()
