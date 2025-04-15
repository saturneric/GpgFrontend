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

#include "MainWindow.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/controller/GnuPGControllerDialog.h"
#include "ui/dialog/controller/ModuleControllerDialog.h"
#include "ui/dialog/controller/SmartCardControllerDialog.h"
#include "ui/dialog/help/AboutDialog.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

void MainWindow::create_actions() {
  new_tab_act_ = create_action(
      "new_tab", tr("New"), ":/icons/misc_doc.png", tr("Open a new file"),
      {QKeySequence(Qt::CTRL | Qt::Key_N), QKeySequence(Qt::CTRL | Qt::Key_T)});
  connect(new_tab_act_, &QAction::triggered, edit_, &TextEdit::SlotNewTab);

  open_act_ = create_action("open", tr("Open..."), ":/icons/fileopen.png",
                            tr("Open an existing file"), {QKeySequence::Open});
  connect(open_act_, &QAction::triggered, edit_, &TextEdit::SlotOpen);

  browser_act_ = create_action(
      "file_browser", tr("File Panel"), ":/icons/file-operator.png",
      tr("Open a file panel"), {QKeySequence(Qt::CTRL | Qt::Key_B)});
  connect(browser_act_, &QAction::triggered, this,
          &MainWindow::slot_open_file_tab);

  save_act_ = create_action("save", tr("Save File"), ":/icons/filesave.png",
                            tr("Save the current File"), {QKeySequence::Save});
  connect(save_act_, &QAction::triggered, edit_, &TextEdit::SlotSave);

  save_as_act_ =
      create_action("save_as", tr("Save As") + "...", ":/icons/filesaveas.png",
                    tr("Save the current File as..."), {QKeySequence::SaveAs});
  connect(save_as_act_, &QAction::triggered, edit_, &TextEdit::SlotSaveAs);

  print_act_ = create_action("print", tr("Print"), ":/icons/fileprint.png",
                             tr("Print Document"), {QKeySequence::Print});
  connect(print_act_, &QAction::triggered, edit_, &TextEdit::SlotPrint);

  close_tab_act_ = create_action("close_tab", tr("Close"), ":/icons/close.png",
                                 tr("Close file"), {QKeySequence::Close});
  connect(close_tab_act_, &QAction::triggered, edit_, &TextEdit::SlotCloseTab);

  quit_act_ = create_action("quit", tr("Quit"), ":/icons/exit.png",
                            tr("Quit Program"), {QKeySequence::Quit});
  connect(quit_act_, &QAction::triggered, this, &MainWindow::close);

  /* Edit Menu */
  undo_act_ = create_action("undo", tr("Undo"), ":/icons/undo.png",
                            tr("Undo Last Edit Action"), {QKeySequence::Undo});
  connect(undo_act_, &QAction::triggered, edit_, &TextEdit::SlotUndo);

  redo_act_ = create_action("redo", tr("Redo"), ":/icons/redo.png",
                            tr("Redo Last Edit Action"), {QKeySequence::Redo});
  connect(redo_act_, &QAction::triggered, edit_, &TextEdit::SlotRedo);

  zoom_in_act_ = create_action("zoom_in", tr("Zoom In"), ":/icons/zoomin.png",
                               tr("Zoom in"), {QKeySequence::ZoomIn});
  connect(zoom_in_act_, &QAction::triggered, edit_, &TextEdit::SlotZoomIn);

  zoom_out_act_ =
      create_action("zoom_out", tr("Zoom Out"), ":/icons/zoomout.png",
                    tr("Zoom out"), {QKeySequence::ZoomOut});
  connect(zoom_out_act_, &QAction::triggered, edit_, &TextEdit::SlotZoomOut);

  paste_act_ =
      create_action("paste", tr("Paste"), ":/icons/button_paste.png",
                    tr("Paste Text From Clipboard"), {QKeySequence::Paste});
  connect(paste_act_, &QAction::triggered, edit_, &TextEdit::SlotPaste);

  cut_act_ =
      create_action("cut", tr("Cut"), ":/icons/button_cut.png",
                    tr("Cut the current selection's contents to the clipboard"),
                    {QKeySequence::Cut});
  connect(cut_act_, &QAction::triggered, edit_, &TextEdit::SlotCut);

  copy_act_ = create_action(
      "copy", tr("Copy"), ":/icons/button_copy.png",
      tr("Copy the current selection's contents to the clipboard"),
      {QKeySequence::Copy});
  connect(copy_act_, &QAction::triggered, edit_, &TextEdit::SlotCopy);

  quote_act_ = create_action("quote", tr("Quote"), ":/icons/quote.png",
                             tr("Quote whole text"));
  connect(quote_act_, &QAction::triggered, edit_, &TextEdit::SlotQuote);

  select_all_act_ =
      create_action("select_all", tr("Select All"), ":/icons/edit.png",
                    tr("Select the whole text"), {QKeySequence::SelectAll});
  connect(select_all_act_, &QAction::triggered, edit_,
          &TextEdit::SlotSelectAll);

  find_act_ = create_action("find", tr("Find"), ":/icons/search.png",
                            tr("Find a word"), {QKeySequence::Find});
  connect(find_act_, &QAction::triggered, this, &MainWindow::slot_find);

  clean_double_line_breaks_act_ = create_action(
      "remove_spacing", tr("Remove spacing"),
      ":/icons/format-line-spacing-triple.png",
      tr("Remove double linebreaks, e.g. in pasted text from Web Mailer"));
  connect(clean_double_line_breaks_act_, &QAction::triggered, this,
          &MainWindow::slot_clean_double_line_breaks);

  open_settings_act_ =
      create_action("settings", tr("Settings"), ":/icons/setting.png",
                    tr("Open settings dialog"), {QKeySequence::Preferences});
  open_settings_act_->setMenuRole(QAction::PreferencesRole);
  connect(open_settings_act_, &QAction::triggered, this,
          &MainWindow::slot_open_settings_dialog);

  /*
   * Crypt Menu
   */
  encrypt_act_ = create_action("encrypt", tr("Encrypt"), ":/icons/lock.png",
                               tr("Encrypt Message"),
                               {QKeySequence(Qt::CTRL | Qt::Key_E)});
  connect(encrypt_act_, &QAction::triggered, this,
          &MainWindow::SlotGeneralEncrypt);

  encrypt_sign_act_ =
      create_action("encrypt_sign", tr("Encrypt Sign"), ":/icons/encr-sign.png",
                    tr("Encrypt and Sign Message"),
                    {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E)});
  connect(encrypt_sign_act_, &QAction::triggered, this,
          &MainWindow::SlotGeneralEncryptSign);

  decrypt_act_ = create_action("decrypt", tr("Decrypt"), ":/icons/unlock.png",
                               tr("Decrypt Message"),
                               {QKeySequence(Qt::CTRL | Qt::Key_D)});
  connect(decrypt_act_, &QAction::triggered, this,
          &MainWindow::SlotGeneralDecrypt);

  decrypt_verify_act_ =
      create_action("decrypt_verify", tr("Decrypt Verify"),
                    ":/icons/decr-verify.png", tr("Decrypt and Verify Message"),
                    {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D)});
  connect(decrypt_verify_act_, &QAction::triggered, this,
          &MainWindow::SlotGeneralDecryptVerify);

  sign_act_ = create_action("sign", tr("Sign"), ":/icons/signature.png",
                            tr("Sign Message"),
                            {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I)});
  connect(sign_act_, &QAction::triggered, this, &MainWindow::SlotGeneralSign);

  verify_act_ = create_action("verify", tr("Verify"), ":/icons/verify.png",
                              tr("Verify Message"),
                              {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V)});
  connect(verify_act_, &QAction::triggered, this,
          &MainWindow::SlotGeneralVerify);

  /*
   * Key Menu
   */
  import_key_from_file_act_ = create_action("import_key_from_file", tr("File"),
                                            ":/icons/import_key_from_file.png",
                                            tr("Import New Key From File"));
  connect(import_key_from_file_act_, &QAction::triggered, this, [=]() {
    CommonUtils::GetInstance()->SlotImportKeyFromFile(
        this, m_key_list_->GetCurrentGpgContextChannel());
  });

  import_key_from_clipboard_act_ =
      create_action("import_key_from_clipboard", tr("Clipboard"),
                    ":/icons/import_key_from_clipboard.png",
                    tr("Import New Key From Clipboard"));
  connect(import_key_from_clipboard_act_, &QAction::triggered, this, [this]() {
    CommonUtils::GetInstance()->SlotImportKeyFromClipboard(
        this, m_key_list_->GetCurrentGpgContextChannel());
  });

  bool forbid_all_gnupg_connection =
      GetSettings()
          .value("network/forbid_all_gnupg_connection", false)
          .toBool();

  import_key_from_key_server_act_ =
      create_action("import_key_from_keyserver", tr("Keyserver"),
                    ":/icons/import_key_from_server.png",
                    tr("Import New Key From Keyserver"));
  import_key_from_key_server_act_->setDisabled(forbid_all_gnupg_connection);
  connect(import_key_from_key_server_act_, &QAction::triggered, this, [this]() {
    CommonUtils::GetInstance()->SlotImportKeyFromKeyServer(
        this, m_key_list_->GetCurrentGpgContextChannel());
  });

  import_key_from_edit_act_ =
      create_action("import_key_from_edit", tr("Editor"), ":/icons/editor.png",
                    tr("Import New Key From Editor"));
  connect(import_key_from_edit_act_, &QAction::triggered, this,
          &MainWindow::slot_import_key_from_edit);

  open_key_management_act_ =
      create_action("open_key_management", tr("Manage Keys"),
                    ":/icons/keymgmt.png", tr("Open Key Management"));
  connect(open_key_management_act_, &QAction::triggered, this,
          &MainWindow::slot_open_key_management);

  clean_gpg_password_cache_act_ =
      create_action("clean_password_cache", tr("Clear Password Cache"),
                    ":/icons/clear-f.png", tr("Clear Password Cache of GnuPG"));
  connect(clean_gpg_password_cache_act_, &QAction::triggered, this,
          &MainWindow::slot_clean_gpg_password_cache);

  reload_components_act_ =
      create_action("reload_components", tr("Reload All Components"),
                    ":/icons/restart.png", tr("Reload All GnuPG's Components"));
  connect(reload_components_act_, &QAction::triggered, this,
          &MainWindow::slot_reload_gpg_components);

  restart_components_act_ = create_action(
      "restart_components", tr("Restart All Components"), ":/icons/restart.png",
      tr("Restart All GnuPG's Components"));
  connect(restart_components_act_, &QAction::triggered, this,
          &MainWindow::slot_restart_gpg_components);

  gnupg_controller_open_act_ = create_action(
      "gnupg_controller_open", tr("Open GnuPG Controller"),
      ":/icons/configure.png", tr("Open GnuPG Controller Dialog"));
  connect(gnupg_controller_open_act_, &QAction::triggered, this,
          [this]() { (new GnuPGControllerDialog(this))->exec(); });

  module_controller_open_act_ =
      create_action("module_controller_open", tr("Open Module Controller"),
                    ":/icons/module.png", tr("Open Module Controller Dialog"));
  connect(module_controller_open_act_, &QAction::triggered, this,
          [this]() { (new ModuleControllerDialog(this))->exec(); });

  smart_card_controller_open_act_ = create_action(
      "smart_card_controller_open", tr("Open Smart Card Controller"),
      ":/icons/smart-card.png", tr("Open Smart Card Controller Dialog"));
  connect(smart_card_controller_open_act_, &QAction::triggered, this,
          [this]() { (new SmartCardControllerDialog(this))->exec(); });

  /**
   * E-Mail Menu
   */
  if (Module::IsModuleActivate(kEmailModuleID)) {
    new_email_tab_act_ =
        create_action("new_email_tab", tr("New E-Mail"), ":/icons/email.png",
                      tr("Create A New E-Mail Tab"));
    connect(new_email_tab_act_, &QAction::triggered, edit_,
            &TextEdit::SlotNewEMailTab);
  }

  /*
   * About Menu
   */
  about_act_ = create_action("about", tr("About"), ":/icons/help.png",
                             tr("Show the application's About box"));
  about_act_->setMenuRole(QAction::AboutRole);
  connect(about_act_, &QAction::triggered, this,
          [=]() { new AboutDialog(0, this); });

  if (Module::IsModuleActivate(kGnuPGInfoGatheringModuleID)) {
    gnupg_act_ = create_action("gnupg_info", tr("GnuPG"), ":/icons/key.png",
                               tr("Information about Gnupg"));
    connect(gnupg_act_, &QAction::triggered, this,
            [=]() { new AboutDialog(tr("GnuPG"), this); });
  }

  translate_act_ =
      create_action("translate", tr("Translate"), ":/icons/translate.png",
                    tr("Information about translation"));
  connect(translate_act_, &QAction::triggered, this,
          [=]() { new AboutDialog(tr("Translators"), this); });

  if (Module::IsModuleActivate(kVersionCheckingModuleID)) {
    check_update_act_ =
        create_action("check_update", tr("Check for Updates"),
                      ":/icons/update.png", tr("Check for updates"));
    connect(check_update_act_, &QAction::triggered, this,
            [=]() { new AboutDialog(tr("Update"), this); });
  }

  start_wizard_act_ =
      create_action("start_wizard", tr("Open Wizard"), ":/icons/wizard.png",
                    tr("Open the wizard"));
  connect(start_wizard_act_, &QAction::triggered, this,
          &MainWindow::slot_start_wizard);

  append_selected_keys_act_ =
      create_action("append_selected_keys", tr("Append Public Key to Editor"),
                    "", tr("Append selected Keypair's Public Key to Editor"));
  connect(append_selected_keys_act_, &QAction::triggered, this,
          &MainWindow::slot_append_selected_keys);

  append_key_create_date_to_editor_act_ = create_action(
      "append_key_create_date", tr("Append Create DateTime to Editor"), "",
      tr("Append selected Key's creation date and time to Editor"));
  connect(append_key_create_date_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_create_datetime);

  append_key_expire_date_to_editor_act_ = create_action(
      "append_key_expire_date", tr("Append Expire DateTime to Editor"), "",
      tr("Append selected Key's expiration date and time to Editor"));
  connect(append_key_expire_date_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_expire_datetime);

  append_key_fingerprint_to_editor_act_ = create_action(
      "append_key_fingerprint", tr("Append Fingerprint to Editor"), "",
      tr("Append selected Key's Fingerprint to Editor"));
  connect(append_key_fingerprint_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_fingerprint);

  copy_mail_address_to_clipboard_act_ =
      create_action("copy_email", tr("Copy Email"), "",
                    tr("Copy selected Keypair's to clipboard"));
  connect(copy_mail_address_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_mail_address_to_clipboard);

  copy_key_default_uid_to_clipboard_act_ =
      create_action("copy_default_uid", tr("Copy Default UID"), "",
                    tr("Copy selected Keypair's default UID to clipboard"));
  connect(copy_key_default_uid_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_default_uid_to_clipboard);

  copy_key_id_to_clipboard_act_ =
      create_action("copy_key_id", tr("Copy Key ID"), "",
                    tr("Copy selected Keypair's ID to clipboard"));
  connect(copy_key_id_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_key_id_to_clipboard);

  show_key_details_act_ =
      create_action("show_key_details", tr("Show Key Details"), "",
                    tr("Show Details for this Key"));
  connect(show_key_details_act_, &QAction::triggered, this,
          &MainWindow::slot_show_key_details);

  add_key_2_favourite_act_ =
      create_action("add_to_favourite", tr("Add To Favourite"), "",
                    tr("Add this key to Favourite Table"));
  add_key_2_favourite_act_->setData(QVariant("add_key_2_favourite_action"));
  connect(add_key_2_favourite_act_, &QAction::triggered, this,
          &MainWindow::slot_add_key_2_favorite);

  remove_key_from_favourtie_act_ =
      create_action("remove_from_favourite", tr("Remove From Favourite"), "",
                    tr("Remove this key from Favourite Table"));
  remove_key_from_favourtie_act_->setData(
      QVariant("remove_key_from_favourtie_action"));
  connect(remove_key_from_favourtie_act_, &QAction::triggered, this,
          &MainWindow::slot_remove_key_from_favorite);

  set_owner_trust_of_key_act_ =
      create_action("set_owner_trust_level", tr("Set Owner Trust Level"), "",
                    tr("Set Owner Trust Level"));
  set_owner_trust_of_key_act_->setData(QVariant("set_owner_trust_level"));
  connect(set_owner_trust_of_key_act_, &QAction::triggered, this,
          &MainWindow::slot_set_owner_trust_level_of_key);

  /* Key-Shortcuts for Tab-Switching-Action */
  switch_tab_up_act_ =
      create_action("switch_tab_up", "", "", "", {QKeySequence::NextChild});
  connect(switch_tab_up_act_, &QAction::triggered, edit_,
          &TextEdit::SlotSwitchTabUp);
  this->addAction(switch_tab_up_act_);

  switch_tab_down_act_ = create_action("switch_tab_down", "", "", "",
                                       {QKeySequence::PreviousChild});
  connect(switch_tab_down_act_, &QAction::triggered, edit_,
          &TextEdit::SlotSwitchTabDown);
  this->addAction(switch_tab_down_act_);

  cut_pgp_header_act_ =
      create_action("cut_pgp_header", tr("Remove PGP Header"),
                    ":/icons/minus.png", tr("Remove PGP Header"));
  connect(cut_pgp_header_act_, &QAction::triggered, this,
          &MainWindow::slot_cut_pgp_header);

  add_pgp_header_act_ = create_action("add_pgp_header", tr("Add PGP Header"),
                                      ":/icons/add.png", tr("Add PGP Header"));
  connect(add_pgp_header_act_, &QAction::triggered, this,
          &MainWindow::slot_add_pgp_header);
}

