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

#include <QSettings>
#include <QTemporaryDir>

#include "core/GFCoreLog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Test {

// The three-layer resolution behind the Advanced settings tab: an ENV.ini key
// is a deployment override that beats the user's stored value, which in turn
// beats the built-in default.

TEST(SettingsLayeringTest, EnvValueWinsOverUserAndFallback) {
  EXPECT_EQ(ResolveLayeredValue(QVariant(2), QVariant(1), QVariant(0)).toInt(),
            2);
}

TEST(SettingsLayeringTest, UserValueWinsWhenEnvAbsent) {
  EXPECT_EQ(ResolveLayeredValue(QVariant(), QVariant(1), QVariant(0)).toInt(),
            1);
}

TEST(SettingsLayeringTest, FallbackUsedWhenNoLayerHasValue) {
  EXPECT_EQ(ResolveLayeredValue(QVariant(), QVariant(), QVariant(0)).toInt(),
            0);
}

TEST(SettingsLayeringTest, FalseAndZeroAreValuesNotAbsence) {
  // A stored `false` / `0` must not fall through to the default — that would
  // make an explicitly disabled self-check silently re-enable itself.
  EXPECT_FALSE(ResolveLayeredValue(QVariant(), QVariant(false), QVariant(true))
                   .toBool());
  EXPECT_EQ(ResolveLayeredValue(QVariant(0), QVariant(1), QVariant(3)).toInt(),
            0);
}

TEST(SettingsLayeringTest, EmptyStringFromEnvStillOverrides) {
  // QSettings returns an invalid QVariant for a missing key but a valid empty
  // string for `Key=`, so an intentionally blanked ENV.ini key must win.
  const auto r =
      ResolveLayeredValue(QVariant(QString()), QVariant("user"), QVariant("d"));
  EXPECT_TRUE(r.isValid());
  EXPECT_TRUE(r.toString().isEmpty());
}

// The INI layer stores everything as text. These are the exact conversions the
// startup resolution performs, and the one that historically bites is "false"
// reading back as boolean true.

TEST(SettingsLayeringTest, IniStringBooleansConvertCorrectly) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("ENV.ini");
  {
    QSettings w(path, QSettings::IniFormat);
    w.setValue("SelfCheck", "false");
    w.setValue("SecureLevel", "2");
    w.setValue("LogRingBufferCapacity", "4096");
    w.sync();
  }

  QSettings s(path, QSettings::IniFormat);
  EXPECT_FALSE(s.value("SelfCheck").toBool());
  EXPECT_EQ(s.value("SecureLevel").toInt(), 2);
  EXPECT_EQ(s.value("LogRingBufferCapacity").toInt(), 4096);

  // A key that is not present must stay invalid so it falls through the layers.
  EXPECT_FALSE(s.value("LogLevel").isValid());
  EXPECT_EQ(ResolveLayeredValue(s.value("LogLevel"), QVariant(),
                                static_cast<int>(GFLogLevel::kCRITICAL))
                .toInt(),
            static_cast<int>(GFLogLevel::kCRITICAL));
}

// GetEarlySettings() hand-resolves the settings file so it can run before the
// secure allocator exists. If it ever drifts from the singleton's own path, the
// Advanced tab would write to one file while startup reads another.
TEST(SettingsLayeringTest, EarlySettingsTargetsSameStoreAsSingleton) {
  EXPECT_EQ(GetEarlySettings().fileName(), GetSettings().fileName());
}

TEST(SettingsLayeringTest, EarlySettingsSeesValueWrittenViaSingleton) {
  const auto* kKey = "advanced/test_roundtrip_probe";
  auto settings = GetSettings();
  const auto previous = settings.value(kKey);

  settings.setValue(kKey, 4242);
  settings.sync();

  EXPECT_EQ(GetEarlySettings().value(kKey).toInt(), 4242);

  if (previous.isValid()) {
    settings.setValue(kKey, previous);
  } else {
    settings.remove(kKey);
  }
  settings.sync();
}

}  // namespace GpgFrontend::Test
