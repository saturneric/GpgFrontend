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

#include <QTemporaryDir>

#include "GFCoreTest.h"
#include "core/module/ModuleManager.h"
#include "core/struct/settings_object/ModuleSO.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

namespace {

/**
 * @brief The image header InspectModuleLibrary() accepts on this platform.
 */
auto NativeImageHeader() -> QByteArray {
#if defined(Q_OS_WINDOWS)
  return QByteArrayLiteral("MZ\x90\x00");
#elif defined(Q_OS_MACOS)
  return QByteArrayLiteral("\xcf\xfa\xed\xfe");
#else
  return QByteArray("\x7f", 1) + "ELF";
#endif
}

auto WriteFile(const QString& path, const QByteArray& data) -> bool {
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly)) return false;
  return f.write(data) == data.size();
}

}  // namespace

TEST_F(GFCoreTest, ModuleSODefaultsAreFalse) {
  const ModuleSO so{};

  EXPECT_FALSE(so.auto_activate);
  EXPECT_FALSE(so.set_by_user);
  EXPECT_TRUE(so.module_id.isEmpty());
}

TEST_F(GFCoreTest, ModuleSOEmptyJsonKeepsFlagsFalse) {
  // a module without stored settings must never be auto activated by accident
  const ModuleSO so{QJsonObject{}};

  EXPECT_FALSE(so.auto_activate);
  EXPECT_FALSE(so.set_by_user);
  EXPECT_TRUE(so.module_id.isEmpty());
  EXPECT_TRUE(so.module_hash.isEmpty());
}

TEST_F(GFCoreTest, ModuleSORoundTrip) {
  ModuleSO so;
  so.module_id = "com.bktus.gpgfrontend.module.test";
  so.module_version = "1.2.3";
  so.module_hash = "a91f00ff";
  so.auto_activate = true;
  so.set_by_user = true;

  const ModuleSO restored{so.ToJson()};

  EXPECT_EQ(restored.module_id, so.module_id);
  EXPECT_EQ(restored.module_version, so.module_version);
  EXPECT_EQ(restored.module_hash, so.module_hash);
  EXPECT_TRUE(restored.auto_activate);
  EXPECT_TRUE(restored.set_by_user);
}

TEST_F(GFCoreTest, ModuleSOIgnoresWrongTypedJsonValues) {
  QJsonObject j;
  j["module_id"] = 42;
  j["auto_activate"] = "yes";

  const ModuleSO so{j};

  EXPECT_TRUE(so.module_id.isEmpty());
  EXPECT_FALSE(so.auto_activate);
}

TEST_F(GFCoreTest, ModuleLibrarySearchPathOfEmptyPathIsEmpty) {
  EXPECT_TRUE(Module::ResolveModuleLibrarySearchPath({}).isEmpty());
}

TEST_F(GFCoreTest, ModuleLibrarySearchPathOfMissingDirectoryIsEmpty) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto path =
      QDir(tmp.path()).absoluteFilePath("no_such_dir/libgf_mod_test.so");

  EXPECT_TRUE(Module::ResolveModuleLibrarySearchPath(path).isEmpty());
}

TEST_F(GFCoreTest, ModuleLibrarySearchPathIsTheModuleDirectory) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_test.so");

  // the module itself need not exist, only the directory it is loaded from
  EXPECT_EQ(Module::ResolveModuleLibrarySearchPath(path),
            QDir::toNativeSeparators(QDir(tmp.path()).absolutePath()));
}

TEST_F(GFCoreTest, CalculateBinaryChacksumOfOpenDeviceMatchesPath) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto path = QDir(tmp.path()).absoluteFilePath("blob.bin");
  ASSERT_TRUE(WriteFile(path, QByteArray(20000, 'x')));

  QFile f(path);
  ASSERT_TRUE(f.open(QIODevice::ReadOnly));

  // the device is deliberately advanced first: the overload must rewind it
  ASSERT_EQ(f.read(8).size(), 8);

  const auto from_device = CalculateBinaryChacksum(f);
  EXPECT_FALSE(from_device.isEmpty());
  EXPECT_EQ(from_device, CalculateBinaryChacksum(path));
}

TEST_F(GFCoreTest, CalculateBinaryChacksumOfClosedDeviceIsEmpty) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto path = QDir(tmp.path()).absoluteFilePath("blob.bin");
  ASSERT_TRUE(WriteFile(path, QByteArrayLiteral("data")));

  QFile f(path);
  EXPECT_TRUE(CalculateBinaryChacksum(f).isEmpty());
}

