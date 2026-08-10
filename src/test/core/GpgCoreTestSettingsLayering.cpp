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
#include "core/profile/Profile.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Test {

// The three-layer resolution behind the Advanced settings tab: a key pinned in
// the profile's own marker is a deployment override that beats the user's
// stored value, which in turn beats the built-in default.

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
  // make an explicitly disabled offline mode silently re-enable itself.
  EXPECT_FALSE(ResolveLayeredValue(QVariant(), QVariant(false), QVariant(true))
                   .toBool());
  EXPECT_EQ(ResolveLayeredValue(QVariant(0), QVariant(1), QVariant(3)).toInt(),
            0);
}

TEST(SettingsLayeringTest, EmptyStringFromTheTopLayerStillOverrides) {
  // A missing key reads back as an invalid QVariant but an intentionally
  // blanked one reads back as a valid empty string, and blanking a key is an
  // answer that must stop the ladder rather than fall through it.
  const auto r =
      ResolveLayeredValue(QVariant(QString()), QVariant("user"), QVariant("d"));
  EXPECT_TRUE(r.isValid());
  EXPECT_TRUE(r.toString().isEmpty());
}

// The user settings layer is INI-backed for every profile with a root of its
// own, and an INI stores everything as text. These are the exact conversions
// the startup resolution performs, and the one that historically bites is
// "false" reading back as boolean true.

TEST(SettingsLayeringTest, IniStringBooleansConvertCorrectly) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("config.ini");
  {
    QSettings w(path, QSettings::IniFormat);
    w.setValue("GnuPGOfflineMode", "false");
    w.setValue("SecureLevel", "2");
    w.setValue("LogRingBufferCapacity", "4096");
    w.sync();
  }

  QSettings s(path, QSettings::IniFormat);
  EXPECT_FALSE(s.value("GnuPGOfflineMode").toBool());
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

/// Resolve with only the pinned deployment layer populated.
auto EnvOnly(const QVariant& protection, const QVariant& secure_level,
             const QVariant& os_secret_store) -> AppKeyProtection {
  return ProfileLoader::ResolveAppKeyProtection(
      protection, secure_level, os_secret_store, kUnset, kUnset, kUnset);
}

/// Resolve with only the user-settings layer populated.
auto UserOnly(const QVariant& protection, const QVariant& secure_level,
              const QVariant& os_secret_store) -> AppKeyProtection {
  return ProfileLoader::ResolveAppKeyProtection(
      kUnset, kUnset, kUnset, protection, secure_level, os_secret_store);
}

}  // namespace

