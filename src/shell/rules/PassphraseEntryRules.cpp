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

#include "shell/rules/PassphraseEntryRules.h"

namespace GpgFrontend::Shell {

auto CheckPassphraseEntry(const QString& passphrase,
                          const QString& confirmation, bool confirm)
    -> PassphraseEntryProblem {
  if (!confirm) return PassphraseEntryProblem::kNone;
  if (passphrase.isEmpty()) return PassphraseEntryProblem::kEmpty;
  if (passphrase != confirmation) return PassphraseEntryProblem::kMismatch;
  return PassphraseEntryProblem::kNone;
}

auto DescribePassphraseEntryProblem(PassphraseEntryProblem problem)
    -> PassphraseEntryProblemText {
  // The desktop's passphrase dialog said these first; its context keeps
  // their translations.
  switch (problem) {
    case PassphraseEntryProblem::kNone:
      return {};
    case PassphraseEntryProblem::kEmpty:
      return {
          QCoreApplication::translate("GpgFrontend::UI::PassphraseDialog",
                                      "Empty Passphrase"),
          QCoreApplication::translate(
              "GpgFrontend::UI::PassphraseDialog",
              "Passphrase cannot be empty. Please enter a valid passphrase."),
      };
    case PassphraseEntryProblem::kMismatch:
      return {
          QCoreApplication::translate("GpgFrontend::UI::PassphraseDialog",
                                      "Passphrase Mismatch"),
          QCoreApplication::translate(
              "GpgFrontend::UI::PassphraseDialog",
              "The two passphrases do not match. Please enter them again."),
      };
  }
  return {};
}

}  // namespace GpgFrontend::Shell
