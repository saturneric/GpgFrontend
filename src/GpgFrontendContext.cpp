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
#include "core/function/AppSecureKeyManager.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/ProfileBootstrap.h"
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
  const auto env_config = QDir::currentPath() + "/ENV.ini";
  const auto has_env = QFileInfo(env_config).exists();
  if (!has_env) {
    qInfo() << "No ENV.ini found, falling back to user settings and defaults.";
  }

  QSettings s(env_config, QSettings::IniFormat);

  // Deployment-only knobs: ENV.ini is the single source for these.
  //
  // GFPortableMode is deliberately absent: it is now a mirror of the resolved
  // profile kind, published by establish_profile_runtime() before this runs.
  // Re-deriving it from ENV.ini here would say "portable" for a session that
  // was explicitly started on a named profile with `--profile`, which is a
  // location ENV.ini has no say over.
  property("GFGnuPGOfflineMode",
           has_env && s.value("GnuPGOfflineMode", false).toBool());
  property("GFPinentryProgramPath",
           has_env ? s.value("PinentryProgramPath", "").toString() : QString());

  // The knobs below are also editable in Settings -> Advanced, so they resolve
  // in three layers: an ENV.ini key wins (deployment override), else the user's
  // stored setting, else the built-in default. Keys present in ENV.ini are
  // recorded in GFEnvLockedKeys so the Advanced tab can show them read-only
  // rather than accepting an edit that would silently revert on restart.
  auto user = GetEarlySettings();
  QStringList locked_keys;

  const auto resolve = [&](const QString& env_key, const QString& user_key,
                           const QVariant& fallback) {
    const auto env_value = has_env ? s.value(env_key) : QVariant();
    if (env_value.isValid()) locked_keys << user_key;
    return ResolveLayeredValue(env_value, user.value(user_key), fallback);
  };

  // Resolve first, so an ENV.ini SelfCheck key still registers as pinned and
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
  // Both now feed one key, so the resolution draws on three ENV.ini keys rather
  // than one and cannot go through resolve(). The migration stays derived — it
  // is recomputed every start and nothing is written back, because this runs
  // before the secure allocator exists.
  const auto env_value = [&](const char* key) {
    return has_env ? s.value(QLatin1String(key)) : QVariant();
  };

  auto protection = ResolveAppKeyProtection(
      env_value("AppKeyProtection"), env_value("SecureLevel"),
      env_value("OSSecretStore"), user.value("advanced/app_key_protection"),
      user.value("advanced/secure_level"),
      user.value("advanced/os_secret_store"));

  // Any ENV.ini key that could have decided the protection pins it, so the
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
  protection =
      ApplyPortableModeRule(protection, property("GFPortableMode").toBool());

  property("GFAppKeyProtection", AppKeyProtectionToString(protection));
  // An unset log level reads back as 0 (== kDEBUG), which would enable debug
  // logging even in release builds. Default to error level explicitly.
  property("GFLogLevel", resolve("LogLevel", "advanced/log_level",
                                 static_cast<int>(GFLogLevel::kCRITICAL))
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

  // Mark the values ENV.ini pinned, so a support log makes it obvious why the
  // Advanced tab is not in charge of a given knob.
  const auto source = [&locked_keys](const QString& user_key) -> QString {
    return locked_keys.contains(user_key) ? QStringLiteral("  (ENV.ini)")
                                          : QString();
  };

  qInfo().noquote().nospace()
      << "\n"
      << "================ GpgFrontend Startup Environment ================\n"
      << "Self Check              : " << self_check
      << source("advanced/self_check") << "\n"
      << "Secure Level            : " << secure_level
      << source("advanced/secure_level") << "\n"
      << "App Key Protection      : " << app_key_protection
      << source("advanced/app_key_protection") << "\n"
      << "Log Level               : " << log_level
      << source("advanced/log_level") << "\n"
      << "Portable Mode           : " << BoolText(portable_mode) << "\n"
      << "GnuPG Offline Mode      : " << BoolText(gpg_offline_mode) << "\n"
      << "Pinentry Program Path   : " << DisplayPath(pinentry_program_path)
      << "\n"
      << "Log Ring Buffer Capacity: " << ring_buffer_capacity
      << source("advanced/log_ring_buffer_capacity") << "\n"
      << "==================================================================";
}

