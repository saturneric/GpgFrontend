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

#include "DataObject.h"

#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

class DataObject::Impl {
 public:
  Impl() = default;

  Impl(std::initializer_list<std::any> init_list) : params_(init_list) {}

  void AppendObject(const std::any& obj) { params_.push_back(obj); }

  auto GetParameter(size_t index) -> std::any {
    if (index >= params_.size()) {
      throw std::out_of_range("index out of range");
    }
    return params_[index];
  }

  auto GetObjectSize() -> size_t { return params_.size(); }

 private:
  QContainer<std::any> params_;
};

DataObject::DataObject() : p_(SecureCreateUniqueObject<Impl>()) {}

DataObject::DataObject(std::initializer_list<std::any> i)
    : p_(SecureCreateUniqueObject<Impl>(i)) {}

DataObject::~DataObject() = default;

DataObject::DataObject(DataObject&&) noexcept = default;

auto DataObject::operator[](size_t index) const -> std::any {
  return p_->GetParameter(index);
}

auto DataObject::GetParameter(size_t index) const -> std::any {
  return p_->GetParameter(index);
}

void DataObject::AppendObject(std::any obj) { return p_->AppendObject(obj); }

auto DataObject::GetObjectSize() const -> size_t { return p_->GetObjectSize(); }

void DataObject::Swap(DataObject& other) noexcept { std::swap(p_, other.p_); }

void DataObject::Swap(DataObject&& other) noexcept { p_ = std::move(other.p_); }

void swap(DataObject& a, DataObject& b) noexcept { a.Swap(b); }

}  // namespace GpgFrontend