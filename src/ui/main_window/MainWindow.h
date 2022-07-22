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

#ifndef __GPGWIN_H__
#define __GPGWIN_H__

#include "core/GpgConstants.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "ui/GpgFrontendUI.h"
#include "KeyMgmt.h"
#include "ui/dialog/WaitingDialog.h"
#include "ui/dialog/Wizard.h"
#include "ui/help/AboutDialog.h"
#include "ui/import_export/KeyUploadDialog.h"
#include "ui/settings/SettingsDialog.h"
#include "ui/widgets/FindWidget.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/TextEdit.h"
#include "ui/main_window/GeneralMainWindow.h"

namespace GpgFrontend::UI {
/**
 * @brief
 *
 */
class MainWindow : public GeneralMainWindow {
  Q_OBJECT

 public:

  /**
   *
   */
  struct CryptoMenu {
    using OperationType = unsigned int;

    static constexpr OperationType None = 0;
    static constexpr OperationType Encrypt = 1 << 0;
    static constexpr OperationType Sign = 1 << 1;
    static constexpr OperationType Decrypt = 1 << 2;
    static constexpr OperationType Verify = 1 << 3;
    static constexpr OperationType EncryptAndSign = 1 << 4;
    static constexpr OperationType DecryptAndVerify = 1 << 5;
  };

  /**
   * @brief
   *
   */
  MainWindow();

  /**
   * @details ONLY Called from main()
   */
  void Init() noexcept;

  /**
   * @details refresh and enable specify crypto-menu actions.
   */
  void SetCryptoMenuStatus(CryptoMenu::OperationType type);

 signals:

  /**
   * @brief
   */
  void SignalLoaded();

 public slots:

  /**
   * @brief
   */
  void SlotSetStatusBarText(const QString& text);

 protected:
  /**
   * @details Close event shows a save dialog, if there are unsaved documents on
   * exit.
   * @param event
   */
  void closeEvent(QCloseEvent* event) override;

 public slots:

  /**
   * @details Open a new tab for path
   */
  void SlotOpenFile(QString& path);

  /**
   * @details Open dialog for encrypting file.
   */
  void SlotFileEncrypt();

  /**
   * @details Open dialog for decrypting file.
   */
  void SlotFileDecrypt();

  /**
   * @details Open dialog for signing file.
   */
  void SlotFileSign();

  /**
   * @details Open dialog for verifying file.
   */
  void SlotFileVerify();

  /**
   * @details Open dialog for signing file.
   */
  void SlotFileEncryptSign();

  /**
   * @details Open dialog for verifying file.
   */
  void SlotFileDecryptVerify();

  /**
   * @details get value of member restartNeeded to needed.
   * @param needed true, if application has to be restarted
   */
  void SlotSetRestartNeeded(bool needed);

 private slots:

  /**
   * @details encrypt the text of currently active textedit-page
   * with the currently checked keys
   */
  void slot_encrypt();

  /**
   * @details encrypt and sign the text of currently active textedit-page
   * with the currently checked keys
   */
  void slot_encrypt_sign();

  /**
   * @details Show a passphrase dialog and decrypt the text of currently active
   * tab.
   */
  void slot_decrypt();

  /**
   * @details Sign the text of currently active tab with the checked private
   * keys
   */
  void slot_sign();

  /**
   * @details Verify the text of currently active tab and show verify
   * information. If document is signed with a key, which is not in keylist,
   * show import missing key from keyserver in Menu of verifynotification.
   */
  void slot_verify();

  /**
   * @details decrypt and verify the text of currently active textedit-page
   * with the currently checked keys
   */
  void slot_decrypt_verify();

  /**
   * @details Show the details of the first of the first of selected keys
   */
  void slot_show_key_details();

  /**
   * @details Refresh key information of selected keys from default keyserver
   */
  void refresh_keys_from_key_server();

  /**
   * @details upload the selected key to the keyserver
   */
  void upload_key_to_server();

  /**
   * @details Open find widget.
   */
  void slot_find();

  /**
   * @details start the wizard
   */
  void slot_start_wizard();

  /**
   * @details Import keys from currently active tab to keylist if possible.
   */
  void slot_import_key_from_edit();

  /**
   * @details Append the selected keys to currently active textedit.
   */
  void slot_append_selected_keys();

  /**
   * @details Copy the mailaddress of selected key to clipboard.
   * Method for keylists contextmenu.
   */
  void slot_copy_mail_address_to_clipboard();

  /**
   * @details Open key management dialog.
   */
  void slot_open_key_management();

  /**
   * @details Open File Opera Tab
   */
  void slot_open_file_tab();

  /**
   * @details Open settings-dialog.
   */
  void slot_open_settings_dialog();

  /**
   * @details Replace double linebreaks by single linebreaks in currently active
   * tab.
   */
  void slot_clean_double_line_breaks();

  /**
   * @details Cut the existing PGP header and footer from current tab.
   */
  void slot_cut_pgp_header();

  /**
   * @details Add PGP header and footer to current tab.
   */
  void slot_add_pgp_header();

  /**
   * @details Disable tab related actions, if number of tabs is 0.
   * @param number number of the opened tabs and -1, if no tab is opened
   */
  void slot_disable_tab_actions(int number);

  /**
   * @details called when need to upgrade.
   */
  void slot_version_upgrade(const SoftwareVersion& version);

 private:
  /**
   * @details Create actions for the main-menu and the context-menu of the
   * keylist.
   */
  void create_actions();

  /**
   * @details create the menu of the main-window.
   */
  void create_menus();

