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

#include "KeyNewUIDDialog.h"
#include "KeyUIDSignDialog.h"
#include "core/function/gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {

class KeyPairUIDTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key Pair U I D Tab object
   *
   * @param key_id
   * @param parent
   */
  KeyPairUIDTab(const std::string& key_id, QWidget* parent);

 signals:

  /**
   * @brief
   *
   */
  void SignalUpdateUIDInfo();

 private:
  GpgKey m_key_;
  QTableWidget* uid_list_{};                          ///<
  QTableWidget* sig_list_{};                          ///<
  QTabWidget* tofu_tabs_{};                           ///<
  QMenu* manage_selected_uid_menu_{};                 ///<
  QMenu* uid_popup_menu_{};                           ///<
  QMenu* sign_popup_menu_{};                          ///<
  std::vector<GpgUID> buffered_uids_;                 ///<
  std::vector<GpgKeySignature> buffered_signatures_;  ///<

  /**
   * @brief Create a uid list object
   *
   */
  void create_uid_list();

  /**
   * @brief Create a sign list object
   *
   */
  void create_sign_list();

  /**
   * @brief Create a manage uid menu object
   *
   */
  void create_manage_uid_menu();

  /**
   * @brief Create a uid popup menu object
   *
   */
  void create_uid_popup_menu();

  /**
   * @brief Create a sign popup menu object
   *
   */
  void create_sign_popup_menu();

  /**
   * @brief Get the uid checked object
   *
   * @return UIDArgsListPtr
   */
  UIDArgsListPtr get_uid_checked();

  /**
   * @brief Get the uid selected object
   *
   * @return UIDArgsListPtr
   */
  UIDArgsListPtr get_uid_selected();

  /**
   * @brief Get the sign selected object
   *
   * @return SignIdArgsListPtr
   */
  SignIdArgsListPtr get_sign_selected();

 private slots:

  /**
   * @brief
   *
   */
  void slot_refresh_uid_list();

  /**
   * @brief
   *
   */
  void slot_refresh_tofu_info();

  /**
   * @brief
   *
   */
  void slot_refresh_sig_list();

  /**
   * @brief
   *
   */
  void slot_add_sign();

  /**
   * @brief
   *
   */
  void slot_add_sign_single();

  /**
   * @brief
   *
   */
  void slot_add_uid();

  /**
   * @brief
   *
   */
  void slot_del_uid();

  /**
   * @brief
   *
   */
  void slot_del_uid_single();

  /**
   * @brief
   *
   */
  void slot_set_primary_uid();

  /**
   * @brief
   *
   */
  void slot_del_sign();

  /**
   * @brief
   *
   */
  void slot_refresh_key();

  /**
   * @brief
   *
   * @param result
   */
  static void slot_add_uid_result(int result);

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void contextMenuEvent(QContextMenuEvent* event) override;
};

}  // namespace GpgFrontend::UI
