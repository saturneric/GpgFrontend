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

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGroup.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

namespace {

// The repository refuses to build a cyclic group forest, but the forest is
// restored from a JSON blob on disk. The model must not be what hangs when
// that blob is wrong.
constexpr int kMaxKeyGroupNestingDepth = 8;

}  // namespace

// The build mode deliberately sits after parent: existing call sites pass
// `this` as the fourth argument, and a pointer would not convert to the enum,
// so inserting it earlier would silently mean something else.
GpgKeyTreeModel::GpgKeyTreeModel(int channel, const GpgAbstractKeyPtrList &keys,
                                 Detector checkable_detector, QObject *parent,
                                 GpgKeyTreeBuildMode mode)
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
      checkable_detector_(std::move(checkable_detector)),
      build_mode_(mode) {
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
  // A tree has one column count, not one per item.
  Q_UNUSED(parent)
  return static_cast<int>(root_->ColumnCount());
}

auto GpgKeyTreeModel::data(const QModelIndex &index, int role) const
    -> QVariant {
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
    emit SignalKeyCheckedChanged(item->Key(), value == Qt::Checked);
    emit dataChanged(index, index);
    return true;
  }

  return false;
}

auto GpgKeyTreeModel::GetGpgContextChannel() const -> int {
  return gpg_context_channel_;
}

