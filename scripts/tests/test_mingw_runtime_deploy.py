#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
#
# This file is part of GpgFrontend.
#
# GpgFrontend is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GpgFrontend is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""The MinGW runtime deployment, exercised on any host.

cmake/DeployMinGWRuntime.cmake reads PE import tables with objdump, and a
Linux binutils reads PE just as well, so the script is tested here against
PE files generated on the spot: each one imports exactly the DLLs a test says,
which no binary found lying around on a machine would.

Two hosts differ in ways the tests stay clear of. On Linux CMake neither
consults a system32 (the tests pass a stand-in) nor folds case when it looks
a file up, so every generated name is lower case.

Run: python3 -m unittest discover -s scripts/tests
"""

import os
import re
import shutil
import struct
import subprocess
import tempfile
import time
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "cmake" / "DeployMinGWRuntime.cmake"
MODULE = ROOT / "cmake" / "MinGWRuntimeDeploy.cmake"


def _align(value, alignment):
    return (value + alignment - 1) // alignment * alignment


def make_pe(imports, dll=True, salt=b""):
    """A minimal PE32+ image whose import directory names `imports`.

    Not loadable, and not meant to be: objdump -p only needs the headers and
    the import descriptors to print the "DLL Name:" lines CMake reads.
    `salt` changes the bytes without changing the imports.
    """
    rva = 0x1000
    body = bytearray(20 * (len(imports) + 1))
    descriptors = []
    for name in imports:
        ilt = rva + len(body)
        body += b"\0" * 16
        iat = rva + len(body)
        body += b"\0" * 16
        hint = rva + len(body)
        body += b"\0\0f\0"
        name_rva = rva + len(body)
        body += name.encode() + b"\0"
        if len(body) % 2:
            body += b"\0"
        struct.pack_into("<Q", body, ilt - rva, hint)
        struct.pack_into("<Q", body, iat - rva, hint)
        descriptors.append((ilt, name_rva, iat))
    for i, (ilt, name_rva, iat) in enumerate(descriptors):
        struct.pack_into("<IIIII", body, 20 * i, ilt, 0, 0, name_rva, iat)
    body += salt
    raw = _align(max(len(body), 1), 0x200)
    body += b"\0" * (raw - len(body))

    coff = struct.pack("<HHIIIHH", 0x8664, 1, 0, 0, 0, 240,
                       0x2022 if dll else 0x0022)
    optional = struct.pack("<HBBIIIII", 0x20B, 2, 30, 0, raw, 0, 0, rva)
    optional += struct.pack(
        "<QIIHHHHHHIIIIHHQQQQII",
        0x180000000 if dll else 0x140000000, 0x1000, 0x200,
        6, 0, 0, 0, 6, 0, 0, rva + _align(raw, 0x1000), 0x200, 0, 3, 0x160,
        0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    directories = [(0, 0)] * 16
    if imports:
        directories[1] = (rva, 20 * (len(imports) + 1))
    for entry in directories:
        optional += struct.pack("<II", *entry)
    section = struct.pack("<8sIIIIIIHHI", b".idata", len(body), rva, raw,
                          0x200, 0, 0, 0, 0, 0xC0000040)
    dos = bytearray(64)
    dos[0:2] = b"MZ"
    struct.pack_into("<I", dos, 0x3C, 0x40)
    header = bytes(dos) + b"PE\0\0" + coff + optional + section
    header += b"\0" * (0x200 - len(header))
    return header + bytes(body)


def _tool_problem():
    """Why these tests cannot run here, or None."""
    cmake = shutil.which("cmake")
    objdump = shutil.which("objdump")
    if not cmake or not objdump:
        return "cmake and objdump are required"
    version = subprocess.run([cmake, "--version"], capture_output=True,
                             text=True).stdout
    match = re.search(r"(\d+)\.(\d+)", version)
    if not match or (int(match[1]), int(match[2])) < (3, 24):
        return "cmake >= 3.24 is required"
    formats = subprocess.run([objdump, "-i"], capture_output=True,
                             text=True).stdout
    if "pei-x86-64" not in formats:
        return "this objdump cannot read PE files"
    return None


TOOL_PROBLEM = _tool_problem()


def _bracket(value):
    return "[==[" + str(value) + "]==]"


class Tree:
    """A build tree, an "MSYS2 bin" and a "system32", all with spaces."""

    def __init__(self, root):
        self.root = root
        self.binary = root / "build dir"
        self.dest = self.binary / "artifacts" / "bin"
        self.modules = self.binary / "artifacts" / "modules"
        self.mingw = root / "msys64 root" / "mingw64" / "bin"
        self.system = root / "Windows" / "System32"
        for d in (self.dest, self.modules, self.mingw, self.system):
            d.mkdir(parents=True, exist_ok=True)
        self.manifest = self.binary / "deploy" / "rt-Release.cmake"
        self.scan = self.binary / "deploy" / "rt-Release.scan"
        self.report = self.binary / "deploy" / "rt-Release.txt"

    def pe(self, path, imports, dll=True, salt=b""):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(make_pe(imports, dll, salt))
        return path

    def system_dll(self, name):
        return self.pe(self.system / name, [])

    def write_manifest(self, executables=(), libraries=(), modules=(),
                       extras=(), allow=False, scan=None, **overrides):
        values = {
            "GF_DEPLOY_EXECUTABLES": list(executables),
            "GF_DEPLOY_LIBRARIES": list(libraries),
            "GF_DEPLOY_MODULES": list(modules),
            "GF_DEPLOY_SEARCH_DIRS": [self.mingw],
            "GF_DEPLOY_EXTRA_DLLS": list(extras),
            "GF_DEPLOY_SYSTEM_DIRS": [self.system],
            "GF_DEPLOY_DEST": [self.dest],
            "GF_DEPLOY_BINARY_DIR": [self.binary],
            "GF_DEPLOY_SCAN_DIR": [scan or self.scan],
            "GF_DEPLOY_REPORT": [self.report],
            "GF_DEPLOY_OBJDUMP": [shutil.which("objdump")],
        }
        values.update(overrides)
        lines = []
        for key, items in values.items():
            if items is None:
                continue
            body = " ".join(_bracket(i) for i in items)
            lines.append(f"set({key} {body})")
        lines.append(f"set(GF_DEPLOY_ALLOW_UNRESOLVED {'ON' if allow else 'OFF'})")
        self.manifest.parent.mkdir(parents=True, exist_ok=True)
        self.manifest.write_text("\n".join(lines) + "\n")

    def run(self, *defines):
        args = ["cmake", f"-DGF_DEPLOY_MANIFEST={self.manifest}"]
        args += [f"-D{d}" for d in defines]
        args += ["-P", str(SCRIPT)]
        return subprocess.run(args, capture_output=True, text=True)

    def deployed(self):
        return sorted(p.name for p in self.dest.iterdir() if p.is_file())


@unittest.skipIf(TOOL_PROBLEM, TOOL_PROBLEM or "")
class DeployScriptTest(unittest.TestCase):

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(prefix="gf deploy ")
        self.t = Tree(Path(self._tmp.name))
        self.t.system_dll("kernel32.dll")

    def tearDown(self):
        self._tmp.cleanup()

    def ok(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result

    def fails(self, result, pattern):
        self.assertNotEqual(result.returncode, 0, result.stdout)
        # CMake wraps a long message wherever it likes.
        text = " ".join((result.stdout + result.stderr).split())
        self.assertRegex(text, pattern)
        return result

    def basic_app(self):
        """app.exe -> libgf_core.dll (project) -> libfoo-1.dll -> libbar.dll."""
        t = self.t
        app = t.pe(t.dest / "app.exe",
                   ["libgf_core.dll", "kernel32.dll",
                    "api-ms-win-crt-runtime-l1-1-0.dll"], dll=False)
        core = t.pe(t.dest / "libgf_core.dll", ["libfoo-1.dll", "kernel32.dll"])
        t.pe(t.mingw / "libfoo-1.dll", ["libbar.dll", "kernel32.dll"])
        t.pe(t.mingw / "libbar.dll", ["kernel32.dll"])
        t.pe(t.mingw / "libunrelated.dll", [])
        return app, core

    # ---- the basic contract ---------------------------------------------

    def test_fresh_destination_gets_the_transitive_closure(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core])
        out = self.ok(self.t.run()).stdout
        self.assertEqual(self.t.deployed(),
                         ["app.exe", "libbar.dll", "libfoo-1.dll",
                          "libgf_core.dll"])
        self.assertIn("2 third-party DLL(s) (2 copied, 0 up to date)", out)
        self.assertEqual(self.t.report.read_text().split(),
                         ["libbar.dll", "libfoo-1.dll"])

    def test_system_and_api_set_dlls_are_never_copied(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core])
        self.ok(self.t.run())
        self.assertNotIn("kernel32.dll", self.t.deployed())

    def test_repeated_run_copies_nothing_and_deletes_nothing(self):
        app, core = self.basic_app()
        (self.t.dest / "stale-from-elsewhere.dll").write_bytes(b"keep me")
        self.t.write_manifest([app], [core])
        self.ok(self.t.run())
        out = self.ok(self.t.run()).stdout
        self.assertIn("(0 copied, 2 up to date)", out)
        self.assertNotIn("MinGW runtime: copied", out)
        self.assertEqual((self.t.dest / "stale-from-elsewhere.dll").read_bytes(),
                         b"keep me")

    def test_deleted_copy_is_restored(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core])
        self.ok(self.t.run())
        (self.t.dest / "libbar.dll").unlink()
        out = self.ok(self.t.run()).stdout
        self.assertIn("copied libbar.dll", out)
        self.assertTrue((self.t.dest / "libbar.dll").exists())

    def test_refreshed_source_is_copied_again(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core])
        self.ok(self.t.run())
        source = self.t.pe(self.t.mingw / "libbar.dll", ["kernel32.dll"],
                           salt=b"v2")
        later = time.time() + 5
        os.utime(source, (later, later))
        self.ok(self.t.run())
        self.assertEqual((self.t.dest / "libbar.dll").read_bytes(),
                         source.read_bytes())

    def test_stale_copy_in_destination_does_not_hide_new_dependency(self):
        # The first run deploys libfoo-1 as it was. MSYS2 then ships a libfoo-1
        # that needs libnew. The stale copy beside the executable does not
        # need it, so a scan of artifacts/bin would never find libnew.
        app, core = self.basic_app()
        self.t.write_manifest([app], [core])
        self.ok(self.t.run())
        self.t.pe(self.t.mingw / "libnew.dll", [])
        foo = self.t.pe(self.t.mingw / "libfoo-1.dll",
                        ["libbar.dll", "libnew.dll"])
        later = time.time() + 5
        os.utime(foo, (later, later))
        self.ok(self.t.run())
        self.assertIn("libnew.dll", self.t.deployed())

    def test_empty_dependency_list(self):
        app = self.t.pe(self.t.dest / "app.exe", ["kernel32.dll"], dll=False)
        self.t.write_manifest([app])
        out = self.ok(self.t.run()).stdout
        self.assertIn("0 third-party DLL(s)", out)
        self.assertEqual(self.t.report.read_text(), "")

    def test_paths_with_spaces_and_modules_in_their_own_directory(self):
        app, core = self.basic_app()
        native = self.t.modules / "com example mod" / "native"
        module = self.t.pe(native / "libmod.dll", ["libmodonly.dll"])
        self.t.pe(self.t.mingw / "libmodonly.dll", [])
        self.t.write_manifest([app], [core], [module])
        self.ok(self.t.run())
        self.assertIn("libmodonly.dll", self.t.deployed())
        # Copied beside the executable, where the loader looks, and not into
        # the module's namespace, whose contents its descriptor binds.
        self.assertEqual(sorted(p.name for p in native.iterdir()), ["libmod.dll"])

    # ---- runtime-loaded extras ------------------------------------------

    def test_extra_dll_is_copied_with_its_dependencies(self):
        app, core = self.basic_app()
        self.t.pe(self.t.mingw / "libssl-3-x64.dll",
                  ["libcrypto-3-x64.dll", "kernel32.dll"])
        self.t.pe(self.t.mingw / "libcrypto-3-x64.dll", ["libz.dll"])
        self.t.pe(self.t.mingw / "libz.dll", [])
        self.t.write_manifest([app], [core], extras=["libssl-3-x64.dll"])
        out = self.ok(self.t.run()).stdout
        for name in ("libssl-3-x64.dll", "libcrypto-3-x64.dll", "libz.dll"):
            self.assertIn(name, self.t.deployed())
        self.assertIn("1 of them runtime-loaded", out)

    def test_missing_extra_dll_fails(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core], extras=["libssl-3-x64.dll"])
        self.fails(self.t.run(), r"runtime-loaded 'libssl-3-x64.dll' is in none")

    # ---- failures --------------------------------------------------------

    def test_unresolved_dependency_fails(self):
        app = self.t.pe(self.t.dest / "app.exe", ["libmissing.dll"], dll=False)
        self.t.write_manifest([app])
        self.fails(self.t.run(), r"libmissing\.dll.*pacman -F")

    def test_unresolved_dependency_can_be_allowed(self):
        app = self.t.pe(self.t.dest / "app.exe",
                        ["libmissing.dll", "libbar.dll"], dll=False)
        self.t.pe(self.t.mingw / "libbar.dll", [])
        self.t.write_manifest([app], allow=True)
        result = self.ok(self.t.run())
        self.assertIn("libmissing.dll", result.stderr)
        self.assertIn("libbar.dll", self.t.deployed())

    def test_same_name_in_two_places_fails(self):
        # The project builds a libbar.dll, and MSYS2 has another one that one
        # of its own DLLs imports: two different files behind one name.
        t = self.t
        app = t.pe(t.dest / "app.exe", ["libbar.dll", "libfoo-1.dll"], dll=False)
        bar = t.pe(t.dest / "libbar.dll", [])
        t.pe(t.mingw / "libfoo-1.dll", ["libbar.dll"])
        t.pe(t.mingw / "libbar.dll", [])
        t.write_manifest([app], [bar])
        self.fails(t.run(), r"'libbar\.dll' resolves to more than one file")

    def test_missing_input_fails(self):
        self.t.write_manifest([self.t.dest / "nope.exe"])
        self.fails(self.t.run(), r"a binary to scan does not exist")

    def test_missing_parameter_fails(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core], GF_DEPLOY_OBJDUMP=None)
        self.fails(self.t.run(), r"GF_DEPLOY_OBJDUMP is not set")

    def test_missing_manifest_fails(self):
        result = subprocess.run(
            ["cmake", "-DGF_DEPLOY_MANIFEST=/nonexistent/m.cmake", "-P",
             str(SCRIPT)], capture_output=True, text=True)
        self.fails(result, r"the manifest does not exist")

    def test_no_inputs_fails(self):
        self.t.write_manifest()
        self.fails(self.t.run(), r"names no binaries to scan")

    # ---- the scan directory is handled with care ------------------------

    def test_scan_directory_outside_deploy_is_refused(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core], scan=self.t.binary / "elsewhere")
        self.fails(self.t.run(), r"must be inside")

    def test_scan_directory_overlapping_destination_is_refused(self):
        app, core = self.basic_app()
        # Inside deploy/ is not enough when the destination is inside it too.
        dest = self.t.binary / "deploy" / "out"
        dest.mkdir(parents=True)
        self.t.write_manifest([app], [core], scan=dest / "scan",
                              GF_DEPLOY_DEST=[dest])
        self.fails(self.t.run(), r"overlaps")

    def test_unmarked_scan_directory_is_left_alone(self):
        app, core = self.basic_app()
        self.t.scan.mkdir(parents=True)
        precious = self.t.scan / "precious.txt"
        precious.write_text("mine")
        self.t.write_manifest([app], [core])
        self.fails(self.t.run(), r"no \.gf-deploy-scan marker")
        self.assertEqual(precious.read_text(), "mine")

    def test_scan_directory_is_recreated_without_leftovers(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core])
        self.ok(self.t.run())
        (self.t.scan / "bin" / "leftover.dll").write_bytes(make_pe([]))
        self.ok(self.t.run())
        self.assertFalse((self.t.scan / "bin" / "leftover.dll").exists())
        # Hard links (or copies) of the inputs; never the inputs themselves.
        self.assertTrue(app.exists() and core.exists())

    # ---- Qt plugins placed by windeployqt -------------------------------

    def test_plugins_are_scanned_only_when_asked(self):
        app, core = self.basic_app()
        self.t.pe(self.t.dest / "imageformats" / "qjpeg.dll", ["libjpeg-8.dll"])
        self.t.pe(self.t.mingw / "libjpeg-8.dll", [])
        # The old flat module directory is not a plugin directory.
        self.t.pe(self.t.dest / "modules" / "libold.dll", ["libnothere.dll"])
        self.t.write_manifest([app], [core])
        self.ok(self.t.run())
        self.assertNotIn("libjpeg-8.dll", self.t.deployed())
        out = self.ok(self.t.run("GF_DEPLOY_SCAN_PLUGINS=ON")).stdout
        self.assertIn("1 plugin(s)", out)
        self.assertIn("libjpeg-8.dll", self.t.deployed())

    # ---- depfile -------------------------------------------------------------

    def test_depfile_lists_inputs_sources_and_copies(self):
        app, core = self.basic_app()
        self.t.write_manifest([app], [core])
        stamp = self.t.binary / "deploy" / "rt.stamp"
        depfile = self.t.binary / "deploy" / "rt.d"
        self.ok(self.t.run(f"GF_DEPLOY_STAMP={stamp}",
                           f"GF_DEPLOY_DEPFILE={depfile}"))
        self.assertTrue(stamp.exists())
        text = depfile.read_text().replace("\\ ", " ")
        for path in (app, core, self.t.mingw / "libbar.dll",
                     self.t.dest / "libbar.dll", self.t.manifest):
            self.assertIn(str(path), text)


@unittest.skipIf(TOOL_PROBLEM or not shutil.which("ninja"),
                 TOOL_PROBLEM or "ninja is required")
class DeployTargetTest(unittest.TestCase):
    """gf_add_mingw_runtime_deploy() wired into a real Ninja build.

    The project binaries are IMPORTED targets over generated PE files, so no
    compiler is involved: what is under test is when the build reruns the
    deployment, not what a linker produces.
    """

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(prefix="gf deploy ")
        root = Path(self._tmp.name)
        self.src = root / "src dir"
        self.build = root / "build dir"
        self.prebuilt = root / "prebuilt"
        self.mingw = root / "msys64" / "mingw64" / "bin"
        self.system = root / "Windows" / "System32"
        self.dest = self.build / "artifacts" / "bin"
        for d in (self.src, self.prebuilt, self.mingw, self.system, self.dest):
            d.mkdir(parents=True)

        (self.system / "kernel32.dll").write_bytes(make_pe([]))
        (self.mingw / "libfoo-1.dll").write_bytes(make_pe(["libbar.dll"]))
        (self.mingw / "libbar.dll").write_bytes(make_pe(["kernel32.dll"]))
        (self.prebuilt / "app.exe").write_bytes(
            make_pe(["libfoo-1.dll", "kernel32.dll"], dll=False))
        (self.prebuilt / "libplugin.dll").write_bytes(make_pe(["libbar.dll"]))

        def cm(path):
            return path.as_posix()

        (self.src / "CMakeLists.txt").write_text(f"""
