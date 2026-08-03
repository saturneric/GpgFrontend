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
 * @brief Why a key table has no rows to show.
 *
 * All four look identical on screen — a blank table — but they call for
 * completely different next steps, and a user cannot tell them apart without
 * being told.
 */
enum class KeyTableEmptyReason {
  kNotEmpty,       ///< there are rows; no message
  kKeyringEmpty,   ///< no keys at all: generate or import one
  kNoSearchMatch,  ///< the search excluded everything
  kCategoryEmpty,  ///< a custom category nothing has been filed under yet
  kFilteredOut,    ///< a built-in tab no key qualifies for
};

/**
 * @brief Work out why a table is empty.
 *
 * @param visible_rows rows after filtering
 * @param source_rows rows in the whole keyring, which is what separates "you
 *        have no keys" from "this tab excludes them all"
 * @param has_search_keyword whether the search box holds anything
 * @param has_category_filter whether this tab restricts to a category
 * @return the reason, or kNotEmpty when there is nothing to explain
 */
auto GF_UI_EXPORT ClassifyKeyTableEmptyState(int visible_rows, int source_rows,
                                             bool has_search_keyword,
                                             bool has_category_filter)
    -> KeyTableEmptyReason;

/**
 * @brief The message to draw over an empty table.
 *
 * @param reason as returned by ClassifyKeyTableEmptyState()
 * @param keyword current search term, interpolated into the no-match message
 * @return a translated message, or an empty string for kNotEmpty
 */
auto GF_UI_EXPORT DescribeKeyTableEmptyState(KeyTableEmptyReason reason,
                                             const QString& keyword) -> QString;

}  // namespace GpgFrontend::UI
