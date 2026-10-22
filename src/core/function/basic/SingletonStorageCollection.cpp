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

#include "SingletonStorageCollection.h"

#include <memory>

#include "core/function/basic/SingletonStorage.h"

namespace {
GpgFrontend::SecureUniquePtr<GpgFrontend::SingletonStorageCollection>
    global_instance = nullptr;
}

namespace GpgFrontend {

class SingletonStorageCollection::Impl {
 public:
  /**
   * @brief Get the Singleton Storage object
   *
   * @param singleton_function_object
   * @return SingletonStorage*
   */
  auto GetSingletonStorage(const std::type_info& type_id) -> SingletonStorage* {
    const auto hash = type_id.hash_code();

    // One lock across the lookup and the insert. An iterator taken under the
    // lock and used after it was dropped could be invalidated by a concurrent
    // insert rehashing the map. The storages themselves are never removed
    // before Destroy(), so the pointer stays valid after the lock goes.
    QMutexLocker lock(&storages_mutex_);
    auto ins_it = storages_map_.find(hash);
    if (ins_it == storages_map_.end()) {
      ins_it = storages_map_
                   .insert({hash, SecureCreateUniqueObject<SingletonStorage>()})
                   .first;
    }
    return ins_it->second.get();
  }

 private:
  QMutex storages_mutex_;  ///< mutex for storages_map_
  std::unordered_map<size_t, SingletonStoragePtr> storages_map_;
};

SingletonStorageCollection::SingletonStorageCollection() noexcept
    : p_(SecureCreateUniqueObject<Impl>()) {}

SingletonStorageCollection::~SingletonStorageCollection() = default;

auto GpgFrontend::SingletonStorageCollection::GetInstance()
    -> GpgFrontend::SingletonStorageCollection* {
  if (global_instance == nullptr) {
    global_instance = SecureCreateUniqueObject<SingletonStorageCollection>();
    FLOG_D("a new global singleton storage collection created, address: %p",
           static_cast<void*>(global_instance.get()));
  }
  return global_instance.get();
}

void SingletonStorageCollection::Destroy() {
  LOG_D()
      << "global singleton storage collection is about to destroy, address: "
      << static_cast<void*>(global_instance.get());
  global_instance->p_.reset();
  global_instance.reset();
}

auto SingletonStorageCollection::GetSingletonStorage(
    const std::type_info& type_id) -> GpgFrontend::SingletonStorage* {
  return p_->GetSingletonStorage(type_id);
}

}  // namespace GpgFrontend