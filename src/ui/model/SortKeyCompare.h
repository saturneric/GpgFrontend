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
 * @brief Three-way comparison of two key-table sort keys.
 *
 * The key table sorts on values of several types — trust levels and subkey
 * counts as numbers, dates as dates, everything else as text — so the
 * comparison cannot just defer to QVariant. Text is compared locale-aware, so
 * accented names land where a reader of that language expects them rather than
 * after "z". An invalid value means the column had nothing to offer and sorts
 * last, keeping rows without a value out of the way in an ascending sort.
 *
 * Kept free of the proxy model so the ordering rules can be tested without a
 * model, a view, or a gpg context.
 *
 * @param a left value
 * @param b right value
 * @return negative if a sorts first, positive if b does, 0 if equivalent
 */
auto GF_UI_EXPORT CompareSortKeys(const QVariant& a, const QVariant& b) -> int;

}  // namespace GpgFrontend::UI
