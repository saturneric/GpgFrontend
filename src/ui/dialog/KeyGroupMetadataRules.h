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
 * @brief What is wrong with the metadata a user typed for a key group.
 *
 * Kept out of the dialogs so the creation form and the edit form cannot drift
 * apart on what counts as a valid group.
 */
enum class KeyGroupMetadataProblem {
  kNone,           ///< the metadata is usable
  kNameTooShort,   ///< the name is blank or shorter than the minimum
  kEmailMalformed  ///< an email was given but does not look like one
};

/**
 * @brief Check the metadata a user typed for a key group.
 *
 * Leading and trailing spaces do not count towards the name length, and an
 * empty email is allowed.
 *
 * @param name display name
 * @param email email address, may be empty
 * @return the first problem found, or kNone
 */
auto GF_UI_EXPORT ValidateKeyGroupMetadata(const QString& name,
                                           const QString& email)
    -> KeyGroupMetadataProblem;

/**
 * @brief The message to show for a metadata problem.
 *
 * @param problem as returned by ValidateKeyGroupMetadata()
 * @return a translated message, or an empty string for kNone
 */
auto GF_UI_EXPORT
DescribeKeyGroupMetadataProblem(KeyGroupMetadataProblem problem) -> QString;

/**
 * @brief Summarise what a key group holds.
 *
 * @param direct number of direct members that are ordinary keys
 * @param nested number of direct members that are key groups themselves
 * @param missing number of member ids that no longer resolve to a key
 * @return a translated one-line summary
 */
auto GF_UI_EXPORT DescribeKeyGroupMembership(int direct, int nested,
                                             int missing) -> QString;

/**
 * @brief Say what a new key group is about to be created from.
 *
 * The keys were checked in another window and are no longer on screen by the
 * time the creation form opens, so the count is worth restating.
 *
 * @param keys number of keys the group will start with
 * @return a translated one-line summary
 */
auto GF_UI_EXPORT DescribeKeyGroupCreation(int keys) -> QString;

/**
 * @brief The confirmation text for deleting a key group.
 *
 * Naming the groups that will lose this one matters: quietly shrinking an
 * unrelated group's recipient list is the kind of thing that is discovered
 * after sending the wrong email.
 *
 * @param name display name of the group being deleted
 * @param parent_names display names of the groups holding it as a member
 * @return a translated confirmation message, meant to be shown as plain text
 */
auto GF_UI_EXPORT DescribeKeyGroupDeletion(const QString& name,
                                           const QStringList& parent_names)
    -> QString;

}  // namespace GpgFrontend::UI
