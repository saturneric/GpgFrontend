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

#include "GpgKeyTableProxyModel.h"

#include <utility>

#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/CacheObject.h"
#include "core/model/GpgKey.h"

namespace GpgFrontend {

GpgKeyTableProxyModel::GpgKeyTableProxyModel(
    QSharedPointer<GpgKeyTableModel> model, GpgKeyTableDisplayMode display_mode,
    GpgKeyTableColumn columns, KeyFilter filter, QObject *parent)
    : QSortFilterProxyModel(parent),
      model_(std::move(model)),
      display_mode_(display_mode),
      filter_columns_(columns),
      custom_filter_(std::move(filter)) {
  setSourceModel(model_.get());

  connect(this, &GpgKeyTableProxyModel::SignalFavoritesChanged, this,
          &GpgKeyTableProxyModel::slot_update_favorites);
  connect(this, &GpgKeyTableProxyModel::SignalColumnTypeChange, this,
          &GpgKeyTableProxyModel::slot_update_column_type);

  emit SignalFavoritesChanged();
}

auto GpgKeyTableProxyModel::filterAcceptsRow(
    int source_row, const QModelIndex &sourceParent) const -> bool {
  auto index = sourceModel()->index(source_row, 6, sourceParent);
  auto key_id = sourceModel()->data(index).toString();
  auto key = GpgKeyGetter::GetInstance().GetKey(key_id);

  if (!(display_mode_ & GpgKeyTableDisplayMode::kPRIVATE_KEY) &&
      key.IsPrivateKey()) {
    return false;
  }

  if (!(display_mode_ & GpgKeyTableDisplayMode::kPUBLIC_KEY) &&
      !key.IsPrivateKey()) {
    return false;
  }

  if (!custom_filter_(key)) return false;

  if (display_mode_ & GpgKeyTableDisplayMode::kFAVORITES &&
      !favorite_fingerprints_.contains(key.GetFingerprint())) {
    return false;
  }

  if (filter_keywords_.isEmpty()) return true;

  QStringList infos;
  for (int column = 0; column < sourceModel()->columnCount(); ++column) {
    auto index = sourceModel()->index(source_row, column, sourceParent);
    infos << sourceModel()->data(index).toString();

    const auto uids = key.GetUIDs();
    for (const auto &uid : *uids) {
      infos << uid.GetUID();
    }
  }

  return std::any_of(infos.cbegin(), infos.cend(), [&](const QString &info) {
    return info.contains(filter_keywords_, Qt::CaseInsensitive);
  });

  return false;
}

auto GpgKeyTableProxyModel::filterAcceptsColumn(
    int sourceColumn, const QModelIndex &sourceParent) const -> bool {
  switch (sourceColumn) {
    case 0: {
      return true;
    }
    case 1: {
      return (filter_columns_ & GpgKeyTableColumn::kTYPE) !=
             GpgKeyTableColumn::kNONE;
    }
    case 2: {
      return (filter_columns_ & GpgKeyTableColumn::kNAME) !=
             GpgKeyTableColumn::kNONE;
    }
    case 3: {
      return (filter_columns_ & GpgKeyTableColumn::kEMAIL_ADDRESS) !=
             GpgKeyTableColumn::kNONE;
    }
    case 4: {
      return (filter_columns_ & GpgKeyTableColumn::kUSAGE) !=
             GpgKeyTableColumn::kNONE;
    }
    case 5: {
      return (filter_columns_ & GpgKeyTableColumn::kOWNER_TRUST) !=
             GpgKeyTableColumn::kNONE;
    }
    case 6: {
      return (filter_columns_ & GpgKeyTableColumn::kKEY_ID) !=
             GpgKeyTableColumn::kNONE;
    }
    case 7: {
      return (filter_columns_ & GpgKeyTableColumn::kCREATE_DATE) !=
             GpgKeyTableColumn::kNONE;
    }
    case 8: {
      return (filter_columns_ & GpgKeyTableColumn::kALGO) !=
             GpgKeyTableColumn::kNONE;
    }
    case 9: {
      return (filter_columns_ & GpgKeyTableColumn::kSUBKEYS_NUMBER) !=
             GpgKeyTableColumn::kNONE;
    }
    case 10: {
      return (filter_columns_ & GpgKeyTableColumn::kCOMMENT) !=
             GpgKeyTableColumn::kNONE;
    }
    default:
      return false;
  }
}

void GpgKeyTableProxyModel::SetSearchKeywords(const QString &keywords) {
  this->filter_keywords_ = keywords;
  invalidateFilter();
}

void GpgKeyTableProxyModel::slot_update_favorites() {
  // load cache
  auto json_data = CacheObject("favourite_key_pair");
  if (!json_data.isArray()) return;

  auto key_fprs = json_data.array();
  for (const auto &key_fpr : key_fprs) {
    if (key_fpr.isString()) favorite_fingerprints_.append(key_fpr.toString());
  }

  invalidateFilter();
}

void GpgKeyTableProxyModel::slot_update_column_type(
    GpgKeyTableColumn filter_columns) {
  filter_columns_ = filter_columns;
#ifdef QT5_BUILD
  invalidateFilter();
#else
  invalidateColumnsFilter();
#endif
}

}  // namespace GpgFrontend