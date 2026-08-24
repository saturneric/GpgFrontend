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

#include "ui/function/AppearanceFont.h"

namespace GpgFrontend::UI {

auto IsFixedPitchFontFamily(const QString& family) -> bool {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return QFontDatabase::isFixedPitch(family);
#else
  QFontDatabase font_database;
  return font_database.isFixedPitch(family);
#endif
}

auto ResolveAppearanceFont(const QString& family, int point_size) -> QFont {
  // Cached: this is asked again for every text surface each time appearance
  // settings are applied, and enumerating the installed families is not cheap.
  static const auto kFamilies = []() -> QStringList {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QFontDatabase::families();
#else
    QFontDatabase font_database;
    return font_database.families();
#endif
  }();

  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setStyleHint(QFont::Monospace);
  font.setFixedPitch(true);

  const auto installed =
      !family.isEmpty() &&
      std::any_of(kFamilies.cbegin(), kFamilies.cend(),
                  [&family](const QString& item) {
                    return item.compare(family, Qt::CaseInsensitive) == 0;
                  });
  if (installed) {
    // The family the user picked wins over the fallback, and so do its own
    // metrics: asking for a proportional family while still requesting fixed
    // pitch lets Qt substitute something else entirely, which is how a chosen
    // script-specific font ends up rendered in the wrong one.
    const auto fixed_pitch = IsFixedPitchFontFamily(family);
    font.setFamily(family);
    font.setStyleHint(fixed_pitch ? QFont::Monospace : QFont::AnyStyle);
    font.setFixedPitch(fixed_pitch);
  }

  font.setPointSize(point_size);
  return font;
}

}  // namespace GpgFrontend::UI
