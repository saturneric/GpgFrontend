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

namespace GpgFrontend {

struct GPGFRONTEND_CORE_EXPORT GpgOpenPGPCard {
 public:
  QString reader;
  QString serial_number;
  QString card_type;
  int card_version;
  QString app_type;
  int app_version;
  int manufacturer_id;
  QString manufacturer;
  QString card_holder;
  QString display_language;
  QString display_sex;
  int sig_counter = 0;

  struct GpgCardKeyInfo {
    QString fingerprint;
    QDateTime created;
    QString grip;
    QString key_type;
    QString algo;
    QString usage;
  };

  QMap<int, GpgCardKeyInfo> card_keys_info;

  int kdf_do_enabled;
  struct UIFStatus {
    bool sign = false;
    bool encrypt = false;
    bool auth = false;
  } uif;

  int chv1_cached = -1;
  std::array<int, 3> chv_max_len = {-1, -1, -1};
  std::array<int, 3> chv_retry = {-1, -1, -1};

  struct ExtCapability {
    bool ki = false;
    bool aac = false;
    bool bt = false;
    bool kdf = false;
    int status_indicator = -1;
  } ext_cap;

  QMap<QString, QString> additional_card_infos;
  bool good = false;

  GpgOpenPGPCard() = default;

  explicit GpgOpenPGPCard(const QStringList& status);

 private:
  /**
   * @brief
   *
   * @param name
   * @param value
   */
  void parse_card_info(const QString& name, const QString& value);

  /**
   * @brief
   *
   * @param name
   * @param value
   */
  void parse_chv_status(const QString& value);

  /**
   * @brief
   *
   * @param value
   */
  void parse_ext_capability(const QString& value);

  /**
   * @brief
   *
   * @param value
   */
  void parse_kdf_status(const QString& value);

  /**
   * @brief
   *
   * @param name
   * @param value
   */
  void parse_uif(const QString& name, const QString& value);

  /**
   * @brief
   *
   * @param keyword
   * @param value
   */
  void parse_card_key_info(const QString& name, const QString& value);
};

}  // namespace GpgFrontend