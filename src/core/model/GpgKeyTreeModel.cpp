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

#include "GpgKeyTreeModel.h"

#include <utility>

#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgKeyTreeModel::GpgKeyTreeModel(int channel, const GpgKeyList &keys,
                                 Detector checkable_detector, QObject *parent)
    : QAbstractItemModel(parent),
      gpg_context_channel_(channel),
      column_headers_({
          tr("Select"),
          tr("Type"),
          tr("Identity"),
          tr("Key ID"),
          tr("Usage"),
          tr("Algorithm"),
          tr("Create Date"),
      }),
      checkable_detector_(std::move(checkable_detector)) {
  setup_model_data(keys);
}

auto GpgKeyTreeModel::index(int row, int column,
                            const QModelIndex &parent) const -> QModelIndex {
  if (!hasIndex(row, column, parent)) return {};

  GpgKeyTreeItem *i_parent =
      parent.isValid() ? static_cast<GpgKeyTreeItem *>(parent.internalPointer())
                       : root_.get();

  auto *i_child = i_parent->Child(row);
  if (i_child != nullptr) {
    return createIndex(row, column, i_child);
  }

  return {};
}

auto GpgKeyTreeModel::rowCount(const QModelIndex &parent) const -> int {
  if (parent.column() > 0) return 0;

  const GpgKeyTreeItem *i_parent =
      parent.isValid()
          ? static_cast<const GpgKeyTreeItem *>(parent.internalPointer())
          : root_.get();

  return static_cast<int>(i_parent->ChildCount());
}

auto GpgKeyTreeModel::columnCount(const QModelIndex &parent) const -> int {
  if (parent.isValid()) {
    return static_cast<int>(
        static_cast<GpgKeyTreeItem *>(parent.internalPointer())->ColumnCount());
  }
  return static_cast<int>(root_->ColumnCount());
}

auto GpgKeyTreeModel::data(const QModelIndex &index,
                           int role) const -> QVariant {
  if (!index.isValid()) return {};

  const auto *item =
      static_cast<const GpgKeyTreeItem *>(index.internalPointer());

  if (role == Qt::CheckStateRole) {
    if (index.column() == 0 && item->Checkable()) {
      return item->Checked() ? Qt::Checked : Qt::Unchecked;
    }
  }

  if (role == Qt::DisplayRole) {
    if (index.column() == 0) return item->Row();
    return item->Data(index.column());
  }

  if (role == Qt::TextAlignmentRole) {
    switch (index.column()) {
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
        return Qt::AlignCenter;
      default:
        return {};
    }
  }

  return {};
}

auto GpgKeyTreeModel::headerData(int section, Qt::Orientation orientation,
                                 int role) const -> QVariant {
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    return root_->Data(section);
  }
  return {};
}

auto GpgKeyTreeModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  if (!index.isValid()) return Qt::NoItemFlags;

  const auto *item =
      static_cast<const GpgKeyTreeItem *>(index.internalPointer());

  if (index.column() == 0) {
    return (item->Checkable() ? Qt::ItemIsUserCheckable : Qt::ItemFlags{0}) |
           Qt::ItemIsSelectable |
           (item->Enable() ? Qt::ItemIsEnabled : Qt::ItemFlags{0});
  }

  return Qt::ItemIsSelectable |
         (item->Enable() ? Qt::ItemIsEnabled : Qt::ItemFlags{0});
}

auto GpgKeyTreeModel::setData(const QModelIndex &index, const QVariant &value,
                              int role) -> bool {
  if (!index.isValid()) return false;

  auto *item = static_cast<GpgKeyTreeItem *>(index.internalPointer());

  if (index.column() == 0 && role == Qt::CheckStateRole) {
    item->SetChecked(value == Qt::Checked);
    emit dataChanged(index, index);
    return true;
  }

  return false;
}

auto GpgKeyTreeModel::GetGpgContextChannel() const -> int {
  return gpg_context_channel_;
}

void GpgKeyTreeModel::setup_model_data(const GpgKeyList &keys) {
  auto root = QSharedPointer<GpgKeyTreeItem>::create(nullptr, column_headers_);
  cached_items_.clear();

  for (const auto &key : keys) {
    auto pi_key = create_gpg_key_tree_items(key);
    root->AppendChild(pi_key);
  }

  std::swap(root_, root);
}

auto GpgKeyTreeModel::parent(const QModelIndex &index) const -> QModelIndex {
  if (!index.isValid()) return {};

  auto *i_child = static_cast<GpgKeyTreeItem *>(index.internalPointer());
  GpgKeyTreeItem *i_parent = i_child->ParentItem();

  return i_parent != root_.get()
             ? createIndex(static_cast<int>(i_parent->Row()), 0, i_parent)
             : QModelIndex{};
}

