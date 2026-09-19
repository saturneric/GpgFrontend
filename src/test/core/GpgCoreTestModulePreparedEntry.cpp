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

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleManifest.h"
#include "core/module/ModulePreparedEntry.h"

/**
 * @file GpgCoreTestModulePreparedEntry.cpp
 * @brief The seal a preparation step leaves, and what reading it refuses.
 *
 * The seal exists because the tools that rewrite a prepared native --
 * `patchelf`, `linuxdeployqt`, `install_name_tool` -- run outside the build
 * graph, so "the descriptor is newer than the library" proves nothing. Every
 * case here is about the seal being understood completely or not at all: a
 * half-read seal is worse than no seal, because the half that was understood
 * would look like a passing check.
 */

namespace GpgFrontend::Test {

namespace {

auto Sealed() -> Module::PreparedEntrySeal {
  Module::PreparedEntrySeal seal;
  seal.module_id = "com.bktus.gpgfrontend.module.email";
  seal.build_id = "gfb1-0123456789abcdef0123456789abcdef";
  seal.entry_native_name = "gf_mod_email";
  seal.mode = Module::ModuleEntryVerificationMode::kFILE_SHA256;
  seal.value = QString(64, u'a');
  seal.size = 2118344;
  return seal;
}

/// Write @p text verbatim, so a case can say exactly what the file holds.
auto WriteRaw(const QString& path, const QByteArray& text) -> bool {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
  file.write(text);
  file.close();
  return true;
}

class ModulePreparedEntryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    path_ = dir_.path() + "/prepared.json";
  }

  QTemporaryDir dir_;
  QString path_;
};

}  // namespace

TEST_F(ModulePreparedEntryTest, ASealRoundTrips) {
  const auto written = Sealed();
  QString why;
  ASSERT_TRUE(Module::WritePreparedEntrySeal(path_, written, why)) << why.toStdString();

  Module::PreparedEntrySeal read;
  ASSERT_TRUE(Module::ReadPreparedEntrySeal(path_, read, why)) << why.toStdString();

  EXPECT_EQ(read.module_id, written.module_id);
  EXPECT_EQ(read.build_id, written.build_id);
  EXPECT_EQ(read.entry_native_name, written.entry_native_name);
  EXPECT_EQ(read.mode, written.mode);
  EXPECT_EQ(read.value, written.value);
  EXPECT_EQ(read.size, written.size);
}

TEST_F(ModulePreparedEntryTest, WritingReplacesAStaleSeal) {
  // The failure this whole mechanism exists to notice is a stale seal
  // surviving, so the write must truncate rather than leave a longer old file
  // half in place.
  ASSERT_TRUE(WriteRaw(path_, QByteArray(4096, 'x')));

  auto seal = Sealed();
  QString why;
  ASSERT_TRUE(Module::WritePreparedEntrySeal(path_, seal, why)) << why.toStdString();

  Module::PreparedEntrySeal read;
  EXPECT_TRUE(Module::ReadPreparedEntrySeal(path_, read, why)) << why.toStdString();
  EXPECT_EQ(read.value, seal.value);
}

TEST_F(ModulePreparedEntryTest, ASizelessSealIsFine) {
  auto seal = Sealed();
  seal.size = -1;
  QString why;
  ASSERT_TRUE(Module::WritePreparedEntrySeal(path_, seal, why)) << why.toStdString();

  Module::PreparedEntrySeal read;
  ASSERT_TRUE(Module::ReadPreparedEntrySeal(path_, read, why)) << why.toStdString();
  EXPECT_LT(read.size, 0);
}

TEST_F(ModulePreparedEntryTest, ANonLinuxModeCarriesNoSize) {
  // Same rule as the manifest's: Windows and macOS signing both change file
  // size, so a size under either is an invariant that legitimately breaks.
  auto seal = Sealed();
  seal.mode = Module::ModuleEntryVerificationMode::kAPPLE_BINDING_ID;
  seal.size = -1;
  QString why;
  ASSERT_TRUE(Module::WritePreparedEntrySeal(path_, seal, why)) << why.toStdString();

  Module::PreparedEntrySeal read;
  ASSERT_TRUE(Module::ReadPreparedEntrySeal(path_, read, why)) << why.toStdString();
  EXPECT_EQ(read.mode, Module::ModuleEntryVerificationMode::kAPPLE_BINDING_ID);
}

TEST_F(ModulePreparedEntryTest, ASizeUnderANonLinuxModeIsRefused) {
  ASSERT_TRUE(WriteRaw(path_, R"({
    "schema": 1,
    "module_id": "com.bktus.gpgfrontend.module.email",
    "build_id": "gfb1-x",
    "entry_native_name": "gf_mod_email",
    "mode": "apple-binding-id",
    "value": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "size": 12
  })"));

  Module::PreparedEntrySeal read;
  QString why;
  EXPECT_FALSE(Module::ReadPreparedEntrySeal(path_, read, why));
  EXPECT_TRUE(why.contains("size")) << why.toStdString();
}