void MainWindow::create_menus() {
  file_menu_ = menuBar()->addMenu(tr("File"));
  file_menu_->addAction(new_tab_act_);

  if (Module::IsModuleActivate(kEmailModuleID)) {
    file_menu_->addAction(new_email_tab_act_);
  }

  file_menu_->addAction(browser_act_);
  file_menu_->addAction(open_act_);
  file_menu_->addSeparator();
  file_menu_->addAction(save_act_);
  file_menu_->addAction(save_as_act_);
  file_menu_->addSeparator();
  file_menu_->addAction(print_act_);
  file_menu_->addSeparator();
  file_menu_->addAction(close_tab_act_);
  file_menu_->addAction(quit_act_);

  edit_menu_ = menuBar()->addMenu(tr("Edit"));
  edit_menu_->addAction(undo_act_);
  edit_menu_->addAction(redo_act_);
  edit_menu_->addSeparator();
  edit_menu_->addAction(zoom_in_act_);
  edit_menu_->addAction(zoom_out_act_);
  edit_menu_->addSeparator();
  edit_menu_->addAction(copy_act_);
  edit_menu_->addAction(cut_act_);
  edit_menu_->addAction(paste_act_);
  edit_menu_->addAction(select_all_act_);
  edit_menu_->addAction(find_act_);
  edit_menu_->addSeparator();
  edit_menu_->addAction(quote_act_);
  edit_menu_->addAction(clean_double_line_breaks_act_);
  edit_menu_->addAction(add_pgp_header_act_);
  edit_menu_->addAction(cut_pgp_header_act_);
  edit_menu_->addSeparator();
  edit_menu_->addAction(open_settings_act_);

  crypt_menu_ = menuBar()->addMenu(tr("Crypt"));
  crypt_menu_->addAction(encrypt_act_);
  crypt_menu_->addAction(encrypt_sign_act_);
  crypt_menu_->addAction(decrypt_act_);
  crypt_menu_->addAction(decrypt_verify_act_);
  crypt_menu_->addSeparator();
  crypt_menu_->addAction(sign_act_);
  crypt_menu_->addAction(verify_act_);
  crypt_menu_->addSeparator();

  key_menu_ = menuBar()->addMenu(tr("Keys"));
  import_key_menu_ = key_menu_->addMenu(tr("Import Key"));
  import_key_menu_->setIcon(QIcon(":/icons/key_import.png"));
  import_key_menu_->addAction(import_key_from_file_act_);
  import_key_menu_->addAction(import_key_from_edit_act_);
  import_key_menu_->addAction(import_key_from_clipboard_act_);
  import_key_menu_->addAction(import_key_from_key_server_act_);
  key_menu_->addAction(open_key_management_act_);

  advance_menu_ = menuBar()->addMenu(tr("Advanced"));
  advance_menu_->addAction(clean_gpg_password_cache_act_);
  advance_menu_->addAction(reload_components_act_);
  advance_menu_->addAction(restart_components_act_);
  advance_menu_->addSeparator();
  advance_menu_->addAction(gnupg_controller_open_act_);
  advance_menu_->addAction(module_controller_open_act_);
  advance_menu_->addAction(smart_card_controller_open_act_);

  view_menu_ = menuBar()->addMenu(tr("View"));

  help_menu_ = menuBar()->addMenu(tr("Help"));
  help_menu_->addAction(start_wizard_act_);
  help_menu_->addSeparator();

  if (Module::IsModuleActivate(kGnuPGInfoGatheringModuleID)) {
    help_menu_->addAction(gnupg_act_);
  }

  help_menu_->addAction(translate_act_);

  if (Module::IsModuleActivate(kVersionCheckingModuleID)) {
    help_menu_->addAction(check_update_act_);
  }

  help_menu_->addAction(about_act_);
}

