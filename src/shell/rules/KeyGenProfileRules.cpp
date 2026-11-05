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

#include "shell/rules/KeyGenProfileRules.h"

namespace GpgFrontend::Shell {

auto CheckKeyGenProfileAction(const KeyGenProfileState& state,
                              KeyGenProfileAction action)
    -> KeyGenProfileRefusal {
  switch (action) {
    case KeyGenProfileAction::kEdit:
      // Edited with the current engine's algorithms, which this one lacks;
      // everything else still works on it.
      return state.available ? KeyGenProfileRefusal::kNone
                             : KeyGenProfileRefusal::kUnavailable;
    case KeyGenProfileAction::kDelete:
      return state.is_default ? KeyGenProfileRefusal::kIsDefault
                              : KeyGenProfileRefusal::kNone;
    case KeyGenProfileAction::kToggleHidden:
      // Showing one again is always fine; hiding the default is not.
      return state.is_default && !state.hidden
                 ? KeyGenProfileRefusal::kIsDefault
                 : KeyGenProfileRefusal::kNone;
    case KeyGenProfileAction::kMakeDefault:
      if (state.is_default) return KeyGenProfileRefusal::kAlreadyDefault;
      if (state.hidden || !state.available) {
        return KeyGenProfileRefusal::kNotOffered;
      }
      return KeyGenProfileRefusal::kNone;
  }
  return KeyGenProfileRefusal::kNone;
}

auto DescribeKeyGenProfileRefusal(KeyGenProfileRefusal refusal) -> QString {
  // The desktop's settings page said these first; its context keeps their
  // translations.
  switch (refusal) {
    case KeyGenProfileRefusal::kNone:
      return {};
    case KeyGenProfileRefusal::kIsDefault:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyGenerationTab",
          "This is the default profile. Choose another default first.");
    case KeyGenProfileRefusal::kAlreadyDefault:
      return QCoreApplication::translate("GpgFrontend::UI::KeyGenerationTab",
                                         "This is already the default.");
    case KeyGenProfileRefusal::kNotOffered:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyGenerationTab",
          "Only a profile the wizard offers can be the default.");
    case KeyGenProfileRefusal::kUnavailable:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyGenerationTab",
          "The current key database cannot make this profile, so it cannot "
          "be edited here.");
  }
  return {};
}

}  // namespace GpgFrontend::Shell
