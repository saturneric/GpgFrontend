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

#include "ui/function/ProfileController.h"

#include <QProcess>

#include "core/function/GlobalSettingStation.h"
#include "core/function/ProfileBootstrap.h"
#include "core/function/ProfileLock.h"

namespace GpgFrontend::UI {

namespace {

/// Options whose value is a separate argument, so stripping the option also
/// strips what belongs to it.
const QStringList kProfileValueOptions = {"--profile", "--profile-root"};

/**
 * @brief Start a second instance of this application, detached.
 *
 * Detached on purpose: the new window outlives whichever window opened it, and
 * neither is the other's parent in any sense that matters.
 *
 * @param args arguments for the new instance, argv[0] excluded
 * @return kSTARTED when the process was launched
 */
auto Launch(const QStringList& args) -> ProfileLaunchResult {
  ProfileLaunchResult result;
  LOG_I() << "opening a new window with args" << args;

#ifdef Q_OS_MACOS
  // A bundled application is launched through the workspace, not by running the
  // executable: that is what gives the new instance its own Dock entry and
  // activation, and it is already how the deep restart comes back.
  if (!RelaunchApplication(args)) {
    result.status = ProfileLaunchStatus::kFAILED;
    result.detail = QObject::tr("The new window could not be started.");
  }
#else
  if (!QProcess::startDetached(QCoreApplication::applicationFilePath(), args)) {
    result.status = ProfileLaunchStatus::kFAILED;
    result.detail = QObject::tr("The new window could not be started.");
  }
#endif
  return result;
}

}  // namespace

auto StripProfileArgs(const QStringList& args) -> QStringList {
  QStringList out;
  if (args.isEmpty()) return out;

  out << args.first();
  for (int i = 1; i < args.size(); ++i) {
    const auto& arg = args.at(i);

    if (kProfileValueOptions.contains(arg)) {
      ++i;  // and its value
      continue;
    }
    if (arg.startsWith("--profile=") || arg.startsWith("--profile-root=")) {
      continue;
    }
    // a package named on the command line selected the *previous* profile; a
    // switch away from it must not silently re-open it
    if (!arg.startsWith('-') &&
        arg.endsWith(".gfprofile", Qt::CaseInsensitive)) {
      continue;
    }
    out << arg;
  }
  return out;
}

auto BuildProfileLaunchArgs(const QStringList& args, const QString& profile_id)
    -> QStringList {
  auto stripped = StripProfileArgs(args);
  if (!stripped.isEmpty()) stripped.removeFirst();  // argv[0]

  if (!profile_id.isEmpty() && profile_id != "classic" &&
      profile_id != "portable") {
    stripped << "--profile" << profile_id;
  }
  return stripped;
}

auto BuildPackageLaunchArgs(const QStringList& args,
                            const QString& package_path) -> QStringList {
  auto stripped = StripProfileArgs(args);
  if (!stripped.isEmpty()) stripped.removeFirst();  // argv[0]

  if (!package_path.isEmpty()) stripped << package_path;
  return stripped;
}

auto CurrentProfileRoots() -> ProfileRoots {
  const auto& profile = ProfileRuntime::Instance();

  ProfileRoots roots;
  roots.profiles_root = profile.profiles_root;
  roots.classic_root =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

  // Only offer the portable entry where one actually exists. Listing it on an
  // installed copy would show a profile the user can never open.
  if (profile.kind == ProfileRootKind::kPORTABLE ||
      profile.profiles_root.startsWith(ResolvePortableDataPath())) {
    roots.portable_root = ResolvePortableDataPath();
  }
  return roots;
}

auto LoadProfiles() -> ProfileRegistryData {
  const auto roots = CurrentProfileRoots();
  auto data = LoadProfileRegistry(roots.profiles_root, roots.classic_root,
                                  roots.portable_root);

  // The profile this window is using is always in the list, even when it is an
  // ad-hoc root named on the command line that the registry has never heard
  // of. A list of profiles that leaves out the one in front of the user
  // invites them to read some other row as the one they are in.
  const auto& profile = ProfileRuntime::Instance();
  if (!data.Find(profile.id).has_value()) {
    ProfileRegistryEntry entry;
    entry.id = profile.id;
    entry.root = profile.root;
    entry.name = CurrentProfileDisplayName();
    entry.kind = profile.kind;
    entry.implicit = true;  // nothing to persist, and nothing to delete
    data.profiles.prepend(entry);
  }
  return data;
}

auto ProfileKindDisplayName(ProfileRootKind kind) -> QString {
  switch (kind) {
    case ProfileRootKind::kCLASSIC:
      return QObject::tr("Default");
    case ProfileRootKind::kPORTABLE:
      return QObject::tr("Portable");
    case ProfileRootKind::kPACKAGE_LINKED:
      return QObject::tr("From a package");
    default:
      return QObject::tr("Local");
  }
}

auto CurrentProfileDisplayName() -> QString {
  const auto& profile = ProfileRuntime::Instance();

  switch (profile.kind) {
    case ProfileRootKind::kCLASSIC:
      return QObject::tr("Default");
    case ProfileRootKind::kPORTABLE:
      return QObject::tr("Portable");
    default:
      break;
  }

  // Read from the profile's own marker rather than the registry: the marker
  // travels with the profile and is already on the way in, while the registry
  // is a machine-local index that a read here would also make us reconcile.
  const auto marker = ReadProfileMarker(ProfileMarkerPathFor(profile.root));
  if (marker.has_value() && !marker->display_name.isEmpty()) {
    return marker->display_name;
  }
  return profile.id;
}

auto OpenProfileInNewWindow(const QString& profile_id) -> ProfileLaunchResult {
  ProfileLaunchResult result;

  if (profile_id == ProfileRuntime::Instance().id) {
    result.status = ProfileLaunchStatus::kALREADY_OPEN;
    result.detail = QObject::tr("This window is already using that profile.");
    return result;
  }

  const auto roots = CurrentProfileRoots();
  const auto data = LoadProfileRegistry(roots.profiles_root, roots.classic_root,
                                        roots.portable_root);
  const auto entry = data.Find(profile_id);
  if (!entry.has_value()) {
    result.status = ProfileLaunchStatus::kNOT_FOUND;
    result.detail =
        QObject::tr("There is no profile called \"%1\".").arg(profile_id);
    return result;
  }

  // Asked before the process is launched, so a busy profile is a message here
  // rather than a window that appears and immediately refuses.
  const auto lock = ProfileLock::Probe(entry->root);
  if (lock.status == ProfileLockStatus::kHELD_ELSEWHERE) {
    result.status = ProfileLaunchStatus::kALREADY_OPEN;
    result.detail =
        lock.pid > 0
            ? QObject::tr(
                  "\"%1\" is open in another window (process %2 on %3).")
                  .arg(entry->name)
                  .arg(lock.pid)
                  .arg(lock.host)
            : QObject::tr("\"%1\" is open in another window.").arg(entry->name);
    return result;
  }

  TouchProfile(roots.profiles_root, roots.classic_root, roots.portable_root,
               profile_id,
               QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

  return Launch(BuildProfileLaunchArgs(qApp->arguments(), profile_id));
}

auto OpenPackageInNewWindow(const QString& package_path)
    -> ProfileLaunchResult {
  ProfileLaunchResult result;

  if (!QFileInfo::exists(package_path)) {
    result.status = ProfileLaunchStatus::kNOT_FOUND;
    result.detail = QObject::tr("This file is no longer there:") + "\n" +
                    QDir::toNativeSeparators(package_path);
    return result;
  }

  return Launch(BuildPackageLaunchArgs(
      qApp->arguments(), QFileInfo(package_path).absoluteFilePath()));
}

}  // namespace GpgFrontend::UI
