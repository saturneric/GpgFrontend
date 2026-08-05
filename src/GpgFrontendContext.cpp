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

#include "GpgFrontendContext.h"

#include <qapplication.h>
#include <qcoreapplication.h>
#include <qobject.h>
#include <qthread.h>

#include "core/GFCoreLog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfileRegistry.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "ui/GpgFrontendApplication.h"

namespace {
auto BoolText(bool value) -> QString {
  return value ? QStringLiteral("true") : QStringLiteral("false");
}

auto DisplayPath(const QString& path) -> QString {
  return path.isEmpty() ? QStringLiteral("<empty>") : path;
}
}  // namespace

namespace GpgFrontend {

void GpgFrontendContext::load_env_conf_set_properties() {
  // The deployment overrides this profile pins, from its own profile.json. They
  // used to live in an ENV.ini read from the process working directory, which
  // meant the same installation resolved differently depending on where it was
  // started from, and that one file governed every profile at once even though
  // settings have always been per-profile.
  const auto& session = ProfileSession::Instance();
  const auto& deployment = session.Marker().deployment;

  const auto env_value = [&deployment](const char* key) {
    return deployment.value(QLatin1String(key));
  };

  // Deployment-only knobs: the marker is the single source for these.
  //
  // GFPortableMode is deliberately absent: it is a mirror of the resolved
  // profile kind, published by mirror_profile_properties() before this runs.
  // Re-deriving it here would say "portable" for a session that was explicitly
  // started on a named profile with `--profile`, which is a location the
  // build flavour has no say over.
  property("GFGnuPGOfflineMode", env_value("GnuPGOfflineMode").toBool());
  property("GFPinentryProgramPath",
           env_value("PinentryProgramPath").toString());

  // The knobs below are also editable in Settings -> Advanced, so they resolve
  // in three layers: a pinned key wins (deployment override), else the user's
  // stored setting, else the built-in default. Pinned keys are recorded in
  // GFEnvLockedKeys so the Advanced tab can show them read-only rather than
  // accepting an edit that would silently revert on restart.
  auto user = session.Settings();
  QStringList locked_keys;

  const auto resolve = [&](const char* env_key, const QString& user_key,
                           const QVariant& fallback) {
    const auto pinned = env_value(env_key);
    if (pinned.isValid()) locked_keys << user_key;
    return ResolveLayeredValue(pinned, user.value(user_key), fallback);
  };

  // Resolve first, so a pinned SelfCheck key still registers as pinned and
  // the Advanced tab stays consistent, then let the build flavour have the last
  // word: a nightly ships no build-time signatures, so a check asked for there
  // could only ever fail and is forced off.
  const auto self_check_requested =
      resolve("SelfCheck", "advanced/self_check", false).toBool();
  property("GFSelfCheck", IsSelfCheckAvailable() && self_check_requested);
  property("GFSecureLevel",
           resolve("SecureLevel", "advanced/secure_level", 0).toInt());

  // How the key file is protected at rest used to live in two settings keys:
  // os_secret_store for the credential store, and secure_level >= 3 for a PIN.
  // Both now feed one key, so the resolution draws on three pinned keys rather
  // than one and cannot go through resolve(). The migration stays derived — it
  // is recomputed every start and nothing is written back, because this runs
  // before the secure allocator exists.
  auto protection = ProfileLoader::ResolveAppKeyProtection(
      env_value("AppKeyProtection"), env_value("SecureLevel"),
      env_value("OSSecretStore"), user.value("advanced/app_key_protection"),
      user.value("advanced/secure_level"),
      user.value("advanced/os_secret_store"));

  // Any pinned key that could have decided the protection pins it, so the
  // Advanced tab shows it read-only rather than accepting an edit that would
  // silently revert on the next start.
  if (env_value("AppKeyProtection").isValid() ||
      env_value("OSSecretStore").isValid() ||
      (env_value("SecureLevel").isValid() &&
       env_value("SecureLevel").toInt() >= 3)) {
    locked_keys << "advanced/app_key_protection";
  }

  // Portable installs allow only "none" and "pin". Resolving that here rather
  // than at each reader keeps the startup banner, the Advanced tab, and the key
  // loader from disagreeing about what is actually in effect.
  protection = ProfileLoader::ApplyProfilePortabilityRule(
      protection, ProfileSession::Instance().Profile().AllowsSystemKeychain());

  property("GFAppKeyProtection", AppKeyProtectionToString(protection));

  // `--log-level` sits above every stored layer. It has to be resolved here
  // rather than merely applied in main(), because PreInit() puts the property
  // below into effect -- so a flag that did not reach the property would be
  // silently undone by whatever advanced/log_level happened to say.
  //
  // An unset log level reads back as 0 (== kDEBUG), which would enable debug
  // logging even in release builds. Default to error level explicitly.
  const auto stored_log_level =
      resolve("LogLevel", "advanced/log_level",
              static_cast<int>(GFLogLevel::kCRITICAL));
  property("GFLogLevel", ResolveLayeredValue(cli_log_level, stored_log_level,
                                             stored_log_level)
                             .toInt());
  property("GFLogRingBufferCapacity",
           resolve("LogRingBufferCapacity", "advanced/log_ring_buffer_capacity",
                   1024)
               .toInt());

  property("GFEnvLockedKeys", locked_keys);

  const auto self_check = property("GFSelfCheck").toInt();
  const auto secure_level = property("GFSecureLevel").toInt();
  const auto app_key_protection = property("GFAppKeyProtection").toString();
  const auto log_level = property("GFLogLevel").toInt();
  const auto portable_mode = property("GFPortableMode").toBool();
  const auto gpg_offline_mode = property("GFGnuPGOfflineMode").toBool();
  const auto pinentry_program_path =
      property("GFPinentryProgramPath").toString();
  const auto ring_buffer_capacity = property("GFLogRingBufferCapacity").toInt();

  // Mark the values the profile pinned, so a support log makes it obvious why
  // the Advanced tab is not in charge of a given knob.
  const auto source = [&locked_keys](const QString& user_key) -> QString {
    return locked_keys.contains(user_key) ? QStringLiteral("  (profile.json)")
                                          : QString();
  };

  // The command line is deliberately not in locked_keys: it applies to this run
  // only, so showing the Advanced row as permanently read-only would be a lie.
  // The banner still has to say where the value came from.
  const auto log_level_source = cli_log_level.isValid()
                                    ? QStringLiteral("  (command line)")
                                    : source("advanced/log_level");

  qInfo().noquote().nospace()
      << "\n"
      << "================ GpgFrontend Startup Environment ================\n"
      << "Self Check              : " << self_check
      << source("advanced/self_check") << "\n"
      << "Secure Level            : " << secure_level
      << source("advanced/secure_level") << "\n"
      << "App Key Protection      : " << app_key_protection
      << source("advanced/app_key_protection") << "\n"
      << "Log Level               : " << log_level << log_level_source << "\n"
      << "Portable Mode           : " << BoolText(portable_mode) << "\n"
      << "GnuPG Offline Mode      : " << BoolText(gpg_offline_mode) << "\n"
      << "Pinentry Program Path   : " << DisplayPath(pinentry_program_path)
      << "\n"
      << "Log Ring Buffer Capacity: " << ring_buffer_capacity
      << source("advanced/log_ring_buffer_capacity") << "\n"
      << "==================================================================";
}

void GpgFrontendContext::resolve_profile_selection() {
  ProfileSelectionInput in;
  in.args = QCoreApplication::arguments();
  in.env_profile = qEnvironmentVariable("GF_PROFILE");
  in.portable_build = IsPortableBuild();
  in.portable_root = ResolvePortableDataPath();
  in.installed_root =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

  // A scan rather than an index, so there is nothing here that can disagree
  // with the filesystem. Cheap, and it is the only thing that makes naming a
  // profile that does not exist an error rather than a request to create one.
  const auto base = in.portable_build ? in.portable_root : in.installed_root;
  for (const auto& entry : ScanProfilesRoot(base + "/profiles")) {
    in.known_ids << entry.id;
  }

  auto result = ResolveProfileSelection(in);
  profile_error = result.error;
  profile_selection = result.selection;
}

void GpgFrontendContext::mirror_profile_properties() {
  const auto& session = ProfileSession::Instance();
  const auto& profile = session.Profile();

  property("GFPortableMode", profile.Kind() == ProfileKind::kPORTABLE_ROOT);
  property("GFProfileId", profile.Id());
  property("GFProfileKind", ProfileKindToString(profile.Kind()));
  property("GFProfilesRoot", profile_selection.profiles_root);
  property("GFProfileSelfContained", profile.Policy().self_contained);
  property("GFProfileRoot", profile.Root());
}

void GpgFrontendContext::LoadEnvProperties() {
  mirror_profile_properties();
  load_env_conf_set_properties();
}

void GpgFrontendContext::InitApplication() {
  app_ = new UI::GpgFrontendApplication(argc, argv);

  // Only decides *which* profile. Opening it needs a passphrase prompt and a
  // lock, so it belongs to the loader — and nothing here may read a setting
  // until that has happened, because where the settings live is exactly what
  // is being decided.
  resolve_profile_selection();
}

auto GpgFrontendContext::GetApp() -> QApplication* { return app_; }

GpgFrontendContext::GpgFrontendContext(int argc, char** argv)
    : argc(argc), argv(argv) {}

GpgFrontendContext::~GpgFrontendContext() { delete app_; }

auto GpgFrontendContext::property(const char* name) -> QVariant {
  if (app_ != nullptr) return app_->property(name);
  return {};
}

auto GpgFrontendContext::property(const char* name, const QVariant& value)
    -> bool {
  if (app_ != nullptr) return app_->setProperty(name, value);
  return false;
}
}  // namespace GpgFrontend