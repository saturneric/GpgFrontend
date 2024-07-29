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

#include "GlobalRegisterTable.h"

#include <any>
#include <optional>
#include <shared_mutex>
#include <utility>
#include <vector>

#include "GlobalRegisterTableTreeModel.h"
#include "function/SecureMemoryAllocator.h"
#include "utils/MemoryUtils.h"

namespace GpgFrontend::Module {

class GlobalRegisterTable::Impl {
 public:
  struct RTNode {
    QString name;
    QString type = tr("NODE");
    int version = 0;
    const std::type_info* value_type = nullptr;
    std::optional<std::any> value = std::nullopt;
    QMap<QString, QSharedPointer<RTNode>> children;
    QWeakPointer<RTNode> parent;

    explicit RTNode(QString name, const QSharedPointer<RTNode>& parent)
        : name(std::move(name)), parent(parent) {}
  };

  using RTNodePtr = QSharedPointer<RTNode>;

  explicit Impl(GlobalRegisterTable* parent)
      : parent_(parent),
        root_node_(SecureCreateQSharedObject<RTNode>("", nullptr)) {}

  auto PublishKV(const Namespace& n, const Key& k, std::any v) -> bool {
    QStringList const segments = (n + "." + k).split('.');
    int version = 0;

    {
      std::unique_lock lock(lock_);

      auto current = root_node_;
      for (const QString& segment : segments) {
        auto it = current->children.find(segment);
        if (it == current->children.end()) {
          it = current->children.insert(
              segment, SecureCreateQSharedObject<RTNode>(segment, current));
        }
        current = it.value();
      }

      current->name = segments.back();
      current->type = tr("LEAF");
      current->value = v;
      current->value_type = &v.type();
      version = ++current->version;
    }

    emit parent_->SignalPublish(n, k, version, v);
    return true;
  }

  auto LookupKV(const Namespace& n, const Key& k) -> std::optional<std::any> {
    QStringList const segments = (n + "." + k).split('.');

    std::optional<std::any> rtn = std::nullopt;
    {
      std::shared_lock const lock(lock_);

      auto current = root_node_;
      for (const QString& segment : segments) {
        auto it = current->children.find(segment);
        if (it == current->children.end()) return std::nullopt;
        current = it.value();
      }
      rtn = current->value;
    }
    return rtn;
  }

  auto ListChildKeys(const Namespace& n, const Key& k) -> std::vector<Key> {
    QStringList const segments = (n + "." + k).split('.');

    std::vector<Key> rtn;
    {
      std::shared_lock lock(lock_);

      auto current = root_node_;
      for (const QString& segment : segments) {
        auto it = current->children.find(segment);
        if (it == current->children.end()) return {};
        current = it.value();
      }

      for (auto& key : current->children.keys()) rtn.emplace_back(key);
    }
    return rtn;
  }

  auto ListenPublish(QObject* o, const Namespace& n, const Key& k,
                     LPCallback c) -> bool {
    if (o == nullptr) return false;
    return QObject::connect(parent_, &GlobalRegisterTable::SignalPublish, o,
                            [n, k, c](const Namespace& pn, const Key& pk,
                                      int ver, std::any value) {
                              if (pn == n && pk == k) {
                                c(pn, pk, ver, std::move(value));
                              }
                            }) == nullptr;
  }

  auto RootRTNode() -> RTNodePtr { return root_node_; }

 private:
  std::shared_mutex lock_;
  GlobalRegisterTable* parent_;

  RTNodePtr root_node_;
};

class GlobalRegisterTableTreeModel::Impl {
 public:
  using RTNode = GlobalRegisterTable::Impl::RTNode;

  Impl(GlobalRegisterTableTreeModel* parent, GlobalRegisterTable::Impl* grt)
      : parent_(parent), root_node_(grt->RootRTNode()) {}

  [[nodiscard]] auto RowCount(const QModelIndex& parent) const -> int {
    auto* parent_node = !parent.isValid()
                            ? root_node_.get()
                            : static_cast<RTNode*>(parent.internalPointer());
    return parent_node->children.size();
  }

  [[nodiscard]] auto ColumnCount(const QModelIndex& parent) const -> int {
    return 4;
  }

  [[nodiscard]] auto Data(const QModelIndex& index,
                          int role) const -> QVariant {
    if (!index.isValid()) return {};

    if (role == Qt::DisplayRole) {
      auto* node = static_cast<RTNode*>(index.internalPointer());
      switch (index.column()) {
        case 0:
          return node->name;
        case 1:
          return node->type;
        case 2:
          return QString(node->value && node->value.has_value()
                             ? node->value->type().name()
                             : "");
        case 3:
          return Any2QVariant(node->value);
        default:
          return {};
      }
    }
    return {};
  }