void GpgKeyTreeModel::setup_model_data(const GpgAbstractKeyPtrList &keys) {
  auto root =
      SecureCreateSharedObject<GpgKeyTreeItem>(nullptr, column_headers_);
  cached_items_.clear();

  for (const auto &key : keys) {
    // AbstractKeyRepository::GetKeys() yields a nullptr for an id that no
    // longer resolves; such a member simply gets no row.
    if (key == nullptr) continue;

    auto pi_key = create_tree_items(key, 0, {});
    if (pi_key != nullptr) root->AppendChild(pi_key);
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

auto GpgKeyTreeModel::GetAllCheckedKeys() -> GpgAbstractKeyPtrList {
  auto ret = GpgAbstractKeyPtrList{};
  for (const auto &item : cached_items_) {
    if (!item->Checkable() || !item->Checked()) continue;
    ret.push_back(
        QSharedPointer<GpgAbstractKey>(item->Key(), [](GpgAbstractKey *) {}));
  }
  return ret;
}

auto GpgKeyTreeModel::create_tree_items(const GpgAbstractKeyPtr &key, int depth,
                                        const QSet<QString> &visiting)
    -> QSharedPointer<GpgKeyTreeItem> {
  if (key == nullptr) return nullptr;

  if (build_mode_ == GpgKeyTreeBuildMode::kKEY_GROUP_MEMBERS &&
      key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP) {
    return create_key_group_tree_items(key, depth, visiting);
  }

  return create_gpg_key_tree_items(key, depth);
}

void GpgKeyTreeModel::finish_tree_item(
    const QSharedPointer<GpgKeyTreeItem> &item, int depth) {
  // Outside a key group tree every row is a root as far as checking goes.
  const auto direct =
      build_mode_ != GpgKeyTreeBuildMode::kKEY_GROUP_MEMBERS || depth == 0;

  item->SetEnable(direct);
  item->SetCheckable(direct && checkable_detector_(item->Key()));
  item->SetChecked(false);

  // Only rows that can actually be checked take part in the checked-key
  // lookups. Keeping the deeper ones out makes it impossible for a key that
  // several nested groups share to be reported more than once.
  if (direct) cached_items_.push_back(item);
}

auto GpgKeyTreeModel::create_gpg_key_tree_items(const GpgAbstractKeyPtr &key,
                                                int depth)
    -> QSharedPointer<GpgKeyTreeItem> {
  QVariantList columns;
  columns << "/";

  if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) return nullptr;

  auto g_key = qSharedPointerDynamicCast<GpgKey>(key);

  QString type;
  type += g_key->IsPrivateKey() ? "pub/sec" : "pub";
  if (g_key->IsPrivateKey() && !g_key->IsHasMasterKey()) type += "#";
  if (g_key->IsHasCardKey()) type += "^";
  columns << type;

  columns << g_key->UIDs().front().GetUID();
  columns << g_key->ID();

  columns << GetUsagesByAbstractKey(key.get());
  columns << g_key->Algo();
  columns << QLocale().toString(g_key->CreationTime(), "yyyy-MM-dd");

  assert(key != nullptr);
  auto i_key = SecureCreateSharedObject<GpgKeyTreeItem>(key, columns);
  finish_tree_item(i_key, depth);

  for (const auto &s_key : g_key->SubKeys()) {
    // avoid bugs due to duplicate key ids
    if (g_key->ID() == s_key.ID() || s_key.IsADSK()) continue;

    QVariantList columns;
    columns << "/";
    columns << (s_key.IsHasCertCap() ? "primary" : "sub");
    columns << g_key->UIDs().front().GetUID();
    columns << s_key.ID();
    columns << GetUsagesByAbstractKey(&s_key);
    columns << s_key.Algo();
    columns << QLocale().toString(s_key.CreationTime(), "yyyy-MM-dd");

    auto i_s_key = SecureCreateSharedObject<GpgKeyTreeItem>(
        SecureCreateSharedObject<GpgSubKey>(s_key), columns);
    finish_tree_item(i_s_key, depth + 1);
    i_key->AppendChild(i_s_key);
  }

  return i_key;
}

auto GpgKeyTreeModel::create_key_group_tree_items(const GpgAbstractKeyPtr &key,
                                                  int depth,
                                                  QSet<QString> visiting)
    -> QSharedPointer<GpgKeyTreeItem> {
  auto key_group = qSharedPointerDynamicCast<GpgKeyGroup>(key);
  if (key_group == nullptr) return nullptr;

  // Stop, but still show the row: a group that is already on this path, or one
  // nested absurdly deep, is rendered as a leaf rather than expanded again.
  const auto stop =
      visiting.contains(key_group->ID()) || depth >= kMaxKeyGroupNestingDepth;

  auto identity = key_group->Name();
  if (!key_group->Email().isEmpty()) {
    identity = QString("%1 <%2>").arg(identity, key_group->Email());
  }
  if (stop) {
    identity = QString("%1 %2").arg(identity, tr("(already shown above)"));
  }

  QVariantList columns;
  columns << "/";
  columns << "group";
  columns << identity;
  columns << key_group->ID();
  columns << GetUsagesByAbstractKey(key.get());
  // A key group has no algorithm of its own; anything put here would be a
  // claim the column header does not support.
  columns << QString{};
  columns << QLocale().toString(key_group->CreationTime(), "yyyy-MM-dd");

  auto i_key_group = SecureCreateSharedObject<GpgKeyTreeItem>(key, columns);
  finish_tree_item(i_key_group, depth);

  if (stop) return i_key_group;

  visiting.insert(key_group->ID());
  auto &getter = AbstractKeyRepository::GetInstance(gpg_context_channel_);

  for (const auto &key_id : key_group->KeyIds()) {
    auto member = getter.GetKey(key_id);
    if (member == nullptr) continue;

    auto i_member = create_tree_items(member, depth + 1, visiting);
    if (i_member != nullptr) i_key_group->AppendChild(i_member);
  }

  return i_key_group;
}

auto GpgKeyTreeModel::GetAllCheckedSubKey() -> QContainer<GpgSubKey> {
  QContainer<GpgSubKey> ret;
  for (const auto &i : cached_items_) {
    if (i->Key()->KeyType() != GpgAbstractKeyType::kGPG_SUBKEY ||
        !i->Checkable() || !i->Checked()) {
      continue;
    }

    LOG_D() << "subkey checked: " << i->Key()->ID()
            << "uid: " << i->Key()->UID() << "checkable: " << i->Checkable()
            << "checked: " << i->Checked();

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
  assert(item != nullptr);

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

auto GpgKeyTreeItem::Key() const -> GpgAbstractKey * {
  assert(key_ != nullptr);
  return key_.get();
}

auto GpgKeyTreeItem::SharedKey() const -> GpgAbstractKeyPtr {
  assert(key_ != nullptr);
  return key_;
}

auto GpgKeyTreeItem::Enable() const -> bool { return enable_; }

void GpgKeyTreeItem::SetEnable(bool enable) { enable_ = enable; }

}  // namespace GpgFrontend
