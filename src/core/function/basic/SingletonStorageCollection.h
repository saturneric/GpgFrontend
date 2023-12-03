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

#pragma once

#include "core/function/SecureMemoryAllocator.h"

namespace GpgFrontend {
class SingletonStorage;

using SingletonStoragePtr =
    std::unique_ptr<SingletonStorage, SecureObjectDeleter<SingletonStorage>>;

class GPGFRONTEND_CORE_EXPORT SingletonStorageCollection {
 public:
  /**
   * @brief
   *
   */
  SingletonStorageCollection() noexcept;

  /**
   * @brief
   *
   */
  ~SingletonStorageCollection();

  /**
   * @brief Get the Instance object
   *
   * @return SingletonStorageCollection*
   */
  static auto GetInstance(bool force_refresh) -> SingletonStorageCollection*;

  /**
   * @brief
   *
   */
  static void Destroy();

  /**
   * @brief Get the Singleton Storage object
   *
   * @param singleton_function_object
   * @return SingletonStorage*
   */
  auto GetSingletonStorage(const std::type_info&) -> SingletonStorage*;

 private:
  class Impl;
  std::unique_ptr<Impl> p_;
};

}  // namespace GpgFrontend