TEST(AppKeyProtectionSettingsTest, NothingSetMeansNoProtection) {
  EXPECT_EQ(ProfileLoader::ResolveAppKeyProtection(kUnset, kUnset, kUnset,
                                                   kUnset, kUnset, kUnset),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, EnvProtectionWinsOverEverything) {
  // A pinned key is a deployment override: whatever it says about the
  // protection beats every legacy key and every user choice.
  EXPECT_EQ(ProfileLoader::ResolveAppKeyProtection(
                QVariant("keychain"), QVariant(3), QVariant(false),
                QVariant("pin"), QVariant(3), QVariant(true)),
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
  // a pinned key that switched the keychain off would be overridden by a stale
  // user setting that had switched it on.
  EXPECT_EQ(
      ProfileLoader::ResolveAppKeyProtection(kUnset, kUnset, QVariant(false),
                                             kUnset, kUnset, QVariant(true)),
      AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, UserProtectionWinsOverItsOwnLegacyKeys) {
  // Once the user has made an explicit choice it is final, even when the keys
  // it replaced still hold their old values.
  EXPECT_EQ(UserOnly(QVariant("none"), QVariant(3), QVariant(true)),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, EnvLayerIsTriedWholeBeforeTheUserLayer) {
  // A marker that only pins the legacy OSSecretStore key still beats a user
  // setting on the new key — otherwise a deployment override would be silently
  // demoted the moment the user touched the combo.
  EXPECT_EQ(
      ProfileLoader::ResolveAppKeyProtection(kUnset, kUnset, QVariant(true),
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
  // A typo in a pinned key must leave the key unprotected rather than demand a
  // PIN that nobody ever set, which would be an unopenable profile.
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
  EXPECT_EQ(ProfileLoader::ApplyProfilePortabilityRule(
                AppKeyProtection::kKEYCHAIN, false),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, PortableAllowsPinAndNoProtection) {
  EXPECT_EQ(
      ProfileLoader::ApplyProfilePortabilityRule(AppKeyProtection::kPIN, false),
      AppKeyProtection::kPIN);
  EXPECT_EQ(ProfileLoader::ApplyProfilePortabilityRule(AppKeyProtection::kNONE,
                                                       false),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, InstalledAllowsEveryMode) {
  for (const auto p : {AppKeyProtection::kNONE, AppKeyProtection::kKEYCHAIN,
                       AppKeyProtection::kPIN}) {
    EXPECT_EQ(ProfileLoader::ApplyProfilePortabilityRule(p, true), p);
  }
}

TEST(AppKeyProtectionSettingsTest, PortableRuleOverridesAPinnedKey) {
  // A profile that travels cannot know which machine it will be plugged into,
  // so even an explicit deployment request for the keychain is downgraded.
  const auto resolved = EnvOnly(QVariant("keychain"), kUnset, kUnset);
  EXPECT_EQ(ProfileLoader::ApplyProfilePortabilityRule(resolved, false),
            AppKeyProtection::kNONE);
}

TEST(AppKeyProtectionSettingsTest, IniStringFormsResolveCorrectly) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("config.ini");
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

// The session's settings and the singleton's settings are the same file. They
// used to be resolved twice — once by hand, early, before the singleton could
// safely exist — and two resolutions of one path can disagree. Now the session
// owns it and the singleton asks. These assert that they cannot come apart
// again, since a
// secure allocator exists. If it ever drifts from the singleton's own path, the
// Advanced tab would write to one file while startup reads another.
TEST(SettingsLayeringTest, EarlySettingsTargetsSameStoreAsSingleton) {
  EXPECT_EQ(ProfileSession::Instance().Settings().fileName(),
            GetSettings().fileName());
}

TEST(SettingsLayeringTest, EarlySettingsSeesValueWrittenViaSingleton) {
  const auto* kKey = "advanced/test_roundtrip_probe";
  auto settings = GetSettings();
  const auto previous = settings.value(kKey);

  settings.setValue(kKey, 4242);
  settings.sync();

  EXPECT_EQ(ProfileSession::Instance().Settings().value(kKey).toInt(), 4242);

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
// Profile selection: which profile a process runs against.
//
// Pure, so the whole precedence ladder is assertable without starting a
// process — the same shape ResolveAppKeyProtection() uses above.
// ---------------------------------------------------------------------------

namespace {

constexpr auto kInstalledRoot = "/data/classic";
constexpr auto kPortableRoot = "/media/usb/GpgFrontend";

auto MakeInput(const QStringList& args = {}) -> ProfileSelectionInput {
  ProfileSelectionInput in;
  in.args = QStringList{"gpgfrontend"} + args;
  in.installed_root = kInstalledRoot;
  in.portable_root = kPortableRoot;
  // What a scan of the profiles root would have found. Naming anything outside
  // this is an error, so a test that names a profile has to say it exists.
  in.known_ids = QStringList{"work", "home", "ci", "cli", "env", "pinned"};
  return in;
}

}  // namespace

TEST(ProfileSelectionTest, NoArgumentsLandsOnTheInstalledRoot) {
  const auto r = ResolveProfileSelection(MakeInput());

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.selection.kind, ProfileKind::kINSTALLED_ROOT);
  EXPECT_EQ(r.selection.id, QString("classic"));
  EXPECT_EQ(r.selection.root, QString(kInstalledRoot));
}

TEST(ProfileSelectionTest, NamedProfileResolvesUnderProfilesRoot) {
  for (const auto& args :
       {QStringList{"--profile", "work"}, QStringList{"--profile=work"}}) {
    const auto r = ResolveProfileSelection(MakeInput(args));

    EXPECT_TRUE(r.error.isEmpty());
    EXPECT_EQ(r.selection.kind, ProfileKind::kPERSIST);
    EXPECT_EQ(r.selection.id, QString("work"));
    EXPECT_EQ(r.selection.root, QString(kInstalledRoot) + "/profiles/work");
  }
}

TEST(ProfileSelectionTest, PositionalPackageIsSelectedAsAPackage) {
  const auto r = ResolveProfileSelection(MakeInput({"/home/x/work.gfp"}));

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.selection.kind, ProfileKind::kPACKAGED);
  EXPECT_EQ(r.selection.package_path, QString("/home/x/work.gfp"));
  // deliberately no root: a package has none until it is extracted, and the
  // profile derives one from the package's own path
  EXPECT_TRUE(r.selection.root.isEmpty());
}

TEST(ProfileSelectionTest, AnOptionValueIsNotMistakenForAPackage) {
  // "--log-level" takes a value; a scan that did not know that could read the
  // next argument as a positional and open the wrong thing
  const auto r = ResolveProfileSelection(MakeInput({"--log-level", "x.gfp"}));

  EXPECT_NE(r.selection.kind, ProfileKind::kPACKAGED);
}

TEST(ProfileSelectionTest, TheEnvironmentIsConsulted) {
  auto in = MakeInput();
  in.env_profile = "ci";
  const auto r = ResolveProfileSelection(in);
  EXPECT_EQ(r.selection.kind, ProfileKind::kPERSIST);
  EXPECT_EQ(r.selection.id, QString("ci"));
}

TEST(ProfileSelectionTest, CommandLineOutranksEnvironment) {
  auto in = MakeInput({"--profile", "cli"});
  in.env_profile = "env";
  EXPECT_EQ(ResolveProfileSelection(in).selection.id, QString("cli"));
}

TEST(ProfileSelectionTest, APackageOutranksTheEnvironment) {
  auto in = MakeInput({"/home/x/work.gfp"});
  in.env_profile = "env";
  EXPECT_EQ(ResolveProfileSelection(in).selection.kind, ProfileKind::kPACKAGED);
}

// ------------------------------------------- documents handed over by the OS

// macOS does not put a double-clicked file on the command line; it arrives as
// an event and is appended to the arguments once it does. Standing must not
// depend on where in the list it lands, and it must survive an option that
// takes a value sitting in front of it.
TEST(ProfileSelectionTest, AHandedOverPackageIsFoundBehindEveryOtherArgument) {
  const auto r = ResolveProfileSelection(
      MakeInput({"--log-level", "debug", "/Users/x/Work.GFP"}));

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.selection.kind, ProfileKind::kPACKAGED);
  EXPECT_EQ(r.selection.package_path, QString("/Users/x/Work.GFP"));
  EXPECT_EQ(r.selection.id, QString("Work"));
}

TEST(ProfileSelectionTest, AHandedOverPackageDoesNotOutrankANamedProfile) {
  // Appending gives the document the standing a typed package has, which is
  // below an explicit --profile. Anything else and asking for a profile by name
  // would be silently overruled by whatever the Finder passed along.
  const auto r = ResolveProfileSelection(
      MakeInput({"--profile", "work", "/Users/x/other.gfp"}));

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.selection.kind, ProfileKind::kPERSIST);
  EXPECT_EQ(r.selection.id, QString("work"));
}

TEST(ProfileSelectionTest, TheFirstPackageOnTheLineWins) {
  // The other half of appending: a document added at the end must not override
  // a package that was actually typed.
  const auto r = ResolveProfileSelection(MakeInput({"/a.gfp", "/b.gfp"}));

  EXPECT_EQ(r.selection.package_path, QString("/a.gfp"));
}

// The anti-drift test. This spelling is duplicated outside the compiler's
// reach -- a freedesktop glob, a Windows registry value, a macOS plist tag --
// so the scan and the registration are pinned to one constant.
TEST(ProfileSelectionTest, TheScannedExtensionIsTheRegisteredOne) {
  EXPECT_EQ(QString(kProfilePackageExtension), QString(".gfp"));

  const auto r = ResolveProfileSelection(
      MakeInput({QString("/home/x/work") + kProfilePackageExtension}));

  EXPECT_EQ(r.selection.kind, ProfileKind::kPACKAGED);
}

TEST(ProfileSelectionTest, NothingIsRememberedBetweenRuns) {
  // An instance always starts on its root profile. There is deliberately no
  // "reopen what was open last" rung: another profile is opened from the root
  // into a new window, which is the only way two can be used at once anyway.
  auto in = MakeInput();

  const auto r = ResolveProfileSelection(in);

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.selection.kind, ProfileKind::kINSTALLED_ROOT);
  EXPECT_EQ(r.selection.id, QString("classic"));
}

TEST(ProfileSelectionTest, UnknownIdIsAnErrorNotASilentFallback) {
  auto in = MakeInput({"--profile", "ghost"});

  const auto r = ResolveProfileSelection(in);

  // Opening the wrong keyring silently is the worst outcome available here,
  // and provisioning a fresh empty profile over a typo is the second worst:
  // both look to the user exactly like their keys having disappeared.
  EXPECT_FALSE(r.error.isEmpty());
  EXPECT_EQ(r.selection.kind, ProfileKind::kINSTALLED_ROOT);
}

TEST(ProfileSelectionTest, InvalidIdsAreRefused) {
  for (const auto* id : {"..", ".", "a/b", "", "CON", "Work", "-lead",
                         "0123456789012345678901234567890123456789012345678901"
                         "23456789012345"}) {
    auto in = MakeInput({"--profile", QString::fromUtf8(id)});
    const auto r = ResolveProfileSelection(in);
    if (QString::fromUtf8(id).isEmpty()) continue;  // empty means "not given"
    EXPECT_FALSE(r.error.isEmpty()) << "id should have been refused: " << id;
  }
}

// ----------------------------------------------------- portable composition

TEST(ProfileSelectionTest, PortableAloneIsItsOwnRootNotANamedProfile) {
  auto in = MakeInput();
  in.portable_build = true;

  const auto r = ResolveProfileSelection(in);

  EXPECT_TRUE(r.error.isEmpty());
  EXPECT_EQ(r.selection.kind, ProfileKind::kPORTABLE_ROOT);
  // every existing portable tree already has its data directly here; putting
  // it under profiles/<id> would strand all of them
  EXPECT_EQ(r.selection.root, QString(kPortableRoot));
}

// The precedence rule most likely to be implemented wrong.
TEST(ProfileSelectionTest, ExplicitProfileOutranksPortableEnvIni) {
  auto in = MakeInput({"--profile", "work"});
  in.portable_build = true;

  const auto r = ResolveProfileSelection(in);

  EXPECT_EQ(r.selection.kind, ProfileKind::kPERSIST);
  // the build flavour still decides where the profiles root sits...
  EXPECT_EQ(r.selection.root, QString(kPortableRoot) + "/profiles/work");
  // ...but not whether this profile is self-contained. That comes from the
  // profile's own profile.json, which selection has not read.
  EXPECT_FALSE(MakeProfile(r.selection)->Policy().self_contained);
}

TEST(ProfileSelectionTest, ProfilesRootFollowsThePortableBase) {
  auto in = MakeInput();
  in.portable_build = true;
  EXPECT_EQ(ResolveProfileSelection(in).selection.profiles_root,
            QString(kPortableRoot) + "/profiles");

  in.portable_build = false;
  EXPECT_EQ(ResolveProfileSelection(in).selection.profiles_root,
            QString(kInstalledRoot) + "/profiles");
}

// --------------------------------------------------- where that base comes
// from
//
// The selection tests above take portable_root as given. This is where the
// value itself is decided, and getting it wrong moves the user's keys.

TEST(PortableDataPathTest, ABinLayoutClimbsOutOfTheBinaryDirectory) {
  // Windows and a Linux install tree both put the executable in bin/, so the
  // deployment root -- what the archive unpacks to -- is one level up.
  const auto app_dir = QCoreApplication::applicationDirPath();

  const auto previous = qgetenv("APPIMAGE");
  qunsetenv("APPIMAGE");

  EXPECT_EQ(ResolveApplicationDirPath(), app_dir);
  EXPECT_EQ(ResolvePortableDataPath(), QDir(app_dir + "/../").canonicalPath());

  if (!previous.isEmpty()) qputenv("APPIMAGE", previous);
}

#ifdef Q_OS_LINUX

TEST(PortableDataPathTest, AnAppImageKeepsItsDataBesideTheImage) {
  QTemporaryDir stick;
  ASSERT_TRUE(stick.isValid());

  const auto image = stick.path() + "/GpgFrontend.AppImage";
  QFile file(image);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.close();

  const auto previous = qgetenv("APPIMAGE");
  qputenv("APPIMAGE", image.toLocal8Bit());

  // An AppImage is a single file: the folder holding it *is* the deployment
  // root. Climbing out of it the way a bin/ layout does would write the
  // profile next to the folder the user copied onto the stick, not into it.
  EXPECT_EQ(ResolveApplicationDirPath(), QDir(stick.path()).canonicalPath());
  EXPECT_EQ(ResolvePortableDataPath(), QDir(stick.path()).canonicalPath());

  if (previous.isEmpty()) {
    qunsetenv("APPIMAGE");
  } else {
    qputenv("APPIMAGE", previous);
  }
}

TEST(PortableDataPathTest, AnUnresolvableAppImagePathIsNotFollowed) {
  // $APPIMAGE names a file that is not there: nothing about that path can be
  // trusted, so both helpers fall back to the ordinary application directory
  // instead of composing a location out of it.
  const auto previous = qgetenv("APPIMAGE");
  qputenv("APPIMAGE", "/nonexistent/does-not-exist/GpgFrontend.AppImage");

  const auto app_dir = QCoreApplication::applicationDirPath();
  EXPECT_EQ(ResolveApplicationDirPath(), app_dir);
  EXPECT_EQ(ResolvePortableDataPath(), QDir(app_dir + "/../").canonicalPath());

  if (previous.isEmpty()) {
    qunsetenv("APPIMAGE");
  } else {
    qputenv("APPIMAGE", previous);
  }
}

#endif

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

TEST(ProfileIdTest, GeneratedIdsAreUsableIds) {
  // Ids are no longer derived from what the user typed: they are generated, and
  // a directory name that cannot be rejected is one fewer thing to explain.
  for (int i = 0; i < 16; ++i) {
    const auto id =
        QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
    EXPECT_EQ(id.size(), 32);
    EXPECT_TRUE(IsValidProfileId(id)) << id.toStdString();
  }
}

TEST(ProfileKindTest, SpellingRoundTrips) {
  for (const auto kind :
       {ProfileKind::kINSTALLED_ROOT, ProfileKind::kPORTABLE_ROOT,
        ProfileKind::kPERSIST, ProfileKind::kPACKAGED}) {
    EXPECT_EQ(ProfileKindFromString(ProfileKindToString(kind)), kind);
  }
  EXPECT_EQ(ProfileKindFromString("nonsense"), ProfileKind::kINSTALLED_ROOT);
}

// The stored spellings are a wire format. Renaming one would make this build
// unable to recognise a profile it wrote itself.
TEST(ProfileKindTest, StoredSpellingsAreTheOnesAlreadyOnDisk) {
  EXPECT_EQ(ProfileKindToString(ProfileKind::kINSTALLED_ROOT), "classic");
  EXPECT_EQ(ProfileKindToString(ProfileKind::kPORTABLE_ROOT), "portable");
  EXPECT_EQ(ProfileKindToString(ProfileKind::kPERSIST), "named");
  EXPECT_EQ(ProfileKindToString(ProfileKind::kPACKAGED), "package_linked");
}

// ------------------------------------------------- marker-backed overrides

// The deployment overrides used to live in an ENV.ini read from the process
// working directory, so the same installation resolved differently depending on
// where it was started from, and one file governed every profile at once even
// though settings have always been per-profile.

TEST(MarkerDeploymentTest, PinnedKeysSurviveARoundTripThroughTheMarker) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto path = ProfileMarkerPathFor(dir.path());

  ProfileMarker marker;
  marker.schema_version = GetAppProfileSchemaVersion();
  marker.deployment["SecureLevel"] = 3;
  marker.deployment["GnuPGOfflineMode"] = true;
  marker.deployment["PinentryProgramPath"] = "/usr/bin/pinentry";
  ASSERT_TRUE(WriteProfileMarker(path, marker));

  const auto read = ReadProfileMarker(path);
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->deployment.value("SecureLevel").toInt(), 3);
  EXPECT_TRUE(read->deployment.value("GnuPGOfflineMode").toBool());
  EXPECT_EQ(read->deployment.value("PinentryProgramPath").toString(),
            QString("/usr/bin/pinentry"));
}

TEST(MarkerDeploymentTest, AnAbsentKeyStaysAbsentRatherThanBecomingFalsy) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto path = ProfileMarkerPathFor(dir.path());

  ProfileMarker marker;
  marker.deployment["SecureLevel"] = 0;
  ASSERT_TRUE(WriteProfileMarker(path, marker));

  const auto read = ReadProfileMarker(path);
  ASSERT_TRUE(read.has_value());

  // The distinction the whole ladder rests on: an explicit 0 is an answer that
  // stops the ladder, while a missing key has to fall through to the next
  // layer. Collapsing the two would let a profile that pins nothing override
  // every user setting with a default-constructed value.
  EXPECT_TRUE(read->deployment.value("SecureLevel").isValid());
  EXPECT_FALSE(read->deployment.value("LogLevel").isValid());
  EXPECT_EQ(ResolveLayeredValue(read->deployment.value("LogLevel"), QVariant(2),
                                QVariant(0))
                .toInt(),
            2);
}

TEST(MarkerDeploymentTest, NoPinnedKeysWritesNoScaffolding) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto path = ProfileMarkerPathFor(dir.path());

  ProfileMarker marker;
  ASSERT_TRUE(WriteProfileMarker(path, marker));

  const auto read = ReadProfileMarker(path);
  ASSERT_TRUE(read.has_value());
  EXPECT_TRUE(read->deployment.isEmpty());
}