  static auto Any2QVariant(std::optional<std::any> op) -> QVariant {
    if (!op) return tr("<EMPTY>");

    auto& o = op.value();
    if (o.type() == typeid(QString)) {
      return QVariant::fromValue(std::any_cast<QString>(o));
    }
    if (o.type() == typeid(std::string)) {
      return QVariant::fromValue(
          QString::fromStdString(std::any_cast<std::string>(o)));
    }
    if (o.type() == typeid(int)) {
      return QVariant::fromValue(std::any_cast<int>(o));
    }
    if (o.type() == typeid(long)) {
      return QVariant::fromValue(
          static_cast<qlonglong>(std::any_cast<long>(o)));
    }
    if (o.type() == typeid(long long)) {
      return QVariant::fromValue(std::any_cast<long long>(o));
    }
    if (o.type() == typeid(unsigned)) {
      return QVariant::fromValue(std::any_cast<unsigned>(o));
    }
    if (o.type() == typeid(unsigned long)) {
      return QVariant::fromValue(
          static_cast<qulonglong>(std::any_cast<unsigned long>(o)));
    }
    if (o.type() == typeid(unsigned long long)) {
      return QVariant::fromValue(std::any_cast<unsigned long long>(o));
    }
    if (o.type() == typeid(float)) {
      return QVariant::fromValue(std::any_cast<float>(o));
    }
    if (o.type() == typeid(double)) {
      return QVariant::fromValue(std::any_cast<double>(o));
    }
    if (o.type() == typeid(bool)) {
      return QVariant::fromValue(std::any_cast<bool>(o));
    }
    return tr("<UNSUPPORTED>");
  }

  [[nodiscard]] auto Index(int row, int column,
                           const QModelIndex& parent) const -> QModelIndex {
    if (!parent_->hasIndex(row, column, parent)) return {};

    auto* parent_node = !parent.isValid()
                            ? root_node_.get()
                            : static_cast<RTNode*>(parent.internalPointer());
    auto key = parent_node->children.keys().at(row);
    auto child_node = parent_node->children.value(key);
    return parent_->createIndex(row, column, child_node.get());
  }

  [[nodiscard]] auto Parent(const QModelIndex& index) const -> QModelIndex {
    if (!index.isValid()) return {};

    auto* child_node = static_cast<RTNode*>(index.internalPointer());
    auto parent_node = child_node->parent.lock();

    if (!parent_node || parent_node == root_node_) return {};

    int const row = static_cast<int>(
        parent_node->parent.lock()->children.keys().indexOf(parent_node->name));
    return parent_->createIndex(row, 0, parent_node.data());
  }

  [[nodiscard]] static auto HeaderData(int section, Qt::Orientation orientation,
                                       int role) -> QVariant {
    if (role != Qt::DisplayRole) return {};

    if (orientation == Qt::Horizontal) {
      switch (section) {
        case 0:
          return tr("Key");
        case 1:
          return tr("Type");
        case 2:
          return tr("Value Type");
        case 3:
          return tr("Value");
        default:
          return {};
      }
    }
    return {};
  }

 private:
  GlobalRegisterTableTreeModel* parent_;
  GlobalRegisterTable::Impl::RTNodePtr root_node_;
};

GlobalRegisterTable::GlobalRegisterTable()
    : p_(SecureCreateUniqueObject<Impl>(this)) {}

GlobalRegisterTable::~GlobalRegisterTable() = default;

auto GlobalRegisterTable::PublishKV(Namespace n, Key k, std::any v) -> bool {
  return p_->PublishKV(n, k, v);
}

auto GlobalRegisterTable::LookupKV(Namespace n,
                                   Key v) -> std::optional<std::any> {
  return p_->LookupKV(n, v);
}

auto GlobalRegisterTable::ListenPublish(QObject* o, Namespace n, Key k,
                                        LPCallback c) -> bool {
  return p_->ListenPublish(o, n, k, c);
}

auto GlobalRegisterTable::ListChildKeys(Namespace n,
                                        Key k) -> std::vector<Key> {
  return p_->ListChildKeys(n, k);
}

GlobalRegisterTableTreeModel::GlobalRegisterTableTreeModel(
    GlobalRegisterTable* grt, QObject* parent)
    : QAbstractItemModel(parent),
      p_(SecureCreateUniqueObject<Impl>(this, grt->p_.get())) {}

auto GlobalRegisterTableTreeModel::rowCount(const QModelIndex& parent) const
    -> int {
  return p_->RowCount(parent);
}

auto GlobalRegisterTableTreeModel::columnCount(const QModelIndex& parent) const
    -> int {
  return p_->ColumnCount(parent);
}

auto GlobalRegisterTableTreeModel::data(const QModelIndex& index,
                                        int role) const -> QVariant {
  return p_->Data(index, role);
}

auto GlobalRegisterTableTreeModel::index(
    int row, int column, const QModelIndex& parent) const -> QModelIndex {
  return p_->Index(row, column, parent);
}

auto GlobalRegisterTableTreeModel::parent(const QModelIndex& index) const
    -> QModelIndex {
  return p_->Parent(index);
}

auto GlobalRegisterTableTreeModel::headerData(int section,
                                              Qt::Orientation orientation,
                                              int role) const -> QVariant {
  return p_->HeaderData(section, orientation, role);
}
}  // namespace GpgFrontend::Module