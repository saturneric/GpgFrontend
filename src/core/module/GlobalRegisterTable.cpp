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

namespace GpgFrontend::Module {

class GlobalRegisterTable::Impl {
 public:
  struct Value {
    std::any value;
    int version = 0;
    const std::type_info& type;

    Value(std::any v) : value(v), type(v.type()) {}
  };

  Impl() : global_register_table_() {}

  bool PublishKV(Namespace n, Key k, std::any v) {
    std::unique_lock lock(lock_);
    auto& sub_table =
        global_register_table_.emplace(n, SubTable{}).first->second;

    auto sub_it = sub_table.find(k);
    if (sub_it == sub_table.end()) {
      sub_it = sub_table.emplace(k, std::make_unique<Value>(Value{v})).first;
    } else {
      if (sub_it->second->type != v.type()) {
        return false;
      }
      sub_it->second->value = v;
    }

    sub_it->second->version++;
    return true;
  }

  std::optional<std::any> LookupKV(Namespace n, Key k) {
    std::shared_lock lock(lock_);
    auto it = global_register_table_.find(n);
    if (it == global_register_table_.end()) return std::nullopt;

    auto& sub_table = it->second;
    auto sub_it = sub_table.find(k);
    return (sub_it != sub_table.end())
               ? std::optional<std::any>{sub_it->second->value}
               : std::nullopt;
  }

 private:
  using SubTable = std::unordered_map<Key, std::unique_ptr<Value>>;
  using Table = std::map<Namespace, SubTable>;
  std::shared_mutex lock_;

  Table global_register_table_;
};

GlobalRegisterTable::GlobalRegisterTable() : p_(std::make_unique<Impl>()) {}

GlobalRegisterTable::~GlobalRegisterTable() = default;

bool GlobalRegisterTable::PublishKV(Namespace n, Key k, std::any v) {
  return p_->PublishKV(n, k, v);
}

std::optional<std::any> GlobalRegisterTable::LookupKV(Namespace n, Key v) {
  return p_->LookupKV(n, v);
}

}  // namespace GpgFrontend::Module