TEST(MarkerDeploymentTest, LastOpenedRoundTrips) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto path = ProfileMarkerPathFor(dir.path());

  ProfileMarker marker;
  marker.last_opened = "2026-08-05T10:00:00Z";
  ASSERT_TRUE(WriteProfileMarker(path, marker));

  const auto read = ReadProfileMarker(path);
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->last_opened, QString("2026-08-05T10:00:00Z"));
}

// A newer build's extra fields must survive an older build touching the file,
// and the two new keys must not have broken that.
TEST(MarkerDeploymentTest, UnknownFieldsStillSurviveAlongsideTheNewKeys) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto path = ProfileMarkerPathFor(dir.path());

  ProfileMarker marker;
  marker.last_opened = "2026-08-05T10:00:00Z";
  marker.deployment["SecureLevel"] = 1;
  marker.unknown_fields["from_a_newer_build"] = "keep me";
  ASSERT_TRUE(WriteProfileMarker(path, marker));

  const auto read = ReadProfileMarker(path);
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->unknown_fields.value("from_a_newer_build").toString(),
            QString("keep me"));
  EXPECT_EQ(read->deployment.value("SecureLevel").toInt(), 1);
}

// -------------------------------------------------------- log level parsing

TEST(LogLevelTest, NamesParseAndNonsenseDoesNot) {
  EXPECT_EQ(ParseLogLevelName("debug").value_or(-1),
            static_cast<int>(GFLogLevel::kDEBUG));
  EXPECT_EQ(ParseLogLevelName("INFO").value_or(-1),
            static_cast<int>(GFLogLevel::kINFO));
  EXPECT_EQ(ParseLogLevelName(" warn ").value_or(-1),
            static_cast<int>(GFLogLevel::kWARNING));
  EXPECT_EQ(ParseLogLevelName("error").value_or(-1),
            static_cast<int>(GFLogLevel::kCRITICAL));

  // "none" is the option's declared placeholder, not a level anybody asks for
  EXPECT_FALSE(ParseLogLevelName("none").has_value());
  EXPECT_FALSE(ParseLogLevelName("").has_value());
  EXPECT_FALSE(ParseLogLevelName("verbose").has_value());
}

