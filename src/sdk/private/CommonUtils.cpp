/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "CommonUtils.h"

#include <core/utils/MemoryUtils.h>

auto GFStrDup(const QString& str) -> char* {
  auto utf8_str = str.toUtf8();
  auto* c_str = static_cast<char*>(
      GpgFrontend::SecureMalloc((utf8_str.size() + 1) * sizeof(char)));

  memcpy(c_str, utf8_str.constData(), utf8_str.size());
  c_str[utf8_str.size()] = '\0';
  return c_str;
}

auto GFUnStrDup(char* str) -> QString {
  auto qt_str = QString::fromUtf8(str);
  GpgFrontend::SecureFree(static_cast<void*>(const_cast<char*>(str)));
  return qt_str;
}

auto GFUnStrDup(const char* str) -> QString {
  return GFUnStrDup(const_cast<char*>(str));
}

auto CharArrayToQMap(char** char_array, int size) -> QMap<QString, QString> {
  QMap<QString, QString> map;
  for (int i = 0; i < size; i += 2) {
    QString const key = GFUnStrDup(char_array[i]);
    QString const value = QString::fromUtf8(char_array[i + 1]);
    map.insert(key, value);
  }
  return map;
}

auto QMapToCharArray(const QMap<QString, QString>& map, int& size) -> char** {
  size = map.size() * 2;
  char** char_array = new char*[size];

  int index = 0;
  for (auto it = map.begin(); it != map.end(); ++it) {
    QByteArray const key = it.key().toUtf8();
    QByteArray const value = it.value().toUtf8();

    char_array[index] = new char[key.size() + 1];
    std::strcpy(char_array[index], key.constData());
    index++;

    char_array[index] = new char[value.size() + 1];
    std::strcpy(char_array[index], value.constData());
    index++;
  }

  return char_array;
}
