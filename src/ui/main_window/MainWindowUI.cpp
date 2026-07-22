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
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/function/openpgp/support/KeyGenerationOpSupport.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/controller/ModuleControllerDialog.h"
#include "ui/dialog/controller/SmartCardControllerDialog.h"
#include "ui/dialog/help/AboutDialog.h"
#include "ui/dialog/key_generate/KeyGenerateDialog.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

void MainWindow::create_actions() {
  new_tab_act_ = create_action(
      "new_tab", tr("New Text Editor"), ":/icons/misc_doc.png",
      tr("Open a new text editor"),
      {QKeySequence(Qt::CTRL | Qt::Key_N), QKeySequence(Qt::CTRL | Qt::Key_T)});
  connect(new_tab_act_, &QAction::triggered, edit_, &TextEdit::SlotNewTab);

  browser_act_ = create_action(
      "file_browser_dir", tr("New File Panel"), ":/icons/file-operator.png",
      tr("Open a new file panel"), {QKeySequence(Qt::CTRL | Qt::Key_B)});
  connect(browser_act_, &QAction::triggered, this,
          &MainWindow::slot_default_file_tab);

  browser_file_act_ =
      create_action("file_browser", tr("File..."), ":/icons/file-operator.png",
                    tr("Open a file in the file panel"));
  connect(browser_file_act_, &QAction::triggered, this,
          &MainWindow::slot_open_file_tab);

  browser_dir_act_ = create_action(
      "open_directory_in_file_panel", tr("Directory..."),
      ":/icons/file-operator.png", tr("Open a directory in the file panel"));
  connect(browser_dir_act_, &QAction::triggered, this,
          &MainWindow::slot_open_file_tab_with_directory);

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

  close_tab_act_ =
      create_action("close_tab", tr("Close Tab"), ":/icons/close.png",
                    tr("Close the current tab"), {QKeySequence::Close});
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
      create_action("encrypt_sign", tr("Encrypt && Sign"),
                    ":/icons/encr-sign.png", tr("Encrypt and Sign Message"),
                    {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E)});
  connect(encrypt_sign_act_, &QAction::triggered, this,
          &MainWindow::SlotGeneralEncryptSign);

  decrypt_act_ = create_action("decrypt", tr("Decrypt"), ":/icons/unlock.png",
                               tr("Decrypt Message"),
                               {QKeySequence(Qt::CTRL | Qt::Key_D)});
  connect(decrypt_act_, &QAction::triggered, this,
          &MainWindow::SlotGeneralDecrypt);

  decrypt_verify_act_ =
      create_action("decrypt_verify", tr("Decrypt && Verify"),
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

  sym_encrypt_act_ = create_action(
      "symmetric_encryption", tr("Sym. Encrypt"),
      ":/icons/symmetric_encryption.png", tr("Encrypt Message (Symmetric)"),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_E)});
  connect(sym_encrypt_act_, &QAction::triggered, this,
          &MainWindow::SlotGeneralEncrypt);

  /*
   * Key Menu
   */

  generate_key_pair_act_ =
      create_action("generate_key_pair", tr("New Keypair"),
                    ":/icons/keypairs.png", tr("Generate KeyPair"));
  connect(generate_key_pair_act_, &QAction::triggered, this, [=]() -> void {
    new KeyGenerateDialog(m_key_list_->GetCurrentGpgContextChannel(), this);
  });
  generate_key_pair_act_->setDisabled(!IsOpSupported<GenerateKeyTag>(
      m_key_list_->GetCurrentGpgContextChannel()));

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

  /*
   * About Menu
   */
  about_act_ = create_action("about", tr("About"), ":/icons/help.png",
                             tr("Show the application's About box"));
  about_act_->setMenuRole(QAction::AboutRole);
  connect(about_act_, &QAction::triggered, this, [=]() {
    auto* dialog = new AboutDialog(0, this);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
  });

  start_wizard_act_ =
      create_action("start_wizard", tr("Open Wizard"), ":/icons/wizard.png",
                    tr("Open the wizard"));
  connect(start_wizard_act_, &QAction::triggered, this,
          &MainWindow::slot_start_wizard);

  show_log_view_act_ =
      create_action("show_log_view", tr("Show Application Log"),
                    ":/icons/log.png", tr("Show the application log view"));
  connect(show_log_view_act_, &QAction::triggered, this,
          &MainWindow::slot_show_log_view);

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

  im_encrypt_act_ = create_action(
      "im_encrypt", tr("Encrypt for Instant Messaging"), ":/icons/email.png",
      tr("Encrypt the current text into a compact, single-line format that is "
         "safe to paste into instant messengers. Recipients decrypt it with "
         "the normal Decrypt action."),
      {QKeySequence(Qt::CTRL | Qt::Key_M)});
  connect(im_encrypt_act_, &QAction::triggered, this,
          &MainWindow::slot_im_encrypt_message);

  im_encrypt_sign_act_ = create_action(
      "im_encrypt_sign", tr("Encrypt && Sign for Instant Messaging"),
      ":/icons/email-check.png",
      tr("Encrypt and sign the current text into a compact, single-line format "
         "that is safe to paste into instant messengers. Recipients decrypt "
         "and verify it with the normal Decrypt & Verify action."),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M)});
  connect(im_encrypt_sign_act_, &QAction::triggered, this,
          &MainWindow::slot_im_encrypt_sign_message);
}