cmake_minimum_required(VERSION 3.24)
project(deploy_harness NONE)
include("{cm(MODULE)}")
add_executable(app IMPORTED)
set_target_properties(app PROPERTIES IMPORTED_LOCATION "{cm(self.prebuilt)}/app.exe")
add_library(plugin MODULE IMPORTED)
set_target_properties(plugin PROPERTIES IMPORTED_LOCATION "{cm(self.prebuilt)}/libplugin.dll")
add_library(archive STATIC IMPORTED)
set_target_properties(archive PROPERTIES IMPORTED_LOCATION "{cm(self.prebuilt)}/libx.a")
gf_add_mingw_runtime_deploy(gf_deploy_runtime
  TARGETS app plugin archive
  SEARCH_DIRS "{cm(self.mingw)}"
  DESTINATION "{cm(self.dest)}"
  SYSTEM_DIRS "{cm(self.system)}"
  OBJDUMP "{shutil.which('objdump')}")
""")
        self.cmake("-S", self.src, "-B", self.build, "-G", "Ninja")

    def tearDown(self):
        self._tmp.cleanup()

    def cmake(self, *args):
        result = subprocess.run(["cmake", *map(str, args)],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    @staticmethod
    def wait_for_clock():
        # A change made "now" rather than dated into the future: a future
        # input stays newer than the stamp however often the build reruns.
        # The pause lets a coarse filesystem clock tell the two apart.
        time.sleep(1.1)

    def build_all(self):
        out = self.cmake("--build", self.build)
        return "Deploying MinGW runtime DLLs" in out

    def test_incremental_behaviour(self):
        self.assertTrue(self.build_all())
        self.assertTrue((self.dest / "libfoo-1.dll").exists())
        self.assertTrue((self.dest / "libbar.dll").exists())

        # Nothing changed: nothing reruns.
        self.assertFalse(self.build_all())

        # A deleted copy is restored by a plain build, with nothing relinked.
        (self.dest / "libbar.dll").unlink()
        self.assertTrue(self.build_all())
        self.assertTrue((self.dest / "libbar.dll").exists())
        self.assertFalse(self.build_all())

        # A refreshed MSYS2 DLL is copied again, with nothing relinked.
        source = self.mingw / "libfoo-1.dll"
        self.wait_for_clock()
        source.write_bytes(make_pe(["libbar.dll"], salt=b"v2"))
        self.assertTrue(self.build_all())
        self.assertEqual((self.dest / "libfoo-1.dll").read_bytes(),
                         source.read_bytes())

        # A rebuilt project binary (here, one module) reruns it.
        self.wait_for_clock()
        os.utime(self.prebuilt / "libplugin.dll")
        self.assertTrue(self.build_all())
        self.assertFalse(self.build_all())

    def test_rescan_always_runs(self):
        self.build_all()
        for _ in range(2):
            out = self.cmake("--build", self.build, "--target",
                             "gf_deploy_runtime_rescan")
            self.assertIn("Rescanning MinGW runtime DLLs", out)

    def test_static_library_is_not_scanned(self):
        manifest = next((self.build / "deploy").glob("gf_deploy_runtime-*.cmake"))
        self.assertNotIn("libx.a", manifest.read_text())


if __name__ == "__main__":
    unittest.main()
