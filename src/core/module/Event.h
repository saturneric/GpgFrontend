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

#include <any>
#include <functional>
#include <optional>

#include "core/model/GFBuffer.h"
#include "core/typedef/CoreTypedef.h"

struct GFModuleEvent;

namespace GpgFrontend::Module {

class Event;

using EventReference = QSharedPointer<Event>;
using EventIdentifier = QString;
using EventTriggerIdentifier = QString;
using Events = QContainer<Event>;

class GF_CORE_EXPORT Event {
 public:
  using ParameterValue = std::any;
  using EventIdentifier = QString;
  using ListenerIdentifier = QString;
  using Params = QMap<QString, GFBuffer>;

  using EventCallback =
      std::function<void(EventIdentifier, ListenerIdentifier, Params)>;
  struct ParameterInitializer {
    QString key;
    GFBuffer value;
  };

  explicit Event(const QString&, const Params& = {}, EventCallback = nullptr);

  ~Event();

  auto operator[](const QString& key) const -> std::optional<ParameterValue>;

  auto operator==(const Event& other) const -> bool;

  auto operator!=(const Event& other) const -> bool;

  auto operator<(const Event& other) const -> bool;

  auto operator<=(const Event& other) const -> bool;

  explicit operator QString() const;

  auto GetIdentifier() -> EventIdentifier;

  auto GetTriggerIdentifier() -> EventTriggerIdentifier;

  void AddParameter(const QString& key, const GFBuffer& value);

  void ExecuteCallback(ListenerIdentifier, const Params&);

  auto ToModuleEvent() -> GFModuleEvent*;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

template <typename... Args>
auto MakeEvent(const EventIdentifier& event_id, const Event::Params& params,
               Event::EventCallback e_cb) -> EventReference {
  return GpgFrontend::SecureCreateSharedObject<Event>(event_id, params, e_cb);
}

const Event::EventCallback kEmptyEventCallback =
    [](const Event::EventIdentifier&, const Event::ListenerIdentifier&,
       const Event::Params&) {};

}  // namespace GpgFrontend::Module