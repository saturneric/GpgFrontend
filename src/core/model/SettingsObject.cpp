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

#include "SettingsObject.h"

#include "core/function/DataObjectOperator.h"

namespace GpgFrontend {

SettingsObject::SettingsObject(QString settings_name)
    : settings_name_(std::move(settings_name)) {
  try {
    auto json_optional =
        DataObjectOperator::GetInstance().GetDataObject(settings_name_);

    if (json_optional.has_value() && json_optional->isObject()) {
      QJsonObject::operator=(json_optional.value().object());
    } else {
      QJsonObject::operator=({});
    }

  } catch (std::exception& e) {
    qCWarning(core) << "load setting object error: {}" << e.what();
  }
}

SettingsObject::SettingsObject(QJsonObject sub_json)
    : QJsonObject(std::move(sub_json)) {}

SettingsObject::~SettingsObject() {
  if (!settings_name_.isEmpty()) {
    DataObjectOperator::GetInstance().SaveDataObj(settings_name_,
                                                  QJsonDocument(*this));
  }
}

void SettingsObject::Store(const QJsonObject& json) {
  auto* parent = (static_cast<QJsonObject*>(this));
  *parent = json;
}
}  // namespace GpgFrontend