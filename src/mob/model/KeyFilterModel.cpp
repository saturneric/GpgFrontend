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

#include "model/KeyFilterModel.h"

#include "core/utils/KeySearchMatcher.h"
#include "model/KeyRoleModel.h"

namespace GpgFrontend::Mobile {

KeyFilterModel::KeyFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
  sort(0);
  for (const auto& signal :
       {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved}) {
    connect(this, signal, this, &KeyFilterModel::SignalCountChanged);
  }
  connect(this, &QAbstractItemModel::modelReset, this,
          &KeyFilterModel::SignalCountChanged);
  connect(this, &QAbstractItemModel::layoutChanged, this,
          &KeyFilterModel::SignalCountChanged);
}

void KeyFilterModel::setSourceModel(QAbstractItemModel* source) {
  if (sourceModel() == source) return;
  QSortFilterProxyModel::setSourceModel(source);
  emit SignalSourceModelChanged();
  emit SignalCountChanged();
}

auto KeyFilterModel::SearchText() const -> QString { return search_text_; }

void KeyFilterModel::SetSearchText(const QString& text) {
  if (search_text_ == text) return;
  search_text_ = text;
  refilter();
}

auto KeyFilterModel::OnlySecret() const -> bool { return only_secret_; }

void KeyFilterModel::SetOnlySecret(bool only) {
  if (only_secret_ == only) return;
  only_secret_ = only;
  refilter();
}

auto KeyFilterModel::OnlyEncryptCapable() const -> bool {
  return only_encrypt_capable_;
}

void KeyFilterModel::SetOnlyEncryptCapable(bool only) {
  if (only_encrypt_capable_ == only) return;
  only_encrypt_capable_ = only;
  refilter();
}

auto KeyFilterModel::OnlySignCapable() const -> bool {
  return only_sign_capable_;
}

void KeyFilterModel::SetOnlySignCapable(bool only) {
  if (only_sign_capable_ == only) return;
  only_sign_capable_ = only;
  refilter();
}

auto KeyFilterModel::rowCount(const QModelIndex& parent) const -> int {
  return QSortFilterProxyModel::rowCount(parent);
}

auto KeyFilterModel::filterAcceptsRow(int source_row,
                                      const QModelIndex& source_parent) const
    -> bool {
  const auto index = sourceModel()->index(source_row, 0, source_parent);
  const auto role = [&index](int r) { return index.data(r); };

  if (!role(KeyRoleModel::kIsKeyRole).toBool()) return false;
  if (only_secret_ && !role(KeyRoleModel::kHasSecretRole).toBool()) {
    return false;
  }
  if (only_encrypt_capable_ && (!role(KeyRoleModel::kCanEncryptRole).toBool() ||
                                !role(KeyRoleModel::kUsableRole).toBool())) {
    return false;
  }
  if (only_sign_capable_ && (!role(KeyRoleModel::kCanSignRole).toBool() ||
                             !role(KeyRoleModel::kUsableRole).toBool())) {
    return false;
  }

  // The desktop's search: a pasted fingerprint matches in any grouping.
  return KeySearchMatches({role(KeyRoleModel::kNameRole).toString(),
                           role(KeyRoleModel::kEmailRole).toString(),
                           role(KeyRoleModel::kKeyIdRole).toString()},
                          role(KeyRoleModel::kFingerprintRole).toString(),
                          search_text_.trimmed());
}

auto KeyFilterModel::lessThan(const QModelIndex& left,
                              const QModelIndex& right) const -> bool {
  const auto l = left.data(KeyRoleModel::kNameRole).toString();
  const auto r = right.data(KeyRoleModel::kNameRole).toString();
  const auto order =
      QString::localeAwareCompare(l.toCaseFolded(), r.toCaseFolded());
  if (order != 0) return order < 0;
  return left.data(KeyRoleModel::kFingerprintRole).toString() <
         right.data(KeyRoleModel::kFingerprintRole).toString();
}

void KeyFilterModel::refilter() {
  invalidateFilter();
  emit SignalFilterChanged();
  emit SignalCountChanged();
}

}  // namespace GpgFrontend::Mobile