auto GpgKeyTreeModel::GetAllCheckedKeyIds() -> KeyIdArgsList {
  auto ret = KeyIdArgsList{};
  for (const auto &item : cached_items_) {
    if (!item->Checkable() || !item->Checked()) continue;
    ret.push_back(item->Key()->ID());
  }
  return ret;
}

auto GpgKeyTreeModel::create_gpg_key_tree_items(const GpgKey &key)
    -> QSharedPointer<GpgKeyTreeItem> {
  QVariantList columns;
  columns << "/";

  QString type;
  type += key.IsPrivateKey() ? "pub/sec" : "pub";
  if (key.IsPrivateKey() && !key.IsHasMasterKey()) type += "#";
  if (key.IsHasCardKey()) type += "^";
  columns << type;

  columns << key.UIDs().front().GetUID();
  columns << key.ID();

  columns << GetUsagesByKey(key);
  columns << key.PublicKeyAlgo();
  columns << key.Algo();
  columns << QLocale().toString(key.CreationTime(), "yyyy-MM-dd");

  auto i_key = QSharedPointer<GpgKeyTreeItem>::create(
      QSharedPointer<GpgKey>::create(key), columns);
  i_key->SetEnable(true);
  i_key->SetCheckable(checkable_detector_(i_key->Key()));
  cached_items_.push_back(i_key);

  for (const auto &s_key : key.SubKeys()) {
    QVariantList columns;
    columns << "/";
    columns << (s_key.IsHasCertCap() ? "primary" : "sub");
    columns << key.UIDs().front().GetUID();
    columns << s_key.ID();
    columns << GetUsagesBySubkey(s_key);
    columns << s_key.PublicKeyAlgo();
    columns << s_key.Algo();
    columns << QLocale().toString(s_key.CreationTime(), "yyyy-MM-dd");

    auto i_s_key = QSharedPointer<GpgKeyTreeItem>::create(
        QSharedPointer<GpgSubKey>::create(s_key), columns);
    i_s_key->SetEnable(true);
    i_s_key->SetCheckable(checkable_detector_(i_s_key->Key()));
    i_key->AppendChild(i_s_key);
    cached_items_.push_back(i_s_key);
  }

  return i_key;
}

auto GpgKeyTreeModel::GetAllCheckedSubKey() -> QContainer<GpgSubKey> {
  QContainer<GpgSubKey> ret;
  for (const auto &i : cached_items_) {
    if (!i->Key()->IsSubKey() || !i->Checkable() || !i->Checked()) continue;

    auto *s_key = dynamic_cast<GpgSubKey *>(i->Key());
    if (s_key == nullptr) continue;

    ret.push_back(*s_key);
  }
  return ret;
}

auto GpgKeyTreeModel::GetKeyByIndex(QModelIndex index) -> GpgAbstractKey * {
  if (!index.isValid()) return nullptr;

  const auto *item =
      static_cast<const GpgKeyTreeItem *>(index.internalPointer());

  return item->Key();
}

GpgKeyTreeItem::GpgKeyTreeItem(QSharedPointer<GpgAbstractKey> key,
                               QVariantList data)
    : data_(std::move(data)), key_(std::move(key)) {}

void GpgKeyTreeItem::AppendChild(const QSharedPointer<GpgKeyTreeItem> &child) {
  child->parent_ = this;
  children_.append(child);
}

auto GpgKeyTreeItem::Child(int row) -> GpgKeyTreeItem * {
  return row >= 0 && row < ChildCount() ? children_.at(row).get() : nullptr;
}

auto GpgKeyTreeItem::ChildCount() const -> qsizetype {
  return children_.size();
}

auto GpgKeyTreeItem::ColumnCount() const -> qsizetype { return data_.count(); }

auto GpgKeyTreeItem::Data(qsizetype column) const -> QVariant {
  return data_.value(column);
}

auto GpgKeyTreeItem::Row() const -> qsizetype {
  if (parent_ == nullptr) return 0;
  const auto it =
      std::find_if(parent_->children_.cbegin(), parent_->children_.cend(),
                   [this](const auto &item) { return item.get() == this; });

  if (it != parent_->children_.cend()) {
    return std::distance(parent_->children_.cbegin(), it);
  }

  Q_ASSERT(false);
  return -1;
}

auto GpgKeyTreeItem::ParentItem() -> GpgKeyTreeItem * { return parent_; }

auto GpgKeyTreeItem::Checked() const -> bool { return checked_; }

auto GpgKeyTreeItem::Checkable() const -> bool { return checkable_; }

void GpgKeyTreeItem::SetChecked(bool checked) { checked_ = checked; }

void GpgKeyTreeItem::SetCheckable(bool checkable) { checkable_ = checkable; }

auto GpgKeyTreeItem::Key() const -> GpgAbstractKey * { return key_.get(); }

auto GpgKeyTreeItem::Enable() const -> bool { return enable_; }

void GpgKeyTreeItem::SetEnable(bool enable) { enable_ = enable; }

}  // namespace GpgFrontend
