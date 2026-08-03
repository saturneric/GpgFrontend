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

#pragma once

#include "core/function/ProfileRegistry.h"

namespace GpgFrontend::UI {

/**
 * @brief Remove every profile-selecting argument from a command line.
 *
 * A new window inherits this process's arguments so that logging level and the
 * like carry over — but never *which profile*, which is the one thing the new
 * window is being given explicitly. Without stripping, opening three profiles
 * in turn would leave three `--profile` flags and the resolver would honour the
 * oldest.
 *
 * Pure, so the accumulation case is assertable without launching anything.
 *
 * @param args argument list, argv[0] included
 * @return the list with `--profile`, `--profile-root` and any positional
 * package removed
 */
auto GF_UI_EXPORT StripProfileArgs(const QStringList &args) -> QStringList;

/**
 * @brief The command line for a new instance opening a local profile.
 *
 * @param args this process's arguments, argv[0] included
 * @param profile_id profile to open, or empty for the implicit default
 * @return arguments excluding argv[0]
 */
auto GF_UI_EXPORT BuildProfileLaunchArgs(const QStringList &args,
                                         const QString &profile_id)
    -> QStringList;

/**
 * @brief The command line for a new instance opening a package.
 *
 * The package is passed positionally, exactly as a file manager would hand it
 * over, so the two ways of opening one converge on a single code path.
 *
 * @param args this process's arguments, argv[0] included
 * @param package_path absolute path to a `.gfprofile`
 * @return arguments excluding argv[0]
 */
auto GF_UI_EXPORT BuildPackageLaunchArgs(const QStringList &args,
                                         const QString &package_path)
    -> QStringList;

/**
 * @brief Where this machine keeps its profiles, and the two implicit roots.
 *
 * Bundled because every registry call needs all three and reassembling them at
 * each call site is how they drift apart.
 */
struct GF_UI_EXPORT ProfileRoots {
  QString profiles_root;
  QString classic_root;
  QString portable_root;  ///< empty when this is not a portable installation
};

/**
 * @brief Resolve the roots for the running process.
 *
 * @return the three roots
 */
auto GF_UI_EXPORT CurrentProfileRoots() -> ProfileRoots;

/**
 * @brief Load the profile list as the user should see it.
 *
 * @return the registry, implicit entries included
 */
auto GF_UI_EXPORT LoadProfiles() -> ProfileRegistryData;

/**
 * @brief What to call the profile this window is using.
 *
 * The name the user gave it, falling back to the folder name, and to a plain
 * word for the two profiles nobody names — the default location and a portable
 * installation.
 *
 * @return a name fit to show in the window
 */
auto GF_UI_EXPORT CurrentProfileDisplayName() -> QString;

/**
 * @brief What to call a kind of profile in front of a user.
 *
 * Shared rather than spelled out at each call site: the profile list and the
 * about dialog naming the same thing differently is how a user concludes they
 * are two different things.
 *
 * @param kind the kind
 * @return a word for it
 */
auto GF_UI_EXPORT ProfileKindDisplayName(ProfileRootKind kind) -> QString;

/**
 * @brief Why a profile could not be opened in a new window.
 */
enum class ProfileLaunchStatus {
  kSTARTED,
  kALREADY_OPEN,  ///< another window — possibly this one — has it
  kNOT_FOUND,     ///< no such profile, or the package is gone
  kFAILED,        ///< the process could not be started
};

/**
 * @brief Outcome of opening a profile, with enough to explain a refusal.
 */
struct GF_UI_EXPORT ProfileLaunchResult {
  ProfileLaunchStatus status = ProfileLaunchStatus::kSTARTED;
  QString detail;

  [[nodiscard]] auto Ok() const -> bool {
    return status == ProfileLaunchStatus::kSTARTED;
  }
};

/**
 * @brief Open a profile in a new window, leaving this one alone.
 *
 * Singletons are process-global and a channel means one key database, not one
 * profile, so two profiles cannot coexist in one process. A new window is
 * therefore a new *process* given a different command line — which is also what
 * makes leaving the current window open the safe option: neither instance
 * touches the other's root, and each takes its own lock.
 *
 * @param profile_id profile to open
 * @return kSTARTED once the process is launched
 */
auto GF_UI_EXPORT OpenProfileInNewWindow(const QString &profile_id)
    -> ProfileLaunchResult;

/**
 * @brief Open a profile package in a new window.
 *
 * @param package_path absolute path to a `.gfprofile`
 * @return kSTARTED once the process is launched
 */
auto GF_UI_EXPORT OpenPackageInNewWindow(const QString &package_path)
    -> ProfileLaunchResult;

}  // namespace GpgFrontend::UI
