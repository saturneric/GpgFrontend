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

#include <QJsonObject>
#include <QString>
#include <map>

/**
 * @brief  Use to record some info about gnupg
 *
 */
class GpgInfo {
 public:
  QString GnuPGHomePath;  ///< value of ---homedir

  std::map<QString, std::vector<QString>> ComponentsInfo;        ///<
  std::map<QString, std::vector<QString>> ConfigurationsInfo;    ///<
  std::map<QString, std::vector<QString>> OptionsInfo;           ///<
  std::map<QString, std::vector<QString>> AvailableOptionsInfo;  ///<
};

/**
 * @brief  Use to record some info about gnupg components
 *
 */
struct GpgComponentInfo {
  QString name;
  QString desc;
  QString version;
  QString path;
  QString binary_checksum;

  GpgComponentInfo() = default;

  explicit GpgComponentInfo(const QJsonObject &j);

  [[nodiscard]] auto Json() const -> QJsonObject;
};

/**
 * The format of each line is:
 * name:flags:level:description:type:alt-type:argname:default:argdef:value
 */
struct GpgOptionsInfo {
  QString name;
  QString flags;
  QString level;
  QString description;
  QString type;
  QString alt_type;
  QString argname;
  QString default_value;
  QString argdef;
  QString value;

  GpgOptionsInfo() = default;

  explicit GpgOptionsInfo(const QJsonObject &j);

  [[nodiscard]] auto Json() const -> QJsonObject;
};
