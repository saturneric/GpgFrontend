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

/**
 * @brief A named, parameterized event in the module system.
 *
 * Each Event carries a string identifier, an optional key-value parameter map
 * (Params), and an optional callback. The callback is always invoked on the
 * thread that constructed the event. A unique trigger UUID is generated for
 * every instance so that individual dispatches can be tracked.
 */
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

  /**
   * @brief Construct an event with the given identifier, optional parameters,
   * and optional callback.
   *
   * @param event_id string identifier for this event type
   * @param params initial key-value parameters (default: empty)
   * @param callback function invoked on the construction thread when
   * ExecuteCallback() is called (default: none)
   */
  explicit Event(const QString& event_id, const Params& params = {},
                 EventCallback callback = nullptr);

  ~Event();

  /**
   * @brief Look up a parameter value by key.
   *
   * @param key parameter key
   * @return the value if found, or empty if the key is absent
   */
  auto operator[](const QString& key) const -> std::optional<ParameterValue>;

  /**
   * @brief Return true if both events refer to the same underlying object.
   */
  auto operator==(const Event& other) const -> bool;

  /**
   * @brief Return true if the events refer to different underlying objects.
   */
  auto operator!=(const Event& other) const -> bool;

  /**
   * @brief Order events by their implementation pointer (for use in sorted
   * containers).
   */
  auto operator<(const Event& other) const -> bool;

  auto operator<=(const Event& other) const -> bool;

  /**
   * @brief Return the event identifier as a string.
   */
  explicit operator QString() const;

  /**
   * @brief Return the event type identifier (e.g. "core.KeyUpdated").
   *
   * @return event identifier string
   */
  auto GetIdentifier() -> EventIdentifier;

  /**
   * @brief Return the unique trigger UUID generated for this dispatch instance.
   *
   * @return trigger identifier string (UUID)
   */
  auto GetTriggerIdentifier() -> EventTriggerIdentifier;

  /**
   * @brief Add or update a key-value parameter on the event.
   *
   * @param key parameter key
   * @param value parameter value
   */
  void AddParameter(const QString& key, const GFBuffer& value);

  /**
   * @brief Invoke the event callback on the construction thread.
   *
   * Delivers @p listener_id and @p params to the callback via
   * QMetaObject::invokeMethod to ensure cross-thread safety.
   *
   * @param listener_id identifier of the module that handled the event
   * @param params result parameters to pass back to the caller
   */
  void ExecuteCallback(ListenerIdentifier listener_id, const Params& params);

  /**
   * @brief Serialize the event to a C-ABI GFModuleEvent struct for SDK use.
   *
   * The returned pointer is heap-allocated with SMAMalloc and must be freed
   * by the caller.
   *
   * @return pointer to the allocated GFModuleEvent struct
   */
  auto ToModuleEvent() -> GFModuleEvent*;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

/**
 * @brief Construct an EventReference (shared Event) with the given parameters.
 *
 * @tparam Args unused (kept for API compatibility)
 * @param event_id event type identifier
 * @param params key-value parameters
 * @param e_cb optional callback invoked when the event is handled
 * @return shared pointer to the new Event
 */
template <typename... Args>
auto MakeEvent(const EventIdentifier& event_id, const Event::Params& params,
               Event::EventCallback e_cb) -> EventReference {
  return GpgFrontend::SecureCreateSharedObject<Event>(event_id, params, e_cb);
}

// A no-op EventCallback suitable as a default when no callback is needed.
const Event::EventCallback kEmptyEventCallback =
    [](const Event::EventIdentifier&, const Event::ListenerIdentifier&,
       const Event::Params&) {};

}  // namespace GpgFrontend::Module