void MainWindow::create_menus() {
  file_menu_ = menuBar()->addMenu(tr("File"));

  open_menu_ = file_menu_->addMenu(tr("Open"));
  open_menu_->setToolTipsVisible(true);
  open_menu_->addAction(browser_file_act_);
  open_menu_->addAction(browser_dir_act_);

  workspace_menu_ = file_menu_->addMenu(tr("Workspace"));
  workspace_menu_->setToolTipsVisible(true);
  workspace_menu_->addAction(browser_act_);
  workspace_menu_->addAction(new_tab_act_);

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
  edit_menu_->addSeparator();
  edit_menu_->addAction(open_settings_act_);

  crypt_menu_ = menuBar()->addMenu(tr("Operations"));
  crypt_menu_->addAction(sym_encrypt_act_);
  crypt_menu_->addAction(encrypt_act_);
  crypt_menu_->addAction(im_encrypt_act_);
  crypt_menu_->addAction(encrypt_sign_act_);
  crypt_menu_->addAction(im_encrypt_sign_act_);
  crypt_menu_->addAction(decrypt_act_);
  crypt_menu_->addAction(decrypt_verify_act_);
  crypt_menu_->addSeparator();
  crypt_menu_->addAction(sign_act_);
  crypt_menu_->addAction(verify_act_);
  crypt_menu_->addSeparator();

  key_menu_ = menuBar()->addMenu(tr("Keys"));
  key_menu_->addAction(generate_key_pair_act_);
  import_key_menu_ = key_menu_->addMenu(tr("Import Key"));
  import_key_menu_->setIcon(QIcon(":/icons/key_import.png"));
  import_key_menu_->addAction(import_key_from_file_act_);
  import_key_menu_->addAction(import_key_from_edit_act_);
  import_key_menu_->addAction(import_key_from_clipboard_act_);
  key_menu_->addAction(open_key_management_act_);

  advance_menu_ = menuBar()->addMenu(tr("Advanced"));
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    advance_menu_->addAction(clean_gpg_password_cache_act_);
    advance_menu_->addAction(reload_components_act_);
    advance_menu_->addAction(restart_components_act_);
    advance_menu_->addSeparator();
  }
  // Only show GnuPG Controller and Smart Card Controller if GnuPG is
  // supported, since they are not useful for other engines.
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    advance_menu_->addAction(smart_card_controller_open_act_);
  }
  advance_menu_->addAction(module_controller_open_act_);

  // Hide the "Advanced" menu if running in sandbox, since most of the features
  // in the "Advanced" menu are not useful in sandbox and may cause confusion to
  // users.
  if (IsRunningInSandBox()) {
    advance_menu_->menuAction()->setVisible(false);
  }

  view_menu_ = menuBar()->addMenu(tr("View"));

  help_menu_ = menuBar()->addMenu(tr("Help"));
  help_menu_->addAction(start_wizard_act_);
  help_menu_->addAction(show_log_view_act_);
  help_menu_->addSeparator();

  help_menu_->addAction(about_act_);

  Module::TriggerEvent(
      "MAINWINDOW_MENU_MOUNTED",
      {
          {"main_window", GFBuffer(RegisterQObject(this))},
          {"file_menu", GFBuffer(RegisterQObject(file_menu_))},
          {"file_open_menu", GFBuffer(RegisterQObject(open_menu_))},
          {"file_workspace_menu", GFBuffer(RegisterQObject(workspace_menu_))},
          {"edit_menu", GFBuffer(RegisterQObject(edit_menu_))},
          {"crypt_menu", GFBuffer(RegisterQObject(crypt_menu_))},
          {"key_menu", GFBuffer(RegisterQObject(key_menu_))},
          {"advance_menu", GFBuffer(RegisterQObject(advance_menu_))},
          {"help_menu", GFBuffer(RegisterQObject(help_menu_))},
          {"view_menu", GFBuffer(RegisterQObject(view_menu_))},
          {"import_key_menu", GFBuffer(RegisterQObject(import_key_menu_))},
      });
}

