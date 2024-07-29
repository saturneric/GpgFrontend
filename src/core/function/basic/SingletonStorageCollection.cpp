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

#include "SingletonStorageCollection.h"

#include <memory>
#include <shared_mutex>

#include "core/function/SecureMemoryAllocator.h"
#include "core/function/basic/SingletonStorage.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

SecureUniquePtr<SingletonStorageCollection> global_instance = nullptr;

class SingletonStorageCollection::Impl {
 public:
  /**
   * @brief Get the Instance object
   *
   * @return SingletonStorageCollection*
   */
  static auto GetInstance(bool force_refresh) -> SingletonStorageCollection* {
    if (force_refresh || global_instance == nullptr) {
      global_instance = SecureCreateUniqueObject<SingletonStorageCollection>();
      qCDebug(core,
              "a new global singleton storage collection created, address: %p",
              static_cast<void*>(global_instance.get()));
    }
    return global_instance.get();
  }

  /**
   * @brief Get the Instance object
   *
   * @return SingletonStorageCollection*
   */
  static void Destroy() { global_instance = nullptr; }

  /**
   * @brief Get the Singleton Storage object
   *
   * @param singleton_function_object
   * @return SingletonStorage*
   */
  auto GetSingletonStorage(const std::type_info& type_id) -> SingletonStorage* {
    const auto hash = type_id.hash_code();

    while (true) {
      decltype(storages_map_.end()) ins_it;
      {
        std::shared_lock<std::shared_mutex> lock(storages_mutex_);
        ins_it = storages_map_.find(hash);
      }
      if (ins_it == storages_map_.end()) {
        auto storage = SecureCreateUniqueObject<SingletonStorage>();
        {
          std::unique_lock<std::shared_mutex> lock(storages_mutex_);
          storages_map_.insert({hash, std::move(storage)});
        }
        continue;
      }
      return ins_it->second.get();
    }
  }

 private:
  std::shared_mutex storages_mutex_;  ///< mutex for storages_map_
  std::map<size_t, SingletonStoragePtr> storages_map_;
};

SingletonStorageCollection::SingletonStorageCollection() noexcept
    : p_(SecureCreateUniqueObject<Impl>()) {}

SingletonStorageCollection::~SingletonStorageCollection() = default;

auto GpgFrontend::SingletonStorageCollection::GetInstance(bool force_refresh)
    -> GpgFrontend::SingletonStorageCollection* {
  return Impl::GetInstance(force_refresh);
}

void SingletonStorageCollection::Destroy() {
  qCDebug(
      core,
      "global singleton storage collection is about to destroy, address: %p",
      static_cast<void*>(global_instance.get()));
  return SingletonStorageCollection::Impl::Destroy();
}

auto SingletonStorageCollection::GetSingletonStorage(
    const std::type_info& type_id) -> GpgFrontend::SingletonStorage* {
  return p_->GetSingletonStorage(type_id);
}

}  // namespace GpgFrontend