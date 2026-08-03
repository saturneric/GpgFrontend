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
#include "core/utils/BuildInfoUtils.h"
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

// The startup self-check compares the shipped files against signatures that are
// only made when an official stable release is built. A nightly has none, so
// the build flavour overrules every settings layer: whatever ENV.ini or the
// user's stored value asks for, the check stays off there.

TEST(SettingsLayeringTest, SelfCheckAvailabilityFollowsBuildFlavour) {
  EXPECT_EQ(IsSelfCheckAvailable(), IsStableBuild());
}

TEST(SettingsLayeringTest, SelfCheckIsForcedOffOnNonStableBuilds) {
  // Exactly the composition GpgFrontendContext applies: resolve across the
  // layers first, then let the build flavour have the last word.
  const auto effective = [](const QVariant& env, const QVariant& user) {
    return IsSelfCheckAvailable() &&
           ResolveLayeredValue(env, user, false).toBool();
  };

  if (IsStableBuild()) {
    EXPECT_TRUE(effective(QVariant(), QVariant(true)));
    EXPECT_TRUE(effective(QVariant(true), QVariant(false)));
  } else {
    EXPECT_FALSE(effective(QVariant(), QVariant(true)));
    EXPECT_FALSE(effective(QVariant(true), QVariant(false)));
  }

  // An explicit "off" is off on every build, whichever layer says so.
  EXPECT_FALSE(effective(QVariant(false), QVariant(true)));
  EXPECT_FALSE(effective(QVariant(), QVariant(false)));
  EXPECT_FALSE(effective(QVariant(), QVariant()));
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

namespace {

auto MakeMarker(int schema, int min_reader) -> ProfileMarker {
  ProfileMarker marker;
  marker.schema_version = schema;
  marker.min_reader_version = min_reader;
  marker.profile = "GpgFrontend";
  marker.last_writer_version = "2.2.2";
  return marker;
}

}  // namespace

// A first run has no marker and must not be mistaken for an incompatible one.
TEST(ProfileMarkerTest, MissingIsNotAnError) {
  EXPECT_EQ(CheckProfileCompatibility(ProfileMarker{}, false, 2),
            ProfileCompatibility::kMISSING);
}

TEST(ProfileMarkerTest, SameVersionIsCompatible) {
  EXPECT_EQ(CheckProfileCompatibility(MakeMarker(2, 2), true, 2),
            ProfileCompatibility::kOK);
}

// Upgrading in place stays allowed.
TEST(ProfileMarkerTest, OlderSchemaIsCompatible) {
  EXPECT_EQ(CheckProfileCompatibility(MakeMarker(1, 1), true, 2),
            ProfileCompatibility::kOK);
}

// The downgrade regression test: a profile that demands a newer reader must
// stop this build before it touches anything.
TEST(ProfileMarkerTest, NewerMinReaderIsRefused) {
  EXPECT_EQ(CheckProfileCompatibility(MakeMarker(3, 3), true, 2),
            ProfileCompatibility::kTOO_NEW);
}

// The two fields are independent: a newer layout that stayed backwards
// compatible keeps older builds working.
TEST(ProfileMarkerTest, NewerSchemaWithCompatibleReaderIsAccepted) {
  EXPECT_EQ(CheckProfileCompatibility(MakeMarker(3, 2), true, 2),
            ProfileCompatibility::kOK);
}

TEST(ProfileMarkerTest, RoundTripsThroughDisk) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("profile.json");
  const auto written = MakeMarker(2, 2);
  ASSERT_TRUE(WriteProfileMarker(path, written));

  const auto read = ReadProfileMarker(path);
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->schema_version, written.schema_version);
  EXPECT_EQ(read->min_reader_version, written.min_reader_version);
  EXPECT_EQ(read->profile, written.profile);
  EXPECT_EQ(read->last_writer_version, written.last_writer_version);
}

TEST(ProfileMarkerTest, AbsentFileReadsAsNothing) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  EXPECT_FALSE(ReadProfileMarker(dir.filePath("nope.json")).has_value());
}

// A truncated marker after a power cut must not brick the application: it reads
// as nothing, which the caller treats as a first run.
TEST(ProfileMarkerTest, MalformedFileIsNotFatal) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("profile.json");
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("{ this is not json");
  file.close();

  EXPECT_FALSE(ReadProfileMarker(path).has_value());
  EXPECT_EQ(CheckProfileCompatibility(ProfileMarker{}, false, 2),
            ProfileCompatibility::kMISSING);
}

// The whole point of the split: a non-stable build must not land on the same
// on-disk identity as a release.
TEST(ProfileMarkerTest, ProfileNameMatchesBuildFlavour) {
  const auto profile = GetAppProfileName();

  EXPECT_FALSE(profile.isEmpty());
  EXPECT_FALSE(profile.contains(' '));

  if (IsStableBuild()) {
    EXPECT_EQ(profile, GetProjectName());
  } else {
    EXPECT_NE(profile, GetProjectName());
  }
}

