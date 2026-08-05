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

#include "core/profile/ProfileLoader.h"

#include <QUuid>

#include "core/function/AESCryptoHelper.h"
#include "core/profile/ProfileMigration.h"
#include "core/profile/ProfilePackage.h"
#include "core/utils/BuildInfoUtils.h"

namespace {

/// The secure level at which a pre-split profile sealed its key file with a
/// PIN.
constexpr int kLegacyPinSecureLevel = 3;

/// How long to wait for another process to let go of the lock. The wait exists
/// for the deep restart, which by construction has the outgoing process still
/// alive when the incoming one starts.
constexpr int kLockWaitMs = 5000;

/// Timestamp recorded on a migration rung. Passed into the ladder rather than
/// read inside it, so a run stays reproducible in a test.
auto MigrationTimestamp() -> QString {
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

}  // namespace

namespace GpgFrontend {

namespace {

auto FailureFor(ProfileMountStatus status) -> ProfileLoadFailure {
  switch (status) {
    case ProfileMountStatus::kNOT_A_PACKAGE:
      return ProfileLoadFailure::kNOT_A_PACKAGE;
    case ProfileMountStatus::kTOO_NEW:
      return ProfileLoadFailure::kTOO_NEW;
    case ProfileMountStatus::kTAMPERED:
      return ProfileLoadFailure::kPACKAGE_TAMPERED;
    case ProfileMountStatus::kMALFORMED:
      return ProfileLoadFailure::kPACKAGE_MALFORMED;
    default:
      return ProfileLoadFailure::kMOUNT_FAILED;
  }
}

auto FailureFor(ProfileKeyLoadStatus status) -> ProfileLoadFailure {
  switch (status) {
    case ProfileKeyLoadStatus::kREAD_FAILED:
      return ProfileLoadFailure::kKEY_READ_FAILED;
    case ProfileKeyLoadStatus::kDECRYPT_FAILED:
      return ProfileLoadFailure::kKEY_DECRYPT_FAILED;
    case ProfileKeyLoadStatus::kWRITE_FAILED:
      return ProfileLoadFailure::kKEY_WRITE_FAILED;
    case ProfileKeyLoadStatus::kGENERATE_FAILED:
      return ProfileLoadFailure::kKEY_GENERATE_FAILED;
    case ProfileKeyLoadStatus::kOK:
      break;
  }
  return ProfileLoadFailure::kKEY_READ_FAILED;
}

}  // namespace

auto ProfileLoader::CurrentWrapAccount() -> QString {
  const auto& session = ProfileSession::Instance();
  const auto root = session.Root();

  // canonicalPath() is empty for a directory that does not exist yet, which on
  // a first run would fold every fresh profile onto the same derived name
  const auto canonical = QDir(root).canonicalPath();

  return DeriveAppKeyWrapAccount(
      session.Profile().Kind(), session.Profile().Id(),
      canonical.isEmpty() ? QDir::cleanPath(root) : canonical,
      session.Marker().profile_uuid);
}

auto ProfileLoader::AppKeyProtectionFromApp() -> AppKeyProtection {
  if (qApp == nullptr) return AppKeyProtection::kNONE;
  return AppKeyProtectionFromString(
      qApp->property("GFAppKeyProtection").toString());
}

auto ProfileLoader::ApplyProfilePortabilityRule(AppKeyProtection resolved,
                                                bool keychain_allowed)
    -> AppKeyProtection {
  if (!keychain_allowed && resolved == AppKeyProtection::kKEYCHAIN) {
    return AppKeyProtection::kNONE;
  }
  return resolved;
}

auto ProfileLoader::ResolveAppKeyProtection(
    const QVariant& env_protection, const QVariant& env_secure_level,
    const QVariant& env_os_secret_store, const QVariant& user_protection,
    const QVariant& user_secure_level, const QVariant& user_os_secret_store)
    -> AppKeyProtection {
  // Each layer is tried in full — its own key, then the two keys it replaced —
  // before falling through, so a marker that says anything at all about the
  // protection always beats a user setting that says something else.
  const auto layer =
      [](const QVariant& protection, const QVariant& secure_level,
         const QVariant& os_secret_store, AppKeyProtection& out) -> bool {
    if (protection.isValid()) {
      out = AppKeyProtectionFromString(protection.toString());
      return true;
    }
    // A pre-split profile at this level has a PIN-sealed key file, so it has to
    // keep resolving to kPIN or it would fail to open on the next start. Lower
    // levels said nothing about protection and must fall through.
    if (secure_level.isValid() &&
        secure_level.toInt() >= kLegacyPinSecureLevel) {
      out = AppKeyProtection::kPIN;
      return true;
    }
    // An explicit false is an answer, not an absence: it must stop the ladder
    // rather than let a lower layer turn protection back on.
    if (os_secret_store.isValid()) {
      out = os_secret_store.toBool() ? AppKeyProtection::kKEYCHAIN
                                     : AppKeyProtection::kNONE;
      return true;
    }
    return false;
  };

  auto result = AppKeyProtection::kNONE;
  if (layer(env_protection, env_secure_level, env_os_secret_store, result)) {
    return result;
  }
  if (layer(user_protection, user_secure_level, user_os_secret_store, result)) {
    return result;
  }
  return AppKeyProtection::kNONE;
}

ProfileLoader::ProfileLoader(QSharedPointer<Profile> profile,
                             ProfileLoaderDelegate* delegate)
    : profile_(std::move(profile)), delegate_(delegate) {
  Q_ASSERT(!profile_.isNull());
  Q_ASSERT(delegate_ != nullptr);
}

auto ProfileLoader::acquire_lock() -> bool {
  const auto root = profile_->Root();

  auto result = ProfileLock::Acquire(root, kLockWaitMs);
  if (result.Ok()) return true;

  if (result.status == ProfileLockStatus::kIO_FAILED) {
    delegate_->Report({ProfileLoadFailure::kLOCK_UNAVAILABLE, result.path});
    return false;
  }

  if (!delegate_->ConfirmForceUnlock(result)) {
    delegate_->Report({ProfileLoadFailure::kALREADY_OPEN, root});
    return false;
  }

  ProfileLock::ForceUnlock(root);
  if (ProfileLock::Acquire(root, 1000).Ok()) return true;

  delegate_->Report({ProfileLoadFailure::kALREADY_OPEN, root});
  return false;
}

auto ProfileLoader::mount_package(int schema_version)
    -> std::optional<ProfileLoadError> {
  auto& packaged = static_cast<PackagedProfile&>(*profile_);

  // The failure is returned rather than reported here, because reporting it
  // shows a dialog and waits for a human — and the storage this attempt claimed
  // should be given back before that, not after.

  // The header first: it is cheap, and it answers "does this need a passphrase"
  // before anybody is asked for one.
  const auto inspection = packaged.Inspect();
  if (inspection.status != ProfileMountStatus::kOK &&
      inspection.status != ProfileMountStatus::kNEEDS_PASSPHRASE) {
    return ProfileLoadError{FailureFor(inspection.status),
                            packaged.PackagePath(), inspection.detail};
  }

  const bool needs_passphrase =
      inspection.status == ProfileMountStatus::kNEEDS_PASSPHRASE;

  ProfileMountContext ctx;
  ctx.schema_version = schema_version;

  bool retry = false;
  while (true) {
    if (needs_passphrase) {
      auto entered =
          delegate_->AskPackagePassphrase(packaged.PackagePath(), retry);
      if (!entered) {
        return ProfileLoadError{ProfileLoadFailure::kCANCELLED,
                                packaged.PackagePath()};
      }
      ctx.passphrase = *entered;
    }

    const auto mounted = packaged.Mount(ctx);
    if (mounted.Ok()) return {};

    // Only a wrong passphrase is worth asking again about; everything else is a
    // property of the file that another attempt cannot change. Gated on the
    // package actually having a passphrase, so the loop can never run without a
    // prompt in it.
    if (mounted.status == ProfileMountStatus::kBAD_PASSPHRASE &&
        needs_passphrase) {
      retry = true;
      continue;
    }

    return ProfileLoadError{
        FailureFor(mounted.status), packaged.PackagePath(), mounted.detail,
        mounted.status == ProfileMountStatus::kTOO_NEW ? mounted.detail
                                                       : QString{}};
  }
}

auto ProfileLoader::Mount(int schema_version) -> bool {
  if (profile_->Root().isEmpty()) {
    delegate_->Report({ProfileLoadFailure::kSELECTION_INVALID});
    return false;
  }

  // The lock comes before the storage, and takes the root into existence on the
  // way — creating an empty directory is not a data write, and a root that does
  // not exist yet cannot be locked.
  if (!acquire_lock()) return false;

  if (profile_->Kind() == ProfileKind::kPACKAGED) {
    // Anything a crashed session left behind, including a stale copy of this
    // package's own root. Cheap, and this is the one moment when nothing else
    // can be holding one.
    auto& packaged = static_cast<PackagedProfile&>(*profile_);
    SweepTransientProfileRoots(packaged.ProfilesRoot(), packaged.Root());

    if (const auto error = mount_package(schema_version)) {
      ProfileLock::Release();

      // The root was created only so that it could be locked, and nothing was
      // extracted into it. Leaving it behind would litter the profiles folder
      // with one empty directory per package that failed to open, and the sweep
      // deliberately ignores those: it only collects roots that carry a marker.
      // Safe to remove, because reaching here means the lock was ours — and
      // done before the report, which waits for a human.
      QDir(packaged.Root()).removeRecursively();

      delegate_->Report(*error);
      return false;
    }
  } else {
    const auto mounted = profile_->Mount({});
    if (!mounted.Ok()) {
      delegate_->Report(
          {FailureFor(mounted.status), profile_->Root(), mounted.detail});
      ProfileLock::Release();
      return false;
    }
  }

  auto accessor = profile_->MakeAccessor();
  for (const auto area :
       {ProfileArea::kRoot, ProfileArea::kConfig, ProfileArea::kDataObjects,
        ProfileArea::kSecure, ProfileArea::kLogs, ProfileArea::kModules}) {
    // The config area is only made where an INI actually lands in it; an
    // installed root on POSIX writes through the native store and has no
    // directory of its own to make.
    if (area == ProfileArea::kConfig &&
        profile_->SettingsFilePath().isEmpty()) {
      continue;
    }
    accessor->Ensure(area);
  }

  session_ = QSharedPointer<ProfileSession>(
      new ProfileSession(profile_, std::move(accessor)));

  // The resolver is pure and never reads a file, but a profile's policy is
  // recorded in its own marker — so it is taken now, once there is a marker to
  // take it from, and subject to whatever the profile's shape forces.
  profile_->ApplyMarkerPolicy(session_->Marker());

  ProfileSession::publish(session_);

  LOG_I() << "profile mounted:" << profile_->Id() << "("
          << ProfileKindToString(profile_->Kind()) << ") at"
          << profile_->Root();
  return true;
}

auto ProfileLoader::run_migrations(int schema_version,
                                   ProfileMigrationStage stage) -> bool {
  const auto path = profile_->MarkerPath();
  const auto marker = ReadProfileMarker(path);

  // A marker that exists but will not parse is treated as absent. A truncated
  // file after a power cut must not brick the application.
  const auto plan =
      PlanProfileMigration(marker.value_or(ProfileMarker{}), marker.has_value(),
                           schema_version, AllProfileMigrationNames());

  if (plan.verdict == ProfileMigrationVerdict::kTOO_NEW ||
      plan.verdict == ProfileMigrationVerdict::kREFUSE) {
    delegate_->Report({plan.verdict == ProfileMigrationVerdict::kTOO_NEW
                           ? ProfileLoadFailure::kTOO_NEW
                           : ProfileLoadFailure::kUPGRADE_REFUSED,
                       path, plan.reason, plan.writer_version});
    return false;
  }

  if (plan.verdict != ProfileMigrationVerdict::kUPGRADE) return true;

  const auto result =
      RunProfileMigration(profile_->Root(), path, plan, stage,
                          profile_->Kind() == ProfileKind::kINSTALLED_ROOT,
                          MigrationTimestamp(), AllProfileMigrations());

  if (!result.ok) {
    delegate_->Report({ProfileLoadFailure::kUPGRADE_FAILED, path, result.detail,
                       result.failed_rung});
    return false;
  }

  session_->reload_marker();
  return true;
}

void ProfileLoader::ensure_identity() {
  if (!session_->Marker().profile_uuid.isEmpty()) return;

  // Minted once and never regenerated: it is what makes deleting a profile and
  // recreating it under the same name a genuinely different identity rather
  // than one that collides with the credential entry the old one left behind.
  // It has to exist before the key file is opened, because the credential
  // account is derived from it.
  session_->UpdateMarker([](ProfileMarker& marker) {
    if (!marker.profile_uuid.isEmpty()) return;

    marker.profile_uuid =
        QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
    marker.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    marker.created_by_version = GetProjectVersion();

    if (marker.schema_version == 0) {
      marker.schema_version = GetAppProfileSchemaVersion();
      marker.min_reader_version = GetAppProfileSchemaVersion();
      marker.profile = GetAppProfileName();
    }
  });
}

void ProfileLoader::stamp_marker() {
  const auto kind = ProfileKindToString(profile_->Kind());
  const auto id = profile_->Id();
  const auto self_contained = profile_->Policy().self_contained;

  session_->UpdateMarker([&](ProfileMarker& marker) {
    marker.schema_version = GetAppProfileSchemaVersion();
    marker.profile = GetAppProfileName();
    marker.last_writer_version = GetProjectVersion();
    marker.last_writer_stable = IsStableBuild();

    // Only ever raised by a migration that genuinely makes the profile
    // unreadable to older builds; stamping must not quietly raise it.
    if (marker.min_reader_version == 0) {
      marker.min_reader_version = GetAppProfileSchemaVersion();
    }

    marker.profile_id = id;
    marker.kind = kind;
    marker.self_contained = self_contained;

    // Every process records itself, here rather than in whichever other window
    // happened to launch it — a profile opened straight from a shell with
    // `--profile` was otherwise never recorded as opened at all.
    marker.last_opened = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  });
}

auto ProfileLoader::reset_key_storage() -> bool {
  const auto key_dir = session_->Accessor().PathOf(ProfileArea::kSecure);
  if (!ProfileSecureKeyManager::ResetKeyStorage(key_dir)) return false;

  if (auto* store = GetSystemSecretStore(); store != nullptr) {
    // this profile's own entry, not the shared one: resetting one profile's key
    // must not strip the protection from every other profile
    store->Remove(CurrentWrapAccount());
  }

  auto settings = session_->Settings();
  settings.setValue("advanced/app_key_protection",
                    AppKeyProtectionToString(AppKeyProtection::kNONE));
  settings.sync();

  qWarning() << "the profile secure key was reset at the user's request";
  delegate_->Note(ProfileLoadNotice::kKEY_WAS_RESET, key_dir);
  return true;
}

auto ProfileLoader::resolve_secret(AppKeyProtection protection, GFBuffer& pin,
                                   GFBuffer& wrap) -> bool {
  const auto key_path =
      session_->Accessor().PathOf(ProfileArea::kSecure, "app.key");

  // A packaged profile has exactly one secret. The passphrase that opened the
  // package is also what seals the key inside it, so there is nothing to ask
  // for and nothing to resolve from the settings.
  //
  // Which of the two forms the key is in is read from the file rather than
  // assumed, exactly as ResolveWrapSecret() does below: an unprotected package
  // unpacks a plaintext key, and a session whose protection was turned off
  // while it was open comes back plaintext too. Handing a passphrase to a
  // plaintext file would fail to open a key that is sitting right there.
  if (profile_->Kind() == ProfileKind::kPACKAGED) {
    const auto stored =
        session_->Accessor().Read(ProfileArea::kSecure, "app.key");
    if (stored && AESCryptoHelper::IsEncryptedBuffer(*stored)) {
      pin = static_cast<PackagedProfile&>(*profile_).Passphrase();
    }
    return true;
  }

  if (protection != AppKeyProtection::kPIN) {
    const auto result = ProfileSecureKeyManager::ResolveWrapSecret(
        key_path, GetSystemSecretStore(),
        protection == AppKeyProtection::kKEYCHAIN, CurrentWrapAccount());

    switch (result.status) {
      case AppKeyWrapStatus::kNOT_WRAPPED:
      case AppKeyWrapStatus::kWRAPPED:
      case AppKeyWrapStatus::kJUST_ENABLED:
      case AppKeyWrapStatus::kJUST_DISABLED:
        wrap = result.secret;
        return true;

      case AppKeyWrapStatus::kSTORE_UNAVAILABLE: {
        // Turn the preference back off rather than retrying, and failing, at
        // every launch. The key stays exactly as it was, just unprotected.
        auto settings = session_->Settings();
        settings.setValue("advanced/app_key_protection",
                          AppKeyProtectionToString(AppKeyProtection::kNONE));
        settings.sync();
        delegate_->Note(ProfileLoadNotice::kKEYCHAIN_UNAVAILABLE,
                        result.detail);
        return true;
      }

      case AppKeyWrapStatus::kIO_FAILED:
        delegate_->Report({ProfileLoadFailure::kKEY_IO_FAILED, result.detail});
        return false;

      case AppKeyWrapStatus::kLOCKED_OUT:
        // Never reset on the user's behalf: resetting is unrecoverable, whereas
        // quitting costs nothing and lets them unlock the keychain and retry.
        if (!delegate_->ConfirmKeyReset(
                ProfileKeyResetReason::kKEYCHAIN_SECRET_LOST)) {
          delegate_->Report(
              {ProfileLoadFailure::kKEY_LOCKED_OUT, key_path, result.detail});
          return false;
        }
        if (!reset_key_storage()) {
          delegate_->Report({ProfileLoadFailure::kKEY_IO_FAILED, key_path});
          return false;
        }
        return true;
    }
    return false;
  }

  // A PIN: trial-unseal it here rather than leaving it to the key loader,
  // because once inside the loader a mistyped PIN and a corrupted key file are
  // indistinguishable — and that turns the first into a refusal to launch.
  AppKeyPinRequest request;
  request.key_path = key_path;

  if (!QFileInfo::exists(key_path)) {
    // A fresh profile that opted into a PIN before a key existed: collect it
    // once and use it to seal the key about to be generated.
    request.creating = true;
    const auto answer = delegate_->AskAppKeyPin(request);
    if (answer.action != AppKeyPinAnswer::Action::kUsePin) {
      delegate_->Report({ProfileLoadFailure::kCANCELLED, key_path});
      return false;
    }
    pin = answer.pin;
    return true;
  }

  auto on_disk = session_->Accessor().Read(ProfileArea::kSecure, "app.key");
  if (!on_disk) {
    // The file exists but will not read: an unlock loop can only repeat, so say
    // what is actually wrong and stop.
    delegate_->Report({ProfileLoadFailure::kKEY_READ_FAILED, key_path});
    return false;
  }

  if (!AESCryptoHelper::IsEncryptedBuffer(*on_disk)) {
    // The setting and the file disagree — a crashed transition or a hand-edited
    // ini. The key is intact, so proceed unprotected rather than refusing.
    auto settings = session_->Settings();
    settings.setValue("advanced/app_key_protection",
                      AppKeyProtectionToString(AppKeyProtection::kNONE));
    settings.sync();
    delegate_->Note(ProfileLoadNotice::kPIN_SET_BUT_KEY_PLAINTEXT, key_path);
    return true;
  }

  // A locked keyring has a lockout; a PIN does not, so there is nothing to be
  // gained by capping attempts — only the user's own patience limits them.
  while (true) {
    const auto answer = delegate_->AskAppKeyPin(request);

    switch (answer.action) {
      case AppKeyPinAnswer::Action::kQuit:
        delegate_->Report({ProfileLoadFailure::kCANCELLED, key_path});
        return false;

      case AppKeyPinAnswer::Action::kResetKey:
        // Already confirmed with the user by the delegate; a reset key file
        // proceeds with no PIN onto freshly generated material.
        if (!reset_key_storage()) {
          delegate_->Report({ProfileLoadFailure::kKEY_IO_FAILED, key_path});
          return false;
        }
        return true;

      case AppKeyPinAnswer::Action::kUsePin:
        if (ProfileSecureKeyManager::UnsealKey(answer.pin, {}, *on_disk)) {
          pin = answer.pin;
          return true;
        }
        ++request.failures;
        continue;
    }
  }
}

auto ProfileLoader::Open(int schema_version, bool rotation_requested) -> bool {
  Q_ASSERT(!session_.isNull());

  // The compatibility decision comes first and is pure: an incompatible profile
  // is not written to at all — not the marker, not a timestamp, not a
  // directory. A profile written by a newer build has to survive being opened
  // by an older one exactly as it was, or looking at it is what corrupts it.
  if (!run_migrations(schema_version, ProfileMigrationStage::kPRE_KEY)) {
    return false;
  }

  // After the gate, before the key: the credential account is derived from the
  // profile uuid, so the uuid has to exist by the time the key file is opened.
  ensure_identity();

  const auto protection = ApplyProfilePortabilityRule(
      AppKeyProtectionFromApp(), profile_->AllowsSystemKeychain());

  // The three backends are mutually exclusive and stay distinct because their
  // secrets are not alike: a PIN is low entropy and is stretched with Argon2id,
  // while the credential store's secret is 32 random bytes and uses a fast
  // derivation. Whichever is in use fills exactly one of the two slots below.
  GFBuffer pin;
  GFBuffer wrap;
  if (!resolve_secret(protection, pin, wrap)) return false;

  auto keys =
      QSharedPointer<ProfileSecureKeyManager>::create(profile_->MakeAccessor());

  // The security level asks; the profile has the last word. A packaged profile
  // would write a rotated key into storage this process deletes on the way out.
  const auto result = keys->Load(
      pin, wrap, rotation_requested && profile_->AllowsKeyRotation());
  if (!result.Ok()) {
    delegate_->Report({FailureFor(result.status), result.detail});
    return false;
  }

  session_->attach_keys(std::move(keys));

  // The rungs that could not run earlier: everything under data_objs/ is sealed
  // and only decrypts now. Before the stamp, so the stamp records a profile
  // that really is at the version it claims.
  if (!run_migrations(schema_version, ProfileMigrationStage::kPOST_KEY)) {
    return false;
  }

  // Only now: claiming a profile this build could not open would stamp our
  // version onto data we never successfully read.
  stamp_marker();
  return true;
}

}  // namespace GpgFrontend