void MainWindow::create_tool_bars() {
  file_tool_bar_ = addToolBar(tr("File"));
  file_tool_bar_->setObjectName("fileToolBar");
  file_tool_bar_->addAction(new_tab_act_);

  if (Module::IsModuleActivate(kEmailModuleID)) {
    file_tool_bar_->addAction(new_email_tab_act_);
  }

  file_tool_bar_->addAction(open_act_);
  file_tool_bar_->addAction(browser_act_);
  view_menu_->addAction(file_tool_bar_->toggleViewAction());

  crypt_tool_bar_ = addToolBar(tr("Operations"));
  crypt_tool_bar_->setObjectName("cryptToolBar");

  view_menu_->addAction(crypt_tool_bar_->toggleViewAction());

  key_tool_bar_ = addToolBar(tr("Key"));
  key_tool_bar_->setObjectName("keyToolBar");
  key_tool_bar_->addAction(open_key_management_act_);
  view_menu_->addAction(key_tool_bar_->toggleViewAction());

  edit_tool_bar_ = addToolBar(tr("Edit"));
  edit_tool_bar_->setObjectName("editToolBar");
  edit_tool_bar_->addAction(copy_act_);
  edit_tool_bar_->addAction(paste_act_);
  edit_tool_bar_->addAction(select_all_act_);
  edit_tool_bar_->hide();
  view_menu_->addAction(edit_tool_bar_->toggleViewAction());

  special_edit_tool_bar_ = addToolBar(tr("Special Edit"));
  special_edit_tool_bar_->setObjectName("specialEditToolBar");
  special_edit_tool_bar_->addAction(quote_act_);
  special_edit_tool_bar_->addAction(clean_double_line_breaks_act_);
  special_edit_tool_bar_->addAction(add_pgp_header_act_);
  special_edit_tool_bar_->addAction(cut_pgp_header_act_);
  special_edit_tool_bar_->hide();
  view_menu_->addAction(special_edit_tool_bar_->toggleViewAction());

  // Add dropdown menu for key import to keytoolbar
  import_button_ = new QToolButton();
  import_button_->setMenu(import_key_menu_);
  import_button_->setPopupMode(QToolButton::InstantPopup);
  import_button_->setIcon(QIcon(":/icons/key_import.png"));
  import_button_->setToolTip(tr("Import key from..."));
  import_button_->setText(tr("Import key"));
  key_tool_bar_->addWidget(import_button_);
}

