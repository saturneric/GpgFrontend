# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The sideload MSIX of a stable release: made from its signed MSI.

No CI job builds an MSIX. The offline stage makes it (stable.py), once the
programs inside the MSI are signed: the MSI's files, those signed programs
among them, laid under VFS/ as the MSI would install them, with the
manifest of resource/msix/ around them and the logos it names, drawn from
the application's icon as the package is made (make_logos.ps1 there). So it holds nothing CI
did not build, and its programs are signed without being signed again. The
package itself is then signed with the MSI, in the same run of the signer.

A package installs only when its manifest names the signing certificate's
subject as its publisher, spelled as Windows spells it (publisher()); the
subject is the one the trust root pins.

Packing is Windows' own (PowerShell draws the logos, the SDK's makepri
indexes them and its makeappx makes the package), so an MSIX is only made
on Windows. What it
holds is read anywhere, a package being a zip: the signing host checks that
every file of it beyond the package's own is one of the MSI's, byte for
byte (check_derived()), which is all that stands in for a build hash.
"""

from __future__ import annotations

import re
import tempfile
import zipfile
from pathlib import Path
from urllib.parse import unquote
from xml.sax.saxutils import quoteattr

from ...domain.formats import parse_version
from ...errors import ReleaseError
from ...paths import ROOT
from ..process import WINDOWS, run
from . import authenticode, msi

RESOURCES = ROOT / "resource" / "msix"
MANIFEST = "AppxManifest.xml"
LOGOS = "make_logos.ps1"
# What the logos are drawn from: the icon, in its largest form, the one PNG
# of this folder (named by the application's id, which is the product's).
ICONS = ROOT / "resource" / "lfs" / "hicolor" / "512x512" / "apps"
# Where the MSI's files lie in the package: the folders Windows shows the
# application in place of its own (the MSI installs under AppData).
PAYLOAD = "VFS"
# What a package holds of its own: its manifest, what packing and signing
# add, and the logos.
_OWN = {MANIFEST.lower(), "appxblockmap.xml", "[content_types].xml",
        "appxsignature.p7x", "appxmetadata/codeintegrity.cat",
        "resources.pri"}
_LOGO = re.compile(r"assets/[^/]+\.png", re.I)

# A subject's attributes as Windows names them, by the names a pin is
# written with (OpenSSL's, upper case as authenticode reads them).
_WINDOWS_NAMES = {"CN": "CN", "O": "O", "OU": "OU", "L": "L", "ST": "S",
                  "C": "C", "STREET": "STREET", "DC": "DC",
                  "EMAILADDRESS": "E", "SERIALNUMBER": "SERIALNUMBER",
                  "POSTALCODE": "PostalCode", "TITLE": "T", "GN": "G",
                  "SN": "SN"}


# What makepri is to index: the files of the tree, a name's ".scale-200" or
# ".targetsize-16" saying which one of a logo it is. `makepri createconfig`
# writes the same and, besides, splits the index by scale into several
# files, which a package installed whole has no use for.
_PRI_CONFIG = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<resources targetOsVersion="10.0.0" majorVersion="1">
  <index root="\\" startIndexAt="\\">
    <default>
      <qualifier name="Language" value="en-US"/>
      <qualifier name="Contrast" value="standard"/>
      <qualifier name="Scale" value="100"/>
      <qualifier name="HomeRegion" value="001"/>
      <qualifier name="TargetSize" value="256"/>
      <qualifier name="LayoutDirection" value="LTR"/>
      <qualifier name="Theme" value="dark"/>
      <qualifier name="AlternateForm" value=""/>
      <qualifier name="DXFeatureLevel" value="DX9"/>
      <qualifier name="Configuration" value=""/>
      <qualifier name="DeviceFamily" value="Universal"/>
      <qualifier name="Custom" value=""/>
    </default>
    <indexer-config type="folder" foldernameAsQualifier="true"
                    filenameAsQualifier="true" qualifierDelimiter="."/>
  </index>
</resources>
"""


def available() -> bool:
    """Whether a package can be made here."""
    return WINDOWS


def name_for(msi_name: str) -> str:
    """What the MSIX made from the MSI @p msi_name is called."""
    return msi_name[:-len(".msi")] + ".msix"


def publisher(subject: str) -> str:
    """@p subject (a pin: "/C=../O=../CN=.." or RFC 2253) as a manifest's
    Publisher: last attribute first, Windows' names, a value with a
    character that would end it in quotes."""
    parts = []
    for attr, value in reversed(authenticode._subject_rdns(subject)):
        if attr not in _WINDOWS_NAMES:
            raise ReleaseError(f"no MSIX publisher can name the subject's "
                               f"{attr}")
        if re.search(r'[,+"=<>#;]|^\s|\s$', value):
            value = '"' + value.replace('"', '""') + '"'
        parts.append(f"{_WINDOWS_NAMES[attr]}={value}")
    return ", ".join(parts)


def package_version(version: str) -> str:
    """A package's version has four parts: the release's, and 0."""
    v = parse_version(version)
    if not v.is_release:
        raise ReleaseError(f"{version}: an MSIX is made of a final release")
    return f"{v}.0"