// ---------------------------------------------------------------------------
// Profile bootstrap: which profile a process runs against.
//
// Pure, so the whole precedence ladder is assertable without starting a
// process — the same shape ResolveAppKeyProtection() uses above.
// ---------------------------------------------------------------------------

namespace {

constexpr auto kClassicRoot = "/data/classic";
constexpr auto kPortableRoot = "/media/usb/GpgFrontend";

auto MakeInput(const QStringList& args = {}) -> ProfileBootstrapInput {
  ProfileBootstrapInput in;
  in.args = QStringList{"gpgfrontend"} + args;
  in.classic_root = kClassicRoot;
  in.portable_root = kPortableRoot;
  return in;
}

}  // namespace

TEST(ProfileBootstrapTest, NoArgumentsLandsOnClassic) {
  const auto r = ResolveProfileBootstrap(MakeInput());

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.state.kind, ProfileRootKind::kCLASSIC);
  EXPECT_EQ(r.state.id, QString("classic"));
  EXPECT_EQ(r.state.root, QString(kClassicRoot));
  EXPECT_FALSE(r.state.policy.self_contained);
}

TEST(ProfileBootstrapTest, NamedProfileResolvesUnderProfilesRoot) {
  for (const auto& args :
       {QStringList{"--profile", "work"}, QStringList{"--profile=work"}}) {
    const auto r = ResolveProfileBootstrap(MakeInput(args));

    EXPECT_TRUE(r.error.isEmpty());
    EXPECT_EQ(r.state.kind, ProfileRootKind::kNAMED);
    EXPECT_EQ(r.state.id, QString("work"));
    EXPECT_EQ(r.state.root, QString(kClassicRoot) + "/profiles/work");
  }
}

TEST(ProfileBootstrapTest, ExplicitRootOutranksEverything) {
  auto in = MakeInput({"--profile-root", "/srv/gf", "--profile", "work"});
  in.env_profile = "other";
  const auto r = ResolveProfileBootstrap(in);

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.state.kind, ProfileRootKind::kEXPLICIT_ROOT);
  EXPECT_EQ(r.state.root, QString("/srv/gf"));
}

TEST(ProfileBootstrapTest, RelativeExplicitRootIsRefused) {
  const auto r = ResolveProfileBootstrap(MakeInput({"--profile-root", "gf"}));

  EXPECT_FALSE(r.error.isEmpty());
  // the fallback has to be usable: nothing downstream should ever have to cope
  // with a half-resolved process
  EXPECT_EQ(r.state.kind, ProfileRootKind::kCLASSIC);
  EXPECT_EQ(r.state.root, QString(kClassicRoot));
}

TEST(ProfileBootstrapTest, PositionalPackageBecomesPending) {
  const auto r = ResolveProfileBootstrap(MakeInput({"/home/x/work.gfprofile"}));

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.state.kind, ProfileRootKind::kPACKAGE_PENDING);
  EXPECT_EQ(r.state.pending_package, QString("/home/x/work.gfprofile"));
}

TEST(ProfileBootstrapTest, AnOptionValueIsNotMistakenForAPackage) {
  // "--log-level" takes a value; a scan that did not know that could read the
  // next argument as a positional and open the wrong thing
  const auto r =
      ResolveProfileBootstrap(MakeInput({"--log-level", "x.gfprofile"}));

  EXPECT_NE(r.state.kind, ProfileRootKind::kPACKAGE_PENDING);
}

TEST(ProfileBootstrapTest, EnvironmentVariablesAreConsulted) {
  {
    auto in = MakeInput();
    in.env_profile_root = "/srv/env";
    const auto r = ResolveProfileBootstrap(in);
    EXPECT_EQ(r.state.kind, ProfileRootKind::kEXPLICIT_ROOT);
    EXPECT_EQ(r.state.root, QString("/srv/env"));
  }
  {
    auto in = MakeInput();
    in.env_profile = "ci";
    const auto r = ResolveProfileBootstrap(in);
    EXPECT_EQ(r.state.kind, ProfileRootKind::kNAMED);
    EXPECT_EQ(r.state.id, QString("ci"));
  }
}

TEST(ProfileBootstrapTest, CommandLineOutranksEnvironment) {
  auto in = MakeInput({"--profile", "cli"});
  in.env_profile = "env";
  EXPECT_EQ(ResolveProfileBootstrap(in).state.id, QString("cli"));
}

