/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <gtest/gtest.h>

#include <QByteArray>

#include "GpgFrontendTest.h"
#include "core/module/ModuleEntryBinding.h"

/**
 * @file GpgCoreTestModuleEntryBindingModes.cpp
 * @brief The three ways a descriptor binds its entry, and why they differ.
 *
 * A full-file digest is right on Linux and wrong on the other two, because
 * Windows Authenticode signing and macOS code signing both rewrite bytes of a
 * file that is otherwise unchanged. Binding to raw bytes there would either
 * forbid normal platform signing or force the descriptor to be regenerated
 * after it -- and on macOS, regenerating it after signing is what would drag a
 * build-produced packager into a privileged signing job.
 *
 * So each platform binds what is stable on it, and the interesting assertions
 * here are the ones that say what each mode deliberately does NOT notice.
 *
 * ## What is NOT here
 *
 * The Windows mode. It is computed by `ImageGetDigestStream()` now, so there
 * is no GpgFrontend implementation of it left to test, and a synthetic PE
 * exercising Microsoft's code would only assert that Microsoft's code exists.
 * Its contract is proven where it means something: the Windows CI gate signs
 * the module DLLs this build produced with a throwaway certificate and
 * re-verifies them, and then mutates executable content and requires the
 * verification to FAIL.
 *
 * That pair -- certificates invisible, code not -- is the whole trust boundary.
 * It used to be half-proven here against a fixture chosen for convenience,
 * which is exactly how a padding bug survived: the synthetic image happened to
 * be eight-byte aligned, and so did the mistake.
 *
 * The Linux mode is not here either. It is a SHA-256 of the whole file; the
 * test that used to stand in for it flipped a byte in two local arrays and
 * called no GpgFrontend function at all.
 */

namespace GpgFrontend::Test {

namespace {

/// Little-endian, for the synthetic Mach-O headers below.
void PutU32(QByteArray& b, qsizetype at, quint32 v) {
  for (int i = 0; i < 4; ++i) {
    b[at + i] = static_cast<char>((v >> (8 * i)) & 0xFF);
  }
}

}  // namespace

TEST(ModuleEntryBindingModesTest, TheBindingIdIsDeterministic) {
  const Module::ModuleEntryBindingContext a{"com.example.module.email",
                                            "gfb1-abc", 3};
  EXPECT_EQ(Module::ModuleEntryBindingId(a), Module::ModuleEntryBindingId(a));
  EXPECT_EQ(Module::ModuleEntryBindingId(a).size(), 64);
}

TEST(ModuleEntryBindingModesTest, EveryBindingIdInputChangesIt) {
  // Three separate cases rather than one combined change: a derivation that
  // silently ignored an input would pass a test that varied them together.
  const Module::ModuleEntryBindingContext base{"com.example.module.email",
                                               "gfb1-abc", 3};
  const auto reference = Module::ModuleEntryBindingId(base);

  auto other_module = base;
  other_module.module_id = "com.example.module.other";
  EXPECT_NE(Module::ModuleEntryBindingId(other_module), reference)
      << "a dylib from another module must not satisfy this descriptor";

  auto other_build = base;
  other_build.build_id = "gfb1-def";
  EXPECT_NE(Module::ModuleEntryBindingId(other_build), reference)
      << "a module from another build must not satisfy this descriptor";

  auto other_abi = base;
  other_abi.sdk_abi = 4;
  EXPECT_NE(Module::ModuleEntryBindingId(other_abi), reference)
      << "an incompatible ABI must not satisfy this descriptor";
}

TEST(ModuleEntryBindingModesTest, TheBindingIdSeparatorsAreUnambiguous) {
  // Without separators these two would hash the same bytes. They are
  // different modules and must have different bindings.
  const Module::ModuleEntryBindingContext a{"com.example.ab", "c", 3};
  const Module::ModuleEntryBindingContext b{"com.example.a", "bc", 3};
  EXPECT_NE(Module::ModuleEntryBindingId(a), Module::ModuleEntryBindingId(b));
}

TEST(ModuleEntryBindingModesTest, AUniversalBinaryIsRefusedNotGuessedAt) {
  // No build in this matrix produces one, so picking a slice would be
  // choosing which architecture was authoritative.
  QByteArray fat(64, '\0');
  PutU32(fat, 0, 0xBEBAFECA);  // FAT_MAGIC, big-endian on disk

  QString reason;
  EXPECT_TRUE(Module::MachOBindingSection(fat, reason).isEmpty());
  EXPECT_TRUE(reason.contains("universal")) << reason.toStdString();
}

TEST(ModuleEntryBindingModesTest, AMachOWithoutABindingSectionIsRefused) {
  QByteArray macho(128, '\0');
  PutU32(macho, 0, 0xFEEDFACF);  // MH_MAGIC_64
  PutU32(macho, 16, 0);          // no load commands

  QString reason;
  EXPECT_TRUE(Module::MachOBindingSection(macho, reason).isEmpty());
  EXPECT_TRUE(reason.contains("binding")) << reason.toStdString();
}

}  // namespace GpgFrontend::Test