def manifest(resources: Path, version: str, subject: str) -> str:
    """resources' manifest with this release's version and publisher."""
    try:
        text = (resources / MANIFEST).read_text("utf-8")
    except OSError as e:
        raise ReleaseError(f"{resources / MANIFEST}: {e}") from e
    for field, value in (("publisher", publisher(subject)),
                         ("version", package_version(version))):
        if f'"{{{field}}}"' not in text:
            raise ReleaseError(f"{resources / MANIFEST}: no {{{field}}} to "
                               "fill in")
        text = text.replace(f'"{{{field}}}"', quoteattr(value))
    return text


def icon() -> Path:
    """The application's icon in its largest form."""
    found = sorted(ICONS.glob("*.png"))
    if len(found) != 1:
        raise ReleaseError(f"{ICONS}: no single icon to draw the logos from "
                           f"({len(found)} PNG files)")
    return found[0]


def _pack(tree: Path, files: dict[str, bytes], out: Path,
          resources: Path) -> None:
    """@p tree, holding a package's manifest, packed into @p out with the
    logos @p resources draws and @p files under VFS/."""
    run(["powershell", "-NoProfile", "-NonInteractive", "-ExecutionPolicy",
         "Bypass", "-File", resources / LOGOS, "-Icon", icon(),
         "-Assets", tree / "Assets"],
        timeout=600)
    # The index of the logos, before anything else is in the tree: makepri
    # lists every file it finds there.
    config = tree.with_name("priconfig.xml")
    config.write_text(_PRI_CONFIG, "utf-8")
    run([authenticode.find_sdk_tool("makepri"), "new", "/pr", tree,
         "/cf", config, "/mn", tree / MANIFEST,
         "/of", tree / "resources.pri", "/o"])
    for path, data in files.items():
        local = tree / PAYLOAD / path
        local.parent.mkdir(parents=True, exist_ok=True)
        local.write_bytes(data)
    run([authenticode.find_sdk_tool("makeappx"), "pack", "/d", tree,
         "/p", out, "/h", "SHA256", "/o"], timeout=1800)


def build(msi_file: Path, out: Path, *, version: str, subject: str,
          resources: Path | None = None) -> None:
    """The MSIX of @p msi_file (signed inside already, or the package's
    programs would not be) written to @p out, unsigned."""
    if not available():
        raise ReleaseError("an MSIX is packed by the Windows SDK's makeappx: "
                           "only on Windows")
    resources = Path(resources or RESOURCES)
    text = manifest(resources, version, subject)
    files = msi.Payload.read(msi_file).files()
    started = re.search(r'\bExecutable="([^"]+)"', text)
    program = started.group(1).replace("\\", "/") if started else ""
    if not program.startswith(PAYLOAD + "/") or \
            program[len(PAYLOAD) + 1:] not in files:
        raise ReleaseError(f"{msi_file.name} holds no "
                           f"{program[len(PAYLOAD) + 1:] or 'program'}, which "
                           f"{resources / MANIFEST} starts")
    with tempfile.TemporaryDirectory(prefix="gfr-msix-") as tmp:
        tree = Path(tmp) / "package"
        tree.mkdir()
        (tree / MANIFEST).write_text(text, "utf-8")
        if out.exists():
            out.unlink()
        _pack(tree, files, out, resources)
    if not out.is_file():
        raise ReleaseError(f"makeappx made no {out.name}")
    check_derived(out, msi_file)


def contents(file: Path) -> dict[str, bytes]:
    """Everything a package holds, by path ("/"-separated; a package
    spells some characters of a name as %XX)."""
    try:
        with zipfile.ZipFile(file) as z:
            return {unquote(i.filename): z.read(i) for i in z.infolist()
                    if not i.is_dir()}
    except (zipfile.BadZipFile, OSError) as e:
        raise ReleaseError(f"{file.name}: {e}") from e


def check_derived(file: Path, msi_file: Path) -> None:
    """The package @p file is @p msi_file's and nothing more: under VFS/
    exactly the MSI's files, byte for byte, and beyond them only what a
    package holds of its own."""
    inside = contents(file)
    prefix = PAYLOAD + "/"
    payload = {path[len(prefix):]: data for path, data in inside.items()
               if path.startswith(prefix)}
    wanted = msi.Payload.read(msi_file).files()
    if set(payload) != set(wanted):
        other = sorted(set(payload) ^ set(wanted))
        raise ReleaseError(f"{file.name}: does not hold the files of "
                           f"{msi_file.name} ({other[0]}"
                           f"{', ...' if len(other) > 1 else ''})")
    changed = sorted(p for p in wanted if payload[p] != wanted[p])
    if changed:
        raise ReleaseError(f"{file.name}: {changed[0]} is not the one in "
                           f"{msi_file.name}")
    strange = sorted(p for p in inside if not p.startswith(prefix) and
                     p.lower() not in _OWN and not _LOGO.fullmatch(p))
    if strange:
        raise ReleaseError(f"{file.name}: holds {strange[0]}, which is "
                           f"neither a package's own nor {msi_file.name}'s")
