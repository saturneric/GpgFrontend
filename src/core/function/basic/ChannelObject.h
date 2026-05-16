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

#include "core/GpgConstants.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

/**
 * @brief Base class for all objects participating in the channel system.
 *
 * A "channel" is an integer namespace that allows independent singleton
 * instances of the same type to coexist. Every ChannelObject carries a
 * channel ID and a type name (used for debug logging). The default channel
 * is kGpgFrontendDefaultChannel.
 */
class GF_CORE_EXPORT ChannelObject {
 public:
  /**
   * @brief Construct a ChannelObject on the default channel.
   */
  ChannelObject() noexcept;

  /**
   * @brief Destroy the ChannelObject; logs the type name in debug builds.
   */
  virtual ~ChannelObject() noexcept;

  /**
   * @brief Construct a ChannelObject for the given channel and type name.
   *
   * @param channel integer channel identifier
   * @param type human-readable type name, used for debug logging
   */
  explicit ChannelObject(int channel, QString type);

  /**
   * @brief Return the default channel identifier (kGpgFrontendDefaultChannel).
   *
   * @return default channel ID
   */
  static auto GetDefaultChannel() -> int;

  /**
   * @brief Return the channel ID this object is associated with.
   *
   * @return channel ID
   */
  [[nodiscard]] auto GetChannel() const -> int;

  /**
   * @brief Override the channel ID on this object.
   *
   * Normally set once by SingletonStorage::SetObjectInChannel; use with care.
   *
   * @param channel new channel ID
   */
  void SetChannel(int channel);

 private:
  int channel_ = kGpgFrontendDefaultChannel;  ///< Channel identifier
  QString type_;                              ///< Type name for debug logging
};

/**
 * @brief Transfer ownership of a derived ChannelObject unique_ptr to a
 * base-typed unique_ptr.
 *
 * Releases the derived pointer and re-wraps it as a
 * SecureUniquePtr<ChannelObject> so that derived instances can be stored in
 * the channel registry. Asserts at compile time that Derived is a subclass
 * of ChannelObject.
 *
 * @tparam Derived a class derived from ChannelObject
 * @param derivedPtr source unique_ptr to transfer
 * @return base-typed unique_ptr that owns the same object
 */
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
