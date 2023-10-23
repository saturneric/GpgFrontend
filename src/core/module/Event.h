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
#include <optional>

#include "core/GpgFrontendCore.h"

namespace GpgFrontend::Module {

class Event;

using EventRefrernce = std::shared_ptr<Event>;
using EventIdentifier = std::string;
using Evnets = std::vector<Event>;

class GPGFRONTEND_CORE_EXPORT Event {
 public:
  using ParameterValue = std::any;
  using EventIdentifier = std::string;
  struct ParameterInitializer {
    std::string key;
    ParameterValue value;
  };

  Event(const std::string& event_dientifier,
        std::initializer_list<ParameterInitializer> params_init_list = {});

  ~Event();

  std::optional<ParameterValue> operator[](const std::string& key) const;

  bool operator==(const Event& other) const;

  bool operator!=(const Event& other) const;

  bool operator<(const Event& other) const;

  bool operator<=(const Event& other) const;

  operator std::string() const;

  EventIdentifier GetIdentifier();

  void AddParameter(const std::string& key, const ParameterValue& value);

 private:
  class Impl;
  std::unique_ptr<Impl> p_;
};

template <typename... Args>
EventRefrernce MakeEvent(const EventIdentifier& event_id, Args&&... args) {
  std::initializer_list<Event::ParameterInitializer> params = {
      Event::ParameterInitializer{std::forward<Args>(args)}...};
  return std::make_shared<Event>(event_id, params);
}

}  // namespace GpgFrontend::Module