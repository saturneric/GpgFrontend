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
#include "core/GpgConstants.h"
#include "core/function/CacheManager.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/model/SettingsObject.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/Wizard.h"
#include "ui/dialog/settings/SettingsDialog.h"
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

void MainWindow::slot_open_key_management() {
  auto* dialog = new KeyMgmt(this);
  dialog->setWindowModality(Qt::ApplicationModal);
  dialog->show();
  dialog->raise();
}

void MainWindow::slot_open_file_tab() { edit_->SlotNewFileBrowserTab(); }

void MainWindow::slot_switch_menu_control_mode(int index) {
  auto disable = false;
  if (index == -1) disable = true;
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

    restore_settings();
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
  if (edit_->TabCount() == 0 || edit_->CurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit_->CurPlainText();
  content.replace("\n\n", "\n");
  edit_->SlotFillTextEditWithText(content);
}

void MainWindow::slot_add_pgp_header() {
  if (edit_->TabCount() == 0 || edit_->CurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit_->CurPlainText().trimmed();

  content.prepend("\n\n").prepend(PGP_CRYPT_BEGIN);
  content.append("\n").append(PGP_CRYPT_END);

  edit_->SlotFillTextEditWithText(content);
}

void MainWindow::slot_cut_pgp_header() {
  if (edit_->TabCount() == 0 || edit_->CurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit_->CurPlainText();
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
  MainWindow::OperationMenu::OperationType opera_type = type;

  // refresh status to disable all
  verify_act_->setDisabled(true);
  sign_act_->setDisabled(true);
  encrypt_act_->setDisabled(true);
  encrypt_sign_act_->setDisabled(true);
  decrypt_act_->setDisabled(true);
  decrypt_verify_act_->setDisabled(true);

  // gnupg operations
  if ((opera_type & MainWindow::OperationMenu::kVerify) != 0U) {
    verify_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::OperationMenu::kSign) != 0U) {
    sign_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::OperationMenu::kEncrypt) != 0U) {
    encrypt_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::OperationMenu::kEncryptAndSign) != 0U) {
    encrypt_sign_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::OperationMenu::kDecrypt) != 0U) {
    decrypt_act_->setDisabled(false);
  }
  if ((opera_type & MainWindow::OperationMenu::kDecryptAndVerify) != 0U) {
    decrypt_verify_act_->setDisabled(false);
  }
}

void MainWindow::SlotGeneralEncrypt(bool) {
  if (edit_->CurPageFileTreeView() != nullptr) {
    const auto* file_tree_view = edit_->CurPageFileTreeView();
    const auto paths = file_tree_view->GetSelected();

    this->SlotFileEncrypt(paths);
  }

  if (edit_->CurEMailPage() != nullptr) {
    this->SlotEncryptEML();
    return;
  }

  if (edit_->CurPageTextEdit() != nullptr) {
    this->SlotEncrypt();
  }
}

void MainWindow::SlotGeneralDecrypt(bool) {
  if (edit_->CurPageFileTreeView() != nullptr) {
    const auto* file_tree_view = edit_->CurPageFileTreeView();
    const auto paths = file_tree_view->GetSelected();

    this->SlotFileDecrypt(paths);
  }

  if (edit_->CurEMailPage() != nullptr) {
    this->SlotDecryptEML();
    return;
  }

  if (edit_->CurPageTextEdit() != nullptr) {
    this->SlotDecrypt();
  }
}

void MainWindow::SlotGeneralSign(bool) {
  if (edit_->CurPageFileTreeView() != nullptr) {
    const auto* file_tree_view = edit_->CurPageFileTreeView();
    const auto paths = file_tree_view->GetSelected();

    this->SlotFileSign(paths);
  }

  if (edit_->CurEMailPage() != nullptr) {
    this->SlotSignEML();
    return;
  }

  if (edit_->CurPageTextEdit() != nullptr) this->SlotSign();
}

void MainWindow::SlotGeneralVerify(bool) {
  if (edit_->CurPageFileTreeView() != nullptr) {
    const auto* file_tree_view = edit_->CurPageFileTreeView();
    const auto paths = file_tree_view->GetSelected();

    this->SlotFileVerify(paths);
  }

  if (edit_->CurEMailPage() != nullptr) {
    this->SlotVerifyEML();
    return;
  }

  if (edit_->CurPageTextEdit() != nullptr) this->SlotVerify();
}

void MainWindow::SlotGeneralEncryptSign(bool) {
  if (edit_->CurPageFileTreeView() != nullptr) {
    const auto* file_tree_view = edit_->CurPageFileTreeView();
    const auto paths = file_tree_view->GetSelected();

    this->SlotFileEncryptSign(paths);
  }

  if (edit_->CurEMailPage() != nullptr) {
    this->SlotEncryptSignEML();
    return;
  }

  if (edit_->CurPageTextEdit() != nullptr) {
    this->SlotEncryptSign();
  }
}

void MainWindow::SlotGeneralDecryptVerify(bool) {
  if (edit_->CurPageFileTreeView() != nullptr) {
    const auto* file_tree_view = edit_->CurPageFileTreeView();
    const auto paths = file_tree_view->GetSelected();

    this->SlotFileDecryptVerify(paths);
  }

  if (edit_->CurEMailPage() != nullptr) {
    this->SlotDecryptVerifyEML();
    return;
  }

  if (edit_->CurPageTextEdit() != nullptr) {
    this->SlotDecryptVerify();
  }
}

void MainWindow::slot_clean_gpg_password_cache(bool) {
  GpgFrontend::GpgAdvancedOperator::ClearGpgPasswordCache(
      [=](int err, DataObjectPtr) {
        if (err >= 0) {
          QMessageBox::information(this, tr("Successful Operation"),
                                   tr("Clear password cache successfully"));
        } else {
          QMessageBox::critical(this, tr("Failed Operation"),
                                tr("Failed to clear password cache of GnuPG"));
        }
      });
}

void MainWindow::slot_reload_gpg_components(bool) {
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
}

void MainWindow::slot_restart_gpg_components(bool) {
  GpgFrontend::GpgAdvancedOperator::RestartGpgComponents(
      [=](int err, DataObjectPtr) {
        if (err >= 0) {
          QMessageBox::information(
              this, tr("Successful Operation"),
              tr("Restart all the GnuPG's components successfully"));
        } else {
          QMessageBox::critical(
              this, tr("Failed Operation"),
              tr("Failed to restart all or one of the GnuPG's component(s)"));
        }
      });
}

}  // namespace GpgFrontend::UI
