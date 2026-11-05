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

#include "files/FileFilterModel.h"

#include "files/FileWorkspace.h"

namespace GpgFrontend::Mobile {

FileFilterModel::FileFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
  for (const auto& signal :
       {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved}) {
    connect(this, signal, this, &FileFilterModel::SignalCountChanged);
  }
  connect(this, &QAbstractItemModel::modelReset, this,
          &FileFilterModel::SignalCountChanged);
  connect(this, &QAbstractItemModel::layoutChanged, this,
          &FileFilterModel::SignalCountChanged);
}

void FileFilterModel::setSourceModel(QAbstractItemModel* source) {
  if (sourceModel() == source) return;
  QSortFilterProxyModel::setSourceModel(source);
  emit SignalSourceModelChanged();
  emit SignalCountChanged();
}

auto FileFilterModel::SearchText() const -> QString { return search_text_; }

void FileFilterModel::SetSearchText(const QString& text) {
  if (search_text_ == text) return;
  refilter([this, &text]() { search_text_ = text; });
}

auto FileFilterModel::Filtering() const -> bool {
  return !search_text_.trimmed().isEmpty();
}

auto FileFilterModel::rowCount(const QModelIndex& parent) const -> int {
  return QSortFilterProxyModel::rowCount(parent);
}

auto FileFilterModel::rowOf(int id) const -> int {
  for (int row = 0; row < rowCount(); ++row) {
    if (index(row, 0).data(FileWorkspace::kIdRole).toInt() == id) return row;
  }
  return -1;
}

auto FileFilterModel::ids() const -> QVariantList {
  QVariantList ids;
  for (int row = 0; row < rowCount(); ++row) {
    ids << index(row, 0).data(FileWorkspace::kIdRole);
  }
  return ids;
}

void FileFilterModel::reset() {
  if (!Filtering()) return;
  refilter([this]() { search_text_.clear(); });
}

auto FileFilterModel::filterAcceptsRow(int source_row,
                                       const QModelIndex& source_parent) const
    -> bool {
  const auto index = sourceModel()->index(source_row, 0, source_parent);
  const auto role = [&index](int r) { return index.data(r).toString(); };

  const auto search = search_text_.trimmed();
  if (!search.isEmpty() &&
      !role(FileWorkspace::kNameRole).contains(search, Qt::CaseInsensitive)) {
    return false;
  }

  return true;
}

void FileFilterModel::refilter(const std::function<void()>& change) {
  beginFilterChange();
  change();
  endFilterChange();
  emit SignalFilterChanged();
  emit SignalCountChanged();
}

}  // namespace GpgFrontend::Mobile