TEST(LogLevelTest, TheFlagOutranksEveryStoredLayer) {
  // The bug this pins: the flag was applied in main() and then silently undone
  // by PreInit(), which re-applied the property resolved from the settings. The
  // flag has to *be* a layer, not a side effect.
  const auto stored = ResolveLayeredValue(QVariant(), QVariant(2), QVariant(0));

  const auto cli = QVariant(static_cast<int>(GFLogLevel::kDEBUG));
  EXPECT_EQ(ResolveLayeredValue(cli, stored, stored).toInt(),
            static_cast<int>(GFLogLevel::kDEBUG));

  // and with no flag given, the stored layer is still what wins
  EXPECT_EQ(ResolveLayeredValue(QVariant(), stored, stored).toInt(), 2);
}

TEST(ProfileSessionTest, TheProcessRanAgainstALoadedProfile) {
  ASSERT_TRUE(ProfileSession::Loaded());

  const auto& session = ProfileSession::Instance();
  EXPECT_FALSE(session.Profile().Id().isEmpty());
  EXPECT_FALSE(session.Root().isEmpty());

  // the keys are attached by Open(), which has run by the time tests do
  EXPECT_TRUE(session.KeysLoaded());

  // the station must agree with the session about where the profile lives
  EXPECT_EQ(GetGSS().GetAppDataPath(), session.Root());

  // and so must the accessor, which is where the station now gets it from
  EXPECT_EQ(session.Accessor().PathOf(ProfileArea::kRoot), session.Root());
}

}  // namespace GpgFrontend::Test
