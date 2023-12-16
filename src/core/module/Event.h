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

#include <any>
#include <functional>
#include <optional>

#include "core/GpgFrontendCore.h"
#include "core/model/DataObject.h"

namespace GpgFrontend::Module {

class Event;

using EventRefrernce = std::shared_ptr<Event>;
using EventIdentifier = std::string;
using Evnets = std::vector<Event>;

class GPGFRONTEND_CORE_EXPORT Event {
 public:
  using ParameterValue = std::any;
  using EventIdentifier = std::string;
  using ListenerIdentifier = std::string;
  using EventCallback =
      std::function<void(EventIdentifier, ListenerIdentifier, DataObjectPtr)>;
  struct ParameterInitializer {
    std::string key;
    ParameterValue value;
  };

  explicit Event(const std::string&,
                 std::initializer_list<ParameterInitializer> = {},
                 EventCallback = nullptr);

  ~Event();

  auto operator[](const std::string& key) const
      -> std::optional<ParameterValue>;

  auto operator==(const Event& other) const -> bool;

  auto operator!=(const Event& other) const -> bool;

  auto operator<(const Event& other) const -> bool;

  auto operator<=(const Event& other) const -> bool;

  explicit operator std::string() const;

  auto GetIdentifier() -> EventIdentifier;

  void AddParameter(const std::string& key, const ParameterValue& value);

  void ExecuteCallback(ListenerIdentifier, DataObjectPtr);

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

template <typename... Args>
auto MakeEvent(const EventIdentifier& event_id, Args&&... args,
               Event::EventCallback e_cb) -> EventRefrernce {
  std::initializer_list<Event::ParameterInitializer> params = {
      Event::ParameterInitializer{std::forward<Args>(args)}...};
  return GpgFrontend::SecureCreateSharedObject<Event>(event_id, params, e_cb);
}

}  // namespace GpgFrontend::Module