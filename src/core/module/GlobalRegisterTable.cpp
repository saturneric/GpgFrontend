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
#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

#include "spdlog/spdlog.h"

namespace GpgFrontend::Module {

class GlobalRegisterTable::Impl {
 public:
  struct Value {
    std::any value;
    int version = 0;
    const std::type_info& type;

    Value(std::any v) : value(v), type(v.type()) {}
  };

  Impl(GlobalRegisterTable* parent)
      : parent_(parent), global_register_table_() {}

  bool PublishKV(Namespace n, Key k, std::any v) {
    std::unique_lock lock(lock_);
    SPDLOG_DEBUG("publishing kv to rt, n: {}, k: {}, v type: {}", n, k,
                 v.type().name());

    auto& sub_table =
        global_register_table_.emplace(n, SubTable{}).first->second;

    auto sub_it = sub_table.find(k);
    if (sub_it == sub_table.end()) {
      sub_it = sub_table.emplace(k, std::make_unique<Value>(Value{v})).first;
      SPDLOG_DEBUG("new kv in rt, created n: {}, k: {}, v type: {}", n, k,
                   v.type().name());
    } else {
      if (sub_it->second->type != v.type()) {
        return false;
      }
      sub_it->second->value = v;
    }

    sub_it->second->version++;
    emit parent_->SignalPublish(n, k, sub_it->second->version);
    return true;
  }

  std::optional<std::any> LookupKV(Namespace n, Key k) {
    std::shared_lock lock(lock_);
    SPDLOG_DEBUG("looking up kv in rt, n: {}, k: {}", n, k);

    auto it = global_register_table_.find(n);
    if (it == global_register_table_.end()) return std::nullopt;

    auto& sub_table = it->second;
    auto sub_it = sub_table.find(k);
    return (sub_it != sub_table.end())
               ? std::optional<std::any>{sub_it->second->value}
               : std::nullopt;
  }

  bool ListenPublish(QObject* o, Namespace n, Key k, LPCallback c) {
    if (o == nullptr) return false;
    return QObject::connect(parent_, &GlobalRegisterTable::SignalPublish, o,
                            [n, k, c](Namespace pn, Key pk, int v) {
                              if (pn == n && pk == k) {
                                c(pn, pk, v);
                              }
                            }) == nullptr;
  }

 private:
  using SubTable = std::unordered_map<Key, std::unique_ptr<Value>>;
  using Table = std::map<Namespace, SubTable>;
  std::shared_mutex lock_;
  GlobalRegisterTable* parent_;

  Table global_register_table_;
};

GlobalRegisterTable::GlobalRegisterTable() : p_(std::make_unique<Impl>(this)) {}

GlobalRegisterTable::~GlobalRegisterTable() = default;

bool GlobalRegisterTable::PublishKV(Namespace n, Key k, std::any v) {
  return p_->PublishKV(n, k, v);
}

std::optional<std::any> GlobalRegisterTable::LookupKV(Namespace n, Key v) {
  return p_->LookupKV(n, v);
}

bool GlobalRegisterTable::ListenPublish(QObject* o, Namespace n, Key k,
                                        LPCallback c) {
  return p_->ListenPublish(o, n, k, c);
}

}  // namespace GpgFrontend::Module