namespace {

auto SetupMenuToolButton(QToolButton* button, QMenu* menu, const QIcon& icon,
                         const QString& text, const QString& tooltip,
                         Qt::ToolButtonStyle style, QSize size) -> void {
  button->setMenu(menu);
  button->setPopupMode(QToolButton::InstantPopup);
  button->setIcon(icon);
  button->setText(text);
  button->setToolTip(tooltip);
  button->setFocusPolicy(Qt::NoFocus);
  button->setAutoRaise(false);
  button->setToolButtonStyle(style);
  button->setIconSize(size);
}

auto SetupToolBar(QToolBar* toolbar, Qt::ToolButtonStyle style, QSize size)
    -> void {
  toolbar->setMovable(true);
  toolbar->setFloatable(false);
  toolbar->setToolButtonStyle(style);
  toolbar->setIconSize(size);
}

}  // namespace

void MainWindow::create_tool_bars() {
  file_tool_bar_ = addToolBar(tr("File"));
  file_tool_bar_->setObjectName("fileToolBar");
  SetupToolBar(file_tool_bar_, icon_style_, icon_size_);

  open_button_ = new QToolButton(this);
  SetupMenuToolButton(open_button_, open_menu_, QIcon(":/icons/open.png"),
                      tr("Open"), tr("Open a file or directory"), icon_style_,
                      icon_size_);
  file_tool_bar_->addWidget(open_button_);

  workspace_button_ = new QToolButton(this);
  SetupMenuToolButton(workspace_button_, workspace_menu_,
                      QIcon(":/icons/workspace.png"), tr("Workspace"),
                      tr("Open a text editor or file panel"), icon_style_,
                      icon_size_);
  file_tool_bar_->addWidget(workspace_button_);

  file_tool_bar_->addSeparator();
  file_tool_bar_->addAction(save_act_);

  view_menu_->addAction(file_tool_bar_->toggleViewAction());

  crypt_tool_bar_ = addToolBar(tr("Operations"));
  crypt_tool_bar_->setObjectName("cryptToolBar");
  SetupToolBar(crypt_tool_bar_, icon_style_, icon_size_);
  view_menu_->addAction(crypt_tool_bar_->toggleViewAction());

  key_tool_bar_ = addToolBar(tr("Keys"));
  key_tool_bar_->setObjectName("keyToolBar");
  SetupToolBar(key_tool_bar_, icon_style_, icon_size_);

  import_button_ = new QToolButton(this);
  SetupMenuToolButton(import_button_, import_key_menu_,
                      QIcon(":/icons/key_import.png"), tr("Import"),
                      tr("Import a key from file, editor, or clipboard"),
                      icon_style_, icon_size_);

  key_tool_bar_->addAction(generate_key_pair_act_);
  key_tool_bar_->addWidget(import_button_);
  key_tool_bar_->addAction(open_key_management_act_);

  view_menu_->addAction(key_tool_bar_->toggleViewAction());

  edit_tool_bar_ = addToolBar(tr("Edit"));
  edit_tool_bar_->setObjectName("editToolBar");
  SetupToolBar(edit_tool_bar_, icon_style_, icon_size_);
  edit_tool_bar_->addAction(copy_act_);
  edit_tool_bar_->addAction(paste_act_);
  edit_tool_bar_->addAction(select_all_act_);
  edit_tool_bar_->hide();
  view_menu_->addAction(edit_tool_bar_->toggleViewAction());

  special_edit_tool_bar_ = addToolBar(tr("Text Tools"));
  special_edit_tool_bar_->setObjectName("specialEditToolBar");
  SetupToolBar(special_edit_tool_bar_, icon_style_, icon_size_);
  special_edit_tool_bar_->addAction(quote_act_);
  special_edit_tool_bar_->addAction(clean_double_line_breaks_act_);
  special_edit_tool_bar_->addAction(im_encrypt_act_);
  special_edit_tool_bar_->addAction(im_encrypt_sign_act_);
  special_edit_tool_bar_->hide();
  view_menu_->addAction(special_edit_tool_bar_->toggleViewAction());
}

