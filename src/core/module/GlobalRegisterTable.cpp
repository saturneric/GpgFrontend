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

#include "GlobalRegisterTableTreeModel.h"
#include "core/utils/MemoryUtils.h"
#include "utils/MemoryUtils.h"

namespace GpgFrontend::Module {

class GlobalRegisterTable::Impl {
 public:
  struct RTNode {
    QString name;
    QString type = "NODE";
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
        root_node_(SecureCreateSharedObject<RTNode>("", nullptr)) {}

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
              segment, SecureCreateSharedObject<RTNode>(segment, current));
        }
        current = it.value();
      }

      current->name = segments.back();
      current->type = "LEAF";
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

  auto ListChildKeys(const Namespace& n, const Key& k) -> QContainer<Key> {
    QStringList const segments = (n + "." + k).split('.');

    QContainer<Key> rtn;
    {
      std::shared_lock lock(lock_);

      auto current = root_node_;
      for (const QString& segment : segments) {
        auto it = current->children.find(segment);
        if (it == current->children.end()) return {};
        current = it.value();
      }

      for (auto& key : current->children.keys()) rtn.push_back(key);
    }
    return rtn;
  }

  auto ListenPublish(QObject* o, const Namespace& n, const Key& k,
                     const LPCallback& c) -> bool {
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

  /**
   * @brief Run @p f over the node tree while holding the read lock.
   *
   * The tree is mutated from module threads, so any traversal outside this
   * class must go through here.
   */
  template <typename F>
  void ReadLocked(F&& f) {
    std::shared_lock const lock(lock_);
    f(root_node_);
  }

 private:
  std::shared_mutex lock_;
  GlobalRegisterTable* parent_;

  RTNodePtr root_node_;
};

class GlobalRegisterTableTreeModel::Impl {
 public:
  using RTNode = GlobalRegisterTable::Impl::RTNode;

  /**
   * @brief Immutable copy of one register table node, safe to read from the
   * GUI thread while modules keep publishing.
   */
  struct DisplayNode {
    QString name;
    QString path;  ///< full dotted path
    bool leaf = false;
    int version = 0;
    QString value_type;
    QVariant value;
    QVector<QSharedPointer<DisplayNode>> children;
    DisplayNode* parent = nullptr;
    int row = 0;
  };

  using DisplayNodePtr = QSharedPointer<DisplayNode>;

  Impl(GlobalRegisterTableTreeModel* parent, GlobalRegisterTable::Impl* grt)
      : parent_(parent), grt_(grt) {
    Rebuild();

    // modules publish in bursts, coalesce them into one model reset
    refresh_timer_.setSingleShot(true);
    refresh_timer_.setInterval(kRefreshCoalesceMs);
    QObject::connect(&refresh_timer_, &QTimer::timeout, parent_,
                     [this]() { parent_->Refresh(); });
  }

  /**
   * @brief Request a refresh, coalescing bursts of publishes.
   */
  void ScheduleRefresh() {
    if (!refresh_timer_.isActive()) refresh_timer_.start();
  }

  /**
   * @brief Take a fresh snapshot of the register table under its read lock.
   */
  void Rebuild() {
    auto root = SecureCreateSharedObject<DisplayNode>();

    grt_->ReadLocked([&](const GlobalRegisterTable::Impl::RTNodePtr& rt_root) {
      CopyChildren(rt_root, root.get(), {});
    });

    root_ = root;
  }

  [[nodiscard]] auto RowCount(const QModelIndex& parent) const -> int {
    const auto* parent_node = NodeOf(parent);
    return parent_node == nullptr
               ? 0
               : static_cast<int>(parent_node->children.size());
  }

  [[nodiscard]] static auto ColumnCount(const QModelIndex& /*parent*/) -> int {
    return 5;
  }

  [[nodiscard]] auto Data(const QModelIndex& index, int role) const
      -> QVariant {
    if (!index.isValid()) return {};

    const auto* node = static_cast<DisplayNode*>(index.internalPointer());
    if (node == nullptr) return {};

    if (role == Qt::DisplayRole) {
      switch (index.column()) {
        case 0:
          return node->name;
        case 1:
          return node->leaf ? tr("Leaf") : tr("Namespace");
        case 2:
          return node->leaf ? node->value_type : QString();
        case 3:
          return node->leaf ? node->value : QVariant();
        case 4:
          return node->leaf ? QVariant(node->version) : QVariant();
        default:
          return {};
      }
    }

    if (role == kGRTPathRole) return node->path;
    if (role == kGRTValueRole) return node->value;

    if (role == Qt::ToolTipRole) {
      if (!node->leaf) return node->path;
      return QString("%1\n%2: %3")
          .arg(node->path, node->value_type, node->value.toString());
    }

    return {};
  }

