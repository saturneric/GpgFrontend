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

#include "ui/model/SortKeyCompare.h"

namespace GpgFrontend::UI {

namespace {

auto IsNumeric(const QVariant& v) -> bool {
  switch (v.typeId()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
    case QMetaType::Float:
      return true;
    default:
      return false;
  }
}

template <typename T>
auto Spaceship(const T& a, const T& b) -> int {
  if (a < b) return -1;
  if (b < a) return 1;
  return 0;
}

}  // namespace

auto CompareSortKeys(const QVariant& a, const QVariant& b) -> int {
  // An invalid value is "this column had nothing to say about this row", which
  // is not the same as "empty" — it belongs after every row that did.
  if (!a.isValid() || !b.isValid()) {
    if (a.isValid()) return -1;
    if (b.isValid()) return 1;
    return 0;
  }

  if (IsNumeric(a) && IsNumeric(b)) {
    return Spaceship(a.toDouble(), b.toDouble());
  }

  if (a.typeId() == QMetaType::QDateTime &&
      b.typeId() == QMetaType::QDateTime) {
    return Spaceship(a.toDateTime(), b.toDateTime());
  }

  if (a.typeId() == QMetaType::QDate && b.typeId() == QMetaType::QDate) {
    return Spaceship(a.toDate(), b.toDate());
  }

  if (a.typeId() == QMetaType::Bool && b.typeId() == QMetaType::Bool) {
    return Spaceship(static_cast<int>(a.toBool()),
                     static_cast<int>(b.toBool()));
  }

  // Names and comments are the common case here, and they are read by a human
  // in their own language, so "Ä" has to sort next to "A" rather than after
  // "Z" the way a raw code-point comparison would put it.
  return QString::localeAwareCompare(a.toString(), b.toString());
}

}  // namespace GpgFrontend::UI
