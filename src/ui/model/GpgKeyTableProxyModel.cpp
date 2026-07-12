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

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyTableModel.h"

namespace GpgFrontend::UI {

GpgKeyTableProxyModel::GpgKeyTableProxyModel(
    QSharedPointer<GpgKeyTableModel> model, GpgKeyTableDisplayMode display_mode,
    GpgKeyTableColumn columns, KeyFilter filter, QObject *parent)
    : QSortFilterProxyModel(parent),
      model_(std::move(model)),
      display_mode_(display_mode),
      filter_columns_(columns),
      custom_filter_(std::move(filter)),
      default_font_("Arial", 14),
      default_metrics_(default_font_) {
  setSourceModel(model_.get());

  connect(this, &GpgKeyTableProxyModel::SignalCategoriesRefresh, this,
          &GpgKeyTableProxyModel::slot_refresh_categories);
  connect(this, &GpgKeyTableProxyModel::SignalColumnTypeChange, this,
          &GpgKeyTableProxyModel::slot_update_column_type);

  emit SignalCategoriesRefresh();
}

auto GpgKeyTableProxyModel::filterAcceptsRow(
    int sourceRow, const QModelIndex &sourceParent) const -> bool {
  auto index = sourceModel()->index(sourceRow, 0, sourceParent);

  auto *i = index.isValid()
                ? static_cast<GpgKeyTableItem *>(index.internalPointer())
                : nullptr;
  assert(i != nullptr);

  auto *key = i->Key();
  assert(key->IsGood());

  if (!(display_mode_ & GpgKeyTableDisplayMode::kPRIVATE_KEY) &&
      key->IsPrivateKey()) {
    return false;
  }

  if (!(display_mode_ & GpgKeyTableDisplayMode::kPUBLIC_KEY) &&
      !key->IsPrivateKey()) {
    return false;
  }

  if (!custom_filter_(key)) return false;

  if (!category_id_.isEmpty() && !category_key_ids_.contains(key->ID())) {
    return false;
  }

  if (filter_keywords_.isEmpty()) return true;

  QStringList infos;
  for (int column = 0; column < sourceModel()->columnCount(); ++column) {
    auto index = sourceModel()->index(sourceRow, column, sourceParent);
    infos << sourceModel()->data(index).toString();

    if (key->KeyType() == GpgAbstractKeyType::kGPG_KEY) {
      auto *k = dynamic_cast<GpgKey *>(key);
      for (const auto &uid : k->UIDs()) {
        infos << uid.GetUID();
      }
    }
  }

  return std::any_of(infos.cbegin(), infos.cend(), [&](const QString &info) {
    return info.contains(filter_keywords_, Qt::CaseInsensitive);
  });

  return false;
}

auto GpgKeyTableProxyModel::filterAcceptsColumn(
    int sourceColumn, const QModelIndex & /*sourceParent*/) const -> bool {
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

auto GpgKeyTableProxyModel::SourceColumnForVisibleColumn(
    int visible_column) const -> int {
  if (visible_column < 0 || model_ == nullptr) return -1;

  int seen = 0;
  const int source_columns = model_->columnCount({});
  for (int source_col = 0; source_col < source_columns; ++source_col) {
    if (!filterAcceptsColumn(source_col, {})) continue;
    if (seen == visible_column) return source_col;
    ++seen;
  }
  return -1;
}

void GpgKeyTableProxyModel::SetSearchKeywords(const QString &keywords) {
  this->filter_keywords_ = keywords;
  invalidateFilter();
}

void GpgKeyTableProxyModel::slot_refresh_categories() {
  refresh_category_cache();
  invalidateFilter();
}

void GpgKeyTableProxyModel::slot_update_column_type(
    GpgKeyTableColumn filter_columns) {
  filter_columns_ = filter_columns;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 4)
  invalidateColumnsFilter();
#else
  invalidateFilter();
#endif
}

void GpgKeyTableProxyModel::ResetGpgKeyTableModel(
    QSharedPointer<GpgKeyTableModel> model) {
  model_ = std::move(model);
  refresh_category_cache();
  setSourceModel(model_.get());
}

void GpgKeyTableProxyModel::refresh_category_cache() {
  category_key_ids_.clear();
  if (category_id_.isEmpty()) return;

  auto key_ids =
      KeyCategoryRepository::GetInstance(model_->GetGpgContextChannel())
          .KeyIdsOf(category_id_);
  category_key_ids_ = QSet<QString>(key_ids.begin(), key_ids.end());
}

void GpgKeyTableProxyModel::SetCategoryFilter(const QString &category_id) {
  category_id_ = category_id;
  refresh_category_cache();
  invalidateFilter();
}

void GpgKeyTableProxyModel::SetFilter(const KeyFilter &filter) {
  this->custom_filter_ = filter;
  invalidateFilter();
}
}  // namespace GpgFrontend::UI