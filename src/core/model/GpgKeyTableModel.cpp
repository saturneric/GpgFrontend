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

#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgKeyTableModel::GpgKeyTableModel(int channel, GpgKeyList keys,
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
  return createIndex(row, column, &cached_items_[row]);
}

auto GpgKeyTableModel::rowCount(const QModelIndex & /*parent*/) const -> int {
  return static_cast<int>(cached_items_.size());
}

auto GpgKeyTableModel::columnCount(const QModelIndex & /*parent*/) const
    -> int {
  return 11;
}

auto GpgKeyTableModel::data(const QModelIndex &index,
                            int role) const -> QVariant {
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
    const auto key = i->Key();

    switch (index.column()) {
      case 0: {
        return index.row();
      }
      case 1: {
        QString type_sym;
        type_sym += key.IsPrivateKey() ? "pub/sec" : "pub";
        if (key.IsPrivateKey() && !key.IsHasMasterKey()) type_sym += "#";
        if (key.IsHasCardKey()) type_sym += "^";
        return type_sym;
      }
      case 2: {
        return key.Name();
      }
      case 3: {
        return key.Email();
      }
      case 4: {
        return GetUsagesByKey(key);
      }
      case 5: {
        return key.OwnerTrust();
      }
      case 6: {
        return key.ID();
      }
      case 7: {
        return QLocale().toString(key.CreationTime(), "yyyy-MM-dd");
      }
      case 8: {
        return key.Algo();
      }
      case 9: {
        return static_cast<int>(key.SubKeys().size());
      }
      case 10: {
        return key.Comment();
      }
      default:
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
    const auto key = i->Key();

    QStringList tooltip_lines;
    tooltip_lines << tr("ID: %1").arg(key.ID());
    tooltip_lines << tr("Algo: %1").arg(key.Algo());
    tooltip_lines << tr("Usage: %1").arg(GetUsagesByKey(key));
    tooltip_lines << tr("Trust: %1").arg(key.OwnerTrust());
    tooltip_lines << tr("Comment: %1")
                         .arg(key.Comment().isEmpty()
                                  ? "<" + tr("No Comment") + ">"
                                  : key.Comment());

    const auto s_keys = key.SubKeys();
    if (!s_keys.empty()) {
      tooltip_lines << "";
      tooltip_lines << tr("SubKeys (up to 8):");

      int count = 0;
      for (const auto &s_key : s_keys) {
        if (count++ >= 8) break;
        const auto usages = GetUsagesBySubkey(s_key);
        tooltip_lines << tr("  - ID: %1 | Algo: %2 | Usage: %3")
                             .arg(s_key.ID())
                             .arg(s_key.Algo())
                             .arg(usages.trimmed());
      }
    }

    return tooltip_lines.join("\n");
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
  if (!index.isValid()) return false;

  auto *i = index.isValid()
                ? static_cast<GpgKeyTableItem *>(index.internalPointer())
                : nullptr;
  if (i == nullptr) return false;

  if (index.column() == 0 && role == Qt::CheckStateRole) {
    i->SetChecked(value == Qt::Checked);
    emit dataChanged(index, index);
    return true;
  }

  return false;
}

auto GpgKeyTableModel::GetAllKeyIds() -> KeyIdArgsList {
  KeyIdArgsList keys;
  for (auto &i : cached_items_) {
    keys.push_back(i.Key().ID());
  }
  return keys;
}

auto GpgKeyTableModel::GetKeyIDByRow(int row) const -> QString {
  if (cached_items_.size() <= row) return {};

  return cached_items_[row].Key().ID();
}

auto GpgKeyTableModel::IsPrivateKeyByRow(int row) const -> bool {
  if (cached_items_.size() <= row) return false;
  return cached_items_[row].Key().IsPrivateKey();
}

auto GpgKeyTableModel::GetGpgContextChannel() const -> int {
  return gpg_context_channel_;
}

GpgKeyTableItem::GpgKeyTableItem(const GpgKey &key) : key_(key) {}

GpgKeyTableItem::GpgKeyTableItem(const GpgKeyTableItem &) = default;

auto GpgKeyTableItem::Key() const -> GpgKey { return key_; }

void GpgKeyTableItem::SetChecked(bool checked) { checked_ = checked; }

auto GpgKeyTableItem::Checked() const -> bool { return checked_; }

}  // namespace GpgFrontend
