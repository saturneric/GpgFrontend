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

#include "ProfileBootstrap.h"

#include <mutex>
#include <optional>

#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

namespace {

/// Options that consume the following argument, so a scan for a positional
/// never mistakes an option's value for one.
const QStringList kValueOptions = {"--profile", "--profile-root", "--log-level",
                                   "-l"};

auto OptionValue(const QStringList &args, const QString &name) -> QString {
  const auto long_form = "--" + name;
  for (int i = 1; i < args.size(); ++i) {
    const auto &arg = args.at(i);
    if (arg == long_form) {
      return i + 1 < args.size() ? args.at(i + 1) : QString{};
    }
    if (arg.startsWith(long_form + "=")) {
      return arg.mid(long_form.size() + 1);
    }
  }
  return {};
}

auto PositionalPackage(const QStringList &args) -> QString {
  for (int i = 1; i < args.size(); ++i) {
    const auto &arg = args.at(i);
    if (kValueOptions.contains(arg)) {
      ++i;  // skip the value
      continue;
    }
    if (arg.startsWith('-')) continue;
    if (arg.endsWith(".gfprofile", Qt::CaseInsensitive)) return arg;
  }
  return {};
}

/// Reserved on Windows regardless of extension; creating one of these produces
/// a directory that exists on Linux and cannot on Windows, which is exactly the
/// kind of asymmetry a portable profile must not have.
auto IsWindowsDeviceName(const QString &id) -> bool {
  static const QStringList kReserved = {
      "con",  "prn",  "aux",  "nul",  "com1", "com2", "com3", "com4",
      "com5", "com6", "com7", "com8", "com9", "lpt1", "lpt2", "lpt3",
      "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"};
  return kReserved.contains(id.toLower());
}

struct RuntimeHolder {
  std::mutex mutex;
  std::optional<ProfileRuntimeState> state;
};

auto Holder() -> RuntimeHolder & {
  static RuntimeHolder holder;
  return holder;
}

}  // namespace

auto ProfileRootKindToString(ProfileRootKind kind) -> QString {
  switch (kind) {
    case ProfileRootKind::kCLASSIC:
      return "classic";
    case ProfileRootKind::kPORTABLE:
      return "portable";
    case ProfileRootKind::kNAMED:
      return "named";
    case ProfileRootKind::kEXPLICIT_ROOT:
      return "explicit_root";
    case ProfileRootKind::kPACKAGE_LINKED:
      return "package_linked";
    case ProfileRootKind::kPACKAGE_PENDING:
      return "package_pending";
  }
  return "classic";
}

auto ProfileRootKindFromString(const QString &s) -> ProfileRootKind {
  const auto v = s.trimmed().toLower();
  if (v == "portable") return ProfileRootKind::kPORTABLE;
  if (v == "named" || v == "local") return ProfileRootKind::kNAMED;
  if (v == "explicit_root") return ProfileRootKind::kEXPLICIT_ROOT;
  if (v == "package_linked") return ProfileRootKind::kPACKAGE_LINKED;
  if (v == "package_pending") return ProfileRootKind::kPACKAGE_PENDING;
  return ProfileRootKind::kCLASSIC;
}

auto ProfileStartupPolicyToString(ProfileStartupPolicy p) -> QString {
  switch (p) {
    case ProfileStartupPolicy::kLAST_USED:
      return "last_used";
    case ProfileStartupPolicy::kASK:
      return "ask";
    case ProfileStartupPolicy::kFIXED:
      return "fixed";
    case ProfileStartupPolicy::kCLASSIC:
      return "classic";
  }
  return "last_used";
}

auto ProfileStartupPolicyFromString(const QString &s) -> ProfileStartupPolicy {
  const auto v = s.trimmed().toLower();
  if (v == "ask") return ProfileStartupPolicy::kASK;
  if (v == "fixed") return ProfileStartupPolicy::kFIXED;
  if (v == "classic") return ProfileStartupPolicy::kCLASSIC;
  return ProfileStartupPolicy::kLAST_USED;
}

auto RequireProfileRoot(const ProfileRuntimeState &state) -> QString {
  if (state.kind == ProfileRootKind::kPACKAGE_PENDING) {
    qFatal(
        "profile root requested while a package is still pending extraction; "
        "package: %s",
        qPrintable(state.pending_package));
  }
  return state.root;
}

