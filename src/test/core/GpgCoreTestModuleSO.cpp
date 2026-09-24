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
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/struct/settings_object/ModuleSO.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

namespace {

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

TEST_F(GFCoreTest, AFileIdentityIsInvalidForAMissingFile) {
  EXPECT_FALSE(Module::CaptureModuleFileIdentity({}).IsValid());
  EXPECT_FALSE(Module::CaptureModuleFileIdentity("/nonexistent/libgf_mod_x.so")
                   .IsValid());
}

TEST_F(GFCoreTest, AFileIdentityIsStableWhileTheFileIsUntouched) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_fake.so");
  ASSERT_TRUE(WriteFile(path, QByteArray(64, 'a')));

  const auto first = Module::CaptureModuleFileIdentity(path);
  ASSERT_TRUE(first.IsValid());
  EXPECT_EQ(first, Module::CaptureModuleFileIdentity(path));
}

TEST_F(GFCoreTest, AFileSwappedInUnderThePathHasAnotherIdentity) {
  // What an attacker with write access to the modules directory would do
  // between verification and load: put different bytes under the same name,
  // the same size, in the same millisecond.
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_fake.so");
  const auto other = QDir(tmp.path()).absoluteFilePath("replacement");
  ASSERT_TRUE(WriteFile(path, QByteArray(64, 'a')));
  const auto verified = Module::CaptureModuleFileIdentity(path);

  ASSERT_TRUE(WriteFile(other, QByteArray(64, 'b')));
  ASSERT_TRUE(QFile::remove(path));
  ASSERT_TRUE(QFile::rename(other, path));

  EXPECT_NE(verified, Module::CaptureModuleFileIdentity(path));
}

TEST_F(GFCoreTest, AFileThatGrewHasAnotherIdentity) {
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  const auto path = QDir(tmp.path()).absoluteFilePath("libgf_mod_fake.so");
  ASSERT_TRUE(WriteFile(path, QByteArray(64, 'a')));
  const auto verified = Module::CaptureModuleFileIdentity(path);

  ASSERT_TRUE(WriteFile(path, QByteArray(65, 'a')));
  EXPECT_NE(verified, Module::CaptureModuleFileIdentity(path));
}

// One policy for a module's stored settings, used by the loader and the
// Module Controller alike. They used to disagree about integrated modules, and
// both reset a choice the user had made the moment the module was rebuilt --
// which an upgrade always does.
TEST_F(GFCoreTest, ASettingsReconcileKeepsTheUsersChoiceAcrossARebuild) {
  const auto id = QStringLiteral("com.example.reconcile.integrated");

  const auto first = Module::ReconcileModuleSettings(id, "hash-1", true);
  EXPECT_TRUE(first.auto_activate) << "integrated modules start on";
  EXPECT_FALSE(first.set_by_user);

  {
    SettingsObject so(QString("module.%1.so").arg(id));
    ModuleSO chosen(so);
    chosen.auto_activate = false;
    chosen.set_by_user = true;
    so.Store(chosen.ToJson());
  }

  const auto upgraded = Module::ReconcileModuleSettings(id, "hash-2", true);
  EXPECT_EQ(upgraded.module_hash, "hash-2");
  EXPECT_FALSE(upgraded.auto_activate) << "the user turned it off";
  EXPECT_TRUE(upgraded.set_by_user);
}

TEST_F(GFCoreTest, ASettingsReconcileDefaultsAnExternalModuleToOff) {
  const auto so = Module::ReconcileModuleSettings(
      QStringLiteral("com.example.reconcile.external"), "h", false);
  EXPECT_FALSE(so.auto_activate);
  EXPECT_FALSE(so.set_by_user);
}

}  // namespace GpgFrontend::Test