  /**
   * @brief Human readable name of the type currently held by a value.
   */
  static auto Any2TypeName(const std::optional<std::any>& op) -> QString {
    if (!op || !op->has_value()) return tr("Empty");

    const auto& o = op.value();
    if (o.type() == typeid(QString) || o.type() == typeid(std::string)) {
      return tr("String");
    }
    if (o.type() == typeid(bool)) return tr("Boolean");
    if (o.type() == typeid(int) || o.type() == typeid(long) ||
        o.type() == typeid(long long) || o.type() == typeid(unsigned) ||
        o.type() == typeid(unsigned long) ||
        o.type() == typeid(unsigned long long)) {
      return tr("Integer");
    }
    if (o.type() == typeid(float) || o.type() == typeid(double)) {
      return tr("Number");
    }
    return tr("Unsupported");
  }

  static auto Any2QVariant(std::optional<std::any> op) -> QVariant {
    if (!op) return "<EMPTY>";

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

  [[nodiscard]] auto Index(int row, int column, const QModelIndex& parent) const
      -> QModelIndex {
    if (!parent_->hasIndex(row, column, parent)) return {};

    const auto* parent_node = NodeOf(parent);
    if (parent_node == nullptr || row >= parent_node->children.size()) {
      return {};
    }

    return parent_->createIndex(row, column,
                                parent_node->children.at(row).get());
  }

  [[nodiscard]] auto Parent(const QModelIndex& index) const -> QModelIndex {
    if (!index.isValid()) return {};

    auto* child_node = static_cast<DisplayNode*>(index.internalPointer());
    if (child_node == nullptr) return {};

    auto* parent_node = child_node->parent;
    if (parent_node == nullptr || parent_node == root_.get()) return {};

    return parent_->createIndex(parent_node->row, 0, parent_node);
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
        case 4:
          return tr("Version");
        default:
          return {};
      }
    }
    return {};
  }

 private:
  static constexpr int kRefreshCoalesceMs = 200;

  GlobalRegisterTableTreeModel* parent_;
  GlobalRegisterTable::Impl* grt_;
  DisplayNodePtr root_;
  QTimer refresh_timer_;

  /**
   * @brief Snapshot node behind an index, or the root for an invalid index.
   */
  [[nodiscard]] auto NodeOf(const QModelIndex& index) const
      -> const DisplayNode* {
    if (!index.isValid()) return root_.get();
    return static_cast<const DisplayNode*>(index.internalPointer());
  }

  /**
   * @brief Deep-copy the children of @p source into @p target.
   *
   * Must be called with the register table's read lock held.
   */
  static void CopyChildren(const GlobalRegisterTable::Impl::RTNodePtr& source,
                           DisplayNode* target, const QString& parent_path) {
    if (source == nullptr || target == nullptr) return;

    auto row = 0;
    for (const auto& key : source->children.keys()) {
      const auto& child = source->children.value(key);
      if (child == nullptr) continue;

      auto node = SecureCreateSharedObject<DisplayNode>();
      node->name = child->name;
      node->path =
          parent_path.isEmpty() ? child->name : parent_path + "." + child->name;
      node->leaf = child->type == "LEAF";
      node->version = child->version;
      node->value_type = Any2TypeName(child->value);
      node->value = node->leaf ? Any2QVariant(child->value) : QVariant();
      node->parent = target;
      node->row = row++;

      CopyChildren(child, node.get(), node->path);
      target->children.push_back(node);
    }
  }
};

GlobalRegisterTable::GlobalRegisterTable()
    : p_(SecureCreateUniqueObject<Impl>(this)) {}

GlobalRegisterTable::~GlobalRegisterTable() = default;

auto GlobalRegisterTable::PublishKV(Namespace n, Key k, std::any v) -> bool {
  return p_->PublishKV(n, k, v);
}

auto GlobalRegisterTable::LookupKV(Namespace n, Key v)
    -> std::optional<std::any> {
  return p_->LookupKV(n, v);
}

auto GlobalRegisterTable::ListenPublish(QObject* o, Namespace n, Key k,
                                        LPCallback c) -> bool {
  return p_->ListenPublish(o, n, k, c);
}

auto GlobalRegisterTable::ListChildKeys(Namespace n, Key k) -> QContainer<Key> {
  return p_->ListChildKeys(n, k);
}

GlobalRegisterTableTreeModel::GlobalRegisterTableTreeModel(
    GlobalRegisterTable* grt, QObject* parent)
    : QAbstractItemModel(parent),
      p_(SecureCreateUniqueObject<Impl>(this, grt->p_.get())) {
  // modules publish from their own threads, hop back to this model's thread
  connect(
      grt, &GlobalRegisterTable::SignalPublish, this,
      [this](const Namespace&, const Key&, int, const std::any&) {
        p_->ScheduleRefresh();
      },
      Qt::QueuedConnection);
}

GlobalRegisterTableTreeModel::~GlobalRegisterTableTreeModel() = default;

void GlobalRegisterTableTreeModel::Refresh() {
  beginResetModel();
  p_->Rebuild();
  endResetModel();
}

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

auto GlobalRegisterTableTreeModel::index(int row, int column,
                                         const QModelIndex& parent) const
    -> QModelIndex {
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