void MainWindow::create_status_bar() {
  // Show the current OpenPGP engine and version in the status bar
  engine_status_label_ = new QLabel(this);
  engine_status_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  engine_status_label_->setToolTip(tr("Current OpenPGP backend and version"));

  statusBar()->addPermanentWidget(engine_status_label_);

  auto* status_bar_box = new QWidget();
  auto* status_bar_box_layout = new QHBoxLayout();

  statusBar()->showMessage(tr("Ready"), 2000);
  status_bar_box->setLayout(status_bar_box_layout);
}

void MainWindow::create_dock_windows() {
  key_list_dock_ = new QDockWidget(tr("Key ToolBox"), this);
  key_list_dock_->setObjectName(QStringLiteral("keyListDock"));
  key_list_dock_->setAllowedAreas(Qt::LeftDockWidgetArea |
                                  Qt::RightDockWidgetArea);
  // Wider default so the vertical category tab column leaves the key table
  // usable.
  key_list_dock_->setMinimumWidth(320);
  key_list_dock_->setMaximumWidth(QWIDGETSIZE_MAX);

  addDockWidget(Qt::RightDockWidgetArea, key_list_dock_);

  // The dock is width-constrained, so use the compact colour rail for the
  // category strip (the Key Management window keeps the full text list).
  m_key_list_->SetCategoryRailCompact(true);

  // The dock is an operation panel (check a key or two, then run a PGP op), so
  // default to a compact, identity-focused column set and persist the choice
  // independently of the Key Management window. The optional columns (Key ID,
  // Trust, ...) remain available through the Columns chooser.
  m_key_list_->SetPersistenceScope(
      "toolbox", GpgKeyTableColumn::kTYPE | GpgKeyTableColumn::kNAME |
                     GpgKeyTableColumn::kEMAIL_ADDRESS |
                     GpgKeyTableColumn::kUSAGE);

  m_key_list_->AddListGroupTab(
      tr("Default"), "default",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgAbstractKey* key) -> bool {
        return !(key->IsRevoked() || key->IsDisabled() || key->IsExpired());
      });

  m_key_list_->AddListGroupTab(
      tr("Key Group"), "key_group", GpgKeyTableDisplayMode::kPUBLIC_KEY,
      [](const GpgAbstractKey* key) -> bool {
        return key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP &&
               !key->IsDisabled();
      });

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

  // Per-window order for the integrated (built-in) tabs; custom categories use
  // a separate, shared order (see KeyList).
  m_key_list_->SetTabOrderSettingsKey("keys/toolbox_tab_order");

  // The dock's checked keys are the everyday working set (the recipients of the
  // next operation), so this is the one key list that may carry them across
  // restarts; the user setting decides whether it actually does.
  m_key_list_->SetRememberCheckedKeys(true);

  m_key_list_->RebuildCategoryTabs();

  m_key_list_->setMinimumWidth(320);
  m_key_list_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_key_list_->SlotRefresh();

  key_list_dock_->setWidget(m_key_list_);
  view_menu_->addAction(key_list_dock_->toggleViewAction());

  info_board_dock_ = new QDockWidget(tr("Status Panel"), this);
  info_board_dock_->setObjectName(QStringLiteral("infoBoardDock"));
  info_board_dock_->setAllowedAreas(Qt::BottomDockWidgetArea);
  info_board_dock_->setMinimumHeight(120);
  info_board_dock_->setMaximumHeight(QWIDGETSIZE_MAX);

  addDockWidget(Qt::BottomDockWidgetArea, info_board_dock_);
  info_board_dock_->setWidget(info_board_);

  if (info_board_dock_->widget() != nullptr &&
      info_board_dock_->widget()->layout() != nullptr) {
    info_board_dock_->widget()->layout()->setContentsMargins(0, 0, 0, 0);
  }

  view_menu_->addAction(info_board_dock_->toggleViewAction());

  connect(m_key_list_, &KeyList::SignalKeyChecked, this,
          [=]() { slot_update_operations_menu_by_checked_keys(~0); });
}

