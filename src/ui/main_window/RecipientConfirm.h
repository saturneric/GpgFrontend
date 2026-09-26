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

#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief Whether encrypting to checked_count recipients should first be
 * confirmed by the user (basic/confirm_multiple_recipients).
 *
 * Every recipient of an OpenPGP message can read the key ids of the others, so
 * a key still checked from an earlier message leaks who else was written to.
 * One recipient is the everyday case and never asks.
 *
 * @param checked_count checked entries (a key group counts as one)
 * @param setting_on the user setting
 * @return bool
 */
auto GF_UI_EXPORT NeedsRecipientConfirmation(qsizetype checked_count,
                                             bool setting_on) -> bool;

/**
 * @brief A key id split into groups of four ("2B5F 4219 7E62 FBCF"), the
 * way people read and compare them aloud.
 *
 * @param id key id
 * @return QString
 */
auto GF_UI_EXPORT FormatKeyIdForDisplay(const QString& id) -> QString;

/**
 * @brief The recipient list shown in the confirmation, as a rich-text table
 * with one row per checked entry: the name in bold, the email in the muted
 * colour and the grouped key id in monospace. A key group shows its name and
 * member count instead of an email and id. Every user-supplied value is
 * HTML-escaped.
 *
 * @param keys the checked entries
 * @param muted colour for the secondary text (the palette's placeholder
 * colour), so the list stays readable in dark themes
 * @return QString
 */
auto GF_UI_EXPORT BuildRecipientConfirmHtml(const GpgAbstractKeyPtrList& keys,
                                            const QColor& muted) -> QString;

}  // namespace GpgFrontend::UI
