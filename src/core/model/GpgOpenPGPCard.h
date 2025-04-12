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

#include "core/model/GpgCardKeyPairInfo.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

struct GPGFRONTEND_CORE_EXPORT GpgOpenPGPCard {
 public:
  QString reader;
  QString serial_number;
  QString card_type;
  int card_version;
  QString app_type;
  int app_version;
  QString ext_capability;
  QString manufacturer;
  QString card_holder;
  QString display_language;
  QString display_sex;
  QString chv_status;
  int sig_counter = 0;

  QContainer<GpgCardKeyPairInfo> keys;
  QMap<int, QString> fprs;
  QMap<QString, QString> card_infos;

  QString kdf;
  QString uif1;
  QString uif2;
  QString uif3;

  bool good = false;

  GpgOpenPGPCard() = default;

  explicit GpgOpenPGPCard(const QStringList& status);

 private:
  void parse_card_info(const QString& name, const QString& value);
};

}  // namespace GpgFrontend