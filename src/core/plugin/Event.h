/**
 * Copyright (C) 2021 Saturneric
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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPGFRONTEND_EVENT_H
#define GPGFRONTEND_EVENT_H

#include "core/GpgFrontendCore.h"

namespace GpgFrontend::Plugin {

class Event;

using EventRefrernce = std::shared_ptr<Event>;
using EventIdentifier = std::string;
using Evnets = std::vector<Event>;

class Event {
 public:
  class ParameterBase {
   public:
    virtual ~ParameterBase() = default;
  };

  struct ParameterInitializer {
    std::string key;
    std::shared_ptr<ParameterBase> value;
  };

  Event(const std::string& event_dientifier,
        std::initializer_list<ParameterInitializer> params_init_list = {});

  template <typename T>
  std::optional<T> operator[](const std::string& key) const {
    return GetParameter<T>(key);
  }

  bool operator==(const Event& other) const;
  bool operator!=(const Event& other) const;
  bool operator<(const Event& other) const;
  bool operator<=(const Event& other) const;
  operator std::string() const;

  EventIdentifier GetIdentifier();

  template <typename T>
  void AddParameter(const std::string& key, const T& value) {
    data_[key] = std::make_shared<ParameterValue<T>>(value);
  }

  void AddParameter(const ParameterInitializer& init) {
    data_[init.key] = init.value;
  }

  template <typename T>
  std::optional<T> GetParameter(const std::string& key) const {
    if (data_.find(key) == data_.end()) {
      throw std::nullopt;
    }
    auto value = std::dynamic_pointer_cast<ParameterValue<T>>(data_.at(key));
    if (!value) {
      throw std::nullopt;
    }
    return value->GetValue();
  }

 private:
  template <typename T>
  class ParameterValue : public ParameterBase {
   public:
    ParameterValue(const T& value) : value_(value) {}

    T GetValue() const { return value_; }

   private:
    T value_;
  };

  EventIdentifier event_identifier_;
  std::map<std::string, std::shared_ptr<ParameterBase>> data_;
};

}  // namespace GpgFrontend::Plugin

#endif  // GPGFRONTEND_EVENT_H