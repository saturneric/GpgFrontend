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

#include <chrono>

/**
 * @file PlatformClipboard.h
 * @brief Copying text that should not stay on the clipboard.
 *
 * A plaintext or a passphrase copied for a moment is cleared again after an
 * expiry -- but only if the clipboard still holds exactly what was copied, so
 * whatever was copied since, here or in another app, is never erased. Where
 * the platform has a notion of sensitive clipboard content (Android 13 and
 * later: no preview, no history), the copy is marked as such.
 *
 * An expiry is checked when the application can run: on a timer while it is
 * in the foreground, when it comes back, and when it quits. A platform that
 * does not let a background application read the clipboard (Android) would
 * not let it check any earlier.
 */

namespace GpgFrontend {

/// Copy @p text plainly: ciphertext, a public key, a fingerprint.
void CopyText(const QString& text);

/**
 * @brief Copy @p text that should not stay: cleared after @p expiry if the
 * clipboard still holds it.
 */
void CopySensitiveText(const QString& text, std::chrono::seconds expiry);

/**
 * @brief Clear a sensitive copy whose time is up, if it is still there.
 *
 * @param now_or_never true at quit: clear it whatever time is left
 */
void ExpireSensitiveCopy(bool now_or_never = false);

/// The platform's way to put sensitive text on the clipboard, or false to
/// fall back to an ordinary copy.
auto SetSensitiveClipboardText(const QString& text) -> bool;

}  // namespace GpgFrontend
