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

#include "GpgFrontendTest.h"
#include "SdkTestContext.h"
#include "core/module/ModuleSettingsPolicy.h"
#include "sdk/GFSDK.hpp"
#include "sdk/GFSDKHostCommands.hpp"

/**
 * @file GpgCoreTestModuleSettingsPolicy.cpp
 * @brief Which settings a module reaches, now that it names keys instead of
 *        holding the application's QSettings object.
 */

namespace GpgFrontend::Test {

using Module::HostSettingAccess;
using Module::ModuleSettingScope;

TEST(ModuleSettingsPolicyTest, TwoModulesKeepTheGroupTheyAlwaysUsed) {
  EXPECT_EQ(Module::ModuleSettingsGroup("com.bktus.gpgfrontend.module.email"),
            "email");
  EXPECT_EQ(Module::ModuleSettingsGroup(
                "com.bktus.gpgfrontend.module.key_server_sync"),
            "key_server_sync");
  EXPECT_EQ(Module::ModuleSettingsGroup("com.example.new"),
            "modules/com.example.new");
}

TEST(ModuleSettingsPolicyTest, AKeyCannotLeaveTheModulesGroup) {
  EXPECT_TRUE(Module::IsValidModuleSettingKey("servers/default"));
  EXPECT_TRUE(Module::IsValidModuleSettingKey("update_checking_api"));
  for (const auto* bad : {"", "/abs", "trailing/", "a//b", "a/../b", "..",
                          "a/./b", "sp ace", "back\\slash"}) {
    EXPECT_FALSE(Module::IsValidModuleSettingKey(bad)) << bad;
  }
  EXPECT_TRUE(Module::ResolveModuleSettingKey(
                  "com.example.m", ModuleSettingScope::kMODULE, "../email/x",
                  false)
                  .isEmpty());
  EXPECT_EQ(Module::ResolveModuleSettingKey("com.example.m",
                                            ModuleSettingScope::kMODULE, "a/b",
                                            true),
            "modules/com.example.m/a/b");
}

TEST(ModuleSettingsPolicyTest, OnlyAllowlistedHostSettingsAreShared) {
  EXPECT_EQ(Module::HostSettingAccessFor("network/prohibit_update_check"),
            HostSettingAccess::kREAD_WRITE);
  EXPECT_EQ(Module::HostSettingAccessFor("network/proxy/password"),
            HostSettingAccess::kNONE);
  EXPECT_EQ(Module::ResolveModuleSettingKey("com.example.m",
                                            ModuleSettingScope::kHOST,
                                            "network/prohibit_update_check",
                                            true),
            "network/prohibit_update_check");
  EXPECT_TRUE(Module::ResolveModuleSettingKey(
                  "com.example.m", ModuleSettingScope::kHOST,
                  "general/confirm_import_keys", false)
                  .isEmpty());
}

// The whole path, through a real minted context: what one module writes the
// other cannot see, and a Host key off the list is refused both ways.
TEST(ModuleSettingsPolicyTest, SettingsRoundTripAndStayPrivate) {
  SdkTestContext a("com.example.settings.a");
  SdkTestContext b("com.example.settings.b");

  ASSERT_TRUE(gf::sdk::SetSetting(a(), GF_SETTING_MODULE, "colour", "blue"));
  EXPECT_EQ(gf::sdk::Setting(a(), GF_SETTING_MODULE, "colour").toString(),
            "blue");
  EXPECT_FALSE(gf::sdk::Setting(b(), GF_SETTING_MODULE, "colour").isValid())
      << "another module's group is invisible";

  ASSERT_TRUE(gf::sdk::SetSetting(a(), GF_SETTING_MODULE, "list",
                                  QStringList{"x", "y"}));
  EXPECT_EQ(gf::sdk::Setting(a(), GF_SETTING_MODULE, "list").toStringList(),
            (QStringList{"x", "y"}));

  EXPECT_FALSE(gf::sdk::SetSetting(a(), GF_SETTING_HOST, "general/anything", 1));
  EXPECT_FALSE(gf::sdk::Setting(a(), GF_SETTING_HOST, "general/anything")
                   .isValid());

  EXPECT_TRUE(gf::sdk::RemoveSetting(a(), GF_SETTING_MODULE, "colour"));
  EXPECT_TRUE(gf::sdk::RemoveSetting(a(), GF_SETTING_MODULE, "list"));
  EXPECT_FALSE(gf::sdk::Setting(a(), GF_SETTING_MODULE, "colour").isValid());

  // Without the storage capability, nothing.
  SdkTestContext none("com.example.settings.none", GF_HOST_CAP_UI);
  EXPECT_FALSE(gf::sdk::SetSetting(none(), GF_SETTING_MODULE, "k", 1));
}

// The Host's commands are types, and everything about them is derived: a
// wrong field here would be a wrong schema everywhere.
TEST(ModuleSettingsPolicyTest, HostCommandDescriptorsAreWellFormed) {
  const std::array<QCborMap, 16> all = {
      gf::cmd::Describe<gf::cmd::host::DocumentNew>(),
      gf::cmd::Describe<gf::cmd::host::DocumentOpen>(),
      gf::cmd::Describe<gf::cmd::host::DocumentSave>(),
      gf::cmd::Describe<gf::cmd::host::DocumentSaveAs>(),
      gf::cmd::Describe<gf::cmd::host::DocumentClose>(),
      gf::cmd::Describe<gf::cmd::host::CryptoEncrypt>(),
      gf::cmd::Describe<gf::cmd::host::CryptoDecrypt>(),
      gf::cmd::Describe<gf::cmd::host::CryptoSign>(),
      gf::cmd::Describe<gf::cmd::host::CryptoVerify>(),
      gf::cmd::Describe<gf::cmd::host::CryptoEncryptSign>(),
      gf::cmd::Describe<gf::cmd::host::CryptoDecryptVerify>(),
      gf::cmd::Describe<gf::cmd::host::KeysImport>(),
      gf::cmd::Describe<gf::cmd::host::KeysOpenManager>(),
      gf::cmd::Describe<gf::cmd::host::ViewOpen>(),
      gf::cmd::Describe<gf::cmd::host::AppOpenSettings>(),
      gf::cmd::Describe<gf::cmd::host::AppMessage>(),
  };
  QStringList ids;
  for (const auto& d : all) {
    const auto id = d.value(QStringLiteral("id")).toString();
    EXPECT_TRUE(id.startsWith("org.gpgfrontend.")) << id.toStdString();
    EXPECT_FALSE(ids.contains(id)) << id.toStdString();
    ids.append(id);
    EXPECT_EQ(d.value(QStringLiteral("args")).toMap().value("type").toString(),
              "object");
    EXPECT_NE(d.value(QStringLiteral("flags")).toInteger() &
                  gf::cmd::kNeedsGuiThread,
              0)
        << id.toStdString() << ": every Host command touches the UI";
  }

  // The secret-bearing fields are Blobs, so their bytes never enter CBOR.
  const auto open_fields = gf::cmd::Describe<gf::cmd::host::DocumentOpen>()
                               .value("args")
                               .toMap()
                               .value("fields")
                               .toArray();
  bool content_is_blob = false;
  for (const auto& f : open_fields) {
    if (f.toMap().value("name").toString() == "content") {
      content_is_blob =
          f.toMap().value("schema").toMap().value("type").toString() == "blob";
    }
  }
  EXPECT_TRUE(content_is_blob);
}

}  // namespace GpgFrontend::Test
