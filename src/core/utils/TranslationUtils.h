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

#include "GFCoreExport.h"

namespace GpgFrontend {

/// A translation catalog: `<directory>/<name><separator><locale>.qm`.
struct GF_CORE_EXPORT TranslationCatalog {
  QString name;
  QString separator;
  QString directory;
  /// A language of its own, for a fallback beneath the one asked for (an
  /// English catalog that spells out plurals); empty: the one asked for.
  QString locale;
};

/**
 * @brief Translate the application into @p locale: Qt's own strings, the
 * application's catalog (the core's and the desktop's), then @p extra.
 *
 * What an earlier call installed is taken out first, so this runs again on a
 * language switch. Installing posts QEvent::LanguageChange. A catalog a
 * locale has none of is skipped, and its strings stay as written. Modules'
 * catalogs are not among these: they are their own.
 *
 * @param locale the language
 * @param extra catalogs of the running application's own, consulted first
 */
void GF_CORE_EXPORT InstallTranslations(
    const QLocale &locale, const QList<TranslationCatalog> &extra = {});

}  // namespace GpgFrontend
