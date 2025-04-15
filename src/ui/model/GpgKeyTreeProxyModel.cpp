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

#include "GpgKeyTreeProxyModel.h"

#include "core/model/CacheObject.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyTreeModel.h"
#include "core/struct/cache_object/AllFavoriteKeyPairsCO.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::UI {

GpgKeyTreeProxyModel::GpgKeyTreeProxyModel(
    QSharedPointer<GpgKeyTreeModel> model, GpgKeyTreeDisplayMode display_mode,
    KeyFilter filter, QObject *parent)
    : QSortFilterProxyModel(parent),
      model_(std::move(model)),
      display_mode_(display_mode),
      custom_filter_(std::move(filter)) {
  setSourceModel(model_.get());

  connect(this, &GpgKeyTreeProxyModel::SignalFavoritesChanged, this,
          &GpgKeyTreeProxyModel::slot_update_favorites);

  emit SignalFavoritesChanged();
}

auto GpgKeyTreeProxyModel::filterAcceptsRow(
    int source_row, const QModelIndex &sourceParent) const -> bool {
  auto index = sourceModel()->index(source_row, 0, sourceParent);
  auto *i = index.isValid()
                ? static_cast<GpgKeyTreeItem *>(index.internalPointer())
                : nullptr;

  const auto *key = i->Key();
  LOG_D() << "get key: " << key->ID()
          << "from channel: " << model_->GetGpgContextChannel();
  assert(key->IsGood());

  if (!(display_mode_ & GpgKeyTreeDisplayMode::kPRIVATE_KEY) &&
      key->IsPrivateKey()) {
    return false;
  }

  if (!(display_mode_ & GpgKeyTreeDisplayMode::kPUBLIC_KEY) &&
      !key->IsPrivateKey()) {
    return false;
  }

  if (!custom_filter_(key)) return false;

  if (filter_keywords_.isEmpty()) return true;

  QStringList infos;
  for (int column = 0; column < sourceModel()->columnCount(); ++column) {
    auto index = sourceModel()->index(source_row, column, sourceParent);
    infos << sourceModel()->data(index).toString();

    if (key->KeyType() != GpgAbstractKeyType::kGPG_SUBKEY) {
      for (const auto &uid : dynamic_cast<const GpgKey *>(key)->UIDs()) {
        infos << uid.GetUID();
      }
    }
  }

  return std::any_of(infos.cbegin(), infos.cend(), [&](const QString &info) {
    return info.contains(filter_keywords_, Qt::CaseInsensitive);
  });

  return false;
}

void GpgKeyTreeProxyModel::SetSearchKeywords(const QString &keywords) {
  this->filter_keywords_ = keywords;
  invalidateFilter();
}

void GpgKeyTreeProxyModel::slot_update_favorites() {
  slot_update_favorites_cache();
  invalidateFilter();
}

void GpgKeyTreeProxyModel::ResetGpgKeyTableModel(
    QSharedPointer<GpgKeyTreeModel> model) {
  model_ = std::move(model);
  slot_update_favorites_cache();
  setSourceModel(model_.get());
}

void GpgKeyTreeProxyModel::slot_update_favorites_cache() {
  auto json_data = CacheObject("all_favorite_key_pairs");
  auto cache_obj = AllFavoriteKeyPairsCO(json_data.object());

  auto key_db_name = GetGpgKeyDatabaseName(model_->GetGpgContextChannel());

  if (cache_obj.key_dbs.contains(key_db_name)) {
    favorite_key_ids_ = cache_obj.key_dbs[key_db_name].key_ids;
  }
}

void GpgKeyTreeProxyModel::SetKeyFilter(const KeyFilter &filter) {
  custom_filter_ = filter;
  invalidateFilter();
}

}  // namespace GpgFrontend::UI