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
#include "core/function/AppSecureKeyManager.h"
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

// How the application key file is protected at rest used to be spread across
// two settings keys: advanced/os_secret_store for the system keychain, and
// advanced/secure_level >= 3 for a PIN. Both now resolve into a single
// advanced/app_key_protection, and a profile written before the split has to go
// on resolving the same way or its key file would stop opening.

namespace {

/// Absent layer, spelled out so the ladder tests read as a table.
const auto kUnset = QVariant();

/// Resolve with only the ENV.ini layer populated.
auto EnvOnly(const QVariant& protection, const QVariant& secure_level,
             const QVariant& os_secret_store) -> AppKeyProtection {
  return ResolveAppKeyProtection(protection, secure_level, os_secret_store,
                                 kUnset, kUnset, kUnset);
}

/// Resolve with only the user-settings layer populated.
auto UserOnly(const QVariant& protection, const QVariant& secure_level,
              const QVariant& os_secret_store) -> AppKeyProtection {
  return ResolveAppKeyProtection(kUnset, kUnset, kUnset, protection,
                                 secure_level, os_secret_store);
}

}  // namespace

TEST(AppKeyProtectionSettingsTest, NothingSetMeansNoProtection) {
  EXPECT_EQ(
      ResolveAppKeyProtection(kUnset, kUnset, kUnset, kUnset, kUnset, kUnset),
      AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, EnvProtectionWinsOverEverything) {
  // ENV.ini is a deployment override: whatever it says about the protection
  // beats every legacy key and every user choice.
  EXPECT_EQ(ResolveAppKeyProtection(QVariant("keychain"), QVariant(3),
                                    QVariant(false), QVariant("pin"),
                                    QVariant(3), QVariant(true)),
            AppKeyProtection::kKEYCHAIN);
}

TEST(AppKeyProtectionSettingsTest, SecureLevelThreeMapsToPin) {
  // The compatibility rung that keeps a pre-split level-3 profile starting: its
  // key file is sealed with a PIN, so it must keep resolving to kPIN.
  EXPECT_EQ(EnvOnly(kUnset, QVariant(3), kUnset), AppKeyProtection::kPIN);
  EXPECT_EQ(UserOnly(kUnset, QVariant(3), kUnset), AppKeyProtection::kPIN);
}

TEST(AppKeyProtectionSettingsTest, SecureLevelBelowThreeSaysNothing) {
  // Levels 0..2 only ever meant memory hardening, so they must fall through
  // rather than assert that the key file is unprotected.
  EXPECT_EQ(EnvOnly(kUnset, QVariant(2), QVariant(true)),
            AppKeyProtection::kKEYCHAIN);
  EXPECT_EQ(UserOnly(kUnset, QVariant(0), QVariant(true)),
            AppKeyProtection::kKEYCHAIN);
}

TEST(AppKeyProtectionSettingsTest, OsSecretStoreMapsToKeychain) {
  EXPECT_EQ(EnvOnly(kUnset, kUnset, QVariant(true)),
            AppKeyProtection::kKEYCHAIN);
  EXPECT_EQ(UserOnly(kUnset, kUnset, QVariant(true)),
            AppKeyProtection::kKEYCHAIN);
}

TEST(AppKeyProtectionSettingsTest, ExplicitlyDisabledStoreDoesNotFallThrough) {
  // An explicit false is an answer, not an absence. If it fell through, an
  // ENV.ini that switched the keychain off would be overridden by a stale user
  // setting that had switched it on.
  EXPECT_EQ(ResolveAppKeyProtection(kUnset, kUnset, QVariant(false), kUnset,
                                    kUnset, QVariant(true)),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, UserProtectionWinsOverItsOwnLegacyKeys) {
  // Once the user has made an explicit choice it is final, even when the keys
  // it replaced still hold their old values.
  EXPECT_EQ(UserOnly(QVariant("none"), QVariant(3), QVariant(true)),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, EnvLayerIsTriedWholeBeforeTheUserLayer) {
  // An ENV.ini that only sets the legacy OSSecretStore key still beats a user
  // setting on the new key — otherwise a deployment override would be silently
  // demoted the moment the user touched the combo.
  EXPECT_EQ(ResolveAppKeyProtection(kUnset, kUnset, QVariant(true),
                                    QVariant("pin"), kUnset, kUnset),
            AppKeyProtection::kKEYCHAIN);
}

TEST(AppKeyProtectionSettingsTest, SpellingsAreCaseInsensitive) {
  EXPECT_EQ(AppKeyProtectionFromString("pin"), AppKeyProtection::kPIN);
  EXPECT_EQ(AppKeyProtectionFromString("PIN"), AppKeyProtection::kPIN);
  EXPECT_EQ(AppKeyProtectionFromString("  Keychain "),
            AppKeyProtection::kKEYCHAIN);
}

TEST(AppKeyProtectionSettingsTest, UnknownSpellingDegradesToNoProtection) {
  // A typo in ENV.ini must leave the key unprotected rather than demand a PIN
  // that nobody ever set, which would be an unopenable profile.
  EXPECT_EQ(AppKeyProtectionFromString("banana"), AppKeyProtection::kNONE);
  EXPECT_EQ(AppKeyProtectionFromString(""), AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, SpellingRoundTrips) {
  for (const auto p : {AppKeyProtection::kNONE, AppKeyProtection::kKEYCHAIN,
                       AppKeyProtection::kPIN}) {
    EXPECT_EQ(AppKeyProtectionFromString(AppKeyProtectionToString(p)), p);
  }
}

// A portable installation is meant to be carried to another computer. A
// keychain secret cannot follow it there, so that mode is refused outright; a
// PIN travels with the directory and is the only real protection available.

TEST(AppKeyProtectionSettingsTest, PortableRefusesTheKeychain) {
  EXPECT_EQ(ApplyPortableModeRule(AppKeyProtection::kKEYCHAIN, true),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, PortableAllowsPinAndNoProtection) {
  EXPECT_EQ(ApplyPortableModeRule(AppKeyProtection::kPIN, true),
            AppKeyProtection::kPIN);
  EXPECT_EQ(ApplyPortableModeRule(AppKeyProtection::kNONE, true),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, InstalledAllowsEveryMode) {
  for (const auto p : {AppKeyProtection::kNONE, AppKeyProtection::kKEYCHAIN,
                       AppKeyProtection::kPIN}) {
    EXPECT_EQ(ApplyPortableModeRule(p, false), p);
  }
}

TEST(AppKeyProtectionSettingsTest, PortableRuleOverridesEnvIni) {
  // ENV.ini cannot know where the directory will be plugged in, so even an
  // explicit deployment request for the keychain is downgraded.
  const auto resolved = EnvOnly(QVariant("keychain"), kUnset, kUnset);
  EXPECT_EQ(ApplyPortableModeRule(resolved, true), AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, IniStringFormsResolveCorrectly) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("ENV.ini");
  {
    QSettings w(path, QSettings::IniFormat);
    w.setValue("AppKeyProtection", "pin");
    w.setValue("SecureLevel", "2");
    w.sync();
  }

  QSettings s(path, QSettings::IniFormat);
  EXPECT_EQ(EnvOnly(s.value("AppKeyProtection"), s.value("SecureLevel"),
                    s.value("OSSecretStore")),
            AppKeyProtection::kPIN);

  // The level is read unclamped and keeps its own meaning: memory hardening,
  // and at 3 the weekly key rotation. It no longer implies a PIN.
  EXPECT_EQ(s.value("SecureLevel").toInt(), 2);
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
