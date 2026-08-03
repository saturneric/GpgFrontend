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

#include <qcommandlineparser.h>
#include <qloggingcategory.h>

//
#include "Application.h"
#include "BinaryValidate.h"
#include "Command.h"
#include "GpgFrontendContext.h"
#include "Initialize.h"
#include "core/function/AESCryptoHelper.h"
#include "core/function/AppSecureKeyManager.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/ProfileBootstrap.h"
#include "core/function/ProfileLock.h"
#include "core/function/ProfileMigration.h"
#include "core/function/SystemSecretStore.h"
#include "core/utils/BuildInfoUtils.h"
#include "platform/PlatformSecretStore.h"
#include "res/GpgFrontendResource.h"
#include "ui/dialog/AppKeyPinDialog.h"

namespace {

/**
 * @brief Report a secure key failure to the user.
 *
 * AppSecureKeyManager lives in gf_core, which does not link QtWidgets, so it
 * reports failures as a status and the presentation happens here.
 *
 * @param result outcome of AppSecureKeyManager::Initialize()
 * @return true when startup may continue
 */
auto ReportAppSecureKeyFailure(
    const GpgFrontend::AppSecureKeyInitResult& result) -> bool {
  switch (result.status) {
    case GpgFrontend::AppSecureKeyStatus::kOK:
      return true;

    case GpgFrontend::AppSecureKeyStatus::kWRITE_FAILED:
      // Continuing would be silent data loss: everything encrypted during this
      // session gets a key ID that no longer exists on the next start, so it
      // would come back as unreadable rather than merely unsaved.
      QMessageBox::critical(
          nullptr, QObject::tr("Save Key Failed"),
          QObject::tr("The application secure key could not be saved: %1")
                  .arg(result.detail) +
              "\n" +
              QObject::tr("Anything saved now would be unreadable after a "
                          "restart, so the application will not continue. "
                          "Please check your storage and permissions."),
          QMessageBox::Ok);
      return false;

    case GpgFrontend::AppSecureKeyStatus::kDECRYPT_FAILED:
      // A wrong PIN is caught and retried before the key loader ever runs, so
      // reaching here means the key file itself will not decrypt.
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("Failed to decrypt the application secure key. The key "
                      "file may be corrupted.") +
              "\n" + QObject::tr("Please clear the secure key and try again."),
          QMessageBox::Ok);
      return false;

    case GpgFrontend::AppSecureKeyStatus::kREAD_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr(
              "Failed to read the application secure key from disk at: %1")
                  .arg(result.detail) +
              "\n" +
              QObject::tr("Please ensure the key file exists and is "
                          "accessible, or try re-initializing the secure key."),
          QMessageBox::Ok);
      return false;

    case GpgFrontend::AppSecureKeyStatus::kGENERATE_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("Secure Key Generation Failed"),
          QObject::tr("Failed to generate an application secure key.") + "\n" +
              QObject::tr("Please check your system's cryptography support."),
          QMessageBox::Ok);
      return false;
  }

  return false;
}

/**
 * @brief The at-rest protection in effect, with the portable rule re-applied.
 *
 * GpgFrontendContext already resolved this, portable mode included. The rule is
 * applied a second time here only because a qApp property is untyped and
 * writable by anything in the process, and wrapping the key with a credential
 * store on a portable installation would strand it. One redundant comparison is
 * cheap insurance on the one file every stored secret depends on.
 *
 * @return the protection the key loader should act on
 */
auto RequestedAppKeyProtection() -> GpgFrontend::AppKeyProtection {
  const auto& profile = GpgFrontend::ProfileRuntime::Instance();
  return GpgFrontend::ApplyProfilePortabilityRule(
      GpgFrontend::AppKeyProtectionFromApp(),
      GpgFrontend::ProfileTravelsBetweenMachines(profile.kind, profile.policy));
}

/**
 * @brief Discard the secure key and drop protection back to the default.
 *
 * The single, deliberately destructive escape hatch for a key that can no
 * longer be opened — a forgotten PIN or an unrecoverable keychain secret. It
 * deletes the on-disk key material, clears the keychain wrap secret, and resets
 * the protection preference to "none", so the next Initialize() generates a
 * fresh, unprotected key. Everything the old key encrypted becomes permanently
 * unreadable, so every caller confirms the choice with the user first.
 *
 * @return true when the key was reset and startup may proceed
 */