void MainWindow::create_status_bar() {
  auto* status_bar_box = new QWidget();
  auto* status_bar_box_layout = new QHBoxLayout();
  // QPixmap* pixmap;

  // icon which should be shown if there are files in attachments-folder
  //  pixmap = new QPixmap(":/icons/statusbar_icon.png");
  //  statusBarIcon = new QLabel();
  //  statusBar()->addWidget(statusBarIcon);
  //
  //  statusBarIcon->setPixmap(*pixmap);
  //  statusBar()->insertPermanentWidget(0, statusBarIcon, 0);
  statusBar()->showMessage(tr("Ready"), 2000);
  status_bar_box->setLayout(status_bar_box_layout);
}

void MainWindow::create_dock_windows() {
  /* KeyList-Dock window
   */
  key_list_dock_ = new QDockWidget(tr("Key ToolBox"), this);
  key_list_dock_->setObjectName("EncryptDock");
  key_list_dock_->setAllowedAreas(Qt::LeftDockWidgetArea |
                                  Qt::RightDockWidgetArea);
  // key_list_dock_->setMinimumWidth(460);
  addDockWidget(Qt::RightDockWidgetArea, key_list_dock_);

  m_key_list_->AddListGroupTab(
      tr("Default"), "default",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgAbstractKey* key) -> bool {
        return !(key->IsRevoked() || key->IsDisabled() || key->IsExpired());
      });

  m_key_list_->AddListGroupTab(
      tr("Favourite"), "favourite",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY |
          GpgKeyTableDisplayMode::kFAVORITES,
      [](const GpgAbstractKey*) -> bool { return true; });

  m_key_list_->AddListGroupTab(
      tr("Only Public Key"), "only_public_key",
      GpgKeyTableDisplayMode::kPUBLIC_KEY,
      [](const GpgAbstractKey* key) -> bool {
        return !key->IsPrivateKey() &&
               !(key->IsRevoked() || key->IsDisabled() || key->IsExpired());
      });

  m_key_list_->AddListGroupTab(
      tr("Has Private Key"), "has_private_key",
      GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgAbstractKey* key) -> bool {
        return key->IsPrivateKey() &&
               !(key->IsRevoked() || key->IsDisabled() || key->IsExpired());
      });

  m_key_list_->SlotRefresh();

  key_list_dock_->setWidget(m_key_list_);
  view_menu_->addAction(key_list_dock_->toggleViewAction());

  info_board_dock_ = new QDockWidget(tr("Status Panel"), this);
  info_board_dock_->setObjectName("Status Panel");
  info_board_dock_->setAllowedAreas(Qt::BottomDockWidgetArea);
  addDockWidget(Qt::BottomDockWidgetArea, info_board_dock_);
  info_board_dock_->setWidget(info_board_);
  info_board_dock_->widget()->layout()->setContentsMargins(0, 0, 0, 0);
  view_menu_->addAction(info_board_dock_->toggleViewAction());

  connect(m_key_list_, &KeyList::SignalKeyChecked, this,
          &MainWindow::slot_update_operations_menu_by_checked_keys);
}

}  // namespace GpgFrontend::UI
