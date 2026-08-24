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

#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileRegistry.h"

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
 * @return the list with `--profile` and any positional
 * package removed
 */
auto GF_UI_EXPORT StripProfileArgs(const QStringList &args) -> QStringList;

/**
 * @brief The name filter every profile-file dialog uses.
 *
 * A function rather than a constant because it is translated: a constant would
 * be built at static-initialisation time, before any translator exists. Shared
 * so that what the dialogs offer and what the argv scan accepts cannot drift.
 *
 * @return a QFileDialog name filter for the package extension
 */
auto GF_UI_EXPORT ProfilePackageNameFilter() -> QString;

/**
 * @brief What a profile file's unencrypted header claims about itself.
 *
 * Shown before a passphrase is typed, so that a file whose claims look wrong
 * gets a second thought first. Deliberately worded as a claim throughout, and
 * paired with the sentence saying why: the header sits outside the sealed
 * payload, so anyone holding the file can write anything into it, and
 * ProfilePackage.h is explicit that it may reject but must never be believed.
 *
 * Pure, so the framing can be asserted rather than trusted to survive editing.
 *
 * @param header the parsed header, from InspectProfilePackage()
 * @return plain text, or empty when the header claims nothing worth showing
 */
auto GF_UI_EXPORT DescribeUnverifiedHeader(const ProfilePackageHeader &header)
    -> QString;

/**
 * @brief What to open in a new window.
 *
 * Exactly one field is set: a profile this machine keeps, or a package to run
 * temporarily. One type rather than two entry points, because everything that
 * happens afterwards — stripping the old selection, refusing a root somebody
 * already has, starting the process — is the same for both, and the two ways of
 * saying it drifted apart the moment they were written separately.
 */
struct GF_UI_EXPORT ProfileTarget {
  QString profile_id;
  QString package_path;

  [[nodiscard]] auto IsPackage() const -> bool {
    return !package_path.isEmpty();
  }
};

/**
 * @brief The directory whose lock says whether the target is already open.
 *
 * For a package this is derived from the package's own path, so this window can
 * ask whether another window already has that package open without either of
 * them recording it anywhere.
 *
 * Deliberately not "where the session's data will be": a session's storage is
 * chosen at mount time and may not be here at all. Probing the wrong one would
 * let two windows open one package and write back over each other.
 *
 * @param target what is to be opened
 * @return an absolute path, or empty when the target names nothing
 */
auto GF_UI_EXPORT ProfileTargetLockRoot(const ProfileTarget &target) -> QString;

/**
 * @brief The command line for a new instance opening a target.
 *
 * A package is passed positionally, exactly as a file manager would hand it
 * over, so opening one from the menu and double-clicking it converge.
 *
 * @param args this process's arguments, argv[0] included
 * @param target what the new instance should open
 * @return arguments excluding argv[0]
 */
auto GF_UI_EXPORT BuildLaunchArgs(const QStringList &args,
                                  const ProfileTarget &target) -> QStringList;

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
auto GF_UI_EXPORT ProfileKindDisplayName(ProfileKind kind) -> QString;

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
 * A package opened this way runs temporarily, and closing it asks whether the
 * changes go back into the file. That is the only difference between the two
 * kinds of target from here on.
 *
 * @param target profile or package to open
 * @return kSTARTED once the process is launched
 */
auto GF_UI_EXPORT OpenProfileInNewWindow(const ProfileTarget &target)
    -> ProfileLaunchResult;

/**
 * @brief Whether this window is already running the package at @p path.
 *
 * The lock probe inside OpenProfileInNewWindow() would also refuse such a
 * request, but it would name this very process as the other window holding it.
 * Asked separately so that double-clicking the file already on screen can be
 * answered by raising that window instead of with a puzzling message.
 *
 * @param path a package path, absolute or not
 * @return true when this process's profile came from that file
 */
auto GF_UI_EXPORT IsCurrentPackageSession(const QString &path) -> bool;

