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

#include "GpgKeyTableModel.h"

#include <QColor>

#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGroup.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgKeyTableModel::GpgKeyTableModel(int channel,
                                   const GpgAbstractKeyPtrList &keys,
                                   QObject *parent)
    : QAbstractTableModel(parent),
      column_headers_({tr("Select"), tr("Type"), tr("Name"),
                       tr("Email Address"), tr("Usage"), tr("Trust"),
                       tr("Key ID"), tr("Create Date"), tr("Algorithm"),
                       tr("Subkey(s)"), tr("Comment")}),
      gpg_context_channel_(channel) {
  for (const auto &key : keys) {
    cached_items_.push_back(GpgKeyTableItem(key));
  }
}

auto GpgKeyTableModel::index(int row, int column,
                             const QModelIndex &parent) const -> QModelIndex {
  if (!hasIndex(row, column, parent) || parent.isValid()) return {};
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return createIndex(row, column,
                     static_cast<const void *>(&cached_items_[row]));
#else
  return createIndex(
      row, column,
      const_cast<void *>(static_cast<const void *>(&cached_items_[row])));
#endif
}

auto GpgKeyTableModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  return static_cast<int>(cached_items_.size());
}

auto GpgKeyTableModel::columnCount(const QModelIndex & /*parent*/) const
    -> int {
  return 11;
}

auto GpgKeyTableModel::table_data_by_gpg_key(const QModelIndex &index,
                                             const GpgAbstractKeyPtr &key) const
    -> QVariant {
  if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) return {};
  auto *gpg_key = dynamic_cast<GpgKey *>(key.get());
  if (gpg_key == nullptr) return {};

  switch (index.column()) {
    case 0: {
      return index.row();
    }
    case 1: {
      QString type_sym;
      type_sym += key->IsPrivateKey() ? "pub/sec" : "pub";
      if (key->IsPrivateKey() && !gpg_key->IsHasMasterKey()) type_sym += "#";
      if (gpg_key->IsHasCardKey()) type_sym += "^";
      return type_sym;
    }
    case 2: {
      return key->Name();
    }
    case 3: {
      return key->Email();
    }
    case 4: {
      return GetUsagesByAbstractKey(gpg_key);
    }
    case 5: {
      return gpg_key->OwnerTrust();
    }
    case 6: {
      return key->ID();
    }
    case 7: {
      return QLocale().toString(key->CreationTime(), "yyyy-MM-dd");
    }
    case 8: {
      return key->Algo();
    }
    case 9: {
      return static_cast<int>(gpg_key->SubKeys().size());
    }
    case 10: {
      return key->Comment();
    }
    default:
      return {};
  }
}

auto GpgKeyTableModel::table_data_by_gpg_key_group(
    const QModelIndex &index, const GpgAbstractKeyPtr &key) const -> QVariant {
  if (key->KeyType() != GpgAbstractKeyType::kGPG_KEYGROUP) return {};

  switch (index.column()) {
    case 0: {
      return index.row();
    }
    case 1: {
      return "group";
    }
    case 2: {
      return key->Name();
    }
    case 3: {
      return key->Email();
    }
    case 4: {
      return GetUsagesByAbstractKey(key.get());
    }
    case 5: {
      const auto gpg_keys = ConvertKey2GpgKeyList(gpg_context_channel_, {key});

      QString owner_trust;
      for (const auto &gpg_key : gpg_keys) {
        const auto key_trust = gpg_key->OwnerTrust();
        if (owner_trust.isEmpty()) owner_trust = key_trust;
        if (owner_trust != key_trust) owner_trust = "*";
        if (owner_trust == "*") break;
      }
      return owner_trust.isEmpty() ? "/" : owner_trust;
    }
    case 6: {
      return key->ID();
    }
    case 7: {
      return QLocale().toString(key->CreationTime(), "yyyy-MM-dd");
    }
    case 8: {
      return key->Algo();
    }
    case 9: {
      auto *key_group = dynamic_cast<GpgKeyGroup *>(key.get());
      if (key_group == nullptr) return {};

      return static_cast<int>(key_group->KeyIds().size());
    }
    case 10: {
      return key->Comment();
    }
    default:
      return {};
  }
}

