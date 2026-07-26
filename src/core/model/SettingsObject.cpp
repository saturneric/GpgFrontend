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

#include "SettingsObject.h"

#include "core/function/DataObjectOperator.h"

namespace GpgFrontend {

SettingsObject::SettingsObject(QString settings_name)
    : settings_name_(std::move(settings_name)) {
  try {
    auto& op = DataObjectOperator::GetInstance();
    auto json_optional = op.GetDataObject(settings_name_);

    if (json_optional.has_value() && json_optional->isObject()) {
      QJsonObject::operator=(json_optional.value().object());
    } else {
      // An empty result is ambiguous: either there are no settings yet (first
      // start) or they are on disk and this session's key cannot open them.
      // Only the file's existence separates the two, and getting it wrong in
      // the second case means overwriting the user's only copy with defaults.
      load_failed_ = op.HasDataObj(settings_name_);
      if (load_failed_) {
        LOG_E() << "settings object exists but could not be read, it will be "
                   "treated as read-only for this session:"
                << settings_name_;
      }
      QJsonObject::operator=({});
    }

  } catch (std::exception& e) {
    LOG_W() << "load setting object error: {}" << e.what();
    // State unknown -- assume the worst and protect what is on disk.
    load_failed_ = true;
  }

  // Remember what we loaded so the destructor can skip the disk write when
  // nothing actually changed -- many call sites construct a SettingsObject
  // only to read a value, and rewriting the encrypted file on every such
  // destruction is a needless encrypt + full-file rewrite.
  original_ = *this;
}

SettingsObject::SettingsObject(QJsonObject sub_json)
    : QJsonObject(std::move(sub_json)) {}

SettingsObject::~SettingsObject() {
  if (settings_name_.isEmpty()) return;

  // Never write back over an object that could not be read. It is still on
  // disk under a key this session does not have, and replacing it with the
  // empty object we fell back to would discard the only copy for good.
  if (load_failed_ && !override_load_failure_) {
    LOG_W() << "settings object was unreadable at load, skip storing:"
            << settings_name_;
    return;
  }

  // Don't touch disk when nothing changed since load.
  if (static_cast<const QJsonObject&>(*this) == original_) return;

  DataObjectOperator::GetInstance().StoreDataObj(settings_name_,
                                                 QJsonDocument(*this));
}

auto SettingsObject::Store(const QJsonObject& json) -> bool {
  // Callers reconstruct defaults and store them unconditionally, so this is
  // the other half of the destructor's guard: without it the reconstructed
  // content would differ from original_ and be written out anyway.
  if (load_failed_ && !override_load_failure_) {
    LOG_E() << "refusing to overwrite unreadable settings object:"
            << settings_name_;
    return false;
  }

  auto* parent = (static_cast<QJsonObject*>(this));
  *parent = json;
  return true;
}

void SettingsObject::StoreOverridingUnreadable(const QJsonObject& json) {
  if (load_failed_) {
    LOG_W() << "resetting unreadable settings object on explicit request:"
            << settings_name_;
  }

  override_load_failure_ = true;
  auto* parent = (static_cast<QJsonObject*>(this));
  *parent = json;
}
}  // namespace GpgFrontend