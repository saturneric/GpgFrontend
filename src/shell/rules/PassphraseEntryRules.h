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
 * @brief What stops a passphrase prompt's answer from being sent.
 */
// NOLINTNEXTLINE(performance-enum-size)
enum class PassphraseEntryProblem {
  kNone,
  kEmpty,     ///< a new passphrase was asked for and none was typed
  kMismatch,  ///< the two entries of a new passphrase differ
};

/**
 * @brief Whether the answer to a passphrase prompt may be sent.
 *
 * Only a new passphrase, typed twice, is checked: an empty answer to an
 * ordinary prompt is the engine's to judge.
 *
 * @param passphrase what was typed
 * @param confirmation what was typed again; ignored unless @p confirm
 * @param confirm the prompt asks for a new passphrase twice
 * @return kNone when it may be sent
 */
auto GF_SHELL_EXPORT CheckPassphraseEntry(const QString& passphrase,
                                          const QString& confirmation,
                                          bool confirm)
    -> PassphraseEntryProblem;

/**
 * @brief The problem in words: a title and a sentence.
 */
struct GF_SHELL_EXPORT PassphraseEntryProblemText {
  QString title;
  QString text;
};

/**
 * @brief The words for @p problem; empty for kNone.
 */
auto GF_SHELL_EXPORT DescribePassphraseEntryProblem(
    PassphraseEntryProblem problem) -> PassphraseEntryProblemText;

}  // namespace GpgFrontend::Shell
