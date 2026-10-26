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
 * @brief What is wrong with the name a user typed for a key category.
 */
enum class KeyCategoryNameProblem {
  kNone,      ///< the name is usable
  kEmpty,     ///< the name is blank
  kTooLong,   ///< the name is longer than kMaxKeyCategoryNameLength
  kDuplicate  ///< another category already has this name
};

constexpr int kMaxKeyCategoryNameLength = 64;
constexpr int kKeyCategoryPresetColorCount = 20;

/**
 * @brief A colour offered as a swatch when creating or editing a category.
 */
struct KeyCategoryPresetColor {
  QColor color;
  QString name;  ///< translated, for tooltips and screen readers
};

/**
 * @brief Check the name a user typed for a key category.
 *
 * Leading and trailing spaces are ignored, and names are compared without
 * regard to case.
 *
 * @param name the typed name
 * @param taken the names of the other categories (not the one being edited)
 * @return the first problem found, or kNone
 */
auto GF_UI_EXPORT ValidateKeyCategoryName(const QString& name,
                                          const QStringList& taken)
    -> KeyCategoryNameProblem;

/**
 * @brief The message to show for a name problem.
 *
 * @param problem as returned by ValidateKeyCategoryName()
 * @return a translated message, or an empty string for kNone
 */
auto GF_UI_EXPORT DescribeKeyCategoryNameProblem(KeyCategoryNameProblem problem)
    -> QString;

/**
 * @brief The swatches offered for a category, in display order.
 */
auto GF_UI_EXPORT KeyCategoryPresetColors()
    -> std::array<KeyCategoryPresetColor, kKeyCategoryPresetColorCount>;

/**
 * @brief The colour to preselect for a new category.
 *
 * The first preset that no existing category (built-in tabs included) uses or
 * comes close to, so a new category stands apart in the rail; once all are
 * taken, the presets are cycled.
 *
 * @param used the colours of the existing categories
 */
auto GF_UI_EXPORT SuggestKeyCategoryColor(const QList<QColor>& used) -> QColor;

}  // namespace GpgFrontend::UI
