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

#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Module {

class Event::Impl {
 public:
  Impl(QString event_id, std::initializer_list<ParameterInitializer> params,
       EventCallback callback)
      : event_identifier_(std::move(event_id)),
        callback_(std::move(callback)),
        callback_thread_(QThread::currentThread()) {
    for (const auto& param : params) {
      AddParameter(param);
    }
  }

  auto operator[](const QString& key) const -> std::optional<ParameterValue> {
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

  explicit operator QString() const { return event_identifier_; }

  auto GetIdentifier() -> EventIdentifier { return event_identifier_; }

  auto GetTriggerIdentifier() -> EventTriggerIdentifier {
    return trigger_uuid_;
  }

  void AddParameter(const QString& key, const QString& value) {
    data_[key] = value;
  }

  void AddParameter(const ParameterInitializer& param) {
    AddParameter(param.key, param.value);
  }

  void ExecuteCallback(ListenerIdentifier listener_id,
                       const DataObjectPtr& data_object) {
    if (callback_) {
      if (!QMetaObject::invokeMethod(
              callback_thread_,
              [callback = callback_, event_identifier = event_identifier_,
               listener_id, data_object]() {
                callback(event_identifier, listener_id, data_object);
              })) {
        qCWarning(core) << "failed to invoke callback for event: "
                        << event_identifier_
                        << " with listener:" << listener_id;
      }
    }
  }

  auto ToModuleEvent() -> GFModuleEvent* {
    auto* event =
        static_cast<GFModuleEvent*>(SecureMalloc(sizeof(GFModuleEvent)));

    event->id = GFStrDup(event_identifier_);
    event->trigger_id = GFStrDup(trigger_uuid_);

    GFModuleEventParam* l_param = nullptr;
    GFModuleEventParam* p_param;

    int index = 0;
    for (const auto& data : data_) {
      p_param = static_cast<GFModuleEventParam*>(
          SecureMalloc(sizeof(GFModuleEventParam)));
      if (index++ == 0) event->params = p_param;

      p_param->name = GFStrDup(data.first);
      p_param->value = GFStrDup(data.second);
      p_param->next = nullptr;

      if (l_param != nullptr) l_param->next = p_param;
      l_param = p_param;
    }

    return event;
  }

 private:
  EventIdentifier event_identifier_;
  EventTriggerIdentifier trigger_uuid_ = QUuid::createUuid().toString();
  std::map<QString, QString> data_;
  EventCallback callback_;
  QThread* callback_thread_ = nullptr;  ///<
};

Event::Event(const QString& event_id,
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

Event::Event::operator QString() const { return static_cast<QString>(*p_); }

auto Event::Event::GetIdentifier() -> EventIdentifier {
  return p_->GetIdentifier();
}

auto Event::Event::GetTriggerIdentifier() -> EventTriggerIdentifier {
  return p_->GetTriggerIdentifier();
}

void Event::AddParameter(const QString& key, const QString& value) {
  p_->AddParameter(key, value);
}

void Event::ExecuteCallback(ListenerIdentifier l_id, DataObjectPtr d_o) {
  p_->ExecuteCallback(std::move(l_id), d_o);
}

auto Event::ToModuleEvent() -> GFModuleEvent* { return p_->ToModuleEvent(); }

}  // namespace GpgFrontend::Module