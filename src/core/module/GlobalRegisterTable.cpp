/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace GpgFrontend::Module {

class GlobalRegisterTable::Impl {
 public:
  struct RTNode {
    std::optional<std::any> value = std::nullopt;
    std::unordered_map<std::string, std::unique_ptr<RTNode>> children;
    int version = 0;
    const std::type_info* type = nullptr;
  };

  explicit Impl(GlobalRegisterTable* parent) : parent_(parent) {}

  auto PublishKV(const Namespace& n, const Key& k, std::any v) -> bool {
    std::istringstream iss(k);
    std::string segment;

    int version = 0;

    {
      std::unique_lock lock(lock_);
      auto& root_rt_node =
          global_register_table_.emplace(n, std::make_unique<RTNode>())
              .first->second;

      RTNode* current = root_rt_node.get();
      while (std::getline(iss, segment, '.')) {
        current = current->children.emplace(segment, std::make_unique<RTNode>())
                      .first->second.get();
      }

      current->value = v;
      current->type = &v.type();
      current->version++;
    }

    emit parent_->SignalPublish(n, k, version, v);
    return true;
  }

  auto LookupKV(const Namespace& n, const Key& k) -> std::optional<std::any> {
    std::istringstream iss(k);
    std::string segment;

    std::optional<std::any> rtn = std::nullopt;
    {
      std::shared_lock lock(lock_);
      auto it = global_register_table_.find(n);
      if (it == global_register_table_.end()) return std::nullopt;

      RTNode* current = it->second.get();
      while (std::getline(iss, segment, '.')) {
        auto it = current->children.find(segment);
        if (it == current->children.end()) return std::nullopt;
        current = it->second.get();
      }
      rtn = current->value;
    }
    return rtn;
  }

  auto ListChildKeys(const Namespace& n, const Key& k) -> std::vector<Key> {
    std::istringstream iss(k);
    std::string segment;

    std::vector<Key> rtn;
    {
      std::shared_lock lock(lock_);
      auto it = global_register_table_.find(n);
      if (it == global_register_table_.end()) return {};

      RTNode* current = it->second.get();
      while (std::getline(iss, segment, '.')) {
        auto it = current->children.find(segment);
        if (it == current->children.end()) return {};
        current = it->second.get();
      }

      for (auto& it : current->children) {
        rtn.emplace_back(it.first);
      }
    }
    return rtn;
  }

  auto ListenPublish(QObject* o, const Namespace& n, const Key& k, LPCallback c)
      -> bool {
    if (o == nullptr) return false;
    return QObject::connect(parent_, &GlobalRegisterTable::SignalPublish, o,
                            [n, k, c](const Namespace& pn, const Key& pk,
                                      int ver, std::any value) {
                              if (pn == n && pk == k) {
                                c(pn, pk, ver, std::move(value));
                              }
                            }) == nullptr;
  }

 private:
  using Table = std::map<Namespace, std::unique_ptr<RTNode>>;
  std::shared_mutex lock_;
  GlobalRegisterTable* parent_;

  Table global_register_table_;
};

GlobalRegisterTable::GlobalRegisterTable() : p_(std::make_unique<Impl>(this)) {}

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

auto GlobalRegisterTable::ListChildKeys(Namespace n, Key k)
    -> std::vector<Key> {
  return p_->ListChildKeys(n, k);
}

}  // namespace GpgFrontend::Module