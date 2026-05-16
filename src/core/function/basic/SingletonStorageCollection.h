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

#pragma once

#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {
class SingletonStorage;

using SingletonStoragePtr =
    std::unique_ptr<SingletonStorage, SecureObjectDeleter<SingletonStorage>>;

/**
 * @brief Process-wide registry that maps type hash codes to their
 * SingletonStorage.
 *
 * This is the root of the channel-based singleton system. Each distinct type
 * gets its own SingletonStorage, lazily created on first access. The global
 * instance is managed via GetInstance() and Destroy(); both are not thread-safe
 * with respect to each other and should only be called from the main thread
 * at startup and shutdown.
 */
class GF_CORE_EXPORT SingletonStorageCollection {
 public:
  /**
   * @brief Construct the collection with an empty storage map.
   */
  SingletonStorageCollection() noexcept;

  /**
   * @brief Destroy the collection and all contained SingletonStorage instances.
   */
  ~SingletonStorageCollection();

  /**
   * @brief Return the global SingletonStorageCollection instance, creating it
   * if needed.
   *
   * @return pointer to the global instance; never null after the first call
   */
  static auto GetInstance() -> SingletonStorageCollection*;

  /**
   * @brief Destroy the global instance, releasing all singleton storage and
   * instances.
   *
   * Must be called from the main thread during application shutdown, after all
   * channels have been released.
   */
  static void Destroy();

  /**
   * @brief Return (or lazily create) the SingletonStorage for the given type.
   *
   * Thread-safe; a new empty SingletonStorage is created on first access for
   * any given type.
   *
   * @param type_id type_info of the singleton type
   * @return pointer to the SingletonStorage for that type; never null
   */
  auto GetSingletonStorage(const std::type_info& type_id) -> SingletonStorage*;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend
