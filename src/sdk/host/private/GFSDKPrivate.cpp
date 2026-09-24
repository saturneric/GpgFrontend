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

#include "GFSDKPrivate.h"

#include <core/utils/MemoryUtils.h>

#include <cstring>

#include "GFSDKTypes.h"

Q_LOGGING_CATEGORY(sdk, "sdk")

auto GFStrDup(const QString& str) -> char* {
  auto utf8_str = str.toUtf8();
  auto* c_str = static_cast<char*>(
      GpgFrontend::SMAMalloc((utf8_str.size() + 1) * sizeof(char)));
  if (c_str == nullptr) return nullptr;

  memcpy(c_str, utf8_str.constData(), utf8_str.size());
  c_str[utf8_str.size()] = '\0';
  return c_str;
}

auto GFBytesDup(const QByteArray& bytes, size_t* size) -> char* {
  auto* c_str = static_cast<char*>(
      GpgFrontend::SMAMalloc((bytes.size() + 1) * sizeof(char)));
  if (c_str == nullptr) {
    if (size != nullptr) *size = 0;
    return nullptr;
  }

  memcpy(c_str, bytes.constData(), bytes.size());
  c_str[bytes.size()] = '\0';
  if (size != nullptr) *size = static_cast<size_t>(bytes.size());
  return c_str;
}

auto GFUnStrDup(char* str) -> QString {
  // Guarded, like the core twin in CommonUtils.cpp. Modules do pass a null
  // here -- GnuPGInfoGatheringModule hands nullptr as a default_value -- and
  // an unguarded SMAFree(nullptr) reached report_invalid_free on some paths.
  if (str == nullptr) return {};

  auto qt_str = QString::fromUtf8(str);
  GpgFrontend::SMAFree(static_cast<void*>(str));
  return qt_str;
}

auto GFStrView(const char* str) -> QString {
  return str == nullptr ? QString() : QString::fromUtf8(str);
}

auto GFUnStrDup(const char* str) -> QString {
  return GFUnStrDup(const_cast<char*>(str));
}

auto ConvertEventParamsToMap(GFModuleEventParam* params)
    -> QMap<QString, GpgFrontend::GFBuffer> {
  QMap<QString, GpgFrontend::GFBuffer> param_map;
  GFModuleEventParam* current = params;
  GFModuleEventParam* last;

  // The list is transferred, so every part of it is freed here, each through
  // the allocator that made it: the runtime copies names with DUP and values
  // with SECDUP, because a value may be a secret.
  while (current != nullptr) {
    param_map[GFStrView(current->name)] = GpgFrontend::GFBuffer{current->value};

    last = current;
    current = current->next;
    if (last->name != nullptr) {
      GpgFrontend::SMAFree(const_cast<char*>(last->name));
    }
    if (last->value != nullptr) {
      GpgFrontend::SMASecFree(const_cast<char*>(last->value));
    }
    GpgFrontend::SMAFree(last);
  }

  return param_map;
}
