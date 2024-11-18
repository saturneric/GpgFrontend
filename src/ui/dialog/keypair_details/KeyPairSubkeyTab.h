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

#include "KeySetExpireDateDialog.h"
#include "core/function/gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"
#include "ui/dialog/key_generate/SubkeyGenerateDialog.h"

namespace GpgFrontend::UI {

class KeyPairSubkeyTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key Pair Subkey Tab object
   *
   * @param key
   * @param parent
   */
  KeyPairSubkeyTab(int channel, const QString& key, QWidget* parent);

 private:
  /**
   * @brief Create a subkey list object
   *
   */
  void create_subkey_list();

  /**
   * @brief Create a subkey opera menu object
   *
   */
  void create_subkey_opera_menu();

  /**
   * @brief Get the selected subkey object
   *
   * @return const GpgSubKey&
   */
  const GpgSubKey& get_selected_subkey();

  int current_gpg_context_channel_;
  GpgKey key_;                               ///<
  QTableWidget* subkey_list_{};              ///<
  std::vector<GpgSubKey> buffered_subkeys_;  ///<

  QGroupBox* list_box_;    ///<
  QGroupBox* detail_box_;  ///<

  QMenu* subkey_opera_menu_{};  ///<

  QLabel* key_type_var_label_;
  QLabel* key_size_var_label_;   ///< Label containing the keys key size
  QLabel* expire_var_label_;     ///< Label containing the keys expiration date
  QLabel* created_var_label_;    ///< Label containing the keys creation date
  QLabel* algorithm_var_label_;  ///< Label containing the keys algorithm
  QLabel* algorithm_detail_var_label_;  ///<
  QLabel* key_id_var_label_;            ///< Label containing the keys keyid
  QLabel* fingerprint_var_label_;  ///< Label containing the keys fingerprint
  QLabel* usage_var_label_;        ///<
  QLabel* master_key_exist_var_label_;  ///<
  QLabel* card_key_label_;              ///<

  QPushButton* export_subkey_button_;
  QAction* export_subkey_act_;

  QAction* edit_subkey_act_;

 private slots:

  /**
   * @brief
   *
   */
  void slot_add_subkey();

  /**
   * @brief
   *
   */
  void slot_refresh_subkey_list();

  /**
   * @brief
   *
   */
  void slot_refresh_subkey_detail();

  /**
   * @brief
   *
   */
  void slot_edit_subkey();

  /**
   * @brief
   *
   */
  void slot_revoke_subkey();

  /**
   * @brief
   *
   */
  void slot_refresh_key_info();

  /**
   * @brief
   *
   */
  void slot_export_subkey();

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void contextMenuEvent(QContextMenuEvent* event) override;
};

}  // namespace GpgFrontend::UI
