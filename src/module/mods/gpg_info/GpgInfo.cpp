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

#include "GpgInfo.h"

GpgOptionsInfo::GpgOptionsInfo(const QJsonObject &j) {
  if (const auto v = j["name"]; v.isString()) name = v.toString();
  if (const auto v = j["flags"]; v.isString()) flags = v.toString();
  if (const auto v = j["level"]; v.isString()) level = v.toString();
  if (const auto v = j["description"]; v.isString()) description = v.toString();
  if (const auto v = j["type"]; v.isString()) type = v.toString();
  if (const auto v = j["alt_type"]; v.isString()) alt_type = v.toString();
  if (const auto v = j["argname"]; v.isString()) argname = v.toString();
  if (const auto v = j["default_value"]; v.isString()) {
    default_value = v.toString();
  }
  if (const auto v = j["argdef"]; v.isString()) argdef = v.toString();
  if (const auto v = j["value"]; v.isString()) value = v.toString();
}

auto GpgOptionsInfo::Json() const -> QJsonObject {
  QJsonObject j;
  j["name"] = name;
  j["flags"] = flags;
  j["level"] = level;
  j["description"] = description;
  j["type"] = type;
  j["alt_type"] = alt_type;
  j["argname"] = argname;
  j["default_value"] = default_value;
  j["argdef"] = argdef;
  j["value"] = value;
  return j;
}

auto GpgComponentInfo::Json() const -> QJsonObject {
  QJsonObject j;
  j["name"] = name;
  j["desc"] = desc;
  j["version"] = version;
  j["path"] = path;
  j["binary_checksum"] = binary_checksum;
  return j;
}

GpgComponentInfo::GpgComponentInfo(const QJsonObject &j) {
  if (const auto v = j["name"]; v.isString()) name = v.toString();
  if (const auto v = j["desc"]; v.isString()) desc = v.toString();
  if (const auto v = j["version"]; v.isString()) version = v.toString();
  if (const auto v = j["path"]; v.isString()) path = v.toString();
  if (const auto v = j["binary_checksum"]; v.isString()) {
    binary_checksum = v.toString();
  }
}