auto ResolveApplicationDirPath() -> QString {
  auto app_path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_LINUX
  if (IsAppImageENV()) {
    QFileInfo info(qEnvironmentVariable("APPIMAGE"));
    const auto dir = info.canonicalPath();
    if (!dir.isEmpty()) app_path = dir;
  }
#endif
  return app_path;
}

auto ResolvePortableDataPath() -> QString {
  const auto app_path = ResolveApplicationDirPath();
  const auto canonical = QDir(app_path + "/../").canonicalPath();
  // canonicalPath() is empty when the parent does not resolve, which would
  // silently turn every derived path into a relative one
  return canonical.isEmpty() ? QDir::cleanPath(app_path + "/..") : canonical;
}

auto IsValidProfileId(const QString &id) -> bool {
  if (id.isEmpty() || id.size() > 64) return false;
  if (IsWindowsDeviceName(id)) return false;

  static const QRegularExpression kPattern(QStringLiteral("^[a-z0-9_-]+$"));
  if (!kPattern.match(id).hasMatch()) return false;

  // "." and ".." cannot match the pattern above, but a leading hyphen would
  // read as an option everywhere the id is passed on a command line
  return !id.startsWith('-');
}

auto MakeProfileId(const QString &name) -> QString {
  QString out;
  out.reserve(name.size());
  for (const auto c : name.toLower()) {
    if ((c.isLetterOrNumber() && c.unicode() < 128) || c == '_' || c == '-') {
      out.append(c);
    } else if (!out.isEmpty() && !out.endsWith('_') && !out.endsWith('-')) {
      // anything else — spaces, punctuation, non-ASCII — collapses to one
      // separator rather than being dropped, so "My Work" stays two words
      out.append('_');
    }
  }
  while (out.endsWith('_') || out.endsWith('-')) out.chop(1);
  while (out.startsWith('_') || out.startsWith('-')) out.remove(0, 1);
  if (out.size() > 64) out.truncate(64);
  return IsValidProfileId(out) ? out : QString{};
}

namespace {

auto ResolveNamed(const QString &id, const ProfileBootstrapInput &in,
                  const QString &profiles_root) -> ProfileBootstrapResult {
  ProfileBootstrapResult r;
  r.state.profiles_root = profiles_root;

  // the fallback must be usable: an error still leaves the process on a sane
  // classic state so nothing downstream has to cope with a half-resolved run
  r.state.kind = ProfileRootKind::kCLASSIC;
  r.state.id = "classic";
  r.state.root = in.classic_root;

  if (!IsValidProfileId(id)) {
    r.error = QString("'%1' is not a valid profile name.").arg(id);
    return r;
  }

  if (in.registry_available && !in.known_ids.contains(id)) {
    r.error = QString("There is no profile named '%1'.").arg(id);
    return r;
  }

  r.state.kind = ProfileRootKind::kNAMED;
  r.state.id = id;
  r.state.root = profiles_root + "/" + id;
  return r;
}

auto ResolveExplicitRoot(const QString &path, const QString &profiles_root,
                         const QString &classic_root)
    -> ProfileBootstrapResult {
  ProfileBootstrapResult r;
  r.state.profiles_root = profiles_root;

  if (!QDir::isAbsolutePath(path)) {
    r.state.kind = ProfileRootKind::kCLASSIC;
    r.state.id = "classic";
    r.state.root = classic_root;
    r.error = QString(
                  "A profile directory must be an absolute path, but '%1' is "
                  "relative.")
                  .arg(path);
    return r;
  }

  r.state.kind = ProfileRootKind::kEXPLICIT_ROOT;
  r.state.root = QDir::cleanPath(path);
  r.state.id = QFileInfo(r.state.root).fileName();
  if (r.state.id.isEmpty()) r.state.id = "explicit";
  return r;
}

}  // namespace

