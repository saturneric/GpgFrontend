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
#include "core/GpgConstants.h"
#include "core/function/GlobalSettingStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/struct/SettingsObject.h"

namespace GpgFrontend::UI {

void MainWindow::SlotSetStatusBarText(const QString& text) {
  statusBar()->showMessage(text, 20000);
}

void MainWindow::slot_start_wizard() {
  auto* wizard = new Wizard(this);
  wizard->show();
  wizard->setModal(true);
}

void MainWindow::slot_import_key_from_edit() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) return;
  CommonUtils::GetInstance()->SlotImportKeys(
      this, edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString());
}

void MainWindow::slot_open_key_management() {
  auto* dialog = new KeyMgmt(this);
  dialog->show();
  dialog->raise();
}

void MainWindow::slot_open_file_tab() { edit_->SlotNewFileTab(); }

void MainWindow::slot_disable_tab_actions(int number) {
  bool disable;

  if (number == -1)
    disable = true;
  else
    disable = false;

  if (edit_->CurFilePage() != nullptr) {
    disable = true;
  }

  print_act_->setDisabled(disable);
  save_act_->setDisabled(disable);
  save_as_act_->setDisabled(disable);
  quote_act_->setDisabled(disable);
  cut_act_->setDisabled(disable);
  copy_act_->setDisabled(disable);
  paste_act_->setDisabled(disable);
  close_tab_act_->setDisabled(disable);
  select_all_act_->setDisabled(disable);
  find_act_->setDisabled(disable);
  verify_act_->setDisabled(disable);
  sign_act_->setDisabled(disable);
  encrypt_act_->setDisabled(disable);
  encrypt_sign_act_->setDisabled(disable);
  decrypt_act_->setDisabled(disable);
  decrypt_verify_act_->setDisabled(disable);

  redo_act_->setDisabled(disable);
  undo_act_->setDisabled(disable);
  zoom_out_act_->setDisabled(disable);
  zoom_in_act_->setDisabled(disable);
  clean_double_line_breaks_act_->setDisabled(disable);
  quote_act_->setDisabled(disable);
  append_selected_keys_act_->setDisabled(disable);
  import_key_from_edit_act_->setDisabled(disable);

  cut_pgp_header_act_->setDisabled(disable);
  add_pgp_header_act_->setDisabled(disable);
}

void MainWindow::slot_open_settings_dialog() {
  auto dialog = new SettingsDialog(this);

  connect(dialog, &SettingsDialog::finished, this, [&]() -> void {
    SettingsObject general_settings_state("general_settings_state");

    int width = general_settings_state.Check("icon_size").Check("width", 24),
        height = general_settings_state.Check("icon_size").Check("height", 24);
    SPDLOG_DEBUG("icon_size: {} {}", width, height);

    general_settings_state.Check("info_font_size", 10);

    // icon_style
    int s_icon_style =
        general_settings_state.Check("icon_style", Qt::ToolButtonTextUnderIcon);
    auto icon_style = static_cast<Qt::ToolButtonStyle>(s_icon_style);
    this->setToolButtonStyle(icon_style);
    import_button_->setToolButtonStyle(icon_style);

    // icons ize
    this->setIconSize(QSize(width, height));
    import_button_->setIconSize(QSize(width, height));

    // restart mainwindow if necessary
    if (get_restart_needed()) {
      if (edit_->MaybeSaveAnyTab()) {
        save_settings();
        qApp->exit(get_restart_needed());
      }
    }
  });
}

void MainWindow::slot_clean_double_line_breaks() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit_->CurTextPage()->GetTextPage()->toPlainText();
  content.replace("\n\n", "\n");
  edit_->SlotFillTextEditWithText(content);
}

void MainWindow::slot_add_pgp_header() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    return;
  }

  QString content =
      edit_->CurTextPage()->GetTextPage()->toPlainText().trimmed();

  content.prepend("\n\n").prepend(GpgConstants::PGP_CRYPT_BEGIN);
  content.append("\n").append(GpgConstants::PGP_CRYPT_END);

  edit_->SlotFillTextEditWithText(content);
}

void MainWindow::slot_cut_pgp_header() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit_->CurTextPage()->GetTextPage()->toPlainText();
  int start = content.indexOf(GpgConstants::PGP_CRYPT_BEGIN);
  int end = content.indexOf(GpgConstants::PGP_CRYPT_END);

  if (start < 0 || end < 0) {
    return;
  }

  // remove head
  int headEnd = content.indexOf("\n\n", start) + 2;
  content.remove(start, headEnd - start);

  // remove tail
  end = content.indexOf(GpgConstants::PGP_CRYPT_END);
  content.remove(end, QString(GpgConstants::PGP_CRYPT_END).size());

  edit_->SlotFillTextEditWithText(content.trimmed());
}

void MainWindow::SlotSetRestartNeeded(int mode) {
  SPDLOG_DEBUG("restart mode: {}", mode);
  this->restart_needed_ = mode;
}

int MainWindow::get_restart_needed() const { return this->restart_needed_; }

void MainWindow::SetCryptoMenuStatus(
    MainWindow::CryptoMenu::OperationType type) {
  SPDLOG_DEBUG("type: {}", type);

  // refresh status to disable all
  verify_act_->setDisabled(true);
  sign_act_->setDisabled(true);
  encrypt_act_->setDisabled(true);
  encrypt_sign_act_->setDisabled(true);
  decrypt_act_->setDisabled(true);
  decrypt_verify_act_->setDisabled(true);

  // enable according to type
  if (type & MainWindow::CryptoMenu::Verify) {
    verify_act_->setDisabled(false);
  }
  if (type & MainWindow::CryptoMenu::Sign) {
    sign_act_->setDisabled(false);
  }
  if (type & MainWindow::CryptoMenu::Encrypt) {
    encrypt_act_->setDisabled(false);
  }
  if (type & MainWindow::CryptoMenu::EncryptAndSign) {
    encrypt_sign_act_->setDisabled(false);
  }
  if (type & MainWindow::CryptoMenu::Decrypt) {
    decrypt_act_->setDisabled(false);
  }
  if (type & MainWindow::CryptoMenu::DecryptAndVerify) {
    decrypt_verify_act_->setDisabled(false);
  }
}

}  // namespace GpgFrontend::UI
