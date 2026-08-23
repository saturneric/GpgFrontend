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

#include "ui/widgets/SecretEntryState.h"

namespace GpgFrontend::UI {

auto EvaluateSecretEntry(const SecretEntryInput& input) -> SecretEntryState {
  // A field that has never been touched and one holding a negative length are
  // the same thing to every rule below, so they are made the same thing here
  // rather than guarded against one at a time.
  const auto current = std::max(0, input.current_length);
  const auto secret = std::max(0, input.secret_length);
  const auto confirm = std::max(0, input.confirm_length);

  // Unlocking: one field, and the floor does not apply. Someone entering a
  // secret that already exists cannot choose how long it is, and a file sealed
  // with four characters has to stay openable.
  if (!input.ask_new) {
    if (current == 0) return {false, SecretEntryProblem::kEmpty};
    return {true, SecretEntryProblem::kNone};
  }

  const auto long_enough = secret >= input.min_length;
  const auto confirmed = input.confirm_matches;
  const auto has_current = !input.ask_current || current > 0;

  const auto acceptable = long_enough && confirmed && has_current;

  // Nothing typed at all is not yet a mistake, whichever field is still empty.
  if (secret == 0 && confirm == 0 && (!input.ask_current || current == 0)) {
    return {acceptable, SecretEntryProblem::kEmpty};
  }

  // Named before the mismatch on purpose: it is the deeper problem, and fixing
  // it means retyping the confirmation anyway.
  if (!long_enough && secret > 0) {
    return {acceptable, SecretEntryProblem::kTooShort};
  }

  // Withheld until there is a second entry worth comparing.
  if (long_enough && confirm > 0 && !confirmed) {
    return {acceptable, SecretEntryProblem::kMismatch};
  }

  if (!has_current) return {acceptable, SecretEntryProblem::kCurrentMissing};

  return {acceptable, SecretEntryProblem::kNone};
}

}  // namespace GpgFrontend::UI
