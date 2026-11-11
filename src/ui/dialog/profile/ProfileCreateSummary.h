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

namespace GpgFrontend::UI {

/**
 * @brief Where the new profile wizard is.
 */
enum class ProfileCreateStep {
  kName,
  kKeys,      ///< an empty keyring of its own, or the system one
  kSettings,  ///< which of this window's settings it starts with
};

/// How many steps the wizard has, for "Step 2 of 3".
constexpr int kProfileCreateStepCount = 3;

/**
 * @brief What a step is called in the header above it.
 *
 * @param step the step
 * @return translated title
 */
auto GF_UI_EXPORT ProfileCreateStepTitle(ProfileCreateStep step) -> QString;

/**
 * @brief Whether a name can be given to a profile: the shell's rule, as the
 * phone has it (not blank, not too long, not one of @p taken whatever its
 * case).
 *
 * @param name what was typed
 * @param taken the names the profiles already have
 * @return true when it may name a new profile
 */
auto GF_UI_EXPORT IsProfileNameAcceptable(const QString& name,
                                           const QStringList& taken = {})
    -> bool;

/**
 * @brief Why @p name cannot name a new profile, said under the field; empty
 * when it can, and while nothing is typed (the button alone waits for it).
 */
auto GF_UI_EXPORT DescribeProfileNameProblem(const QString& name,
                                             const QStringList& taken)
    -> QString;

/**
 * @brief Whether the primary button may leave a step.
 *
 * The name is the only thing that can hold the wizard up. The keyring always
 * has an answer selected, and starting with none of this window's settings is
 * as valid as starting with all of them.
 *
 * @param step the step being left
 * @param name what was typed as the name
 * @param taken the names the profiles already have
 * @return true when the step may be left forwards
 */
auto GF_UI_EXPORT CanAdvanceProfileCreate(ProfileCreateStep step,
                                          const QString& name,
                                          const QStringList& taken = {})
    -> bool;

/**
 * @brief How one answer to the keyring question is offered.
 */
struct ProfileKeyringChoiceText {
  QString title;   ///< the same name the key database settings use for it
  QString detail;  ///< what choosing it means, in one sentence
};

/**
 * @brief Describe where a new profile's built-in key database would live.
 *
 * The question is not "empty or system keyring" any more: every profile has a
 * built-in key database, and the choice is only where it sits. Inside the
 * profile it is `@profile/db` and travels in a profile file. Outside, it is
 * whatever this computer provides: the GnuPG home directory when GnuPG is
 * there, otherwise the rPGP database every non-isolated profile shares.
 *
 * Named with KeyDatabaseKindDisplayName(), so this wizard and the Key
 * Databases settings call the same database the same thing.
 *
 * @param self_contained whether the database lives inside the profile
 * @param gnupg_available whether this computer's database is a GnuPG home
 * @return the tile's title and sentence
 */
auto GF_UI_EXPORT DescribeProfileKeyringChoice(bool self_contained,
                                               bool gnupg_available)
    -> ProfileKeyringChoiceText;

}  // namespace GpgFrontend::UI
