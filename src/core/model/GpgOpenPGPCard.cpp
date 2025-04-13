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
  } else if (name == "KEYPAIRINFO" || name == "KEY-FPR" || name == "KEY-TIME") {
    parse_card_key_info(name, value);
  } else if (name == "MANUFACTURER") {
    const auto values = value.split(QLatin1Char(' '));
    if (values.size() < 2) return;
    manufacturer_id = values.front().toInt();
    manufacturer = values.back();
  } else if (name == "DISP-SEX") {
    display_sex = value == "1" ? "Male" : value == "2" ? "Female" : "Unknown";
  } else if (name == "CHV-STATUS") {
    parse_chv_status(value);
  } else if (name == "EXTCAP") {
    parse_ext_capability(value);
  } else if (name == "KDF") {
    parse_kdf_status(value);
  } else if (name.startsWith("UIF-")) {
    parse_uif(name, value);
  } else {
    additional_card_infos.insert(name, value);
  }

  reader = additional_card_infos.value("READER").replace('+', ' ');
  serial_number = additional_card_infos.value("SERIALNO");
  app_type = additional_card_infos.value("APPTYPE");
  display_language = additional_card_infos.value("DISP-LANG");
}

GpgOpenPGPCard::GpgOpenPGPCard(const QStringList& status) : good(true) {
  for (const QString& line : status) {
    auto tokens = line.split(' ', Qt::SkipEmptyParts);
    auto name = tokens.value(0);
    auto value = tokens.mid(1).join(' ');

    parse_card_info(name, value);
  }
}

void GpgOpenPGPCard::parse_chv_status(const QString& value) {
  auto tokens = value.trimmed().split('+', Qt::SkipEmptyParts);

  int index = 0;

  if (index < tokens.size()) chv1_cached = tokens[index++].toInt();

  // chv_max_len[3]
  for (int i = 0; i < 3 && index < tokens.size(); ++i) {
    chv_max_len[i] = tokens[index++].toInt();
  }

  // chv_retry[3]
  for (int i = 0; i < 3 && index < tokens.size(); ++i) {
    chv_retry[i] = tokens[index++].toInt();
  }
}

void GpgOpenPGPCard::parse_ext_capability(const QString& value) {
  auto parts = value.trimmed().split("+");

  for (const QString& part : parts) {
    auto equal_pos = part.indexOf('=');
    if (equal_pos == -1) continue;

    auto key = part.left(equal_pos).trimmed();
    auto value = part.mid(equal_pos + 1).trimmed();

    bool ok = false;
    int ivalue = value.toInt(&ok);

    if (key == "ki") {
      ext_cap.ki = (ivalue == 1);
    } else if (key == "aac") {
      ext_cap.aac = (ivalue == 1);
    } else if (key == "bt") {
      ext_cap.bt = (ivalue == 1);
    } else if (key == "kdf") {
      ext_cap.kdf = (ivalue == 1);
    } else if (key == "si" && ok) {
      ext_cap.status_indicator = ivalue;
    }
  }
}

void GpgOpenPGPCard::parse_kdf_status(const QString& value) {
  auto decoded = QByteArray::fromPercentEncoding(value.toUtf8());

  if (decoded.size() < 23) {
    kdf_do_enabled = 0;
    return;
  }

  if (static_cast<quint8>(decoded[2]) != 0x03) {
    kdf_do_enabled = 0;
  } else if (static_cast<quint8>(decoded[22]) != 0x85) {
    kdf_do_enabled = 1;
  } else {
    kdf_do_enabled = 2;
  }
}

void GpgOpenPGPCard::parse_uif(const QString& name, const QString& value) {
  auto index = name.back().digitValue() - 1;
  if (index < 0 || index > 2) return;

  auto decoded = QByteArray::fromPercentEncoding(value.toUtf8());
  bool enabled = !decoded.isEmpty() && static_cast<quint8>(decoded[0]) != 0xFF;

  switch (index) {
    case 0:
      uif.sign = enabled;
      break;
    case 1:
      uif.encrypt = enabled;
      break;
    case 2:
      uif.auth = enabled;
      break;
  }
}

void GpgOpenPGPCard::parse_card_key_info(const QString& name,
                                         const QString& value) {
  if (name == "KEY-FPR") {
    auto tokens = value.split(' ');
    if (tokens.size() >= 2) {
      int no = tokens[0].toInt();
      card_keys_info[no].fingerprint = tokens[1].toUpper();
    }
  } else if (name == "KEY-TIME") {
    auto tokens = value.split(' ');
    if (tokens.size() >= 2) {
      int no = tokens.front().toInt();
      qint64 ts = tokens.back().toLongLong();
      card_keys_info[no].created =
          QDateTime::fromSecsSinceEpoch(ts, QTimeZone::UTC);
    }
  } else if (name == "KEYPAIRINFO") {
    auto tokens = value.split(' ');
    if (tokens.size() < 2) return;

    auto key_type_tokens = tokens[1].split('.');
    if (key_type_tokens.size() < 2) return;

    int no = key_type_tokens[1].toInt();
    card_keys_info[no].key_type = key_type_tokens[0];
    card_keys_info[no].grip = tokens[0].toUpper();

    if (tokens.size() >= 3) {
      card_keys_info[no].usage = tokens[2].toUpper();
    }

    if (tokens.size() >= 5) {
      card_keys_info[no].algo = tokens[4].toUpper();
    }
  }
}
}  // namespace GpgFrontend