void GpgFrontendContext::establish_profile_runtime() {
  // PortableMode has to be read here rather than taken from
  // load_env_conf_set_properties(), which has not run yet and cannot: it reads
  // user settings, and where those live is exactly what is being decided.
  const auto env_config = QDir::currentPath() + "/ENV.ini";
  const auto has_env = QFileInfo(env_config).exists();
  QSettings env(env_config, QSettings::IniFormat);

  ProfileBootstrapInput in;
  in.args = QCoreApplication::arguments();
  in.env_profile = qEnvironmentVariable("GF_PROFILE");
  in.env_profile_root = qEnvironmentVariable("GF_PROFILE_ROOT");
  in.env_ini_portable = has_env && env.value("PortableMode", false).toBool();
  in.portable_root = ResolvePortableDataPath();
  in.classic_root =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

  // The registry does not exist yet at this point in the feature's life, so
  // known_ids stays empty and registry_available stays false: an id that is not
  // in a list nobody has written is not an unknown profile.

  auto result = ResolveProfileBootstrap(in);
  profile_error = result.error;

  // Packages resolve to kPACKAGE_PENDING, a state whose root is by definition
  // not usable until the package has been extracted — and nothing extracts one
  // yet. Left as it is, the first setting read would abort the process; turned
  // into an ordinary bootstrap error it is reported in a dialog, and the
  // runtime still gets a real root to be established on so that reporting can
  // happen at all. Replaced by the extraction step when packages land.
  if (result.error.isEmpty() &&
      result.state.kind == ProfileRootKind::kPACKAGE_PENDING) {
    const auto package = result.state.pending_package;
    qWarning() << "cannot open profile package yet:" << package;

    auto fallback = in;
    fallback.args.removeAll(package);
    result = ResolveProfileBootstrap(fallback);

    profile_error = QCoreApplication::translate(
                        "GpgFrontendContext",
                        "Profile packages cannot be opened by this version "
                        "yet:") +
                    "\n" + QDir::toNativeSeparators(package);
  }

  // The resolver is pure and never reads a file, but a profile's policy is
  // recorded in its own marker — so it is loaded here, before the runtime is
  // fixed. Portable keeps the policy the resolver gave it: for that shape the
  // location *is* the decision, and there may be no marker yet at all.
  if (result.error.isEmpty() &&
      result.state.kind != ProfileRootKind::kPORTABLE &&
      result.state.kind != ProfileRootKind::kPACKAGE_PENDING) {
    if (const auto marker =
            ReadProfileMarker(ProfileMarkerPathFor(result.state.root))) {
      result.state.policy.self_contained = marker->self_contained;
    }
  }

  ProfileRuntime::Establish(result.state);
  mirror_profile_properties();
}

void GpgFrontendContext::mirror_profile_properties() {
  const auto& p = ProfileRuntime::Instance();

  property("GFPortableMode", p.kind == ProfileRootKind::kPORTABLE);
  property("GFProfileId", p.id);
  property("GFProfileKind", ProfileRootKindToString(p.kind));
  property("GFProfilesRoot", p.profiles_root);
  property("GFProfileSelfContained", p.policy.self_contained);
  property("GFProfilePendingPackage", p.pending_package);
  property("GFProfileRoot",
           p.kind == ProfileRootKind::kPACKAGE_PENDING ? QString{} : p.root);
}

void GpgFrontendContext::ReloadEnvProperties() {
  mirror_profile_properties();
  load_env_conf_set_properties();
}

void GpgFrontendContext::InitApplication() {
  app_ = new UI::GpgFrontendApplication(argc, argv);

  // Before anything reads a setting: every path below is keyed by the profile
  // root, and GetEarlySettings() is called from load_env_conf_set_properties().
  establish_profile_runtime();

  load_env_conf_set_properties();
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