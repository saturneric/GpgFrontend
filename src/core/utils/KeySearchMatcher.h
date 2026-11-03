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
 * @brief Reduce a fingerprint or a fingerprint-shaped query to its bare hex.
 *
 * Fingerprints get copied around in whatever grouping the tool that printed
 * them chose — spaced in blocks of four, colon-separated, upper or lower case —
 * while gpg reports one unbroken string. Stripping the separators from both
 * sides is what lets a pasted fingerprint match at all.
 *
 * @param text fingerprint or query
 * @return lower-cased text with spaces, colons and dashes removed
 */
auto GF_UI_EXPORT KeySearchNormalize(const QString& text) -> QString;

/**
 * @brief Decide whether one key matches the search box.
 *
 * @param fields display text of every searchable column, plus every user ID
 * @param fingerprint the key's fingerprint, which no column carries
 * @param keyword raw contents of the search box
 * @return true if the key should stay visible
 */
auto GF_UI_EXPORT KeySearchMatches(const QStringList& fields,
                                   const QString& fingerprint,
                                   const QString& keyword) -> bool;

}  // namespace GpgFrontend::UI
