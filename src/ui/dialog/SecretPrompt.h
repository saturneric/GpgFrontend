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

#include "ui/widgets/MetaListPanel.h"

namespace GpgFrontend::UI {

/**
 * @brief What a secret prompt asks for, and therefore what shape it has.
 *
 * Structure only: how many fields exist and which of them have to agree. What
 * the prompt *says* is a separate axis, carried by SecretPromptSubject, so that
 * asking for a profile file's passphrase does not have to become a fourth mode.
 */
enum class SecretPromptMode {
  kSET,     ///< choose a new secret: new field, confirmation, strength meter
  kUNLOCK,  ///< enter an existing secret: one field, retryable
  kCHANGE,  ///< the current secret, then a new one with confirmation
};

/**
 * @brief What the secret protects, and therefore what the prompt calls it.
 *
 * The application PIN and a profile file's passphrase are different secrets
 * protecting different things. A prompt that borrowed the other's word would
 * invite someone to type one where the other belongs, so the vocabulary is
 * chosen here rather than left to each call site.
 */
enum class SecretPromptSubject {
  kAppKey,          ///< the application key file kept on this computer
  kProfilePackage,  ///< a profile file being opened, imported, or re-sealed
};

/// Shortest secret accepted when choosing one. The strength meter stays advice,
/// but a floor keeps a two-character secret from being offered as real
/// protection. A caller re-sealing a file somebody else already sealed lowers
/// it deliberately; see DefaultSecretPromptTexts().
inline constexpr int kMinSecretLength = 8;

/// Comfortable touch-sized height for the entry fields; the platform default is
/// cramped for something the eye has to land on precisely.
inline constexpr int kSecretFieldHeight = 34;

/**
 * @brief Everything a secret prompt says, with nothing about how it behaves.
 *
 * Held apart from the dialog so the wording for a new kind of secret is a table
 * entry rather than another branch inside a constructor, and — because the
 * defaults come from a pure function — so the strings the existing prompts show
 * can be asserted rather than reviewed.
 */
struct GF_UI_EXPORT SecretPromptTexts {
  QString
      window_title;  ///< the window title, and the heading by the lock badge
  QString subtitle;  ///< one wrapped line saying what the secret is for

  /// What this prompt is about, as a list: the file's name, where it is, and
  /// whatever it claims about itself. Empty for the application key, which is
  /// not about a file at all. Built by a caller from BuildProfilePackageRows()
  /// rather than described here, so that every dialog naming a package names it
  /// the same way.
  QVector<MetaListRow> context_rows;

  QString current_label;     ///< label for the existing-secret field
  QString new_label;         ///< label for the new-secret field
  QString confirm_label;     ///< label for the confirmation field
  QString reveal_label;      ///< the show-the-secret checkbox
  QString strength_caption;  ///< caption beside the strength bar

  QString accept_button;  ///< names what accepting does, never just "OK"
  QString cancel_button;  ///< empty keeps the platform's own wording

  QString hint;     ///< dimmed guidance for the message row while idle
  QString warning;  ///< the irreversibility line; empty hides the warning row

  // What to say for each problem EvaluateSecretEntry() can report. Kept beside
  // the rest of the wording rather than inside the rule, so the rule stays a
  // pure verdict and the words stay translatable per subject. An empty string
  // means "say nothing and leave the hint up", which is what a mode that gates
  // silently on a field wants.
  QString too_short_message;  ///< carries %1 for the floor
  QString mismatch_message;
  QString current_missing_message;

  int min_length = kMinSecretLength;
};

/**
 * @brief The wording for one kind of prompt.
 *
 * Pure, so what the three application-key prompts show can be pinned by a test
 * instead of by reading the dialog's constructor.
 *
 * @param subject what the secret protects
 * @param mode what is being asked for
 * @return the strings to render
 */
auto GF_UI_EXPORT DefaultSecretPromptTexts(SecretPromptSubject subject,
                                           SecretPromptMode mode)
    -> SecretPromptTexts;

}  // namespace GpgFrontend::UI
