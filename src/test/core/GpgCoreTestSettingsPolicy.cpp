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

#include "core/function/SettingCatalog.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend::Test {

namespace {

auto PolicyFiles() -> QFileInfoList {
  return QDir(QStringLiteral(GF_TEST_SOURCE_DIR) + "/resource/settings_policy")
      .entryInfoList({"*.json"}, QDir::Files);
}

}  // namespace

// Every flavor's settings policy names settings the catalog has, with values
// a setting can hold: a key it renamed would otherwise be dropped unnoticed.
TEST(SettingsPolicyTest, EveryPolicyNamesOnlyCatalogSettings) {
  const auto files = PolicyFiles();
  ASSERT_FALSE(files.isEmpty());

  for (const auto& info : files) {
    QFile file(info.absoluteFilePath());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly))
        << info.fileName().toStdString();
    const auto json = file.readAll();

    const auto raw =
        QJsonDocument::fromJson(json).object().value("defaults").toObject();
    ASSERT_FALSE(raw.isEmpty()) << info.fileName().toStdString();
    EXPECT_EQ(ParseSettingsPolicy(json).size(), raw.size())
        << info.fileName().toStdString()
        << " names a setting the catalog does not have, or an unusable value";

    // A profile the engine knows, or the default would be one it cannot use.
    const auto profile =
        ParseSettingsPolicy(json).value("engine/argon2_s2k_profile");
    if (profile.isValid()) {
      EXPECT_TRUE(profile == QString(kRpgpArgon2ProfileHighMemory) ||
                  profile == QString(kRpgpArgon2ProfileLowMemory))
          << info.fileName().toStdString();
    }
  }
}

TEST(SettingsPolicyTest, AnUnknownSettingOrValueIsLeftOut) {
  const auto defaults = ParseSettingsPolicy(
      R"({"defaults": {"no/such_setting": 1,
                       "engine/argon2_s2k_profile": ["low_memory"],
                       "network/auto_import_missing_key": true}})");
  EXPECT_EQ(defaults.size(), 1);
  EXPECT_EQ(defaults.value("network/auto_import_missing_key"), QVariant(true));
}

// Where the build compiled a policy in, its defaults are the catalog's.
TEST(SettingsPolicyTest, TheFlavorsPolicyGivesTheDefaults) {
  QFile compiled(QStringLiteral(":/gf/settings_policy.json"));
  if (!compiled.open(QIODevice::ReadOnly)) {
    EXPECT_FALSE(
        FlavorSettingDefault(SettingItems::kArgon2S2kProfile.key).isValid());
    EXPECT_EQ(SettingItems::kArgon2S2kProfile.Default(), "high_memory");
    GTEST_SKIP() << "this flavor has no settings policy";
  }

  const auto defaults = ParseSettingsPolicy(compiled.readAll());
  for (auto it = defaults.cbegin(); it != defaults.cend(); ++it) {
    EXPECT_EQ(FlavorSettingDefault(it.key().toLatin1().constData()), *it)
        << it.key().toStdString();
  }
  if (defaults.contains("engine/argon2_s2k_profile")) {
    EXPECT_EQ(SettingItems::kArgon2S2kProfile.Default(),
              defaults.value("engine/argon2_s2k_profile").toString());
  }
}

}  // namespace GpgFrontend::Test