TEST(ProfileBootstrapTest, StartupPolicyIsHonouredWhenNothingIsNamed) {
  {
    auto in = MakeInput();
    in.startup_policy = ProfileStartupPolicy::kLAST_USED;
    in.registry_last_used = "work";
    EXPECT_EQ(ResolveProfileBootstrap(in).state.id, QString("work"));
  }
  {
    auto in = MakeInput();
    in.startup_policy = ProfileStartupPolicy::kFIXED;
    in.registry_startup_profile = "pinned";
    in.registry_last_used = "work";
    EXPECT_EQ(ResolveProfileBootstrap(in).state.id, QString("pinned"));
  }
  {
    // pinning the legacy location has to stay reachable forever
    auto in = MakeInput();
    in.startup_policy = ProfileStartupPolicy::kCLASSIC;
    in.registry_last_used = "work";
    EXPECT_EQ(ResolveProfileBootstrap(in).state.kind,
              ProfileRootKind::kCLASSIC);
  }
}

TEST(ProfileBootstrapTest, UnknownIdIsAnErrorNotASilentFallback) {
  auto in = MakeInput({"--profile", "ghost"});
  in.registry_available = true;
  in.known_ids = QStringList{"work", "home"};

  const auto r = ResolveProfileBootstrap(in);

  // opening the wrong keyring silently is the worst outcome available here
  EXPECT_FALSE(r.error.isEmpty());
  EXPECT_EQ(r.state.kind, ProfileRootKind::kCLASSIC);
}

TEST(ProfileBootstrapTest, UnknownIdIsAcceptedBeforeTheRegistryExists) {
  // an id missing from a list nobody has written yet is not an unknown profile
  auto in = MakeInput({"--profile", "fresh"});
  in.registry_available = false;

  const auto r = ResolveProfileBootstrap(in);
  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.state.kind, ProfileRootKind::kNAMED);
}

TEST(ProfileBootstrapTest, InvalidIdsAreRefused) {
  for (const auto* id : {"..", ".", "a/b", "", "CON", "Work", "-lead",
                         "0123456789012345678901234567890123456789012345678901"
                         "23456789012345"}) {
    auto in = MakeInput({"--profile", QString::fromUtf8(id)});
    const auto r = ResolveProfileBootstrap(in);
    if (QString::fromUtf8(id).isEmpty()) continue;  // empty means "not given"
    EXPECT_FALSE(r.error.isEmpty()) << "id should have been refused: " << id;
  }
}

// ----------------------------------------------------- portable composition

TEST(ProfileBootstrapTest, PortableAloneIsItsOwnRootNotANamedProfile) {
  auto in = MakeInput();
  in.env_ini_portable = true;

  const auto r = ResolveProfileBootstrap(in);

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.state.kind, ProfileRootKind::kPORTABLE);
  // every existing portable tree already has its data directly here; putting
  // it under profiles/<id> would strand all of them
  EXPECT_EQ(r.state.root, QString(kPortableRoot));
  EXPECT_TRUE(r.state.policy.self_contained);
}

// The precedence rule most likely to be implemented wrong.
TEST(ProfileBootstrapTest, ExplicitProfileOutranksPortableEnvIni) {
  auto in = MakeInput({"--profile", "work"});
  in.env_ini_portable = true;

  const auto r = ResolveProfileBootstrap(in);

  EXPECT_EQ(r.state.kind, ProfileRootKind::kNAMED);
  // ENV.ini still decides where the profiles root sits...
  EXPECT_EQ(r.state.root, QString(kPortableRoot) + "/profiles/work");
  // ...but not whether this profile is self-contained. That comes from the
  // profile's own profile.json, which the bootstrap has not read.
  EXPECT_FALSE(r.state.policy.self_contained);
}

TEST(ProfileBootstrapTest, ProfilesRootFollowsThePortableBase) {
  auto in = MakeInput();
  in.env_ini_portable = true;
  EXPECT_EQ(ResolveProfileBootstrap(in).state.profiles_root,
            QString(kPortableRoot) + "/profiles");

  in.env_ini_portable = false;
  EXPECT_EQ(ResolveProfileBootstrap(in).state.profiles_root,
            QString(kClassicRoot) + "/profiles");
}

// -------------------------------------------------------------- id handling

TEST(ProfileIdTest, ValidityRules) {
  EXPECT_TRUE(IsValidProfileId("work"));
  EXPECT_TRUE(IsValidProfileId("work_2"));
  EXPECT_TRUE(IsValidProfileId("a-b"));

  EXPECT_FALSE(IsValidProfileId(""));
  EXPECT_FALSE(IsValidProfileId("."));
  EXPECT_FALSE(IsValidProfileId(".."));
  EXPECT_FALSE(IsValidProfileId("a/b"));
  EXPECT_FALSE(IsValidProfileId("a\\b"));
  EXPECT_FALSE(IsValidProfileId("Work"));
  EXPECT_FALSE(IsValidProfileId("-work"));
  EXPECT_FALSE(IsValidProfileId(QString(65, 'a')));

  // reserved on Windows regardless of extension: a directory that can exist on
  // one platform and not the other is exactly what a portable profile must not
  // contain
  for (const auto* n : {"con", "CON", "nul", "com1", "lpt9"}) {
    EXPECT_FALSE(IsValidProfileId(QString::fromUtf8(n))) << n;
  }
}

