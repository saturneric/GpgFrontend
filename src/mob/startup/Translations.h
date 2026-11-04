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

#include <QLocale>

namespace GpgFrontend::Mobile {

/**
 * @brief Translate the application into @p locale: Qt's own strings, the
 * core's (the application's catalog, which the desktop shares) and the
 * phone application's own (GpgFrontendMobile), over its English one, which
 * spells out plurals for a language that has no catalog.
 *
 * Called once the profile is open, since the language is one of its
 * settings. Installing posts QEvent::LanguageChange, on which the QML engine
 * translates what is on screen again. A catalog a locale has none of is
 * skipped, and its strings stay as written.
 */
void InstallTranslations(const QLocale& locale);

}  // namespace GpgFrontend::Mobile