  /**
   * @details Create edit-, crypt- and key-toolbars.
   */
  void create_tool_bars();

  /**
   * @details Create statusbar of mainwindow.
   */
  void create_status_bar();

  /**
   * @details Create keylist- and attachment-dockwindows.
   */
  void create_dock_windows();

  /**
   * @details Create attachment dock window.
   */
  void create_attachment_dock();

  /**
   * @details close attachment-dockwindow.
   */
  void close_attachment_dock();

  /**
   * @details Load settings from ini-file.
   */
  void restore_settings();

  /**
   * @details Save settings to ini-file.
   */
  void save_settings();

  /**
   * @brief return true, if restart is needed
   */
  [[nodiscard]] bool get_restart_needed() const;

  TextEdit* edit_{};          ///< Tabwidget holding the edit-windows
  QMenu* file_menu_{};        ///<  Submenu for file-operations
  QMenu* edit_menu_{};        ///<  Submenu for text-operations
  QMenu* crypt_menu_{};       ///<  Submenu for crypt-operations
  QMenu* help_menu_{};        ///<  Submenu for help-operations
  QMenu* key_menu_{};         ///<  Submenu for key-operations
  QMenu* view_menu_{};        ///<  Submenu for view operations
  QMenu* import_key_menu_{};  ///<  Submenu for import operations
#ifdef SMTP_SUPPORT
  QMenu* email_menu_{};  ///<  Submenu for email operations
#endif

  QMenu* steganography_menu_{};  ///<  Submenu for steganography operations
  QToolBar* crypt_tool_bar_{};   ///<  Toolbar holding crypt actions
  QToolBar* file_tool_bar_{};    ///<  Toolbar holding file actions
  QToolBar* edit_tool_bar_{};    ///<  Toolbar holding edit actions
  QToolBar*
      special_edit_tool_bar_{};  ///<  Toolbar holding special edit actions
  QToolBar* key_tool_bar_{};     ///<  Toolbar holding key operations
  QToolButton*
      import_button_{};  ///<  Tool button for import dropdown menu in toolbar
  QDockWidget* key_list_dock_{};    ///<  Encrypt Dock
  QDockWidget* attachment_dock_{};  ///<  Attachment Dock
  QDockWidget* info_board_dock_{};

  QAction* new_tab_act_{};               ///<  Action to create new tab
  QAction* switch_tab_up_act_{};         ///<  Action to switch tab up
  QAction* switch_tab_down_act_{};       ///<  Action to switch tab down
  QAction* open_act_{};                  ///<  Action to open file
  QAction* browser_act_{};               ///<  Action to open file browser
  QAction* save_act_{};                  ///<  Action to save file
  QAction* save_as_act_{};               ///<  Action to save file as
  QAction* print_act_{};                 ///<  Action to print
  QAction* close_tab_act_{};             ///<  Action to print
  QAction* quit_act_{};                  ///<  Action to quit application
  QAction* encrypt_act_{};               ///<  Action to encrypt text
  QAction* encrypt_sign_act_{};          ///<  Action to encrypt and sign text
  QAction* decrypt_verify_act_{};        ///<  Action to encrypt and sign text
  QAction* decrypt_act_{};               ///<  Action to decrypt text
  QAction* sign_act_{};                  ///<  Action to sign text
  QAction* verify_act_{};                ///<  Action to verify text
  QAction* import_key_from_edit_act_{};  ///<  Action to import key from edit
  QAction* clean_double_line_breaks_act_{};  ///<  Action to remove double line
                                             ///<  breaks

  QAction*
      append_selected_keys_act_{};  ///< Action to append selected keys to edit
  QAction* copy_mail_address_to_clipboard_act_{};  ///< Action to copy mail to
                                                   ///< clipboard
  QAction* open_key_management_act_{};   ///< Action to open key management
  QAction* copy_act_{};                  ///< Action to copy text
  QAction* quote_act_{};                 ///< Action to quote text
  QAction* cut_act_{};                   ///< Action to cut text
  QAction* paste_act_{};                 ///< Action to paste text
  QAction* select_all_act_{};            ///< Action to select whole text
  QAction* find_act_{};                  ///< Action to find text
  QAction* undo_act_{};                  ///< Action to undo last action
  QAction* redo_act_{};                  ///< Action to redo last action
  QAction* zoom_in_act_{};               ///< Action to zoom in
  QAction* zoom_out_act_{};              ///< Action to zoom out
  QAction* about_act_{};                 ///< Action to open about dialog
  QAction* check_update_act_{};          ///< Action to open about dialog
  QAction* translate_act_{};             ///< Action to open about dialog
  QAction* open_settings_act_{};         ///< Action to open settings dialog
  QAction* show_key_details_act_{};      ///< Action to open key-details dialog
  QAction* start_wizard_act_{};          ///< Action to open the wizard
  QAction* cut_pgp_header_act_{};        ///< Action for cutting the PGP header
  QAction* add_pgp_header_act_{};        ///< Action for adding the PGP header
  QAction* import_key_from_file_act_{};  ///<
  QAction* import_key_from_clipboard_act_{};   ///<
  QAction* import_key_from_key_server_act_{};  ///<

  QLabel* status_bar_icon_{};  ///<

  KeyList* m_key_list_{};          ///<
  InfoBoardWidget* info_board_{};  ///<

  bool attachment_dock_created_{};         ///<
  bool restart_needed_{};                  ///<
  bool prohibit_update_checking_ = false;  ///<
};

}  // namespace GpgFrontend::UI

#endif  // __GPGWIN_H__