TEST(ProfileIdTest, DisplayNamesBecomeUsableIds) {
  EXPECT_EQ(MakeProfileId("Work"), QString("work"));
  EXPECT_EQ(MakeProfileId("My Work Profile"), QString("my_work_profile"));
  EXPECT_EQ(MakeProfileId("  trailing  "), QString("trailing"));
  EXPECT_EQ(MakeProfileId("a-b"), QString("a-b"));

  // nothing usable survives, and inventing an id would be worse than saying so
  EXPECT_TRUE(MakeProfileId("///").isEmpty());
  EXPECT_TRUE(MakeProfileId("").isEmpty());
}

// --------------------------------------------------------- settings routing

TEST(ProfileSettingsPathTest, RootedProfilesAreIniBackedOnEveryPlatform) {
  // Assertable unconditionally, unlike the platform branch it replaced: a
  // native store is keyed only by organization and application name, so every
  // profile would share one registry key or plist.
  for (const auto kind :
       {ProfileRootKind::kPORTABLE, ProfileRootKind::kNAMED,
        ProfileRootKind::kEXPLICIT_ROOT, ProfileRootKind::kPACKAGE_LINKED}) {
    EXPECT_EQ(ResolveSettingsFilePath(kind, "/srv/p"),
              QString("/srv/p/config/config.ini"))
        << ProfileRootKindToString(kind).toStdString();
  }
}

TEST(ProfileSettingsPathTest, ClassicKeepsItsPlatformStore) {
  const auto path =
      ResolveSettingsFilePath(ProfileRootKind::kCLASSIC, "/srv/p");
#ifdef Q_OS_WINDOWS
  EXPECT_TRUE(path.endsWith("/config.ini"));
#else
  // empty means "use the native QSettings store", which is what every existing
  // POSIX installation already writes to
  EXPECT_TRUE(path.isEmpty());
#endif
}

TEST(ProfileRootKindTest, SpellingRoundTrips) {
  for (const auto kind :
       {ProfileRootKind::kCLASSIC, ProfileRootKind::kPORTABLE,
        ProfileRootKind::kNAMED, ProfileRootKind::kEXPLICIT_ROOT,
        ProfileRootKind::kPACKAGE_LINKED, ProfileRootKind::kPACKAGE_PENDING}) {
    EXPECT_EQ(ProfileRootKindFromString(ProfileRootKindToString(kind)), kind);
  }
  EXPECT_EQ(ProfileRootKindFromString("nonsense"), ProfileRootKind::kCLASSIC);
}

TEST(ProfileStartupPolicyTest, SpellingRoundTrips) {
  for (const auto p :
       {ProfileStartupPolicy::kLAST_USED, ProfileStartupPolicy::kASK,
        ProfileStartupPolicy::kFIXED, ProfileStartupPolicy::kCLASSIC}) {
    EXPECT_EQ(ProfileStartupPolicyFromString(ProfileStartupPolicyToString(p)),
              p);
  }
}

// A pending package has no root. Reading one anyway would resolve to an empty
// path and put the whole application on the wrong directory.
TEST(ProfileRuntimeDeathTest, PendingPackageHasNoRoot) {
  ProfileRuntimeState state;
  state.kind = ProfileRootKind::kPACKAGE_PENDING;
  state.pending_package = "/tmp/x.gfprofile";

  EXPECT_DEATH(RequireProfileRoot(state), "pending");
}

TEST(ProfileRuntimeDeathTest, EstablishingTwiceIsFatal) {
  // main() already established it for this process; a second call must not be
  // allowed to silently move every path
  ProfileRuntimeState state;
  state.kind = ProfileRootKind::kNAMED;
  state.id = "intruder";

  EXPECT_DEATH(ProfileRuntime::Establish(state), "established twice");
}

TEST(ProfileRuntimeTest, TheProcessRanAgainstAResolvedProfile) {
  ASSERT_TRUE(ProfileRuntime::Established());

  const auto& p = ProfileRuntime::Instance();
  EXPECT_FALSE(p.id.isEmpty());
  EXPECT_NE(p.kind, ProfileRootKind::kPACKAGE_PENDING);
  EXPECT_FALSE(RequireProfileRoot(p).isEmpty());

  // the station must agree with the runtime about where the profile lives
  if (p.kind != ProfileRootKind::kCLASSIC) {
    EXPECT_EQ(GetGSS().GetAppDataPath(), p.root);
  }
}

}  // namespace GpgFrontend::Test