TEST_F(GFCoreTest, ModuleLibraryFileNameRuleMatchesTheScanFilter) {
  EXPECT_TRUE(Module::IsModuleLibraryFileName("libgf_mod_test.so"));
  EXPECT_TRUE(Module::IsModuleLibraryFileName("libgf_mod_test.dll"));
  EXPECT_TRUE(Module::IsModuleLibraryFileName("libgf_mod_test.dylib"));

  EXPECT_FALSE(Module::IsModuleLibraryFileName("libgf_mod_"));
  EXPECT_FALSE(Module::IsModuleLibraryFileName("gf_mod_test.so"));
  EXPECT_FALSE(Module::IsModuleLibraryFileName("libgf_core.so"));
  EXPECT_FALSE(Module::IsModuleLibraryFileName({}));
  // the rule is anchored, a prefix elsewhere in the name must not match
  EXPECT_FALSE(Module::IsModuleLibraryFileName("evil_libgf_mod_test.so"));
}

TEST_F(GFCoreTest, InspectModuleLibraryRejectsEmptyPath) {
  const auto r = Module::InspectModuleLibrary({});

  EXPECT_FALSE(r.ok);
  EXPECT_FALSE(r.reason.isEmpty());
  EXPECT_TRUE(r.hash.isEmpty());
}

TEST_F(GFCoreTest, InspectModuleLibraryRejectsMissingFile) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto r = Module::InspectModuleLibrary(
      QDir(tmp.path()).absoluteFilePath("libgf_mod_missing.so"));

  EXPECT_FALSE(r.ok);
  EXPECT_TRUE(r.hash.isEmpty());
}

TEST_F(GFCoreTest, InspectModuleLibraryRejectsDirectory) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  ASSERT_TRUE(QDir(tmp.path()).mkdir("libgf_mod_dir.so"));

  const auto r = Module::InspectModuleLibrary(
      QDir(tmp.path()).absoluteFilePath("libgf_mod_dir.so"));

  EXPECT_FALSE(r.ok);
  EXPECT_TRUE(r.hash.isEmpty());
}

TEST_F(GFCoreTest, InspectModuleLibraryRejectsEmptyFile) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_empty.so");
  ASSERT_TRUE(WriteFile(path, {}));

  const auto r = Module::InspectModuleLibrary(path);

  EXPECT_FALSE(r.ok);
  EXPECT_TRUE(r.hash.isEmpty());
}

TEST_F(GFCoreTest, InspectModuleLibraryRejectsForeignFileName) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  // a perfectly valid image, but not under a module name
  const auto path = QDir(tmp.path()).absoluteFilePath("notamodule.so");
  ASSERT_TRUE(WriteFile(path, NativeImageHeader() + QByteArray(512, '\0')));

  const auto r = Module::InspectModuleLibrary(path);

  EXPECT_FALSE(r.ok);
  EXPECT_TRUE(r.hash.isEmpty());
}

TEST_F(GFCoreTest, InspectModuleLibraryRejectsNonExecutableImage) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_fake.so");
  ASSERT_TRUE(WriteFile(path, QByteArrayLiteral("#!/bin/sh\necho hi\n")));

  const auto r = Module::InspectModuleLibrary(path);

  EXPECT_FALSE(r.ok);
  EXPECT_FALSE(r.reason.isEmpty());
  EXPECT_TRUE(r.hash.isEmpty());
}

TEST_F(GFCoreTest, InspectModuleLibraryRejectsTruncatedImage) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  // shorter than any image header the loader could work with
  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_short.so");
  ASSERT_TRUE(WriteFile(path, NativeImageHeader().left(1)));

  const auto r = Module::InspectModuleLibrary(path);

  EXPECT_FALSE(r.ok);
  EXPECT_TRUE(r.hash.isEmpty());
}

TEST_F(GFCoreTest, InspectModuleLibraryAcceptsImageAndHashesTheWholeFile) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_fake.so");
  const auto content = NativeImageHeader() + QByteArray(9000, 'z');
  ASSERT_TRUE(WriteFile(path, content));

  const auto r = Module::InspectModuleLibrary(path);

  EXPECT_TRUE(r.ok);
  EXPECT_TRUE(r.reason.isEmpty());
  // the header read must not shorten what gets hashed
  EXPECT_EQ(r.hash, CalculateBinaryChacksum(path));
  EXPECT_FALSE(r.hash.isEmpty());
}

TEST_F(GFCoreTest, InspectModuleLibraryHashFollowsTheFileContent) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_fake.so");

  ASSERT_TRUE(WriteFile(path, NativeImageHeader() + QByteArray(64, 'a')));
  const auto first = Module::InspectModuleLibrary(path);
  ASSERT_TRUE(first.ok);

  ASSERT_TRUE(WriteFile(path, NativeImageHeader() + QByteArray(64, 'b')));
  const auto second = Module::InspectModuleLibrary(path);
  ASSERT_TRUE(second.ok);

  EXPECT_NE(first.hash, second.hash);
}

}  // namespace GpgFrontend::Test