/**
 * @brief Offer to write a temporary session back into the package it came from.
 *
 * A session is disposable by construction: the extracted tree is deleted on the
 * way out, so anything done in it is lost unless it is packed again. That makes
 * this the last moment the question can be asked, and the reason it is asked
 * rather than assumed — a package opened to look at something should not be
 * rewritten just because it was opened.
 *
 * Packing is not instant, so a save does not happen inside this call: it
 * starts, and @p on_done runs when it finishes. The caller is expected to hold
 * the close and try again from there.
 *
 * @param parent parent for the dialogs
 * @param on_done invoked once a started write-back has finished successfully
 * @return true to let the close proceed; false to hold it — either the user
 * cancelled, or a write-back is running
 */
auto GF_UI_EXPORT MaybeWriteBackPackageSession(
    QWidget *parent, const std::function<void()> &on_done) -> bool;

/**
 * @brief Create a profile, asking the user what to call it.
 *
 * Here rather than on the profile manager because the manager is only one of
 * the places it is offered from — the Profile menu reaches it without any list
 * being on screen. The two callbacks are what the manager needs and the menu
 * does not, so both are optional.
 *
 * @param parent parent for the dialogs
 * @param on_changed invoked once the profile list has changed
 * @param on_opened invoked when a new window was launched, so a list showing
 * the old state may close itself
 */
void GF_UI_EXPORT CreateProfileInteractive(
    QWidget *parent, const std::function<void()> &on_changed = {},
    const std::function<void()> &on_opened = {});

/**
 * @brief Read a `.gfp` into a new profile on this computer.
 *
 * Asynchronous: reading and decrypting the package runs on a worker, so the
 * callbacks fire well after this returns.
 *
 * @param parent parent for the dialogs
 * @param on_changed invoked once the profile list has changed
 * @param on_opened invoked when a new window was launched
 */
void GF_UI_EXPORT ImportProfileInteractive(
    QWidget *parent, const std::function<void()> &on_changed = {},
    const std::function<void()> &on_opened = {});

/**
 * @brief Import a `.gfp` that has already been named.
 *
 * The same import, minus the file dialog: the file panel already knows which
 * file was double-clicked, and asking for it again would be asking a question
 * that has been answered.
 *
 * @param parent parent for the dialogs
 * @param path the package to import
 * @param on_changed invoked once the profile list has changed
 * @param on_opened invoked when a new window was launched
 */
void GF_UI_EXPORT
ImportProfileInteractive(QWidget *parent, const QString &path,
                         const std::function<void()> &on_changed = {},
                         const std::function<void()> &on_opened = {});

/**
 * @brief What to do with a profile file the user pointed at.
 */
enum class ProfilePackageAction {
  kOPEN,    ///< run it temporarily, leaving it a file
  kIMPORT,  ///< copy it into a profile kept on this computer
  kCANCEL,  ///< neither
};

/**
 * @brief Ask which of the two a profile file is meant for.
 *
 * The Profiles menu keeps "Open" and "Import" apart because they are routinely
 * read as the same act. A double-click cannot say which one is meant, so it is
 * asked rather than guessed — the two leave the computer in different states,
 * and the wrong guess is not undone by closing a window.
 *
 * @param parent parent for the dialog
 * @param path the package the user pointed at, shown in the question
 * @return what the user chose
 */
auto GF_UI_EXPORT AskProfilePackageAction(QWidget *parent, const QString &path)
    -> ProfilePackageAction;

/**
 * @brief The profiles most recently opened, newest first.
 *
 * Sorted on the `last_opened` each profile's own marker carries, which every
 * process stamps for itself — so a profile opened from a shell or by another
 * window appears here just the same.
 *
 * Excludes the profile this window is running, which cannot be opened again,
 * and any that has never been opened at all.
 *
 * @param limit how many to return at most; 0 for all
 * @return the entries, most recent first
 */
auto GF_UI_EXPORT RecentProfiles(int limit = 0) -> QList<ProfileRegistryEntry>;

}  // namespace GpgFrontend::UI