auto ResolveProfileBootstrap(const ProfileBootstrapInput &in)
    -> ProfileBootstrapResult {
  const auto base = in.env_ini_portable ? in.portable_root : in.classic_root;
  const auto profiles_root = base + "/profiles";

  // 1. an explicit directory, used by the deep-restart relaunch and by tests
  const auto cli_root = OptionValue(in.args, "profile-root");
  if (!cli_root.isEmpty()) {
    return ResolveExplicitRoot(cli_root, profiles_root, in.classic_root);
  }

  // 2. a named profile. Explicit selection outranks ENV.ini: on a portable
  // installation this opens <portable-root>/profiles/<id>, and that profile's
  // own policy decides whether it is self-contained — ENV.ini only chooses the
  // implicit default at the bottom of this ladder.
  const auto cli_named = OptionValue(in.args, "profile");
  if (!cli_named.isEmpty()) return ResolveNamed(cli_named, in, profiles_root);

  // 3. a package named on the command line, which cannot be opened here: it
  // needs a passphrase, so the caller extracts it and transitions the state
  const auto package = PositionalPackage(in.args);
  if (!package.isEmpty()) {
    ProfileBootstrapResult r;
    r.state.profiles_root = profiles_root;
    r.state.kind = ProfileRootKind::kPACKAGE_PENDING;
    r.state.pending_package = package;
    r.state.id = QFileInfo(package).completeBaseName();
    return r;
  }

  // 4. the environment, for CI and headless runs
  if (!in.env_profile_root.isEmpty()) {
    return ResolveExplicitRoot(in.env_profile_root, profiles_root,
                               in.classic_root);
  }
  if (!in.env_profile.isEmpty()) {
    return ResolveNamed(in.env_profile, in, profiles_root);
  }

  // 5. whatever the registry was told to do
  switch (in.startup_policy) {
    case ProfileStartupPolicy::kFIXED:
      if (!in.registry_startup_profile.isEmpty() &&
          in.registry_startup_profile != "classic" &&
          in.registry_startup_profile != "portable") {
        return ResolveNamed(in.registry_startup_profile, in, profiles_root);
      }
      break;
    case ProfileStartupPolicy::kLAST_USED:
      if (!in.registry_last_used.isEmpty() &&
          in.registry_last_used != "classic" &&
          in.registry_last_used != "portable") {
        return ResolveNamed(in.registry_last_used, in, profiles_root);
      }
      break;
    case ProfileStartupPolicy::kASK:
      // the picker is a UI concern; with nothing chosen yet the implicit
      // default below is the honest answer
      break;
    case ProfileStartupPolicy::kCLASSIC:
      break;
  }

  // 6. the implicit default
  ProfileBootstrapResult r;
  r.state.profiles_root = profiles_root;
  if (in.env_ini_portable) {
    r.state.kind = ProfileRootKind::kPORTABLE;
    r.state.id = "portable";
    r.state.root = in.portable_root;
    r.state.policy.self_contained = true;
  } else {
    r.state.kind = ProfileRootKind::kCLASSIC;
    r.state.id = "classic";
    r.state.root = in.classic_root;
  }
  return r;
}

void ProfileRuntime::Establish(const ProfileRuntimeState &state) {
  auto &holder = Holder();
  std::unique_lock<std::mutex> const lock(holder.mutex);

  if (holder.state.has_value()) {
    qFatal(
        "the profile runtime was established twice; first as '%s', then as "
        "'%s'",
        qPrintable(holder.state->id), qPrintable(state.id));
  }
  holder.state = state;
}

void ProfileRuntime::EstablishFromPackage(const ProfileRuntimeState &state) {
  auto &holder = Holder();
  std::unique_lock<std::mutex> const lock(holder.mutex);

  if (!holder.state.has_value()) {
    qFatal("a package profile was established before the runtime existed");
  }
  if (holder.state->kind != ProfileRootKind::kPACKAGE_PENDING) {
    qFatal(
        "a package profile may only replace a pending one, but the runtime is "
        "'%s'",
        qPrintable(ProfileRootKindToString(holder.state->kind)));
  }
  holder.state = state;
}

auto ProfileRuntime::Established() -> bool {
  auto &holder = Holder();
  std::unique_lock<std::mutex> const lock(holder.mutex);
  return holder.state.has_value();
}

auto ProfileRuntime::Instance() -> const ProfileRuntimeState & {
  auto &holder = Holder();
  std::unique_lock<std::mutex> const lock(holder.mutex);

  if (!holder.state.has_value()) {
    // Everything below this point derives a path from the profile root.
    // Continuing with an unresolved one would read and write another profile's
    // key material, so this is fatal in release builds too rather than an
    // assertion a release build drops.
    qFatal("the profile runtime was read before it was established");
  }
  return *holder.state;
}

void ProfileRuntime::ResetForTesting() {
  auto &holder = Holder();
  std::unique_lock<std::mutex> const lock(holder.mutex);
  holder.state.reset();
}

}  // namespace GpgFrontend