auto ResetAppSecureKeyToDefault() -> bool {
  auto& key_mgr = GpgFrontend::AppSecureKeyManager::GetInstance();
  if (!GpgFrontend::AppSecureKeyManager::ResetKeyStorage(key_mgr.GetKeyDir())) {
    QMessageBox::critical(
        nullptr, QObject::tr("Reset Secure Key"),
        QObject::tr("The key file at %1 could not be removed.")
            .arg(key_mgr.GetLegacyKeyPath()),
        QMessageBox::Ok);
    return false;
  }

  if (auto* store = GpgFrontend::GetSystemSecretStore(); store != nullptr) {
    // this profile's own entry, not the shared one: resetting one profile's
    // key must not strip the protection from every other profile
    store->Remove(GpgFrontend::AppSecureKeyManager::CurrentWrapAccount());
  }
  GpgFrontend::GetSettings().setValue(
      "advanced/app_key_protection",
      AppKeyProtectionToString(GpgFrontend::AppKeyProtection::kNONE));

  qWarning() << "app secure key was reset at the user's request";
  return true;
}

/**
 * @brief Report the outcome of reconciling the key file's at-rest protection.
 *
 * @param result outcome of AppSecureKeyManager::ResolveWrapSecret()
 * @return true when startup may continue
 */
auto ReportAppKeyWrapOutcome(const GpgFrontend::AppKeyWrapResult& result)
    -> bool {
  switch (result.status) {
    case GpgFrontend::AppKeyWrapStatus::kNOT_WRAPPED:
    case GpgFrontend::AppKeyWrapStatus::kWRAPPED:
    case GpgFrontend::AppKeyWrapStatus::kJUST_ENABLED:
    case GpgFrontend::AppKeyWrapStatus::kJUST_DISABLED:
      return true;

    case GpgFrontend::AppKeyWrapStatus::kSTORE_UNAVAILABLE: {
      // Turn the preference back off rather than retrying, and failing, at
      // every launch. The key stays exactly as it was, just unprotected.
      GpgFrontend::GetSettings().setValue(
          "advanced/app_key_protection",
          AppKeyProtectionToString(GpgFrontend::AppKeyProtection::kNONE));
      QMessageBox box(
          QMessageBox::Warning, QObject::tr("System Keychain Unavailable"),
          QObject::tr("The application key could not be protected using the "
                      "system keychain, so it remains stored unprotected.") +
              "\n" +
              QObject::tr("This setting has been turned off. You can turn it "
                          "on again once a keychain is available."));
      box.addButton(QMessageBox::Ok);

      // The user asked for keychain protection and is being silently dropped
      // back to none, so this is exactly the moment they need to be able to
      // find out what broke rather than guess.
      auto detail = GpgFrontend::SystemSecretStoreUnavailableReason();
      if (auto* store = GpgFrontend::GetSystemSecretStore();
          detail.isEmpty() && store != nullptr) {
        detail = store->LastError();
      }
      if (!detail.isEmpty()) box.setDetailedText(detail);

      box.exec();
      return true;
    }

    case GpgFrontend::AppKeyWrapStatus::kIO_FAILED:
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("The application secure key at %1 could not be read or "
                      "rewritten.")
                  .arg(result.detail) +
              "\n" + QObject::tr("Please check your storage and permissions."),
          QMessageBox::Ok);
      return false;

    case GpgFrontend::AppKeyWrapStatus::kLOCKED_OUT: {
      // Never reset on the user's behalf: resetting is unrecoverable, whereas
      // quitting costs nothing and lets them unlock the keychain and retry.
      QMessageBox box(QMessageBox::Critical,
                      QObject::tr("Secure Key Unavailable"),
                      QObject::tr("The application key is protected by a "
                                  "secret kept in the system keychain, and "
                                  "that secret could not be read."));
      box.setInformativeText(
          QObject::tr("This usually means the keychain is locked, was reset, "
                      "or this profile was copied from another computer or "
                      "user account.") +
          "\n\n" +
          QObject::tr("You can unlock the keychain and start the application "
                      "again. Resetting the key instead lets the application "
                      "start, but everything it previously encrypted becomes "
                      "permanently unreadable."));
      box.setDetailedText(
          QObject::tr("Keychain backend: %1").arg(result.detail));

      auto* quit = box.addButton(QObject::tr("Quit"), QMessageBox::RejectRole);
      auto* reset = box.addButton(QObject::tr("Reset Secure Key"),
                                  QMessageBox::DestructiveRole);
      box.setDefaultButton(quit);
      box.exec();

      if (box.clickedButton() != reset) return false;

      const auto confirm = QMessageBox::warning(
          nullptr, QObject::tr("Reset Secure Key"),
          QObject::tr("Everything the application has encrypted with the old "
                      "key will be permanently unreadable.") +
              "\n\n" + QObject::tr("Reset the secure key?"),
          QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

      if (confirm != QMessageBox::Yes) return false;

      return ResetAppSecureKeyToDefault();
    }
  }

  return false;
}

