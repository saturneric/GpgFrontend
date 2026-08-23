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

#include "core/profile/Profile.h"
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief One reading on the About dialog's Status tab.
 *
 * A form row carries a single string, so a value that needs a sentence to be
 * honest used to be written as one: a value, an em dash, and the explanation
 * trailing after it. That turns the value column into a paragraph and makes it
 * unscannable. Splitting the two apart is what lets the value stay a value and
 * the sentence sit under it.
 *
 * Kept apart from the widgets that render it, for the same reason
 * StatusIndicatorInfo is: the wording is the part worth asserting, and a test
 * cannot build a widget on the thread it runs on.
 */
struct AboutStatusValue {
  QString value;          ///< the thing the user reads at a glance
  QString detail;         ///< the sentence under it, empty when none is needed
  bool degraded = false;  ///< the state is a fallback, not what was asked for
};

/**
 * @brief Describe where a packaged session keeps what it is working on.
 *
 * Opening a package tries for storage this machine does not leave readable and
 * quietly settles for less when there is nowhere. Which of those happened is
 * the user's to know: a silent downgrade is the one thing this whole mechanism
 * must not be, so the plain-folder outcome is the only one marked degraded.
 *
 * @param is_volatile whether the accessor keeps its contents in memory
 * @param encrypted_at_rest whether what it does write is encrypted
 * @return value, detail and whether this is a fallback
 */
auto GF_UI_EXPORT DescribeSessionStorage(bool is_volatile,
                                         bool encrypted_at_rest)
    -> AboutStatusValue;

/**
 * @brief Describe which keyring the window in front of the user is showing.
 *
 * Not the credential store, which the Application Status card reports on its
 * own. For a session opened from a package the distinction matters more than
 * anywhere else: a package exported from a profile whose keys lived outside it
 * carries no keys at all, so the window is showing this computer's, not the
 * ones the sender meant to hand over. That earns a sentence of its own rather
 * than being left to be inferred from two words.
 *
 * @param self_contained whether the profile keeps its keys to itself
 * @param kind what sort of profile the session resolved to
 * @return value and detail for the keys row
 */
auto GF_UI_EXPORT DescribeKeySource(bool self_contained, ProfileKind kind)
    -> AboutStatusValue;

/**
 * @brief Decide whether a reading's sentence belongs on screen or on hover.
 *
 * Every sentence written under a value was a line the page had to grow by, and
 * three of them together pushed the Profile card off the bottom of the dialog.
 * Elaboration does not need to cost that: it can wait behind a hover and still
 * be there when someone goes looking.
 *
 * A fallback is the exception. The whole point of marking a state degraded is
 * that the user did not ask for it and would not otherwise notice, so its
 * reason stays where it cannot be missed. Nothing here is ever dropped: the
 * clipboard summary carries every sentence either way.
 *
 * Deliberately a bool rather than an enum: a typed enum ahead of a Q_OBJECT
 * class drops the namespace from its tr() context in this project, and this
 * header is included by a file full of them.
 *
 * @param detail the sentence under consideration, possibly empty
 * @param degraded whether the reading it explains is a fallback
 * @return true to render it under the value, false to leave it to a tooltip
 */
auto GF_UI_EXPORT ShowsDetailInline(const QString& detail, bool degraded)
    -> bool;

}  // namespace GpgFrontend::UI
