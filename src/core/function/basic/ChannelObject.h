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

#include "core/GpgConstants.h"
#include "core/function/SecureMemoryAllocator.h"
namespace GpgFrontend {

/**
 * @brief object which in channel system is called "channel"
 *
 */
class GPGFRONTEND_CORE_EXPORT ChannelObject {
 public:
  /**
   * @brief Construct a new Default Channel Object object
   *
   */
  ChannelObject() noexcept;

  /**
   * @brief Destroy the Channel Object object
   *
   */
  virtual ~ChannelObject() noexcept;

  /**
   * @brief Construct a new Channel Object object
   *
   * @param channel
   */
  explicit ChannelObject(int channel);

  /**
   * @brief Get the Default Channel object
   *
   * @return int
   */
  static auto GetDefaultChannel() -> int;

  /**
   * @brief Get the Channel object
   *
   * @return int
   */
  [[nodiscard]] auto GetChannel() const -> int;

  /**
   * @brief Set the Channel object
   *
   * @param channel
   */
  void SetChannel(int channel);

 private:
  int channel_ = kGpgFrontendDefaultChannel;  ///< The channel id
};

template <typename Derived>
auto ConvertToChannelObjectPtr(
    std::unique_ptr<Derived, SecureObjectDeleter<Derived>> derivedPtr)
    -> std::unique_ptr<ChannelObject, SecureObjectDeleter<ChannelObject>> {
  static_assert(std::is_base_of_v<ChannelObject, Derived>,
                "Derived must be a subclass of ChannelObject");

  ChannelObject* base_ptr = derivedPtr.release();
  return std::unique_ptr<ChannelObject, SecureObjectDeleter<ChannelObject>>(
      base_ptr);
}

}  // namespace GpgFrontend