/**
 * @brief Outcome of asking the user for the application key PIN.
 *
 * The empty-PIN case is deliberately not overloaded to mean "quit": a reset
 * key file legitimately continues with no PIN, and a cancel must not.
 */
struct AppKeyPinPrompt {
  bool proceed = false;       ///< false means quit
  GpgFrontend::GFBuffer pin;  ///< the PIN, empty when proceeding unprotected
};

/**
 * @brief Confirm and carry out a reset to default after a forgotten PIN.
 *
 * A two-step confirmation, matching the keychain lock-out reset: that
 * everything encrypted becomes permanently unreadable is stated twice, because
 * it cannot be undone. Any back-out returns false and leaves the key untouched.
 *
 * @return true only when the key was actually reset and startup may proceed
 */
auto ConfirmForgottenPinReset() -> bool {
  QMessageBox box(QMessageBox::Warning, QObject::tr("Reset to Default"),
                  QObject::tr("Resetting removes the PIN and lets the "
                              "application start, but everything it previously "
                              "encrypted becomes permanently unreadable."));
  box.setInformativeText(QObject::tr(
      "Only do this if you cannot recall the PIN. There is no other "
      "way to recover the key."));
  auto* back = box.addButton(QObject::tr("Go Back"), QMessageBox::RejectRole);
  auto* reset = box.addButton(QObject::tr("Reset to Default"),
                              QMessageBox::DestructiveRole);
  box.setDefaultButton(back);
  box.exec();

  if (box.clickedButton() != reset) return false;

  const auto confirm = QMessageBox::warning(
      nullptr, QObject::tr("Reset to Default"),
      QObject::tr("Everything the application has encrypted with the current "
                  "key will be permanently unreadable.") +
          "\n\n" + QObject::tr("Reset the secure key?"),
      QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

  if (confirm != QMessageBox::Yes) return false;

  return ResetAppSecureKeyToDefault();
}

/**
 * @brief Prompt for the application key PIN until it opens the key file.
 *
 * The PIN is trial-unsealed here rather than left to Initialize(), because once
 * inside the loader a mistyped PIN and a corrupted key file are
 * indistinguishable: the old code turned the first into a refusal to launch.
 * Here it costs only a retry.
 *
 * Shapes of key file that are handled:
 *  - none yet: a fresh profile that opted into a PIN before a key existed, so
 *    the PIN is collected once and used to seal the key about to be generated;
 *  - present but not encrypted: the setting and the file disagree, which means
 *    a crashed transition or a hand-edited ini. The key is intact, so the
 *    setting is reset to "none" and startup proceeds unprotected rather than
 *    refusing to launch;
 *  - present but unreadable: an I/O error, distinct from a wrong PIN — a retry
 *    loop would only ever repeat, so it is reported and startup stops;
 *  - present and encrypted: the real case, retried until the PIN opens it.
 *
 * After a few misses the dialog reveals a reset option: a user who has truly
 * forgotten the PIN can discard the key for a fresh, unprotected one rather
 * than being trapped between an unopenable key and quitting.
 *
 * @param key_path path of the key file
 * @return whether to proceed, and the PIN to proceed with
 */
/// Where the profile marker lives. Alongside the data it describes, and
/// deliberately outside data_objs/ so the GC never sees it.
auto ProfileMarkerPath() -> QString {
  return GpgFrontend::GetGSS().GetAppDataPath() + "/profile.json";
}

/**
 * @brief Refuse to start on a profile written by a newer, incompatible build.
 *
 * Proceeding would not fail cleanly: the newer profile's key material and data
 * objects would be partially rewritten and garbage-collected by rules this
 * build predates, which is how a downgrade turns into permanent data loss
 * rather than an error message.
 *
 * @return false when the application must stop
 */
/// Timestamp recorded on a migration rung. Passed into the ladder rather than
/// read inside it, so a run is reproducible in a test.
auto MigrationTimestamp() -> QString {
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

/**
 * @brief Report a profile this build must not touch, and stop.
 *
 * @param plan the refusal
 * @param path the profile marker path, so the user can find the folder
 */
void ReportIncompatibleProfile(const GpgFrontend::ProfileMigrationPlan& plan,
                               const QString& path) {
  const auto writer = plan.writer_version.isEmpty()
                          ? QObject::tr("a newer version")
                          : plan.writer_version;

  QMessageBox::critical(
      nullptr,
      plan.verdict == GpgFrontend::ProfileMigrationVerdict::kTOO_NEW
          ? QObject::tr("Profile Is Too New")
          : QObject::tr("Profile Cannot Be Opened"),
      QObject::tr("This application data was last used by %1, which stores it "
                  "in a format this version does not understand.")
              .arg(writer) +
          "\n\n" + plan.reason + "\n\n" +
          QObject::tr("Continuing would damage it. Please use %1 or later, or "
                      "start this version with a different data folder.")
              .arg(writer) +
          "\n\n" + QObject::tr("Data folder: %1").arg(path),
      QMessageBox::Ok);
}

/**
 * @brief Decide what to do with the profile, then run the early migrations.
 *
 * The compatibility decision comes first and is pure: an incompatible profile
 * is not written to at all — not the marker, not a timestamp, not a directory.
 * A profile written by a newer build has to survive being opened by an older
 * one exactly as it was, or looking at it is what corrupts it.
 *
 * @return false when the application must stop
 */
auto CheckProfileOrHalt() -> bool {
  const auto path = ProfileMarkerPath();
  const auto marker = GpgFrontend::ReadProfileMarker(path);

  // A marker that exists but will not parse is treated as absent. A truncated
  // file after a power cut must not brick the application.
  const auto plan = GpgFrontend::PlanProfileMigration(
      marker.value_or(GpgFrontend::ProfileMarker{}), marker.has_value(),
      GpgFrontend::GetAppProfileSchemaVersion(),
      GpgFrontend::AllProfileMigrationNames());

  if (plan.verdict == GpgFrontend::ProfileMigrationVerdict::kTOO_NEW ||
      plan.verdict == GpgFrontend::ProfileMigrationVerdict::kREFUSE) {
    ReportIncompatibleProfile(plan, path);
    return false;
  }

  if (plan.verdict != GpgFrontend::ProfileMigrationVerdict::kUPGRADE) {
    return true;
  }

  const auto& profile = GpgFrontend::ProfileRuntime::Instance();
  const auto result = GpgFrontend::RunProfileMigration(
      GpgFrontend::RequireProfileRoot(profile), path, plan,
      GpgFrontend::ProfileMigrationStage::kPRE_KEY,
      profile.kind == GpgFrontend::ProfileRootKind::kCLASSIC,
      MigrationTimestamp(), GpgFrontend::AllProfileMigrations());

  if (!result.ok) {
    QMessageBox::critical(
        nullptr, QObject::tr("Profile Upgrade Failed"),
        QObject::tr("Upgrading this profile stopped at step '%1'.")
                .arg(result.failed_rung) +
            "\n\n" + result.detail + "\n\n" +
            QObject::tr("The profile is intact at layout version %1. Please "
                        "report this.")
                .arg(result.reached),
        QMessageBox::Ok);
    return false;
  }

  return true;
}

/**
 * @brief Run the migrations that need the application secure key.
 *
 * Split from the early stage because everything under data_objs/ is sealed and
 * does not decrypt until the key is loaded — a data-object rung run too early
 * sees nothing and concludes there is nothing to do.
 *
 * @return false when the application must stop
 */
auto RunPostKeyMigrationOrHalt() -> bool {
  const auto path = ProfileMarkerPath();
  const auto marker = GpgFrontend::ReadProfileMarker(path);

  const auto plan = GpgFrontend::PlanProfileMigration(
      marker.value_or(GpgFrontend::ProfileMarker{}), marker.has_value(),
      GpgFrontend::GetAppProfileSchemaVersion(),
      GpgFrontend::AllProfileMigrationNames());

  if (plan.verdict == GpgFrontend::ProfileMigrationVerdict::kTOO_NEW ||
      plan.verdict == GpgFrontend::ProfileMigrationVerdict::kREFUSE) {
    ReportIncompatibleProfile(plan, path);
    return false;
  }

  if (plan.verdict != GpgFrontend::ProfileMigrationVerdict::kUPGRADE) {
    return true;
  }

  const auto& profile = GpgFrontend::ProfileRuntime::Instance();
  const auto result = GpgFrontend::RunProfileMigration(
      GpgFrontend::RequireProfileRoot(profile), path, plan,
      GpgFrontend::ProfileMigrationStage::kPOST_KEY,
      profile.kind == GpgFrontend::ProfileRootKind::kCLASSIC,
      MigrationTimestamp(), GpgFrontend::AllProfileMigrations());

  if (!result.ok) {
    QMessageBox::critical(
        nullptr, QObject::tr("Profile Upgrade Failed"),
        QObject::tr("Upgrading this profile stopped at step '%1'.")
                .arg(result.failed_rung) +
            "\n\n" + result.detail,
        QMessageBox::Ok);
    return false;
  }

  return true;
}

/**
 * @brief Stamp the profile as belonging to this build.
 *
 * Called only after the secure key opened successfully, so a profile this build
 * could not actually use is never claimed as its own.
 */
/**
 * @brief Take the profile lock, or explain who has it.
 *
 * Two processes on one root corrupt data_objs/ — every write there is a
 * whole-file read-modify-write with no locking of its own — let the garbage
 * collector quarantine objects the other process is writing, and leave the
 * GnuPG home directory unprotected, since SQLite protects only itself.
 *
 * The wait exists for the deep restart, which by construction has the outgoing
 * process still alive when the incoming one starts.
 *
 * @return false when the application must stop
 */
auto AcquireProfileLockOrHalt() -> bool {
  const auto& profile = GpgFrontend::ProfileRuntime::Instance();
  const auto root = GpgFrontend::RequireProfileRoot(profile);

  auto result = GpgFrontend::ProfileLock::Acquire(root, 5000);
  if (result.Ok()) return true;

  if (result.status == GpgFrontend::ProfileLockStatus::kIO_FAILED) {
    QMessageBox::critical(
        nullptr, QObject::tr("Cannot Lock Profile"),
        QObject::tr("The lock file at %1 could not be created.")
                .arg(result.path) +
            "\n" + QObject::tr("Please check your storage and permissions."),
        QMessageBox::Ok);
    return false;
  }

  const auto held_by =
      result.pid != 0
          ? QObject::tr("It is open in process %1 on %2.")
                .arg(result.pid)
                .arg(result.host.isEmpty() ? QObject::tr("this computer")
                                           : result.host)
          : QObject::tr("Another process has it open.");

  QMessageBox box(QMessageBox::Warning, QObject::tr("Profile Is Already Open"),
                  QObject::tr("This profile is already open in another "
                              "window.") +
                      "\n\n" + held_by + "\n\n" +
                      QObject::tr("Opening it twice would corrupt its stored "
                                  "data.") +
                      "\n\n" + QObject::tr("Profile: %1").arg(root));
  auto* quit = box.addButton(QObject::tr("Quit"), QMessageBox::AcceptRole);
  auto* force =
      box.addButton(QObject::tr("Force Unlock"), QMessageBox::DestructiveRole);
  box.setDefaultButton(quit);
  box.exec();

  if (box.clickedButton() != force) return false;

  // Deliberately a second, separate confirmation: if the holder is in fact
  // alive, this reintroduces exactly the concurrent-write window the lock
  // exists to prevent.
  if (QMessageBox::warning(
          nullptr, QObject::tr("Force Unlock"),
          QObject::tr("Only do this if you are certain no other GpgFrontend "
                      "window has this profile open.") +
              "\n\n" +
              QObject::tr("If one does, both copies will corrupt the "
                          "profile's stored data."),
          QMessageBox::Cancel | QMessageBox::Yes,
          QMessageBox::Cancel) != QMessageBox::Yes) {
    return false;
  }

  GpgFrontend::ProfileLock::ForceUnlock(root);
  if (GpgFrontend::ProfileLock::Acquire(root, 1000).Ok()) return true;

  QMessageBox::critical(nullptr, QObject::tr("Force Unlock Failed"),
                        QObject::tr("The profile is still locked."),
                        QMessageBox::Ok);
  return false;
}

/**
 * @brief Give the profile a stable identity before anything depends on it.
 *
 * The credential-store account is derived from the profile uuid, and it is
 * needed before the key file is opened — so the uuid cannot wait for
 * StampProfileMarker(), which only runs once the key has already been read.
 *
 * Runs after the compatibility gate, never before it: a profile this build must
 * not touch must not be given an identity by this build either.
 */
void EnsureProfileIdentity() {
  const auto path = ProfileMarkerPath();
  auto marker = GpgFrontend::ReadProfileMarker(path).value_or(
      GpgFrontend::ProfileMarker{});

  if (!marker.profile_uuid.isEmpty()) return;

  // Minted once and never regenerated: it is what makes deleting a profile and
  // recreating it under the same name a genuinely different identity rather
  // than one that collides with the credential entry the old one left behind.
  marker.profile_uuid =
      QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
  marker.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  marker.created_by_version = GpgFrontend::GetProjectVersion();

  if (marker.schema_version == 0) {
    marker.schema_version = GpgFrontend::GetAppProfileSchemaVersion();
    marker.min_reader_version = GpgFrontend::GetAppProfileSchemaVersion();
    marker.profile = GpgFrontend::GetAppProfileName();
  }

  GpgFrontend::WriteProfileMarker(path, marker);
}

void StampProfileMarker() {
  const auto path = ProfileMarkerPath();

  // Read-modify-write, not a fresh marker: this file now carries the migration
  // history, the profile identity and any field a newer build added. Rebuilding
  // it from scratch would erase all of that on every single start, which is the
  // one way a version story can be lost after it has been established.
  auto marker = GpgFrontend::ReadProfileMarker(path).value_or(
      GpgFrontend::ProfileMarker{});

  const auto& profile = GpgFrontend::ProfileRuntime::Instance();

  marker.schema_version = GpgFrontend::GetAppProfileSchemaVersion();
  marker.profile = GpgFrontend::GetAppProfileName();
  marker.last_writer_version = GpgFrontend::GetProjectVersion();
  marker.last_writer_stable = GpgFrontend::IsStableBuild();

  // Only ever raised by a migration that genuinely makes the profile
  // unreadable to older builds; stamping must not quietly raise it.
  if (marker.min_reader_version == 0) {
    marker.min_reader_version = GpgFrontend::GetAppProfileSchemaVersion();
  }

  // Minted once, and never regenerated: it is what makes deleting and
  // recreating a profile a different identity rather than a colliding one.
  if (marker.profile_uuid.isEmpty()) {
    marker.profile_uuid =
        QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
    marker.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    marker.created_by_version = GpgFrontend::GetProjectVersion();
  }

  marker.profile_id = profile.id;
  marker.kind = GpgFrontend::ProfileRootKindToString(profile.kind);
  marker.self_contained = profile.policy.self_contained;

  GpgFrontend::WriteProfileMarker(path, marker);
}

auto PromptForAppKeyPin(const QString& key_path) -> AppKeyPinPrompt {
  if (!QFileInfo(key_path).exists()) {
    GpgFrontend::UI::AppKeyPinDialog dialog(
        GpgFrontend::UI::AppKeyPinDialog::Mode::kSET);
    if (dialog.exec() != QDialog::Accepted) return {};
    return {true, dialog.Pin()};
  }

  auto on_disk = GpgFrontend::GFBufferFactory::FromFile(key_path);
  if (!on_disk) {
    // The file exists but will not read: an unlock loop can only repeat, so say
    // what is actually wrong and let ReportAppSecureKeyFailure handle the halt.
    QMessageBox::critical(
        nullptr, QObject::tr("App Secure Key Error"),
        QObject::tr("The application secure key at %1 could not be read.")
                .arg(key_path) +
            "\n" + QObject::tr("Please check your storage and permissions."),
        QMessageBox::Ok);
    return {};
  }

  if (!GpgFrontend::AESCryptoHelper::IsEncryptedBuffer(*on_disk)) {
    QMessageBox::warning(
        nullptr, QObject::tr("Application Key Not Protected"),
        QObject::tr("A PIN is configured, but the application key on disk is "
                    "not encrypted.") +
            "\n" +
            QObject::tr("This can happen if a previous change was interrupted. "
                        "The PIN setting has been turned off and the key is "
                        "left as it is."),
        QMessageBox::Ok);
    GpgFrontend::GetSettings().setValue(
        "advanced/app_key_protection",
        AppKeyProtectionToString(GpgFrontend::AppKeyProtection::kNONE));
    // The file really is plaintext, so proceeding with an empty PIN loads it
    // exactly as an unprotected profile would.
    return {true, {}};
  }

  // A locked keyring has a lockout; a PIN does not, so there is nothing to be
  // gained by capping attempts — only the user's own patience limits them.
  GpgFrontend::UI::AppKeyPinDialog dialog(
      GpgFrontend::UI::AppKeyPinDialog::Mode::kUNLOCK);
  int failures = 0;
  while (true) {
    const int code = dialog.exec();

    // The reset escape hatch, offered only after RevealResetOption() below. A
    // confirmed reset proceeds with an empty PIN onto a freshly generated key;
    // backing out drops straight back into the unlock loop.
    if (code == GpgFrontend::UI::AppKeyPinDialog::kResetRequested) {
      if (ConfirmForgottenPinReset()) return {true, {}};
      continue;
    }

    // Anything other than accept is a Quit: it costs nothing and leaves the key
    // intact, so it is never overloaded to mean reset.
    if (code != QDialog::Accepted) return {};

    auto pin = dialog.Pin();
    if (GpgFrontend::AppSecureKeyManager::UnsealKey(pin, {}, *on_disk)) {
      return {true, pin};
    }

    // Clear the field first and set the message last: clearing emits a change
    // that hides the error, so the message has to be the final mutation or it
    // would vanish before the dialog is shown again.
    ++failures;
    dialog.Clear();
    auto message = QObject::tr(
        "That PIN did not unlock the application key. Please try again.");
    // After a few misses, say the thing that actually matters — that a
    // forgotten PIN is not recoverable — and reveal the reset option so a stuck
    // user has a way out other than quitting.
    if (failures >= 3) {
      message += "\n" + QObject::tr(
                            "If you have forgotten your PIN, the application "
                            "key and everything encrypted with it cannot be "
                            "recovered.");
      dialog.RevealResetOption();
    }
    dialog.SetErrorText(message);
  }
}

}  // namespace

/**
 *
 * @param argc
 * @param argv
 * @return
 */
auto main(int argc, char* argv[]) -> int {
  // initialize qt resources (embedded in the gf_res shared library)
  GpgFrontend::InitResources();

  auto const ctx =
      QSharedPointer<GpgFrontend::GpgFrontendContext>::create(argc, argv);

  // create qt core application
  ctx->InitApplication();

  const auto* app = ctx->GetApp();
  Q_ASSERT(app != nullptr);

  // do some early init
  GpgFrontend::PreInit(ctx);

  auto rtn = 0;

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOptions({
      {{"v", "version"}, "show version information"},
      {{"t", "test"}, "run all unit test cases"},
      {{"e", "environment"}, "show environment information"},
      {{"l", "log-level"}, "set log level (debug, info, warn, error)", "none"},
      {{{}, "self-check"}, "check libraries and executables validity"},
      // Declaration only: these were already resolved during InitApplication(),
      // long before this parser existed, because where the settings live is
      // exactly what they decide. Registering them here just stops
      // parser.process() rejecting them as unknown.
      {{{}, "profile"}, "open the named local profile", "id"},
      {{{}, "profile-root"},
       "open the profile stored at this directory",
       "path"},
  });
  parser.addPositionalArgument("file", "a .gfprofile package to open",
                               "[file]");

  // Hold back GoogleTest flags (`--gtest_*`) from the app's parser, which would
  // otherwise reject them as unknown options. They are consumed later by
  // testing::InitGoogleTest, which reads the unmodified argv during RunTest, so
  // `gpgfrontend -t --gtest_filter=...` works without the GTEST_* env vars.
  QStringList parser_args;
  for (const auto& arg : QCoreApplication::arguments()) {
    if (arg.startsWith("--gtest_")) continue;
    parser_args << arg;
  }
  parser.process(parser_args);

  // The --self-check flag is gated on the build flavour just like the setting
  // is: without build-time signatures to compare against there is nothing the
  // check could confirm, so honouring the flag on a nightly would only refuse
  // to start a perfectly good build.
  const auto self_check = app->property("GFSelfCheck").toBool();
  if (GpgFrontend::IsSelfCheckAvailable() &&
      (self_check || parser.isSet("self-check")) && !ValidateLibraries()) {
    QMessageBox::critical(
        nullptr, QObject::tr("Program Self-Test Failed"),
        QObject::tr(
            "The application has detected an issue while verifying essential "
            "libraries and binaries that were digitally signed during the "
            "build. "
            "This means one or more files may have been altered or are being "
            "loaded from the wrong location. For security reasons, the program "
            "must now exit."),
        QMessageBox::Ok);
    return -1;
  }

  if (parser.isSet("v")) {
    return GpgFrontend::PrintVersion();
  }

  if (parser.isSet("l")) {
    GpgFrontend::ParseLogLevel(parser.value("l"));
  }

  // Installed before the -e early return so that environment information can
  // report the credential store. Safe this early: on every platform this only
  // loads a library and registers, never probing the store, so it cannot raise
  // a keyring unlock prompt from a command that just prints to stdout.
  GpgFrontend::InstallPlatformSecretStore();

  if (parser.isSet("e")) {
    return GpgFrontend::PrintEnvInfo();
  }

  // The profile was resolved during InitApplication(), before any setting was
  // read. A failure there is reported here, where there is a message handler
  // and a usable dialog, and still before anything opens key material.
  if (!ctx->profile_error.isEmpty()) {
    qCritical() << "profile bootstrap failed:" << ctx->profile_error;
    QMessageBox::critical(nullptr, QObject::tr("Cannot Open Profile"),
                          ctx->profile_error, QMessageBox::Ok);
    return 1;
  }

  // Checked before anything reads or writes the secure key or a data object:
  // once those start, an incompatible profile is already being damaged.
  // Before the compatibility gate and before anything opens key material: a
  // second process reading this profile while another writes it is exactly what
  // the gate below cannot protect against.
  if (!AcquireProfileLockOrHalt()) return 1;

  if (!CheckProfileOrHalt()) return 1;

  // After the gate, before the key: the credential account is derived from the
  // profile uuid, so the uuid has to exist by the time the key file is opened.
  EnsureProfileIdentity();

  auto& key_mgr = GpgFrontend::AppSecureKeyManager::GetInstance();
  const auto protection = RequestedAppKeyProtection();

  // The three backends are mutually exclusive and stay distinct because their
  // secrets are not alike: a PIN is low entropy and is stretched with Argon2id,
  // while the credential store's secret is 32 random bytes and uses a fast
  // derivation. Whichever is in use fills exactly one of the two slots below.
  GpgFrontend::GFBuffer pin;
  GpgFrontend::GFBuffer wrap;

  if (protection == GpgFrontend::AppKeyProtection::kPIN) {
    const auto prompt = PromptForAppKeyPin(key_mgr.GetLegacyKeyPath());
    // Cancelling quits: it costs the user nothing and leaves the key intact,
    // whereas resetting on their behalf would not. A reset key file proceeds
    // with an empty PIN, loaded exactly as an unprotected profile.
    if (!prompt.proceed) return 1;
    pin = prompt.pin;
  } else {
    const auto wrap_result =
        GpgFrontend::AppSecureKeyManager::ResolveWrapSecret(
            key_mgr.GetLegacyKeyPath(), GpgFrontend::GetSystemSecretStore(),
            protection == GpgFrontend::AppKeyProtection::kKEYCHAIN,
            GpgFrontend::AppSecureKeyManager::CurrentWrapAccount());

    if (!ReportAppKeyWrapOutcome(wrap_result)) return 1;
    wrap = wrap_result.secret;
  }

  const auto key_result = key_mgr.Initialize(pin, wrap);

  if (!ReportAppSecureKeyFailure(key_result)) return 1;

  // The rungs that could not run earlier: everything under data_objs/ is sealed
  // and only decrypts now. Before StampProfileMarker(), so the stamp records a
  // profile that is actually at the version it claims.
  if (!RunPostKeyMigrationOrHalt()) return 1;

  // Only now: claiming a profile this build could not open would stamp our
  // version onto data we never successfully read.
  StampProfileMarker();

  if (parser.isSet("t")) {
    ctx->gather_external_gnupg_info = false;
    ctx->unit_test_mode = true;

    InitGlobalBasicEnvSync(ctx);
    rtn = RunTest(ctx);
    ShutdownGlobalBasicEnv(ctx);
    return rtn;
  }

  ctx->gather_external_gnupg_info = true;
  ctx->unit_test_mode = false;

  InitGlobalBasicEnv(ctx, true);

  rtn = StartApplication(ctx);
  ShutdownGlobalBasicEnv(ctx);
  return rtn;
}
