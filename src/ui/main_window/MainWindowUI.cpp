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

#include "MainWindow.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "dialog/gnupg/GnuPGControllerDialog.h"
#include "ui/UserInterfaceUtils.h"

namespace GpgFrontend::UI {

void MainWindow::create_actions() {
  /* Main Menu
   */
  new_tab_act_ = new QAction(_("New"), this);
  new_tab_act_->setIcon(QIcon(":misc_doc.png"));
  QList<QKeySequence> newTabActShortcutList;
#ifdef GPGFRONTEND_GUI_QT6
  newTabActShortcutList.append(QKeySequence(Qt::CTRL | Qt::Key_N));
  newTabActShortcutList.append(QKeySequence(Qt::CTRL | Qt::Key_T));
#else
#endif
  new_tab_act_->setShortcuts(newTabActShortcutList);
  new_tab_act_->setToolTip(_("Open a new file"));
  connect(new_tab_act_, &QAction::triggered, edit_, &TextEdit::SlotNewTab);

  open_act_ = new QAction(_("Open..."), this);
  open_act_->setIcon(QIcon(":fileopen.png"));
  open_act_->setShortcut(QKeySequence::Open);
  open_act_->setToolTip(_("Open an existing file"));
  connect(open_act_, &QAction::triggered, edit_, &TextEdit::SlotOpen);

  browser_act_ = new QAction(_("File Browser"), this);
  browser_act_->setIcon(QIcon(":file-browser.png"));
#ifdef GPGFRONTEND_GUI_QT6
  browser_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
#else
  browser_act_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
#endif
  browser_act_->setToolTip(_("Open a file browser"));
  connect(browser_act_, &QAction::triggered, this,
          &MainWindow::slot_open_file_tab);

  save_act_ = new QAction(_("Save File"), this);
  save_act_->setIcon(QIcon(":filesave.png"));
  save_act_->setShortcut(QKeySequence::Save);
  save_act_->setToolTip(_("Save the current File"));
  connect(save_act_, &QAction::triggered, edit_, &TextEdit::SlotSave);

  save_as_act_ = new QAction(QString(_("Save As")) + "...", this);
  save_as_act_->setIcon(QIcon(":filesaveas.png"));
  save_as_act_->setShortcut(QKeySequence::SaveAs);
  save_as_act_->setToolTip(_("Save the current File as..."));
  connect(save_as_act_, &QAction::triggered, edit_, &TextEdit::SlotSaveAs);

  print_act_ = new QAction(_("Print"), this);
  print_act_->setIcon(QIcon(":fileprint.png"));
  print_act_->setShortcut(QKeySequence::Print);
  print_act_->setToolTip(_("Print Document"));
  connect(print_act_, &QAction::triggered, edit_, &TextEdit::SlotPrint);

  close_tab_act_ = new QAction(_("Close"), this);
  close_tab_act_->setShortcut(QKeySequence::Close);
  close_tab_act_->setToolTip(_("Close file"));
  connect(close_tab_act_, &QAction::triggered, edit_, &TextEdit::SlotCloseTab);

  quit_act_ = new QAction(_("Quit"), this);
  quit_act_->setShortcut(QKeySequence::Quit);
  quit_act_->setIcon(QIcon(":exit.png"));
  quit_act_->setToolTip(_("Quit Program"));
  connect(quit_act_, &QAction::triggered, this, &MainWindow::close);

  /* Edit Menu
   */
  undo_act_ = new QAction(_("Undo"), this);
  undo_act_->setShortcut(QKeySequence::Undo);
  undo_act_->setToolTip(_("Undo Last Edit Action"));
  connect(undo_act_, &QAction::triggered, edit_, &TextEdit::SlotUndo);

  redo_act_ = new QAction(_("Redo"), this);
  redo_act_->setShortcut(QKeySequence::Redo);
  redo_act_->setToolTip(_("Redo Last Edit Action"));
  connect(redo_act_, &QAction::triggered, edit_, &TextEdit::SlotRedo);

  zoom_in_act_ = new QAction(_("Zoom In"), this);
  zoom_in_act_->setShortcut(QKeySequence::ZoomIn);
  connect(zoom_in_act_, &QAction::triggered, edit_, &TextEdit::SlotZoomIn);

  zoom_out_act_ = new QAction(_("Zoom Out"), this);
  zoom_out_act_->setShortcut(QKeySequence::ZoomOut);
  connect(zoom_out_act_, &QAction::triggered, edit_, &TextEdit::SlotZoomOut);

  paste_act_ = new QAction(_("Paste"), this);
  paste_act_->setIcon(QIcon(":button_paste.png"));
  paste_act_->setShortcut(QKeySequence::Paste);
  paste_act_->setToolTip(_("Paste Text From Clipboard"));
  connect(paste_act_, &QAction::triggered, edit_, &TextEdit::SlotPaste);

  cut_act_ = new QAction(_("Cut"), this);
  cut_act_->setIcon(QIcon(":button_cut.png"));
  cut_act_->setShortcut(QKeySequence::Cut);
  cut_act_->setToolTip(
      _("Cut the current selection's contents to the "
        "clipboard"));
  connect(cut_act_, &QAction::triggered, edit_, &TextEdit::SlotCut);

  copy_act_ = new QAction(_("Copy"), this);
  copy_act_->setIcon(QIcon(":button_copy.png"));
  copy_act_->setShortcut(QKeySequence::Copy);
  copy_act_->setToolTip(
      _("Copy the current selection's contents to the "
        "clipboard"));
  connect(copy_act_, &QAction::triggered, edit_, &TextEdit::SlotCopy);

  quote_act_ = new QAction(_("Quote"), this);
  quote_act_->setIcon(QIcon(":quote.png"));
  quote_act_->setToolTip(_("Quote whole text"));
  connect(quote_act_, &QAction::triggered, edit_, &TextEdit::SlotQuote);

  select_all_act_ = new QAction(_("Select All"), this);
  select_all_act_->setIcon(QIcon(":edit.png"));
  select_all_act_->setShortcut(QKeySequence::SelectAll);
  select_all_act_->setToolTip(_("Select the whole text"));
  connect(select_all_act_, &QAction::triggered, edit_,
          &TextEdit::SlotSelectAll);

  find_act_ = new QAction(_("Find"), this);
  find_act_->setShortcut(QKeySequence::Find);
  find_act_->setToolTip(_("Find a word"));
  connect(find_act_, &QAction::triggered, this, &MainWindow::slot_find);

  clean_double_line_breaks_act_ = new QAction(_("Remove spacing"), this);
  clean_double_line_breaks_act_->setIcon(
      QIcon(":format-line-spacing-triple.png"));
  // cleanDoubleLineBreaksAct->setShortcut(QKeySequence::SelectAll);
  clean_double_line_breaks_act_->setToolTip(
      _("Remove double linebreaks, e.g. in pasted text from Web Mailer"));
  connect(clean_double_line_breaks_act_, &QAction::triggered, this,
          &MainWindow::slot_clean_double_line_breaks);

  open_settings_act_ = new QAction(_("Settings"), this);
  open_settings_act_->setToolTip(_("Open settings dialog"));
  open_settings_act_->setMenuRole(QAction::PreferencesRole);
  open_settings_act_->setShortcut(QKeySequence::Preferences);
  connect(open_settings_act_, &QAction::triggered, this,
          &MainWindow::slot_open_settings_dialog);

  /* Crypt Menu
   */
  encrypt_act_ = new QAction(_("Encrypt"), this);
  encrypt_act_->setIcon(QIcon(":encrypted.png"));
#ifdef GPGFRONTEND_GUI_QT6
  encrypt_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
#else
#endif
  encrypt_act_->setToolTip(_("Encrypt Message"));
  connect(encrypt_act_, &QAction::triggered, this, &MainWindow::slot_encrypt);

  encrypt_sign_act_ = new QAction(_("Encrypt Sign"), this);
  encrypt_sign_act_->setIcon(QIcon(":encrypted_signed.png"));
#ifdef GPGFRONTEND_GUI_QT6
  encrypt_sign_act_->setShortcut(
      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
#else
  encrypt_act_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
#endif
  encrypt_sign_act_->setToolTip(_("Encrypt and Sign Message"));
  connect(encrypt_sign_act_, &QAction::triggered, this,
          &MainWindow::slot_encrypt_sign);

  decrypt_act_ = new QAction(_("Decrypt"), this);
  decrypt_act_->setIcon(QIcon(":decrypted.png"));
#ifdef GPGFRONTEND_GUI_QT6
  decrypt_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
#else
  encrypt_act_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
#endif
  decrypt_act_->setToolTip(_("Decrypt Message"));
  connect(decrypt_act_, &QAction::triggered, this, &MainWindow::slot_decrypt);

  decrypt_verify_act_ = new QAction(_("Decrypt Verify"), this);
  decrypt_verify_act_->setIcon(QIcon(":decrypted_verified.png"));
#ifdef GPGFRONTEND_GUI_QT6
  decrypt_verify_act_->setShortcut(
      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
#else
  encrypt_act_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
#endif
  decrypt_verify_act_->setToolTip(_("Decrypt and Verify Message"));
  connect(decrypt_verify_act_, &QAction::triggered, this,
          &MainWindow::slot_decrypt_verify);

  sign_act_ = new QAction(_("Sign"), this);
  sign_act_->setIcon(QIcon(":signature.png"));
#ifdef GPGFRONTEND_GUI_QT6
  sign_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
#else
  encrypt_act_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
#endif
  sign_act_->setToolTip(_("Sign Message"));
  connect(sign_act_, &QAction::triggered, this, &MainWindow::slot_sign);

  verify_act_ = new QAction(_("Verify"), this);
  verify_act_->setIcon(QIcon(":verify.png"));
#ifdef GPGFRONTEND_GUI_QT6
  verify_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
#else
  encrypt_act_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
#endif
  verify_act_->setToolTip(_("Verify Message"));
  connect(verify_act_, &QAction::triggered, this, &MainWindow::slot_verify);

  /* Key Menu
   */

  import_key_from_file_act_ = new QAction(_("File"), this);
  import_key_from_file_act_->setIcon(QIcon(":import_key_from_file.png"));
  import_key_from_file_act_->setToolTip(_("Import New Key From File"));
  connect(import_key_from_file_act_, &QAction::triggered, this,
          [&]() { CommonUtils::GetInstance()->SlotImportKeyFromFile(this); });

  import_key_from_clipboard_act_ = new QAction(_("Clipboard"), this);
  import_key_from_clipboard_act_->setIcon(
      QIcon(":import_key_from_clipboard.png"));
  import_key_from_clipboard_act_->setToolTip(
      _("Import New Key From Clipboard"));
  connect(import_key_from_clipboard_act_, &QAction::triggered, this, [&]() {
    CommonUtils::GetInstance()->SlotImportKeyFromClipboard(this);
  });

  // get settings
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  // read settings
  bool forbid_all_gnupg_connection = false;
  try {
    forbid_all_gnupg_connection =
        settings.lookup("network.forbid_all_gnupg_connection");
  } catch (...) {
    SPDLOG_ERROR("setting operation error: forbid_all_gnupg_connection");
  }

  import_key_from_key_server_act_ = new QAction(_("Keyserver"), this);
  import_key_from_key_server_act_->setIcon(
      QIcon(":import_key_from_server.png"));
  import_key_from_key_server_act_->setToolTip(
      _("Import New Key From Keyserver"));
  import_key_from_key_server_act_->setDisabled(forbid_all_gnupg_connection);
  connect(import_key_from_key_server_act_, &QAction::triggered, this, [&]() {
    CommonUtils::GetInstance()->SlotImportKeyFromKeyServer(this);
  });

  import_key_from_edit_act_ = new QAction(_("Editor"), this);
  import_key_from_edit_act_->setIcon(QIcon(":txt.png"));
  import_key_from_edit_act_->setToolTip(_("Import New Key From Editor"));
  connect(import_key_from_edit_act_, &QAction::triggered, this,
          &MainWindow::slot_import_key_from_edit);

  open_key_management_act_ = new QAction(_("Manage Keys"), this);
  open_key_management_act_->setIcon(QIcon(":keymgmt.png"));
  open_key_management_act_->setToolTip(_("Open Key Management"));
  connect(open_key_management_act_, &QAction::triggered, this,
          &MainWindow::slot_open_key_management);

  clean_gpg_password_cache_act_ = new QAction(_("Clear Password Cache"), this);
  clean_gpg_password_cache_act_->setIcon(QIcon(":configure.png"));
  clean_gpg_password_cache_act_->setToolTip(_("Clear Password Cache of GnuPG"));
  connect(clean_gpg_password_cache_act_, &QAction::triggered, this, [=]() {
    if (GpgFrontend::GpgAdvancedOperator::GetInstance()
            .ClearGpgPasswordCache()) {
      QMessageBox::information(this, _("Successful Operation"),
                               _("Clear password cache successfully"));
    } else {
      QMessageBox::critical(this, _("Failed Operation"),
                            _("Failed to clear password cache of GnuPG"));
    }
  });

  reload_components_act_ = new QAction(_("Reload All Components"), this);
  reload_components_act_->setIcon(QIcon(":configure.png"));
  reload_components_act_->setToolTip(_("Reload All GnuPG's Components"));
  connect(reload_components_act_, &QAction::triggered, this, [=]() {
    if (GpgFrontend::GpgAdvancedOperator::GetInstance().ReloadGpgComponents()) {
      QMessageBox::information(
          this, _("Successful Operation"),
          _("Reload all the GnuPG's components successfully"));
    } else {
      QMessageBox::critical(
          this, _("Failed Operation"),
          _("Failed to reload all or one of the GnuPG's component(s)"));
    }
  });

  restart_components_act_ = new QAction(_("Restart All Components"), this);
  restart_components_act_->setIcon(QIcon(":configure.png"));
  restart_components_act_->setToolTip(_("Restart All GnuPG's Components"));
  connect(restart_components_act_, &QAction::triggered, this, [=]() {
    if (GpgFrontend::GpgAdvancedOperator::GetInstance()
            .RestartGpgComponents()) {
      QMessageBox::information(
          this, _("Successful Operation"),
          _("Restart all the GnuPG's components successfully"));
    } else {
      QMessageBox::critical(
          this, _("Failed Operation"),
          _("Failed to restart all or one of the GnuPG's component(s)"));
    }
  });

  gnupg_controller_open_act_ = new QAction(_("GnuPG Controller"), this);
  gnupg_controller_open_act_->setIcon(QIcon(":configure.png"));
  gnupg_controller_open_act_->setToolTip(_("Open GnuPG Controller Dialog"));
  connect(gnupg_controller_open_act_, &QAction::triggered, this,
          [this]() { (new GnuPGControllerDialog(this))->exec(); });

  /*
   * About Menu
   */
  about_act_ = new QAction(_("About"), this);
  about_act_->setIcon(QIcon(":help.png"));
  about_act_->setToolTip(_("Show the application's About box"));
  about_act_->setMenuRole(QAction::AboutRole);
  connect(about_act_, &QAction::triggered, this,
          [=]() { new AboutDialog(0, this); });

  gnupg_act_ = new QAction(_("GnuPG"), this);
  gnupg_act_->setIcon(QIcon(":help.png"));
  gnupg_act_->setToolTip(_("Information about Gnupg"));
  connect(gnupg_act_, &QAction::triggered, this,
          [=]() { new AboutDialog(1, this); });

  translate_act_ = new QAction(_("Translate"), this);
  translate_act_->setIcon(QIcon(":help.png"));
  translate_act_->setToolTip(_("Information about translation"));
  connect(translate_act_, &QAction::triggered, this,
          [=]() { new AboutDialog(2, this); });

  /*
   * Check Update Menu
   */
  check_update_act_ = new QAction(_("Check for Updates"), this);
  check_update_act_->setIcon(QIcon(":help.png"));
  check_update_act_->setToolTip(_("Check for updates"));
  connect(check_update_act_, &QAction::triggered, this,
          [=]() { new AboutDialog(3, this); });

  start_wizard_act_ = new QAction(_("Open Wizard"), this);
  start_wizard_act_->setToolTip(_("Open the wizard"));
  connect(start_wizard_act_, &QAction::triggered, this,
          &MainWindow::slot_start_wizard);

  append_selected_keys_act_ =
      new QAction(_("Append Public Key to Editor"), this);
  append_selected_keys_act_->setToolTip(
      _("Append selected Keypair's Public Key to Editor"));
  connect(append_selected_keys_act_, &QAction::triggered, this,
          &MainWindow::slot_append_selected_keys);

  append_key_create_date_to_editor_act_ =
      new QAction(_("Append Create DateTime to Editor"), this);
  append_key_create_date_to_editor_act_->setToolTip(
      _("Append selected Key's creation date and time to Editor"));
  connect(append_key_create_date_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_create_datetime);

  append_key_expire_date_to_editor_act_ =
      new QAction(_("Append Expire DateTime to Editor"), this);
  append_key_expire_date_to_editor_act_->setToolTip(
      _("Append selected Key's expiration date and time to Editor"));
  connect(append_key_expire_date_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_expire_datetime);

  append_key_fingerprint_to_editor_act_ =
      new QAction(_("Append Fingerprint to Editor"), this);
  append_key_expire_date_to_editor_act_->setToolTip(
      _("Append selected Key's Fingerprint to Editor"));
  connect(append_key_fingerprint_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_fingerprint);

  copy_mail_address_to_clipboard_act_ = new QAction(_("Copy Email"), this);
  copy_mail_address_to_clipboard_act_->setToolTip(
      _("Copy selected Keypair's to clipboard"));
  connect(copy_mail_address_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_mail_address_to_clipboard);

  copy_key_default_uid_to_clipboard_act_ =
      new QAction(_("Copy Default UID"), this);
  copy_key_default_uid_to_clipboard_act_->setToolTip(
      _("Copy selected Keypair's default UID to clipboard"));
  connect(copy_key_default_uid_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_default_uid_to_clipboard);

  copy_key_id_to_clipboard_act_ = new QAction(_("Copy Key ID"), this);
  copy_key_id_to_clipboard_act_->setToolTip(
      _("Copy selected Keypair's ID to clipboard"));
  connect(copy_key_id_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_key_id_to_clipboard);

  show_key_details_act_ = new QAction(_("Show Key Details"), this);
  show_key_details_act_->setToolTip(_("Show Details for this Key"));
  connect(show_key_details_act_, &QAction::triggered, this,
          &MainWindow::slot_show_key_details);

  /* Key-Shortcuts for Tab-Switchung-Action
   */
  switch_tab_up_act_ = new QAction(this);
  switch_tab_up_act_->setShortcut(QKeySequence::NextChild);
  connect(switch_tab_up_act_, &QAction::triggered, edit_,
          &TextEdit::SlotSwitchTabUp);
  this->addAction(switch_tab_up_act_);

  switch_tab_down_act_ = new QAction(this);
  switch_tab_down_act_->setShortcut(QKeySequence::PreviousChild);
  connect(switch_tab_down_act_, &QAction::triggered, edit_,
          &TextEdit::SlotSwitchTabDown);
  this->addAction(switch_tab_down_act_);

  cut_pgp_header_act_ = new QAction(_("Remove PGP Header"), this);
  connect(cut_pgp_header_act_, &QAction::triggered, this,
          &MainWindow::slot_cut_pgp_header);

  add_pgp_header_act_ = new QAction(_("Add PGP Header"), this);
  connect(add_pgp_header_act_, &QAction::triggered, this,
          &MainWindow::slot_add_pgp_header);
}

void MainWindow::create_menus() {
  file_menu_ = menuBar()->addMenu(_("File"));
  file_menu_->addAction(new_tab_act_);
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

  edit_menu_ = menuBar()->addMenu(_("Edit"));
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
  edit_menu_->addSeparator();
  edit_menu_->addAction(open_settings_act_);

  crypt_menu_ = menuBar()->addMenu(_("Crypt"));
  crypt_menu_->addAction(encrypt_act_);
  crypt_menu_->addAction(encrypt_sign_act_);
  crypt_menu_->addAction(decrypt_act_);
  crypt_menu_->addAction(decrypt_verify_act_);
  crypt_menu_->addSeparator();
  crypt_menu_->addAction(sign_act_);
  crypt_menu_->addAction(verify_act_);
  crypt_menu_->addSeparator();

  key_menu_ = menuBar()->addMenu(_("Keys"));
  import_key_menu_ = key_menu_->addMenu(_("Import Key"));
  import_key_menu_->setIcon(QIcon(":key_import.png"));
  import_key_menu_->addAction(import_key_from_file_act_);
  import_key_menu_->addAction(import_key_from_edit_act_);
  import_key_menu_->addAction(import_key_from_clipboard_act_);
  import_key_menu_->addAction(import_key_from_key_server_act_);
  key_menu_->addAction(open_key_management_act_);

  gpg_menu_ = menuBar()->addMenu(_("GnuPG"));
  gpg_menu_->addAction(clean_gpg_password_cache_act_);
  gpg_menu_->addSeparator();
  gpg_menu_->addAction(reload_components_act_);
  gpg_menu_->addAction(restart_components_act_);
  gpg_menu_->addSeparator();
  gpg_menu_->addAction(gnupg_controller_open_act_);

  steganography_menu_ = menuBar()->addMenu(_("Steganography"));
  steganography_menu_->addAction(cut_pgp_header_act_);
  steganography_menu_->addAction(add_pgp_header_act_);

  view_menu_ = menuBar()->addMenu(_("View"));

  help_menu_ = menuBar()->addMenu(_("Help"));
  help_menu_->addAction(start_wizard_act_);
  help_menu_->addSeparator();
  help_menu_->addAction(check_update_act_);
  help_menu_->addAction(gnupg_act_);
  help_menu_->addAction(translate_act_);
  help_menu_->addAction(about_act_);
}

void MainWindow::create_tool_bars() {
  file_tool_bar_ = addToolBar(_("File"));
  file_tool_bar_->setObjectName("fileToolBar");
  file_tool_bar_->addAction(new_tab_act_);
  file_tool_bar_->addAction(open_act_);
  file_tool_bar_->addAction(save_act_);
  file_tool_bar_->addAction(browser_act_);
  view_menu_->addAction(file_tool_bar_->toggleViewAction());

  crypt_tool_bar_ = addToolBar(_("Operations"));
  crypt_tool_bar_->setObjectName("cryptToolBar");
  crypt_tool_bar_->addAction(encrypt_act_);
  crypt_tool_bar_->addAction(encrypt_sign_act_);
  crypt_tool_bar_->addAction(decrypt_act_);
  crypt_tool_bar_->addAction(decrypt_verify_act_);
  crypt_tool_bar_->addAction(sign_act_);
  crypt_tool_bar_->addAction(verify_act_);
  view_menu_->addAction(crypt_tool_bar_->toggleViewAction());

  key_tool_bar_ = addToolBar(_("Key"));
  key_tool_bar_->setObjectName("keyToolBar");
  key_tool_bar_->addAction(open_key_management_act_);
  view_menu_->addAction(key_tool_bar_->toggleViewAction());

  edit_tool_bar_ = addToolBar(_("Edit"));
  edit_tool_bar_->setObjectName("editToolBar");
  edit_tool_bar_->addAction(copy_act_);
  edit_tool_bar_->addAction(paste_act_);
  edit_tool_bar_->addAction(select_all_act_);
  edit_tool_bar_->hide();
  view_menu_->addAction(edit_tool_bar_->toggleViewAction());

  special_edit_tool_bar_ = addToolBar(_("Special Edit"));
  special_edit_tool_bar_->setObjectName("specialEditToolBar");
  special_edit_tool_bar_->addAction(quote_act_);
  special_edit_tool_bar_->addAction(clean_double_line_breaks_act_);
  special_edit_tool_bar_->hide();
  view_menu_->addAction(special_edit_tool_bar_->toggleViewAction());

  // Add dropdown menu for key import to keytoolbar
  import_button_ = new QToolButton();
  import_button_->setMenu(import_key_menu_);
  import_button_->setPopupMode(QToolButton::InstantPopup);
  import_button_->setIcon(QIcon(":key_import.png"));
  import_button_->setToolTip(_("Import key from..."));
  import_button_->setText(_("Import key"));
  key_tool_bar_->addWidget(import_button_);
}

void MainWindow::create_status_bar() {
  auto* statusBarBox = new QWidget();
  auto* statusBarBoxLayout = new QHBoxLayout();
  // QPixmap* pixmap;

  // icon which should be shown if there are files in attachments-folder
  //  pixmap = new QPixmap(":statusbar_icon.png");
  //  statusBarIcon = new QLabel();
  //  statusBar()->addWidget(statusBarIcon);
  //
  //  statusBarIcon->setPixmap(*pixmap);
  //  statusBar()->insertPermanentWidget(0, statusBarIcon, 0);
  statusBar()->showMessage(_("Ready"), 2000);
  statusBarBox->setLayout(statusBarBoxLayout);
}

void MainWindow::create_dock_windows() {
  /* KeyList-Dock window
   */
  key_list_dock_ = new QDockWidget(_("Key ToolBox"), this);
  key_list_dock_->setObjectName("EncryptDock");
  key_list_dock_->setAllowedAreas(Qt::LeftDockWidgetArea |
                                  Qt::RightDockWidgetArea);
  key_list_dock_->setMinimumWidth(460);
  addDockWidget(Qt::RightDockWidgetArea, key_list_dock_);

  m_key_list_->AddListGroupTab(
      _("Default"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  m_key_list_->AddListGroupTab(
      _("Only Public Key"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return !key.IsPrivateKey() &&
               !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  m_key_list_->AddListGroupTab(
      _("Has Private Key"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return key.IsPrivateKey() &&
               !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  m_key_list_->SlotRefresh();

  key_list_dock_->setWidget(m_key_list_);
  view_menu_->addAction(key_list_dock_->toggleViewAction());

  info_board_dock_ = new QDockWidget(_("Information Board"), this);
  info_board_dock_->setObjectName("Information Board");
  info_board_dock_->setAllowedAreas(Qt::BottomDockWidgetArea);
  addDockWidget(Qt::BottomDockWidgetArea, info_board_dock_);
  info_board_dock_->setWidget(info_board_);
  info_board_dock_->widget()->layout()->setContentsMargins(0, 0, 0, 0);
  view_menu_->addAction(info_board_dock_->toggleViewAction());
}

}  // namespace GpgFrontend::UI
