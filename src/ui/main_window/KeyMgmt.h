/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef __KEYMGMT_H__
#define __KEYMGMT_H__

#include "ui/GpgFrontendUI.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"
#include "ui/dialog/import_export/KeyServerImportDialog.h"
#include "ui/dialog/key_generate/KeygenDialog.h"
#include "ui/dialog/keypair_details/KeyDetailsDialog.h"
#include "ui/main_window/GeneralMainWindow.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyMgmt : public GeneralMainWindow {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key Mgmt object
   *
   * @param parent
   */
  explicit KeyMgmt(QWidget* parent = nullptr);

 public slots:

  /**
   * @brief
   *
   */
  void SlotGenerateSubKey();

  /**
   * @brief
   *
   */
  void SlotExportKeyToKeyPackage();

  /**
   * @brief
   *
   */
  void SlotExportKeyToClipboard();

  /**
   * @brief
   *
   */
  void SlotExportAsOpenSSHFormat();

  /**
   * @brief
   *
   */
  void SlotDeleteSelectedKeys();

  /**
   * @brief
   *
   */
  void SlotDeleteCheckedKeys();

  /**
   * @brief
   *
   */
  void SlotGenerateKeyDialog();

  /**
   * @brief
   *
   */
  void SlotShowKeyDetails();

  /**
   * @brief
   *
   */
  void SlotImportKeyPackage();

 signals:

  /**
   * @brief
   *
   */
  void SignalStatusBarChanged(QString);

  /**
   * @brief
   *
   */
  void SignalKeyStatusUpdated();

 private:
  /**
   * @brief Create a menus object
   *
   */
  void create_menus();

  /**
   * @brief Create a actions object
   *
   */
  void create_actions();

  /**
   * @brief Create a tool bars object
   *
   */
  void create_tool_bars();

  /**
   * @brief
   *
   * @param uidList
   */
  void delete_keys_with_warning(GpgFrontend::KeyIdArgsListPtr uidList);

  KeyList* key_list_;                            ///<
  QMenu* file_menu_{};                           ///<
  QMenu* key_menu_{};                            ///<
  QMenu* generate_key_menu_{};                   ///<
  QMenu* import_key_menu_{};                     ///<
  QAction* open_key_file_act_{};                 ///<
  QAction* export_key_to_file_act_{};            ///<
  QAction* export_key_as_open_ssh_format_{};     ///<
  QAction* export_key_to_clipboard_act_{};       ///<
  QAction* delete_checked_keys_act_{};           ///<
  QAction* delete_selected_keys_act_{};          ///<
  QAction* generate_key_dialog_act_{};           ///<
  QAction* generate_key_pair_act_{};             ///<
  QAction* generate_subkey_act_{};               ///<
  QAction* import_key_from_clipboard_act_{};     ///<
  QAction* import_key_from_file_act_{};          ///<
  QAction* import_key_from_key_server_act_{};    ///<
  QAction* import_keys_from_key_package_act_{};  ///<
  QAction* close_act_{};                         ///<
  QAction* show_key_details_act_{};              ///<
  KeyServerImportDialog* import_dialog_{};       ///<
};

}  // namespace GpgFrontend::UI

#endif  // __KEYMGMT_H__
