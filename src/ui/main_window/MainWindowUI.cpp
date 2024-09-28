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
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/module/ModuleManager.h"
#include "dialog/controller/ModuleControllerDialog.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/controller/GnuPGControllerDialog.h"
#include "ui/dialog/help/AboutDialog.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

void MainWindow::create_actions() {
  /* Main Menu
   */
  new_tab_act_ = new QAction(tr("New"), this);
  new_tab_act_->setIcon(QIcon(":/icons/misc_doc.png"));
  QList<QKeySequence> new_tab_act_shortcut_list;
  new_tab_act_shortcut_list.append(QKeySequence(Qt::CTRL | Qt::Key_N));
  new_tab_act_shortcut_list.append(QKeySequence(Qt::CTRL | Qt::Key_T));
  new_tab_act_->setShortcuts(new_tab_act_shortcut_list);
  new_tab_act_->setToolTip(tr("Open a new file"));
  connect(new_tab_act_, &QAction::triggered, edit_, &TextEdit::SlotNewTab);

  open_act_ = new QAction(tr("Open..."), this);
  open_act_->setIcon(QIcon(":/icons/fileopen.png"));
  open_act_->setShortcut(QKeySequence::Open);
  open_act_->setToolTip(tr("Open an existing file"));
  connect(open_act_, &QAction::triggered, edit_, &TextEdit::SlotOpen);

  browser_act_ = new QAction(tr("File Browser"), this);
  browser_act_->setIcon(QIcon(":/icons/file-browser.png"));
  browser_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
  browser_act_->setToolTip(tr("Open a file browser"));
  connect(browser_act_, &QAction::triggered, this,
          &MainWindow::slot_open_file_tab);

  save_act_ = new QAction(tr("Save File"), this);
  save_act_->setIcon(QIcon(":/icons/filesave.png"));
  save_act_->setShortcut(QKeySequence::Save);
  save_act_->setToolTip(tr("Save the current File"));
  connect(save_act_, &QAction::triggered, edit_, &TextEdit::SlotSave);

  save_as_act_ = new QAction(tr("Save As") + "...", this);
  save_as_act_->setIcon(QIcon(":/icons/filesaveas.png"));
  save_as_act_->setShortcut(QKeySequence::SaveAs);
  save_as_act_->setToolTip(tr("Save the current File as..."));
  connect(save_as_act_, &QAction::triggered, edit_, &TextEdit::SlotSaveAs);

  print_act_ = new QAction(tr("Print"), this);
  print_act_->setIcon(QIcon(":/icons/fileprint.png"));
  print_act_->setShortcut(QKeySequence::Print);
  print_act_->setToolTip(tr("Print Document"));
  connect(print_act_, &QAction::triggered, edit_, &TextEdit::SlotPrint);

  close_tab_act_ = new QAction(tr("Close"), this);
  close_tab_act_->setIcon(QIcon(":/icons/close.png"));
  close_tab_act_->setShortcut(QKeySequence::Close);
  close_tab_act_->setToolTip(tr("Close file"));
  connect(close_tab_act_, &QAction::triggered, edit_, &TextEdit::SlotCloseTab);

  quit_act_ = new QAction(tr("Quit"), this);
  quit_act_->setShortcut(QKeySequence::Quit);
  quit_act_->setIcon(QIcon(":/icons/exit.png"));
  quit_act_->setToolTip(tr("Quit Program"));
  connect(quit_act_, &QAction::triggered, this, &MainWindow::close);

  /* Edit Menu
   */
  undo_act_ = new QAction(tr("Undo"), this);
  undo_act_->setIcon(QIcon(":/icons/undo.png"));
  undo_act_->setShortcut(QKeySequence::Undo);
  undo_act_->setToolTip(tr("Undo Last Edit Action"));
  connect(undo_act_, &QAction::triggered, edit_, &TextEdit::SlotUndo);

  redo_act_ = new QAction(tr("Redo"), this);
  redo_act_->setIcon(QIcon(":/icons/redo.png"));
  redo_act_->setShortcut(QKeySequence::Redo);
  redo_act_->setToolTip(tr("Redo Last Edit Action"));
  connect(redo_act_, &QAction::triggered, edit_, &TextEdit::SlotRedo);

  zoom_in_act_ = new QAction(tr("Zoom In"), this);
  zoom_in_act_->setIcon(QIcon(":/icons/zoomin.png"));
  zoom_in_act_->setShortcut(QKeySequence::ZoomIn);
  connect(zoom_in_act_, &QAction::triggered, edit_, &TextEdit::SlotZoomIn);

  zoom_out_act_ = new QAction(tr("Zoom Out"), this);
  zoom_out_act_->setIcon(QIcon(":/icons/zoomout.png"));
  zoom_out_act_->setShortcut(QKeySequence::ZoomOut);
  connect(zoom_out_act_, &QAction::triggered, edit_, &TextEdit::SlotZoomOut);

  paste_act_ = new QAction(tr("Paste"), this);
  paste_act_->setIcon(QIcon(":/icons/button_paste.png"));
  paste_act_->setShortcut(QKeySequence::Paste);
  paste_act_->setToolTip(tr("Paste Text From Clipboard"));
  connect(paste_act_, &QAction::triggered, edit_, &TextEdit::SlotPaste);

  cut_act_ = new QAction(tr("Cut"), this);
  cut_act_->setIcon(QIcon(":/icons/button_cut.png"));
  cut_act_->setShortcut(QKeySequence::Cut);
  cut_act_->setToolTip(
      tr("Cut the current selection's contents to the "
         "clipboard"));
  connect(cut_act_, &QAction::triggered, edit_, &TextEdit::SlotCut);

  copy_act_ = new QAction(tr("Copy"), this);
  copy_act_->setIcon(QIcon(":/icons/button_copy.png"));
  copy_act_->setShortcut(QKeySequence::Copy);
  copy_act_->setToolTip(
      tr("Copy the current selection's contents to the "
         "clipboard"));
  connect(copy_act_, &QAction::triggered, edit_, &TextEdit::SlotCopy);

  quote_act_ = new QAction(tr("Quote"), this);
  quote_act_->setIcon(QIcon(":/icons/quote.png"));
  quote_act_->setToolTip(tr("Quote whole text"));
  connect(quote_act_, &QAction::triggered, edit_, &TextEdit::SlotQuote);

  select_all_act_ = new QAction(tr("Select All"), this);
  select_all_act_->setIcon(QIcon(":/icons/edit.png"));
  select_all_act_->setShortcut(QKeySequence::SelectAll);
  select_all_act_->setToolTip(tr("Select the whole text"));
  connect(select_all_act_, &QAction::triggered, edit_,
          &TextEdit::SlotSelectAll);

  find_act_ = new QAction(tr("Find"), this);
  find_act_->setIcon(QIcon(":/icons/search.png"));
  find_act_->setShortcut(QKeySequence::Find);
  find_act_->setToolTip(tr("Find a word"));
  connect(find_act_, &QAction::triggered, this, &MainWindow::slot_find);

  clean_double_line_breaks_act_ = new QAction(tr("Remove spacing"), this);
  clean_double_line_breaks_act_->setIcon(
      QIcon(":/icons/format-line-spacing-triple.png"));
  // cleanDoubleLineBreaksAct->setShortcut(QKeySequence::SelectAll);
  clean_double_line_breaks_act_->setToolTip(
      tr("Remove double linebreaks, e.g. in pasted text from Web Mailer"));
  connect(clean_double_line_breaks_act_, &QAction::triggered, this,
          &MainWindow::slot_clean_double_line_breaks);

  open_settings_act_ = new QAction(tr("Settings"), this);
  open_settings_act_->setIcon(QIcon(":/icons/setting.png"));
  open_settings_act_->setToolTip(tr("Open settings dialog"));
  open_settings_act_->setMenuRole(QAction::PreferencesRole);
  open_settings_act_->setShortcut(QKeySequence::Preferences);
  connect(open_settings_act_, &QAction::triggered, this,
          &MainWindow::slot_open_settings_dialog);

  /* Crypt Menu
   */
  encrypt_act_ = new QAction(tr("Encrypt"), this);
  encrypt_act_->setIcon(QIcon(":/icons/encrypted.png"));
  encrypt_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));

  encrypt_act_->setToolTip(tr("Encrypt Message"));
  connect(encrypt_act_, &QAction::triggered, this, [this]() {
    if (edit_->SlotCurPageFileTreeView() != nullptr) {
      const auto* file_tree_view = edit_->SlotCurPageFileTreeView();
      const auto path = file_tree_view->GetSelected();

      const auto file_info = QFileInfo(path);
      if (file_info.isFile()) {
        this->SlotFileEncrypt(path);
      } else if (file_info.isDir()) {
        this->SlotDirectoryEncrypt(path);
      }
    }
    if (edit_->SlotCurPageTextEdit() != nullptr) {
      this->SlotEncrypt();
    }
  });

  encrypt_sign_act_ = new QAction(tr("Encrypt Sign"), this);
  encrypt_sign_act_->setIcon(QIcon(":/icons/encrypted_signed.png"));
  encrypt_sign_act_->setShortcut(
      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));

  encrypt_sign_act_->setToolTip(tr("Encrypt and Sign Message"));
  connect(encrypt_sign_act_, &QAction::triggered, this, [this]() {
    if (edit_->SlotCurPageFileTreeView() != nullptr) {
      const auto* file_tree_view = edit_->SlotCurPageFileTreeView();
      const auto path = file_tree_view->GetSelected();

      const auto file_info = QFileInfo(path);
      if (file_info.isFile()) {
        this->SlotFileEncryptSign(path);
      } else if (file_info.isDir()) {
        this->SlotDirectoryEncryptSign(path);
      }
    }
    if (edit_->SlotCurPageTextEdit() != nullptr) {
      this->SlotEncryptSign();
    }
  });

  decrypt_act_ = new QAction(tr("Decrypt"), this);
  decrypt_act_->setIcon(QIcon(":/icons/decrypted.png"));
  decrypt_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
  decrypt_act_->setToolTip(tr("Decrypt Message"));
  connect(decrypt_act_, &QAction::triggered, this, [this]() {
    if (edit_->SlotCurPageFileTreeView() != nullptr) {
      const auto* file_tree_view = edit_->SlotCurPageFileTreeView();
      const auto path = file_tree_view->GetSelected();

      const auto file_info = QFileInfo(path);
      if (file_info.isFile()) {
        const QString extension = file_info.completeSuffix();

        if (extension == "tar.gpg" || extension == "tar.asc") {
          this->SlotArchiveDecrypt(path);
        } else {
          this->SlotFileDecrypt(path);
        }
      }
    }
    if (edit_->SlotCurPageTextEdit() != nullptr) {
      this->SlotDecrypt();
    }
  });

  decrypt_verify_act_ = new QAction(tr("Decrypt Verify"), this);
  decrypt_verify_act_->setIcon(QIcon(":/icons/decrypted_verified.png"));
  decrypt_verify_act_->setShortcut(
      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
  decrypt_verify_act_->setToolTip(tr("Decrypt and Verify Message"));
  connect(decrypt_verify_act_, &QAction::triggered, this, [this]() {
    if (edit_->SlotCurPageFileTreeView() != nullptr) {
      const auto* file_tree_view = edit_->SlotCurPageFileTreeView();
      const auto path = file_tree_view->GetSelected();

      const auto file_info = QFileInfo(path);
      if (file_info.isFile()) {
        const QString extension = file_info.completeSuffix();

        if (extension == "tar.gpg" || extension == "tar.asc") {
          this->SlotArchiveDecryptVerify(path);
        } else {
          this->SlotFileDecryptVerify(path);
        }
      }
    }
    if (edit_->SlotCurPageTextEdit() != nullptr) {
      this->SlotDecryptVerify();
    }
  });

  sign_act_ = new QAction(tr("Sign"), this);
  sign_act_->setIcon(QIcon(":/icons/signature.png"));
  sign_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
  sign_act_->setToolTip(tr("Sign Message"));
  connect(sign_act_, &QAction::triggered, this, [this]() {
    if (edit_->SlotCurPageFileTreeView() != nullptr) {
      const auto* file_tree_view = edit_->SlotCurPageFileTreeView();
      const auto path = file_tree_view->GetSelected();

      const auto file_info = QFileInfo(path);
      if (file_info.isFile()) this->SlotFileSign(path);
    }
    if (edit_->SlotCurPageTextEdit() != nullptr) this->SlotSign();
  });

  verify_act_ = new QAction(tr("Verify"), this);
  verify_act_->setIcon(QIcon(":/icons/verify.png"));
  verify_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
  verify_act_->setToolTip(tr("Verify Message"));
  connect(verify_act_, &QAction::triggered, this, [this]() {
    if (edit_->SlotCurPageFileTreeView() != nullptr) {
      const auto* file_tree_view = edit_->SlotCurPageFileTreeView();
      const auto path = file_tree_view->GetSelected();

      const auto file_info = QFileInfo(path);
      if (file_info.isFile()) this->SlotFileVerify(path);
    }
    if (edit_->SlotCurPageTextEdit() != nullptr) this->SlotVerify();
  });

  /* Key Menu
   */

  import_key_from_file_act_ = new QAction(tr("File"), this);
  import_key_from_file_act_->setIcon(QIcon(":/icons/import_key_from_file.png"));
  import_key_from_file_act_->setToolTip(tr("Import New Key From File"));
  connect(import_key_from_file_act_, &QAction::triggered, this,
          [&]() { CommonUtils::GetInstance()->SlotImportKeyFromFile(this); });

  import_key_from_clipboard_act_ = new QAction(tr("Clipboard"), this);
  import_key_from_clipboard_act_->setIcon(
      QIcon(":/icons/import_key_from_clipboard.png"));
  import_key_from_clipboard_act_->setToolTip(
      tr("Import New Key From Clipboard"));
  connect(import_key_from_clipboard_act_, &QAction::triggered, this, [&]() {
    CommonUtils::GetInstance()->SlotImportKeyFromClipboard(this);
  });

  bool forbid_all_gnupg_connection =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("network/forbid_all_gnupg_connection", false)
          .toBool();

  import_key_from_key_server_act_ = new QAction(tr("Keyserver"), this);
  import_key_from_key_server_act_->setIcon(
      QIcon(":/icons/import_key_from_server.png"));
  import_key_from_key_server_act_->setToolTip(
      tr("Import New Key From Keyserver"));
  import_key_from_key_server_act_->setDisabled(forbid_all_gnupg_connection);
  connect(import_key_from_key_server_act_, &QAction::triggered, this, [&]() {
    CommonUtils::GetInstance()->SlotImportKeyFromKeyServer(this);
  });

  import_key_from_edit_act_ = new QAction(tr("Editor"), this);
  import_key_from_edit_act_->setIcon(QIcon(":/icons/editor.png"));
  import_key_from_edit_act_->setToolTip(tr("Import New Key From Editor"));
  connect(import_key_from_edit_act_, &QAction::triggered, this,
          &MainWindow::slot_import_key_from_edit);

  open_key_management_act_ = new QAction(tr("Manage Keys"), this);
  open_key_management_act_->setIcon(QIcon(":/icons/keymgmt.png"));
  open_key_management_act_->setToolTip(tr("Open Key Management"));
  connect(open_key_management_act_, &QAction::triggered, this,
          &MainWindow::slot_open_key_management);

  clean_gpg_password_cache_act_ = new QAction(tr("Clear Password Cache"), this);
  clean_gpg_password_cache_act_->setIcon(QIcon(":/icons/clear-f.png"));
  clean_gpg_password_cache_act_->setToolTip(
      tr("Clear Password Cache of GnuPG"));
  connect(clean_gpg_password_cache_act_, &QAction::triggered, this, [=]() {
    GpgFrontend::GpgAdvancedOperator::ClearGpgPasswordCache([=](int err,
                                                                DataObjectPtr) {
      if (err >= 0) {
        QMessageBox::information(this, tr("Successful Operation"),
                                 tr("Clear password cache successfully"));
      } else {
        QMessageBox::critical(this, tr("Failed Operation"),
                              tr("Failed to clear password cache of GnuPG"));
      }
    });
  });

  reload_components_act_ = new QAction(tr("Reload All Components"), this);
  reload_components_act_->setIcon(QIcon(":/icons/restart.png"));
  reload_components_act_->setToolTip(tr("Reload All GnuPG's Components"));
  connect(reload_components_act_, &QAction::triggered, this, [=]() {
    GpgFrontend::GpgAdvancedOperator::ReloadGpgComponents(
        [=](int err, DataObjectPtr) {
          if (err >= 0) {
            QMessageBox::information(
                this, tr("Successful Operation"),
                tr("Reload all the GnuPG's components successfully"));
          } else {
            QMessageBox::critical(
                this, tr("Failed Operation"),
                tr("Failed to reload all or one of the GnuPG's component(s)"));
          }
        });
  });

  restart_components_act_ = new QAction(tr("Restart All Components"), this);
  restart_components_act_->setIcon(QIcon(":/icons/restart.png"));
  restart_components_act_->setToolTip(tr("Restart All GnuPG's Components"));
  connect(restart_components_act_, &QAction::triggered, this, [=]() {
    GpgFrontend::GpgAdvancedOperator::RestartGpgComponents();
    Module::ListenRTPublishEvent(
        this, "core", "gpg_advanced_operator.restart_gpg_components",
        [=](Module::Namespace, Module::Key, int, std::any value) {
          bool success_state = std::any_cast<bool>(value);
          if (success_state) {
            QMessageBox::information(
                this, tr("Successful Operation"),
                tr("Restart all the GnuPG's components successfully"));
          } else {
            QMessageBox::critical(
                this, tr("Failed Operation"),
                tr("Failed to restart all or one of the GnuPG's component(s)"));
          }
        });
  });

  gnupg_controller_open_act_ = new QAction(tr("Open GnuPG Controller"), this);
  gnupg_controller_open_act_->setIcon(QIcon(":/icons/configure.png"));
  gnupg_controller_open_act_->setToolTip(tr("Open GnuPG Controller Dialog"));
  connect(gnupg_controller_open_act_, &QAction::triggered, this,
          [this]() { (new GnuPGControllerDialog(this))->exec(); });

  module_controller_open_act_ = new QAction(tr("Open Module Controller"), this);
  module_controller_open_act_->setIcon(QIcon(":/icons/module.png"));
  module_controller_open_act_->setToolTip(tr("Open Module Controller Dialog"));
  connect(module_controller_open_act_, &QAction::triggered, this,
          [this]() { (new ModuleControllerDialog(this))->exec(); });

  /*
   * About Menu
   */
  about_act_ = new QAction(tr("About"), this);
  about_act_->setIcon(QIcon(":/icons/help.png"));
  about_act_->setToolTip(tr("Show the application's About box"));
  about_act_->setMenuRole(QAction::AboutRole);
  connect(about_act_, &QAction::triggered, this,
          [=]() { new AboutDialog(0, this); });

  if (Module::IsModuleActivate(kGnuPGInfoGatheringModuleID)) {
    gnupg_act_ = new QAction(tr("GnuPG"), this);
    gnupg_act_->setIcon(QIcon(":/icons/key.png"));
    gnupg_act_->setToolTip(tr("Information about Gnupg"));
    connect(gnupg_act_, &QAction::triggered, this,
            [=]() { new AboutDialog(tr("GnuPG"), this); });
  }

  translate_act_ = new QAction(tr("Translate"), this);
  translate_act_->setIcon(QIcon(":/icons/translate.png"));
  translate_act_->setToolTip(tr("Information about translation"));
  connect(translate_act_, &QAction::triggered, this,
          [=]() { new AboutDialog(tr("Translators"), this); });

  if (Module::IsModuleActivate(kVersionCheckingModuleID)) {
    check_update_act_ = new QAction(tr("Check for Updates"), this);
    check_update_act_->setIcon(QIcon(":/icons/update.png"));
    check_update_act_->setToolTip(tr("Check for updates"));
    connect(check_update_act_, &QAction::triggered, this,
            [=]() { new AboutDialog(tr("Update"), this); });
  }

  start_wizard_act_ = new QAction(tr("Open Wizard"), this);
  start_wizard_act_->setIcon(QIcon(":/icons/wizard.png"));
  start_wizard_act_->setToolTip(tr("Open the wizard"));
  connect(start_wizard_act_, &QAction::triggered, this,
          &MainWindow::slot_start_wizard);

  append_selected_keys_act_ =
      new QAction(tr("Append Public Key to Editor"), this);
  append_selected_keys_act_->setToolTip(
      tr("Append selected Keypair's Public Key to Editor"));
  connect(append_selected_keys_act_, &QAction::triggered, this,
          &MainWindow::slot_append_selected_keys);

  append_key_create_date_to_editor_act_ =
      new QAction(tr("Append Create DateTime to Editor"), this);
  append_key_create_date_to_editor_act_->setToolTip(
      tr("Append selected Key's creation date and time to Editor"));
  connect(append_key_create_date_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_create_datetime);

  append_key_expire_date_to_editor_act_ =
      new QAction(tr("Append Expire DateTime to Editor"), this);
  append_key_expire_date_to_editor_act_->setToolTip(
      tr("Append selected Key's expiration date and time to Editor"));
  connect(append_key_expire_date_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_expire_datetime);

  append_key_fingerprint_to_editor_act_ =
      new QAction(tr("Append Fingerprint to Editor"), this);
  append_key_expire_date_to_editor_act_->setToolTip(
      tr("Append selected Key's Fingerprint to Editor"));
  connect(append_key_fingerprint_to_editor_act_, &QAction::triggered, this,
          &MainWindow::slot_append_keys_fingerprint);

  copy_mail_address_to_clipboard_act_ = new QAction(tr("Copy Email"), this);
  copy_mail_address_to_clipboard_act_->setToolTip(
      tr("Copy selected Keypair's to clipboard"));
  connect(copy_mail_address_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_mail_address_to_clipboard);

  copy_key_default_uid_to_clipboard_act_ =
      new QAction(tr("Copy Default UID"), this);
  copy_key_default_uid_to_clipboard_act_->setToolTip(
      tr("Copy selected Keypair's default UID to clipboard"));
  connect(copy_key_default_uid_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_default_uid_to_clipboard);

  copy_key_id_to_clipboard_act_ = new QAction(tr("Copy Key ID"), this);
  copy_key_id_to_clipboard_act_->setToolTip(
      tr("Copy selected Keypair's ID to clipboard"));
  connect(copy_key_id_to_clipboard_act_, &QAction::triggered, this,
          &MainWindow::slot_copy_key_id_to_clipboard);

  show_key_details_act_ = new QAction(tr("Show Key Details"), this);
  show_key_details_act_->setToolTip(tr("Show Details for this Key"));
  connect(show_key_details_act_, &QAction::triggered, this,
          &MainWindow::slot_show_key_details);

  add_key_2_favourite_act_ = new QAction(tr("Add To Favourite"), this);
  add_key_2_favourite_act_->setToolTip(tr("Add this key to Favourite Table"));
  add_key_2_favourite_act_->setData(QVariant("add_key_2_favourite_action"));
  connect(add_key_2_favourite_act_, &QAction::triggered, this,
          &MainWindow::slot_add_key_2_favourite);

  remove_key_from_favourtie_act_ =
      new QAction(tr("Remove From Favourite"), this);
  remove_key_from_favourtie_act_->setToolTip(
      tr("Remove this key from Favourite Table"));
  remove_key_from_favourtie_act_->setData(
      QVariant("remove_key_from_favourtie_action"));
  connect(remove_key_from_favourtie_act_, &QAction::triggered, this,
          &MainWindow::slot_remove_key_from_favourite);

  set_owner_trust_of_key_act_ = new QAction(tr("Set Owner Trust Level"), this);
  set_owner_trust_of_key_act_->setToolTip(tr("Set Owner Trust Level"));
  set_owner_trust_of_key_act_->setData(QVariant("set_owner_trust_level"));
  connect(set_owner_trust_of_key_act_, &QAction::triggered, this,
          &MainWindow::slot_set_owner_trust_level_of_key);

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

  cut_pgp_header_act_ = new QAction(tr("Remove PGP Header"), this);
  cut_pgp_header_act_->setIcon(QIcon(":/icons/minus.png"));
  connect(cut_pgp_header_act_, &QAction::triggered, this,
          &MainWindow::slot_cut_pgp_header);

  add_pgp_header_act_ = new QAction(tr("Add PGP Header"), this);
  add_pgp_header_act_->setIcon(QIcon(":/icons/add.png"));
  connect(add_pgp_header_act_, &QAction::triggered, this,
          &MainWindow::slot_add_pgp_header);
}

