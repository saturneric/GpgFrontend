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

#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief The interface languages the build ships, in the order they should be
 * offered.
 *
 * SettingsDialog::ListLanguages() hands back a QHash, whose iteration order is
 * not stable, so a combo box filled straight from it reads differently every
 * time it is opened. Sorted here by the native language name.
 *
 * The system-default entry carries the empty key and is left out: it is pinned
 * to the top separately rather than sorted in among the real languages.
 *
 * Pure, so the ordering rule is assertable without a combo box.
 *
 * @param languages locale key -> native name, as ListLanguages() returns it
 * @return the real languages, sorted by native name, empty key excluded
 */
auto GF_UI_EXPORT
SortedLanguageEntries(const QHash<QString, QString>& languages)
    -> QContainer<QPair<QString, QString>>;

/**
 * @brief Fill @p box with the interface languages the build ships.
 *
 * "System Default" is pinned at index 0 and carries an empty key, the rest
 * follow sorted by their native name. Every entry keeps its locale key as item
 * data, so callers read the choice back with currentData() instead of matching
 * on the display text.
 *
 * @param box combo box to fill, cleared first
 * @param current_lang locale key to preselect, empty or unknown selects the
 *                     system default
 */
void GF_UI_EXPORT PopulateLanguageComboBox(QComboBox* box,
                                           const QString& current_lang);

}  // namespace GpgFrontend::UI
