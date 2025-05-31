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

#include <core/function/SecureMemoryAllocator.h>
#include <core/typedef/CoreTypedef.h>

Q_DECLARE_LOGGING_CATEGORY(sdk)

#define LOG_D() qCDebug(sdk)
#define LOG_I() qCInfo(sdk)
#define LOG_W() qCWarning(sdk)
#define LOG_E() qCCritical(sdk)
#define LOG_F() qCFatal(sdk)

struct GFModuleEventParam;

/**
 * @brief
 *
 * @return char*
 */
auto GFStrDup(const QString &) -> char *;

/**
 * @brief
 *
 * @param str
 * @return QString
 */
auto GFUnStrDup(char *str) -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GFUnStrDup(const char *) -> QString;

/**
 * @brief
 *
 * @param char_array
 * @param size
 * @return QMap<QString, QString>
 */
auto CharArrayToQMap(char **char_array, int size) -> QMap<QString, QString>;

/**
 * @brief
 *
 * @param map
 * @param size
 * @return char**
 */
auto QMapToCharArray(const QMap<QString, QString> &map, int &size) -> char **;

/**
 * @brief
 *
 * @param params
 * @return QMap<QString, QString>
 */
auto ConvertEventParamsToMap(GFModuleEventParam *params)
    -> QMap<QString, QString>;

/**
 * @brief
 *
 * @param char_array
 * @param size
 * @return QStringList
 */
auto CharArrayToQStringList(char **char_array, int size) -> QStringList;

/**
 * @brief
 *
 * @param list
 * @param size
 * @return char**
 */
auto QStringListToCharArray(const QStringList &list) -> char **;

template <typename T>
inline auto ArrayToQList(T **pl_components, int size)
    -> GpgFrontend::QContainer<T> {
  if (pl_components == nullptr || size <= 0) {
    return GpgFrontend::QContainer<T>();
  }

  GpgFrontend::QContainer<T> list;
  for (int i = 0; i < size; ++i) {
    list.append(*pl_components[i]);
    GpgFrontend::SecureMemoryAllocator::Deallocate(pl_components[i]);
  }
  GpgFrontend::SecureMemoryAllocator::Deallocate(pl_components);
  return list;
}

template <typename T>
inline auto QListToArray(const GpgFrontend::QContainer<T> &list) -> T ** {
  T **array = static_cast<T **>(
      GpgFrontend::SecureMemoryAllocator::Allocate(list.size() * sizeof(T *)));
  int index = 0;
  for (const T &item : list) {
    auto mem = static_cast<T *>(
        GpgFrontend::SecureMemoryAllocator::Allocate(sizeof(T)));
    array[index] = new (mem) T(item);
    index++;
  }

  return array;
}