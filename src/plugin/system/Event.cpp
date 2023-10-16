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

#include <memory>

namespace GpgFrontend::Plugin {

class Event::Impl {
 public:
  Impl(const std::string& event_dientifier,
       std::initializer_list<ParameterInitializer> params_init_list = {})
      : event_identifier_(event_dientifier) {
    for (const auto& param : params_init_list) {
      AddParameter(param);
    }
  }

  std::optional<ParameterValue> operator[](const std::string& key) const {
    auto it = data_.find(key);
    if (it != data_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  bool operator==(const Event& other) const {
    return event_identifier_ == other.p_->event_identifier_;
  }

  bool operator!=(const Event& other) const { return !(*this == other); }

  bool operator<(const Event& other) const {
    return this->event_identifier_ < other.p_->event_identifier_;
  }

  operator std::string() const { return event_identifier_; }

  EventIdentifier GetIdentifier() { return event_identifier_; }

  void AddParameter(const std::string& key, const ParameterValue& value) {
    data_[key] = value;
  }

  void AddParameter(ParameterInitializer param) {
    AddParameter(param.key, param.value);
  }

 private:
  EventIdentifier event_identifier_;
  std::map<std::string, ParameterValue> data_;
};

Event::Event(const std::string& event_dientifier,
             std::initializer_list<ParameterInitializer> params_init_list)
    : p_(std::make_unique<Impl>(event_dientifier, params_init_list)) {}

Event::~Event() = default;

bool Event::Event::operator==(const Event& other) const {
  return this->p_ == other.p_;
}

bool Event::Event::operator!=(const Event& other) const {
  return this->p_ != other.p_;
}

bool Event::Event::operator<(const Event& other) const {
  return this->p_ < other.p_;
}

Event::Event::operator std::string() const {
  return static_cast<std::string>(*p_);
}

EventIdentifier Event::Event::GetIdentifier() { return p_->GetIdentifier(); }

void Event::AddParameter(const std::string& key, const ParameterValue& value) {
  p_->AddParameter(key, value);
}

}  // namespace GpgFrontend::Plugin