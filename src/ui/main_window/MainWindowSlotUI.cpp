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

#include "MainWindow.h"
#include "core/GpgConstants.h"
#include "core/function/CacheManager.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/model/SettingsObject.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/Wizard.h"
#include "ui/main_window/KeyMgmt.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui/widgets/TextEdit.h"

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
      this, edit_->CurTextPage()->GetTextPage()->toPlainText());
}

void MainWindow::slot_open_key_management() {
  auto* dialog = new KeyMgmt(this);
  dialog->setWindowModality(Qt::ApplicationModal);
  dialog->show();
  dialog->raise();
}

void MainWindow::slot_open_file_tab() { edit_->SlotNewFileTab(); }

void MainWindow::slot_disable_tab_actions(int number) {
  auto disable = number == -1;
  if (edit_->CurFilePage() != nullptr) disable = true;

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
  import_key_from_edit_act_->setDisabled(disable);

  cut_pgp_header_act_->setDisabled(disable);
  add_pgp_header_act_->setDisabled(disable);

  if (edit_->CurFilePage() != nullptr) {
    auto* file_page = edit_->CurFilePage();
    emit file_page->SignalCurrentTabChanged();
  }
}

void MainWindow::slot_open_settings_dialog() {
  auto* dialog = new SettingsDialog(this);

  connect(dialog, &SettingsDialog::finished, this, [&]() -> void {
    AppearanceSO appearance(SettingsObject("general_settings_state"));

    this->setToolButtonStyle(appearance.tool_bar_button_style);
    import_button_->setToolButtonStyle(appearance.tool_bar_button_style);

    // icons ize
    this->setIconSize(
        QSize(appearance.tool_bar_icon_width, appearance.tool_bar_icon_height));
    import_button_->setIconSize(
        QSize(appearance.tool_bar_icon_width, appearance.tool_bar_icon_height));

    // restart main window if necessary
    if (restart_mode_ != kNonRestartCode) {
      if (edit_->MaybeSaveAnyTab()) {
        // clear cache of unsaved page
        CacheManager::GetInstance().SaveDurableCache(
            "editor_unsaved_pages", QJsonDocument(QJsonArray()), true);
        emit SignalRestartApplication(restart_mode_);
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

  content.prepend("\n\n").prepend(PGP_CRYPT_BEGIN);
  content.append("\n").append(PGP_CRYPT_END);

  edit_->SlotFillTextEditWithText(content);
}

void MainWindow::slot_cut_pgp_header() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit_->CurTextPage()->GetTextPage()->toPlainText();
  auto start = content.indexOf(PGP_CRYPT_BEGIN);
  auto end = content.indexOf(PGP_CRYPT_END);

  if (start < 0 || end < 0) {
    return;
  }

  // remove head
  auto head_end = content.indexOf("\n\n", start) + 2;
  content.remove(start, head_end - start);

  // remove tail
  end = content.indexOf(PGP_CRYPT_END);
  content.remove(end, QString(PGP_CRYPT_END).size());

  edit_->SlotFillTextEditWithText(content.trimmed());
}

void MainWindow::SlotSetRestartNeeded(int mode) { this->restart_mode_ = mode; }

void MainWindow::SlotUpdateCryptoMenuStatus(unsigned int type) {
  MainWindow::CryptoMenu::OperationType opera_type = type;

  // refresh status to disable all
  verify_act_->setDisabled(true);
  sign_act_->setDisabled(true);
  encrypt_act_->setDisabled(true);
  encrypt_sign_act_->setDisabled(true);
  decrypt_act_->setDisabled(true);
  decrypt_verify_act_->setDisabled(true);

  // enable according to type
  if ((opera_type & MainWindow::CryptoMenu::Verify) != 0U) {
    verify_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::CryptoMenu::Sign) != 0U) {
    sign_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::CryptoMenu::Encrypt) != 0U) {
    encrypt_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::CryptoMenu::EncryptAndSign) != 0U) {
    encrypt_sign_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::CryptoMenu::Decrypt) != 0U) {
    decrypt_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::CryptoMenu::DecryptAndVerify) != 0U) {
    decrypt_verify_act_->setDisabled(false);
  }
}

}  // namespace GpgFrontend::UI