namespace {

auto ClampInt(int value, int min, int max) -> int {
  return std::clamp(value, min, max);
}

auto CurrentAvailableGeometry(QWidget* widget) -> QRect {
  const auto* screen = widget != nullptr && widget->screen() != nullptr
                           ? widget->screen()
                           : QGuiApplication::primaryScreen();

  if (screen == nullptr) {
    return QRect(0, 0, 1200, 760);
  }

  return screen->availableGeometry();
}

}  // namespace

void MainWindow::apply_default_layout() {
  const QRect available = CurrentAvailableGeometry(this);

  constexpr double kWindowScale = 0.82;
  constexpr double kTargetAspect = 1.45;
  constexpr double kKeyListRatio = 0.35;
  constexpr double kInfoBoardRatio = 0.30;

  int target_width =
      ClampInt(static_cast<int>(available.width() * kWindowScale), 900, 1360);

  int target_height =
      ClampInt(static_cast<int>(target_width / kTargetAspect), 700, 1000);

  if (target_height > static_cast<int>(available.height() * kWindowScale)) {
    target_height =
        ClampInt(static_cast<int>(available.height() * kWindowScale), 680, 960);

    target_width =
        ClampInt(static_cast<int>(target_height * kTargetAspect), 900, 1360);
  }

  resize(target_width, target_height);

  if (key_list_dock_ != nullptr) {
    const int key_min_width = available.width() < 1100 ? 260 : 300;

    const int key_max_width =
        ClampInt(static_cast<int>(target_width * 0.48), 420, 620);

    const int key_width =
        ClampInt(static_cast<int>(target_width * kKeyListRatio), key_min_width,
                 key_max_width);

    key_list_dock_->setMinimumWidth(key_min_width);
    key_list_dock_->setMaximumWidth(QWIDGETSIZE_MAX);

    resizeDocks({key_list_dock_}, {key_width}, Qt::Horizontal);
  }

  if (info_board_dock_ != nullptr) {
    const int info_min_height = available.height() < 750 ? 110 : 135;

    const int info_max_height =
        ClampInt(static_cast<int>(target_height * 0.34), 220, 340);

    const int info_height =
        ClampInt(static_cast<int>(target_height * kInfoBoardRatio),
                 info_min_height, info_max_height);

    info_board_dock_->setMinimumHeight(info_min_height);
    info_board_dock_->setMaximumHeight(QWIDGETSIZE_MAX);

    resizeDocks({info_board_dock_}, {info_height}, Qt::Vertical);
  }

  QRect target_rect = frameGeometry();
  target_rect.setSize(size());
  target_rect.moveCenter(available.center());
  target_rect = ClampRectToAvailableGeometry(target_rect, available);
  move(target_rect.topLeft());
}

void MainWindow::init_main_window_style() {
  setStyleSheet(R"(
QToolBar {
  spacing: 3px;
  padding: 2px;
  border: none;
  background: palette(window);
}

QToolBar::separator {
  width: 1px;
  margin: 4px 5px;
  background: palette(mid);
}

QStatusBar {
  border-top: 1px solid palette(mid);
  background: palette(window);
}
)");
}

void MainWindow::apply_tool_bar_appearance() {
  const auto tool_button_style = icon_style_;
  const auto icon_size = icon_size_;

  const QList<QToolBar*> toolbars = {
      file_tool_bar_, crypt_tool_bar_,        key_tool_bar_,
      edit_tool_bar_, special_edit_tool_bar_,
  };

  for (auto* toolbar : toolbars) {
    if (toolbar == nullptr) continue;

    toolbar->setToolButtonStyle(tool_button_style);
    toolbar->setIconSize(icon_size);
  }

  const QList<QToolButton*> menu_buttons = {
      open_button_,
      workspace_button_,
      import_button_,
  };

  for (auto* button : menu_buttons) {
    if (button == nullptr) continue;

    button->setToolButtonStyle(tool_button_style);
    button->setIconSize(icon_size);
    button->updateGeometry();
    button->update();
  }

  setToolButtonStyle(tool_button_style);
  setIconSize(icon_size);
}

}  // namespace GpgFrontend::UI
