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

#include "GpgOpenPGPCard.h"

#include "core/model/GpgCardKeyPairInfo.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

void GpgFrontend::GpgOpenPGPCard::parse_card_info(const QString& name,
                                                  const QString& value) {
  if (name == "APPVERSION") {
    app_version = ParseHexEncodedVersionTuple(value);
  } else if (name == "CARDTYPE") {
    card_type = value;
  } else if (name == "CARDVERSION") {
    card_version = ParseHexEncodedVersionTuple(value);
  } else if (name == "DISP-NAME") {
    auto list = value.split(QStringLiteral("<<"), Qt::SkipEmptyParts);
    std::reverse(list.begin(), list.end());
    card_holder =
        list.join(QLatin1Char(' ')).replace(QLatin1Char('<'), QLatin1Char(' '));
  } else if (name == "KEYPAIRINFO") {
    const GpgCardKeyPairInfo info = GpgCardKeyPairInfo(value);
    if (info.grip.isEmpty()) {
      LOG_W() << "invalid KEYPAIRINFO status line" << value;
      good = false;
    }
  } else if (name == "KEY-FPR") {
    const auto values = value.split(QLatin1Char(' '));
    if (values.size() < 2) {
      LOG_W() << "invalid KEY-FPR status line" << value;
      good = false;
      return;
    }

    const auto& key_number = values[0].toInt();
    const auto& fpr = values[1];
    fprs.insert(key_number, fpr);

  } else if (name == "MANUFACTURER") {
    // the value of MANUFACTURER is the manufacturer ID as unsigned number
    // optionally followed by the name of the manufacturer, e.g.
    // 6 Yubico
    // 65534 unmanaged S/N range
    // for PKCS#15 cards the manufacturer ID is always 0, e.g.
    // 0 www.atos.net/cardos [R&S]
    auto space_index = value.indexOf(' ');
    if (space_index != -1) {
      card_infos.insert(name, value.mid(space_index + 1).trimmed());
    }
  } else {
    card_infos.insert(name, value);
  }
}
GpgOpenPGPCard::GpgOpenPGPCard(const QStringList& status) : good(true) {
  for (const QString& line : status) {
    auto tokens = line.split(' ', Qt::SkipEmptyParts);
    auto name = tokens.value(0);
    auto value = tokens.mid(1).join(' ');

    parse_card_info(name, value);
  }
}
}  // namespace GpgFrontend