void MainWindow::create_menus() {
  file_menu_ = menuBar()->addMenu(tr("File"));
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

  advance_menu_ = menuBar()->addMenu(tr("Advance"));
  advance_menu_->addAction(clean_gpg_password_cache_act_);
  advance_menu_->addAction(reload_components_act_);
  advance_menu_->addAction(restart_components_act_);
  advance_menu_->addSeparator();
  advance_menu_->addAction(gnupg_controller_open_act_);
  advance_menu_->addAction(module_controller_open_act_);

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
  file_tool_bar_->addAction(open_act_);
  file_tool_bar_->addAction(browser_act_);
  view_menu_->addAction(file_tool_bar_->toggleViewAction());

  crypt_tool_bar_ = addToolBar(tr("Operations"));
  crypt_tool_bar_->setObjectName("cryptToolBar");
  crypt_tool_bar_->addAction(encrypt_act_);
  crypt_tool_bar_->addAction(encrypt_sign_act_);
  crypt_tool_bar_->addAction(decrypt_act_);
  crypt_tool_bar_->addAction(decrypt_verify_act_);
  crypt_tool_bar_->addAction(sign_act_);
  crypt_tool_bar_->addAction(verify_act_);
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
  key_list_dock_->setMinimumWidth(460);
  addDockWidget(Qt::RightDockWidgetArea, key_list_dock_);

  m_key_list_->AddListGroupTab(
      tr("Default"), "default",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgKey& key) -> bool {
        return !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  m_key_list_->AddListGroupTab(
      tr("Favourite"), "favourite",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY |
          GpgKeyTableDisplayMode::kFAVORITES,
      [](const GpgKey& key) -> bool {
        return CommonUtils::GetInstance()->KeyExistsinFavouriteList(key);
      });

  m_key_list_->AddListGroupTab(
      tr("Only Public Key"), "only_public_key",
      GpgKeyTableDisplayMode::kPUBLIC_KEY, [](const GpgKey& key) -> bool {
        return !key.IsPrivateKey() &&
               !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  m_key_list_->AddListGroupTab(
      tr("Has Private Key"), "has_private_key",
      GpgKeyTableDisplayMode::kPRIVATE_KEY, [](const GpgKey& key) -> bool {
        return key.IsPrivateKey() &&
               !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  m_key_list_->SlotRefresh();

  key_list_dock_->setWidget(m_key_list_);
  view_menu_->addAction(key_list_dock_->toggleViewAction());

  info_board_dock_ = new QDockWidget(tr("Information Board"), this);
  info_board_dock_->setObjectName("Information Board");
  info_board_dock_->setAllowedAreas(Qt::BottomDockWidgetArea);
  addDockWidget(Qt::BottomDockWidgetArea, info_board_dock_);
  info_board_dock_->setWidget(info_board_);
  info_board_dock_->widget()->layout()->setContentsMargins(0, 0, 0, 0);
  view_menu_->addAction(info_board_dock_->toggleViewAction());
}

}  // namespace GpgFrontend::UI
