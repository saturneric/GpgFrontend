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
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/module/ModuleManager.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/help/AboutDialog.h"
#include "ui/dialog/import_export/KeyUploadDialog.h"
#include "ui/dialog/keypair_details/KeyDetailsDialog.h"
#include "ui/function/SetOwnerTrustLevel.h"
#include "ui/widgets/FindWidget.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

void MainWindow::slot_find() {
  if (edit_->TabCount() == 0 || edit_->CurTextPage() == nullptr) {
    return;
  }

  // At first close verifynotification, if existing
  edit_->SlotCurPageTextEdit()->CloseNoteByClass("FindWidget");

  auto* fw = new FindWidget(this, edit_->CurTextPage());
  edit_->SlotCurPageTextEdit()->ShowNotificationWidget(fw, "FindWidget");
}

/*
 * Append the selected (not checked!) Key(s) To Textedit
 */
void MainWindow::slot_append_selected_keys() {
  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    GF_UI_LOG_ERROR("no key is selected to export");
    return;
  }

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    GF_UI_LOG_ERROR("selected key for exporting is invalid, key id: {}",
                    key_ids->front());
    return;
  }

  auto [err, gf_buffer] =
      GpgKeyImportExporter::GetInstance().ExportKey(key, false, true, false);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseMessageBox(this, err);
    return;
  }

  edit_->SlotAppendText2CurTextPage(gf_buffer.ConvertToQByteArray());
}

void MainWindow::slot_append_keys_create_datetime() {
  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    GF_UI_LOG_ERROR("no key is selected");
    return;
  }

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }

  auto create_datetime_format_str_local =
      QLocale().toString(key.GetCreateTime()) + " (" + tr("Localize") + ") " +
      "\n";
  auto create_datetime_format_str =
      QLocale().toString(key.GetCreateTime().toUTC()) + " (" + tr("UTC") +
      ") " + "\n ";
  edit_->SlotAppendText2CurTextPage(create_datetime_format_str_local +
                                    create_datetime_format_str);
}

void MainWindow::slot_append_keys_expire_datetime() {
  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    GF_UI_LOG_ERROR("no key is selected");
    return;
  }

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }

  auto expire_datetime_format_str_local =
      QLocale().toString(key.GetCreateTime()) + " (" + tr("Local Time") + ") " +
      "\n";
  auto expire_datetime_format_str =
      QLocale().toString(key.GetCreateTime().toUTC()) + " (UTC) " + "\n";

  edit_->SlotAppendText2CurTextPage(expire_datetime_format_str_local +
                                    expire_datetime_format_str);
}

void MainWindow::slot_append_keys_fingerprint() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }

  auto fingerprint_format_str =
      BeautifyFingerprint(key.GetFingerprint()) + "\n";

  edit_->SlotAppendText2CurTextPage(fingerprint_format_str);
}

void MainWindow::slot_copy_mail_address_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(key.GetEmail());
}

void MainWindow::slot_copy_default_uid_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(key.GetUIDs()->front().GetUID());
}

void MainWindow::slot_copy_key_id_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(key.GetId());
}

void MainWindow::slot_show_key_details() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (key.IsGood()) {
    new KeyDetailsDialog(key, this);
  } else {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
  }
}

void MainWindow::slot_add_key_2_favourite() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  CommonUtils::GetInstance()->AddKey2Favourtie(key);

  emit SignalUIRefresh();
}

void MainWindow::slot_remove_key_from_favourite() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  CommonUtils::GetInstance()->RemoveKeyFromFavourite(key);

  emit SignalUIRefresh();
}

void MainWindow::refresh_keys_from_key_server() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;
  CommonUtils::GetInstance()->ImportKeyFromKeyServer(*key_ids);
}

void MainWindow::slot_set_owner_trust_level_of_key() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto* function = new SetOwnerTrustLevel(this);
  function->Exec(key_ids->front());
  function->deleteLater();
}

void MainWindow::upload_key_to_server() {
  auto key_ids = m_key_list_->GetSelected();
  auto* dialog = new KeyUploadDialog(key_ids, this);
  dialog->show();
  dialog->SlotUpload();
}

void MainWindow::SlotOpenFile(const QString& path) {
  QFileInfo const info(path);
  if (!info.isFile() || !info.isReadable()) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  if (info.size() > static_cast<qint64>(1024 * 1024)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. The file is TOO LARGE (>1MB) for "
           "GpgFrontend Text Editor."));
    return;
  }

  edit_->SlotOpenFile(path);
}

void MainWindow::slot_version_upgrade_nofity() {
  GF_UI_LOG_DEBUG(
      "slot version upgrade notify called, checking version info from rt...");
  auto is_loading_done = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version_checking",
      "version.loading_done", false);

  GF_UI_LOG_DEBUG("checking version info from rt, is loading done state: {}",
                  is_loading_done);
  if (!is_loading_done) {
    GF_UI_LOG_ERROR("invalid version info from rt, loading hasn't done yet");
    return;
  }

  auto is_need_upgrade = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version_checking",
      "version.need_upgrade", false);

  auto is_current_a_withdrawn_version = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version_checking",
      "version.current_a_withdrawn_version", false);

  auto is_current_version_released = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version_checking",
      "version.current_version_released", false);

  auto latest_version = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version_checking",
      "version.latest_version", QString{});

  GF_UI_LOG_DEBUG(
      "got version info from rt, need upgrade: {}, with drawn: {}, "
      "current version released: {}",
      is_need_upgrade, is_current_a_withdrawn_version,
      is_current_version_released);

  if (is_need_upgrade) {
    statusBar()->showMessage(
        tr("GpgFrontend Upgradeable (New Version: %1).").arg(latest_version),
        30000);
    auto* update_button = new QPushButton("Update GpgFrontend", this);
    connect(update_button, &QPushButton::clicked, [=]() {
      auto* about_dialog = new AboutDialog(2, this);
      about_dialog->show();
    });
    statusBar()->addPermanentWidget(update_button, 0);
  } else if (is_current_a_withdrawn_version) {
    QMessageBox::warning(
        this, tr("Withdrawn Version"),

        tr("This version(%1) may have been withdrawn by the developer due "
           "to serious problems. Please stop using this version "
           "immediately and use the latest stable version.")
                .arg(latest_version) +
            "<br/>" +
            tr("You can download the latest stable version(%1) on "
               "Github Releases Page.<br/>")
                .arg(latest_version));
  } else if (!is_current_version_released) {
    statusBar()->showMessage(
        tr("This maybe a BETA Version (Latest Stable Version: %1).")
            .arg(latest_version),
        30000);
  }
}

void MainWindow::slot_refresh_current_file_view() {
  if (edit_->CurFilePage() != nullptr) {
    edit_->CurFilePage()->update();
  }
}

}  // namespace GpgFrontend::UI
