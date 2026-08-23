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
 * @brief Everything the rule is allowed to look at.
 *
 * Lengths and a verdict, never the secrets themselves. Two reasons, and both
 * matter: the rule becomes arithmetic that can be tested without a widget or a
 * single byte of key material, and nothing copies a plaintext secret into an
 * ordinary-heap struct on every keystroke.
 */
struct SecretEntryInput {
  bool ask_current = false;  ///< an existing secret is being asked for
  bool ask_new = true;       ///< a new secret and its confirmation

  int min_length = 8;  ///< the floor; only ever applies to a new secret

  int current_length = 0;
  int secret_length = 0;
  int confirm_length = 0;
  bool confirm_matches = false;  ///< compared at the field, never here
};

/**
 * @brief Why the entry is not acceptable yet, when it is worth saying so.
 *
 * kEmpty is separate from kNone because they call for different things on
 * screen: neither is acceptable, but an untouched form is not a mistake and
 * must not be scolded — it keeps its dimmed hint.
 */
enum class SecretEntryProblem {
  kNone,            ///< nothing to report; the idle hint belongs in the row
  kEmpty,           ///< nothing typed yet: not acceptable, but not yet wrong
  kCurrentMissing,  ///< the existing secret has not been given
  kTooShort,        ///< something is typed, and it is under the floor
  kMismatch,        ///< the confirmation is non-empty and differs
};

/**
 * @brief Whether the entry can be accepted, and what to say if not.
 */
struct SecretEntryState {
  bool acceptable = false;
  SecretEntryProblem problem = SecretEntryProblem::kEmpty;
};

/**
 * @brief Decide whether what has been typed satisfies the prompt.
 *
 * Two restraints are what keep this from feeling fussy, and both are
 * deliberate. An untouched form reports kEmpty rather than kTooShort, because
 * having typed nothing yet is not an error. A mismatch is withheld until the
 * confirmation field has something in it, because telling someone their two
 * entries differ before they have finished the second one is noise.
 *
 * Where both a short secret and a mismatch apply, the short one is reported:
 * it is the deeper problem, and fixing it usually invalidates the other anyway.
 *
 * The floor governs *choosing* a secret and never entering one that already
 * exists — a file sealed with four characters must stay openable.
 *
 * @param input what has been typed, as lengths
 * @return whether it can be accepted, and the problem worth naming
 */
auto GF_UI_EXPORT EvaluateSecretEntry(const SecretEntryInput& input)
    -> SecretEntryState;

}  // namespace GpgFrontend::UI
