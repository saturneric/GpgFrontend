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

#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/GpgUtils.h"
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

auto KeyFilterModel::WithGroups() const -> bool { return with_groups_; }

void KeyFilterModel::SetWithGroups(bool with) {
  if (with_groups_ == with) return;
  with_groups_ = with;
  refilter();
}

auto KeyFilterModel::Category() const -> QString { return category_; }

void KeyFilterModel::SetCategory(const QString& category) {
  if (category_ == category) return;
  category_ = category;
  refreshCategory();
}

void KeyFilterModel::refreshCategory() {
  category_key_ids_.clear();
  if (!category_.isEmpty() && !IsBuiltinKeyCategory(category_) &&
      category_ != QLatin1String(kKeyGroupCategory) &&
      ProfileSession::Loaded()) {
    const auto ids =
        KeyCategoryRepository::GetInstance(UsableDefaultChannelHandle())
            .KeyIdsOf(category_);
    category_key_ids_ = QSet<QString>(ids.begin(), ids.end());
  }
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

  // Key groups: in their own category, and where they are asked for when
  // all are shown; every other category holds keys.
  const bool group = !role(KeyRoleModel::kIsKeyRole).toBool();
  if (category_ == QLatin1String(kKeyGroupCategory)) {
    if (!group) return false;
  } else if (group && (!with_groups_ || !category_.isEmpty())) {
    return false;
  } else if (!category_.isEmpty()) {
    if (IsBuiltinKeyCategory(category_)) {
      // As the desktop's tabs decide it (MatchesBuiltinKeyCategory).
      if (!MatchesBuiltinKeyCategory(category_, KeyRoleModel::KeyAt(index))
               .value_or(false)) {
        return false;
      }
    } else if (!category_key_ids_.contains(
                   role(KeyRoleModel::kKeyIdRole).toString())) {
      return false;
    }
  }
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

auto NaturalCompare(const QString& a, const QString& b) -> int {
  qsizetype i = 0;
  qsizetype j = 0;
  while (i < a.size() && j < b.size()) {
    const bool digit_a = a.at(i).isDigit();
    const bool digit_b = b.at(j).isDigit();
    // A run of digits, or of anything else, from each.
    const auto run = [](const QString& s, qsizetype from, bool digits) {
      auto to = from;
      while (to < s.size() && s.at(to).isDigit() == digits) ++to;
      return s.mid(from, to - from);
    };
    const auto ra = run(a, i, digit_a);
    const auto rb = run(b, j, digit_b);
    i += ra.size();
    j += rb.size();
    if (digit_a && digit_b) {
      // By value: without the leading zeros, the longer is the larger.
      auto na = ra;
      auto nb = rb;
      while (na.size() > 1 && na.startsWith(u'0')) na.remove(0, 1);
      while (nb.size() > 1 && nb.startsWith(u'0')) nb.remove(0, 1);
      if (na.size() != nb.size()) return na.size() < nb.size() ? -1 : 1;
      if (const auto c = na.compare(nb); c != 0) return c;
      continue;
    }
    if (const auto c =
            QString::localeAwareCompare(ra.toCaseFolded(), rb.toCaseFolded());
        c != 0) {
      return c;
    }
  }
  if (i < a.size()) return 1;
  if (j < b.size()) return -1;
  return 0;
}

auto KeyFilterModel::lessThan(const QModelIndex& left,
                              const QModelIndex& right) const -> bool {
  // By what the row shows first: the name, else the address.
  const auto label = [](const QModelIndex& at) {
    const auto name = at.data(KeyRoleModel::kNameRole).toString();
    return name.isEmpty() ? at.data(KeyRoleModel::kEmailRole).toString() : name;
  };
  const auto order = NaturalCompare(label(left), label(right));
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