auto GpgKeyTableModel::table_tooltip_by_gpg_key(const QModelIndex & /*index*/,
                                                const GpgKey *key) const
    -> QVariant {
  QStringList tooltip_lines;

  tooltip_lines << tr("ID") + ": " + key->ID();
  tooltip_lines << tr("Algo") + ": " + key->Algo();
  tooltip_lines << tr("Usage") + ": " + GetUsagesByAbstractKey(key);
  tooltip_lines << tr("Trust") + ": " + key->OwnerTrust();
  tooltip_lines << tr("Comment") + ": " +
                       (key->Comment().isEmpty() ? "<" + tr("No Comment") + ">"
                                                 : key->Comment());

  const auto s_keys = key->SubKeys();
  if (!s_keys.empty()) {
    tooltip_lines << "";
    tooltip_lines << tr("SubKeys (up to 8):");

    int count = 0;
    for (const auto &s_key : s_keys) {
      if (count++ >= 8) break;
      const auto usages = GetUsagesByAbstractKey(&s_key);
      tooltip_lines << "  - " + tr("ID: %1 | Algo: %2 | Usage: %3")
                                    .arg(s_key.ID())
                                    .arg(s_key.Algo())
                                    .arg(usages.trimmed());
    }
  }

  return tooltip_lines.join("\n");
}

auto GpgKeyTableModel::data(const QModelIndex &index, int role) const
    -> QVariant {
  if (!index.isValid() || cached_items_.empty()) return {};

  auto *i = index.isValid()
                ? static_cast<GpgKeyTableItem *>(index.internalPointer())
                : nullptr;
  if (i == nullptr) return {};

  if (role == Qt::CheckStateRole) {
    if (index.column() == 0) {
      return i->Checked() ? Qt::Checked : Qt::Unchecked;
    }
  }

  if (role == Qt::DisplayRole) {
    auto *key = i->Key();
    auto p_key = GpgAbstractKeyPtr(key, [](GpgAbstractKey *) {});
    switch (key->KeyType()) {
      case GpgAbstractKeyType::kGPG_KEY:
        return table_data_by_gpg_key(index, p_key);
      case GpgAbstractKeyType::kGPG_KEYGROUP:
        return table_data_by_gpg_key_group(index, p_key);
      case GpgAbstractKeyType::kNONE:
      case GpgAbstractKeyType::kGPG_SUBKEY:
        return {};
    }
  }

  if (role == Qt::TextAlignmentRole) {
    switch (index.column()) {
      case 0:
      case 1:
      case 4:
      case 5:
      case 6:
      case 7:
      case 9:
        return Qt::AlignCenter;
      default:
        return {};
    }
  }

  if (role == Qt::ToolTipRole) {
    auto *const key = i->Key();
    switch (key->KeyType()) {
      case GpgAbstractKeyType::kGPG_KEY:
        return table_tooltip_by_gpg_key(index, dynamic_cast<GpgKey *>(key));
      case GpgAbstractKeyType::kNONE:
      case GpgAbstractKeyType::kGPG_SUBKEY:
      case GpgAbstractKeyType::kGPG_KEYGROUP:
        return {};
    }
  }

  if (role == Qt::BackgroundRole) {
    auto *const key = i->Key();
    if (key->IsDisabled()) return QColorConstants::DarkRed;
    if (key->IsExpired() || key->IsRevoked()) {
      return QColorConstants::DarkYellow;
    }
    return {};
  }

  return {};
}

auto GpgKeyTableModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const -> QVariant {
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      return column_headers_[section];
    }
  }
  return {};
}

auto GpgKeyTableModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  if (!index.isValid()) return Qt::NoItemFlags;

  if (index.column() == 0) {
    return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  }

  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

auto GpgKeyTableModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) -> bool {
  if (!index.isValid() || index.column() != 0 || role != Qt::CheckStateRole) {
    return false;
  }

  auto *i = static_cast<GpgKeyTableItem *>(index.internalPointer());
  if (i == nullptr) return false;

  const bool state = (value.toInt() == Qt::Checked);
  if (i->Checked() == state) return false;
  i->SetChecked(state);

  emit dataChanged(index, index, {Qt::CheckStateRole});
  return true;
}

auto GpgKeyTableModel::GetAllKeys() const -> GpgAbstractKeyPtrList {
  GpgAbstractKeyPtrList keys;
  for (const auto &i : cached_items_) {
    keys.push_back(i.SharedKey());
  }
  return keys;
}

auto GpgKeyTableModel::GetGpgContextChannel() const -> int {
  return gpg_context_channel_;
}

GpgKeyTableItem::GpgKeyTableItem(GpgAbstractKeyPtr key)
    : key_(std::move(key)) {}

GpgKeyTableItem::GpgKeyTableItem(const GpgKeyTableItem &) = default;

auto GpgKeyTableItem::Key() const -> GpgAbstractKey * { return key_.get(); }

auto GpgKeyTableItem::SharedKey() const -> GpgAbstractKeyPtr { return key_; }

void GpgKeyTableItem::SetChecked(bool checked) { checked_ = checked; }

auto GpgKeyTableItem::Checked() const -> bool { return checked_; }

}  // namespace GpgFrontend
