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

#include "screenshot/ScreenshotOptions.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QTimeZone>

namespace GpgFrontend::Screenshot {

auto ParseSize(const QString& text) -> std::optional<QSize> {
  static const QRegularExpression kRe(u"^(\\d{1,5})x(\\d{1,5})$"_s);
  const auto m = kRe.match(text.trimmed());
  if (!m.hasMatch()) return std::nullopt;
  const auto w = m.captured(1).toInt();
  const auto h = m.captured(2).toInt();
  if (w <= 0 || h <= 0) return std::nullopt;
  return QSize(w, h);
}

auto ParseTheme(const QString& text) -> std::optional<Theme> {
  if (text == "light"_L1) return Theme::kLIGHT;
  if (text == "dark"_L1) return Theme::kDARK;
  return std::nullopt;
}

auto ThemeName(Theme theme) -> QString {
  return theme == Theme::kDARK ? u"dark"_s : u"light"_s;
}

auto ParseClock(const QString& text) -> std::optional<QDateTime> {
  static const QRegularExpression kRe(
      u"^(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})Z$"_s);
  const auto m = kRe.match(text);
  if (!m.hasMatch()) return std::nullopt;

  const QDate date(m.captured(1).toInt(), m.captured(2).toInt(),
                   m.captured(3).toInt());
  const QTime time(m.captured(4).toInt(), m.captured(5).toInt(),
                   m.captured(6).toInt());
  if (!date.isValid() || !time.isValid()) return std::nullopt;
  return QDateTime(date, time, QTimeZone::UTC);
}

auto ParseFontFamilies(const QString& text) -> QStringList {
  QStringList families;
  for (const auto& part : text.split(QLatin1Char(','))) {
    const auto family = part.trimmed();
    if (!family.isEmpty()) families.append(family);
  }
  return families;
}

auto GpgFakedSystemTime(const QDateTime& clock) -> QString {
  return clock.toUTC().toString(u"yyyyMMdd'T'HHmmss"_s) + QLatin1Char('!');
}

auto GeometryJsonPath(const QString& png_path) -> QString {
  const QFileInfo info(png_path);
  return info.dir().filePath(info.completeBaseName() + u".json"_s);
}

}  // namespace GpgFrontend::Screenshot
