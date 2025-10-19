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

#pragma once

#include "core/function/gpg/GpgCommandExecutor.h"

namespace GpgFrontend {
struct GpgAttrInfo {
  QString fpr;
  qint64 octets = 0;  // contains size in bytes
  int type = 0;       // 1=Image (Photo ID)
  int index = 0;      // starts at 0
  int count = 0;      // total number of attributes of this type
  qint64 ts = 0;      // timestamp
  qint64 exp = 0;     // expiration time
  int flags = 0;      // 0x01 primary uid, 0x02 revoked, 0x04 expired
  QByteArray blob;
  QString ext;
  QByteArray payload;
  bool valid = false;
};

class GF_CORE_EXPORT GpgAttributeHelper
    : public SingletonFunctionObject<GpgAttributeHelper> {
 public:
  /**
   * @brief Construct a new Gpg Attribute Helper object
   *
   * @param channel
   */
  explicit GpgAttributeHelper(int channel);

  /**
   * @brief Destroy the Gpg Attribute Helper object
   *
   */
  ~GpgAttributeHelper() override;

  /**
   * @brief Get the Attributes object
   *
   * @param key_id
   * @return QContainer<GpgAttrInfo>
   */
  auto GetAttributes(const QString& key_id) -> QContainer<GpgAttrInfo>;

 private:
  GpgCommandExecutor& gce_ = GpgCommandExecutor::GetInstance(GetChannel());
};

}  // namespace GpgFrontend