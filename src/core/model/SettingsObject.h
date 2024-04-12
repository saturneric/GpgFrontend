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

#include "core/GpgFrontendCoreExport.h"

namespace GpgFrontend {

/**
 * @brief The SettingsObject class
 * This class is used to store data for the application securely.
 *
 */
class GPGFRONTEND_CORE_EXPORT SettingsObject : public QJsonObject {
 public:
  /**
   * @brief Construct a new Settings Object object
   *
   * @param settings_name The name of the settings object
   */
  explicit SettingsObject(QString settings_name);

  /**
   * @brief Construct a new Settings Object object
   *
   * @param _sub_json
   */
  explicit SettingsObject(QJsonObject sub_json);

  /**
   * @brief Destroy the Settings Object object
   *
   */
  ~SettingsObject();

  /**
   * @brief
   *
   */
  void Store(const QJsonObject&);

 private:
  QString settings_name_;  ///<
};
}  // namespace GpgFrontend
