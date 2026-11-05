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

#include "GFShellExport.h"

namespace GpgFrontend::Shell {

/**
 * @brief What can be done to one key generation profile in the list.
 */
// NOLINTNEXTLINE(performance-enum-size)
enum class KeyGenProfileAction {
  kEdit,          ///< open it in the editor
  kDelete,        ///< take it out of the list
  kToggleHidden,  ///< hide it from the wizard, or show it again
  kMakeDefault,   ///< make it the one Quick Generate and the wizard start from
};

/**
 * @brief Why an action is refused; kNone when it is allowed.
 */
// NOLINTNEXTLINE(performance-enum-size)
enum class KeyGenProfileRefusal {
  kNone,
  kIsDefault,       ///< deleting or hiding the default leaves nothing to start
  kAlreadyDefault,  ///< it is the default already
  kNotOffered,      ///< hidden or not makeable here, so the wizard lacks it
  kUnavailable,     ///< the current engine cannot make it, so cannot edit it
};

/**
 * @brief The facts about a profile the actions depend on.
 */
struct GF_SHELL_EXPORT KeyGenProfileState {
  /// It is the profile Quick Generate and the wizard start from, as
  /// PickDefaultKeyGenProfile() picks it among the offered ones.
  bool is_default = false;
  bool hidden = false;    ///< the user hid it from the wizard
  bool available = true;  ///< the current engine can make it
};

/**
 * @brief Whether an action may be done to a profile, and if not, why.
 *
 * The one rule both interfaces apply: the default profile can be neither
 * deleted nor hidden, since the wizard and Quick Generate would then point at
 * nothing; choosing another default first frees it.
 *
 * @param state the profile's facts
 * @param action the action
 * @return kNone when allowed, else the reason
 */
auto GF_SHELL_EXPORT CheckKeyGenProfileAction(const KeyGenProfileState& state,
                                              KeyGenProfileAction action)
    -> KeyGenProfileRefusal;

/**
 * @brief The reason in a sentence, for a tooltip or a notice.
 *
 * @param refusal the reason
 * @return the sentence, empty for kNone
 */
auto GF_SHELL_EXPORT DescribeKeyGenProfileRefusal(KeyGenProfileRefusal refusal)
    -> QString;

}  // namespace GpgFrontend::Shell
