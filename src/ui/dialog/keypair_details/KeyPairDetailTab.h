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

#include "core/model/GpgKey.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend::UI {

class KeyPairDetailTab : public QWidget {
  Q_OBJECT

 private slots:

  /**
   * @details Copy the fingerprint to clipboard
   */
  void slot_copy_fingerprint();

  /**
   * @brief
   *
   */
  void slot_refresh_key_info();

  /**
   * @brief
   *
   */
  void slot_refresh_key();

  /**
   * @brief
   *
   */
  void slot_query_key_publish_state();

  /**
   * @brief
   *
   */
  void slot_refresh_notice(const QString& icon, const QString& info);

 private:
  int current_gpg_context_channel_;
  GpgKeyPtr key_;  ///<

  QGroupBox* owner_box_;        ///< Groupbox containing owner information
  QGroupBox* key_box_;          ///< Groupbox containing key information
  QGroupBox* fingerprint_box_;  ///< Groupbox containing fingerprint information
  QGroupBox* additional_uid_box_;  ///< Groupbox containing information about
                                   ///< additional uids

  QLabel* name_var_label_;      ///< Label containing the keys name
  QLabel* email_var_label_;     ///< Label containing the keys email
  QLabel* comment_var_label_;   ///< Label containing the keys comment
  QLabel* key_size_var_label_;  ///< Label containing the keys key size
  QLabel* expire_var_label_;    ///< Label containing the keys expiration date
  QLabel* created_var_label_;   ///< Label containing the keys creation date
  QLabel* last_update_var_label_;  ///<
  QLabel* algorithm_var_label_;    ///< Label containing the keys algorithm
  QLabel*
      algorithm_detail_var_label_;  ///< containing the keys algorithm detail
  QLabel* key_id_var_label_;        ///< Label containing the keys keyid
  QLabel* fingerprint_var_label_;   ///< Label containing the keys fingerprint
  QLabel* usage_var_label_;
  QLabel* primary_key_exist_var_label_;
  QLabel* owner_trust_var_label_;

  QLabel* icon_label_;  ///<
  QLabel* exp_label_;   ///<

 public:
  /**
   * @brief Construct a new Key Pair Detail Tab object
   *
   * @param key_id
   * @param parent
   */
  explicit KeyPairDetailTab(int channel, GpgKeyPtr key,
                            QWidget* parent = nullptr);
};

}  // namespace GpgFrontend::UI
