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

#include "ui/widgets/KeyTableEmptyState.h"

namespace GpgFrontend::UI {

auto ClassifyKeyTableEmptyState(int visible_rows, int source_rows,
                                bool has_search_keyword,
                                bool has_category_filter)
    -> KeyTableEmptyReason {
  if (visible_rows > 0) return KeyTableEmptyReason::kNotEmpty;

  // An empty keyring outranks everything else: telling someone their search
  // found nothing is useless when they have no keys to search in the first
  // place, and generating one is the only thing that helps.
  if (source_rows <= 0) return KeyTableEmptyReason::kKeyringEmpty;

  if (has_search_keyword) return KeyTableEmptyReason::kNoSearchMatch;
  if (has_category_filter) return KeyTableEmptyReason::kCategoryEmpty;

  return KeyTableEmptyReason::kFilteredOut;
}

auto DescribeKeyTableEmptyState(KeyTableEmptyReason reason,
                                const QString& keyword) -> QString {
  switch (reason) {
    case KeyTableEmptyReason::kNotEmpty:
      return {};

    case KeyTableEmptyReason::kKeyringEmpty:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyTableEmptyState",
          "No keys yet.\n\nUse Key ▸ Generate Key to make one, or "
          "Key ▸ Import Key to bring in one you already have.");

    case KeyTableEmptyReason::kNoSearchMatch:
      return QCoreApplication::translate(
                 "GpgFrontend::UI::KeyTableEmptyState",
                 "No key matches \"%1\".\n\nClear the search to see every key "
                 "again.")
          .arg(keyword);

    case KeyTableEmptyReason::kCategoryEmpty:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyTableEmptyState",
          "This category has no keys yet.\n\nRight-click a key in another tab "
          "and use Category to file it here.");

    case KeyTableEmptyReason::kFilteredOut:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyTableEmptyState",
          "No key in this keyring belongs in this tab.");
  }

  return {};
}

}  // namespace GpgFrontend::UI
