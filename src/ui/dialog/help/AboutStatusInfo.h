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
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/typedef/GFTypedef.h"
#include "ui/widgets/MetaListPanel.h"

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
 * @brief The profile's name and shape, as one row or two.
 *
 * A root profile has no name of its own, so CurrentProfileDisplayName() answers
 * with its kind -- and the tab then printed that same word twice, once as the
 * name and once as the type. Where the two would say the same thing there is
 * one row, and what the second would have said becomes its sentence.
 *
 * @param kind what sort of profile the session resolved to
 * @param display_name what to call it, from CurrentProfileDisplayName()
 * @param transient whether its storage is deleted when the window closes
 * @return one row for a root profile, two for a profile with a name
 */
auto GF_UI_EXPORT BuildProfileIdentityRows(ProfileKind kind,
                                           const QString& display_name,
                                           bool transient)
    -> QVector<MetaListRow>;

/**
 * @brief Describe what protects the application key, and why it is that.
 *
 * The value alone cannot say why the keychain is not in use. Anything that can
 * leave this computer has the choice made for it, the settings page greys it
 * out, and nothing anywhere says what the user is looking at.
 *
 * @param protection the protection in effect, from AppKeyProtectionFromApp()
 * @param allows_keychain whether this profile may use the credential store
 * @return value and detail for the protection row
 */
auto GF_UI_EXPORT DescribeAppKeyProtection(AppKeyProtection protection,
                                           bool allows_keychain)
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
