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

#include "Event.h"

#include <utility>

namespace GpgFrontend::Module {

class Event::Impl {
 public:
  Impl(std::string event_id, std::initializer_list<ParameterInitializer> params,
       EventCallback callback)
      : event_identifier_(std::move(event_id)),
        callback_(std::move(callback)),
        callback_thread_(QThread::currentThread()) {
    for (const auto& param : params) {
      AddParameter(param);
    }
    SPDLOG_DEBUG("create event {}", event_identifier_);
  }

  auto operator[](const std::string& key) const
      -> std::optional<ParameterValue> {
    auto it_data = data_.find(key);
    if (it_data != data_.end()) {
      return it_data->second;
    }
    return std::nullopt;
  }

  auto operator==(const Event& other) const -> bool {
    return event_identifier_ == other.p_->event_identifier_;
  }

  auto operator!=(const Event& other) const -> bool {
    return !(*this == other);
  }

  auto operator<(const Event& other) const -> bool {
    return this->event_identifier_ < other.p_->event_identifier_;
  }

  explicit operator std::string() const { return event_identifier_; }

  auto GetIdentifier() -> EventIdentifier { return event_identifier_; }

  void AddParameter(const std::string& key, const ParameterValue& value) {
    data_[key] = value;
  }

  void AddParameter(const ParameterInitializer& param) {
    AddParameter(param.key, param.value);
  }

  void ExecuteCallback(ListenerIdentifier listener_id,
                       const DataObjectPtr& data_object) {
    SPDLOG_DEBUG("try to execute callback for event {} with listener {}",
                 event_identifier_, listener_id);
    if (callback_) {
      SPDLOG_DEBUG("executing callback for event {} with listener {}",
                   event_identifier_, listener_id);
      if (!QMetaObject::invokeMethod(
              callback_thread_,
              [callback = callback_, event_identifier = event_identifier_,
               listener_id, data_object]() {
                callback(event_identifier, listener_id, data_object);
              })) {
        SPDLOG_ERROR("failed to invoke callback for event {} with listener {}",
                     event_identifier_, listener_id);
      }
    }
  }

 private:
  EventIdentifier event_identifier_;
  std::map<std::string, ParameterValue> data_;
  EventCallback callback_;
  QThread* callback_thread_ = nullptr;  ///<
};

Event::Event(const std::string& event_id,
             std::initializer_list<ParameterInitializer> params,
             EventCallback callback)
    : p_(SecureCreateUniqueObject<Impl>(event_id, params,
                                        std::move(callback))) {}

Event::~Event() = default;

auto Event::Event::operator==(const Event& other) const -> bool {
  return this->p_ == other.p_;
}

auto Event::Event::operator!=(const Event& other) const -> bool {
  return this->p_ != other.p_;
}

auto Event::Event::operator<(const Event& other) const -> bool {
  return this->p_ < other.p_;
}

Event::Event::operator std::string() const {
  return static_cast<std::string>(*p_);
}

auto Event::Event::GetIdentifier() -> EventIdentifier {
  return p_->GetIdentifier();
}

void Event::AddParameter(const std::string& key, const ParameterValue& value) {
  p_->AddParameter(key, value);
}

void Event::ExecuteCallback(ListenerIdentifier l_id, DataObjectPtr d_o) {
  p_->ExecuteCallback(std::move(l_id), d_o);
}

}  // namespace GpgFrontend::Module