TEST_F(ModulePreparedEntryTest, AnUnwritableValueIsRefusedBeforeAnyFileAppears) {
  auto seal = Sealed();
  seal.value = "not a digest";
  QString why;
  EXPECT_FALSE(Module::WritePreparedEntrySeal(path_, seal, why));
  EXPECT_FALSE(QFileInfo::exists(path_))
      << "a refused seal must not leave a file behind for the next step to "
         "read";
}

TEST_F(ModulePreparedEntryTest, AnUnknownModeIsRefusedRatherThanDefaulted) {
  ASSERT_TRUE(WriteRaw(path_, R"({
    "schema": 1,
    "module_id": "a",
    "build_id": "b",
    "entry_native_name": "c",
    "mode": "sha512-of-something",
    "value": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  })"));

  Module::PreparedEntrySeal read;
  QString why;
  EXPECT_FALSE(Module::ReadPreparedEntrySeal(path_, read, why));
  EXPECT_TRUE(why.contains("sha512-of-something")) << why.toStdString();
}

TEST_F(ModulePreparedEntryTest, AMalformedValueIsRefused) {
  ASSERT_TRUE(WriteRaw(path_, R"({
    "schema": 1,
    "module_id": "a",
    "build_id": "b",
    "entry_native_name": "c",
    "mode": "file-sha256",
    "value": "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
  })"));

  // Upper case is not merely unusual here: every comparison in this subsystem
  // is exact, so a seal spelled differently would never match anything.
  Module::PreparedEntrySeal read;
  QString why;
  EXPECT_FALSE(Module::ReadPreparedEntrySeal(path_, read, why));
}

TEST_F(ModulePreparedEntryTest, EveryFieldIsRequired) {
  for (const auto* missing : {"module_id", "build_id", "entry_native_name",
                              "mode", "value"}) {
    auto seal = Sealed();
    QString why;
    ASSERT_TRUE(Module::WritePreparedEntrySeal(path_, seal, why)) << why.toStdString();

    QFile file(path_);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    auto text = QString::fromUtf8(file.readAll());
    file.close();

    // Rename the key rather than delete the line, so the JSON stays valid and
    // the refusal is about the missing field rather than about a parse error.
    text.replace(QString("\"%1\"").arg(missing), "\"was_here\"");
    ASSERT_TRUE(WriteRaw(path_, text.toUtf8()));

    Module::PreparedEntrySeal read;
    EXPECT_FALSE(Module::ReadPreparedEntrySeal(path_, read, why))
        << "a seal with no " << missing << " was accepted";
    EXPECT_TRUE(why.contains(missing)) << missing << ": " << why.toStdString();
  }
}

TEST_F(ModulePreparedEntryTest, AnotherSchemaIsRefusedRatherThanMigrated) {
  // Deliberately not forward-compatible. A seal is consumed minutes after it
  // is written, by the same build, so a version skew means the tree is
  // inconsistent and continuing would be worse than stopping.
  ASSERT_TRUE(WriteRaw(path_, R"({
    "schema": 2,
    "module_id": "a",
    "build_id": "b",
    "entry_native_name": "c",
    "mode": "file-sha256",
    "value": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  })"));

  Module::PreparedEntrySeal read;
  QString why;
  EXPECT_FALSE(Module::ReadPreparedEntrySeal(path_, read, why));
}

TEST_F(ModulePreparedEntryTest, ANonexistentSealIsRefusedAndSaysSo) {
  Module::PreparedEntrySeal read;
  QString why;
  EXPECT_FALSE(
      Module::ReadPreparedEntrySeal(dir_.path() + "/nope.json", read, why));
  EXPECT_FALSE(why.isEmpty());
}

TEST_F(ModulePreparedEntryTest, TheSealedValueIsWhatTheBinderComputes) {
  // The seal is not a separate calculation. If it were, it could agree with
  // the descriptor while both disagreed with the runtime.
  QTemporaryDir native;
  ASSERT_TRUE(native.isValid());

  const auto path = native.path() + "/libgf_mod_probe.so";
  ASSERT_TRUE(WriteRaw(path, QByteArray("\x7f", 1) + "ELF" +
                                 QByteArray(256, '\x11')));

  QString value;
  QString why;
  const Module::ModuleEntryBindingContext context{"com.example.m", "gfb1-x", 3};
  ASSERT_TRUE(Module::ComputeEntryVerificationValue(
      Module::ModuleEntryVerificationMode::kFILE_SHA256, path, context, value,
      why))
      << why.toStdString();

  auto seal = Sealed();
  seal.value = value;
  seal.size = QFileInfo(path).size();
  ASSERT_TRUE(Module::WritePreparedEntrySeal(path_, seal, why)) << why.toStdString();

  Module::PreparedEntrySeal read;
  ASSERT_TRUE(Module::ReadPreparedEntrySeal(path_, read, why)) << why.toStdString();
  EXPECT_EQ(read.value, value);
}

}  // namespace GpgFrontend::Test
