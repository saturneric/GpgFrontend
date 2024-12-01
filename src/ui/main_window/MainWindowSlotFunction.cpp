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
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/module/ModuleManager.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
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
    FLOG_W("no key is selected to export");
    return;
  }

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    LOG_W() << "selected key for exporting is invalid, key id: "
            << key_ids->front();
    return;
  }

  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance(
                              m_key_list_->GetCurrentGpgContextChannel())
                              .ExportKey(key, false, true, false);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseMessageBox(this, err);
    return;
  }

  edit_->SlotAppendText2CurTextPage(gf_buffer.ConvertToQByteArray());
}

void MainWindow::slot_append_keys_create_datetime() {
  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    FLOG_W("no key is selected");
    return;
  }

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
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
    FLOG_W("no key is selected");
    return;
  }

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }

  auto expire_datetime_format_str_local =
      QLocale().toString(key.GetExpireTime()) + " (" + tr("Local Time") + ") " +
      "\n";
  auto expire_datetime_format_str =
      QLocale().toString(key.GetExpireTime().toUTC()) + " (UTC) " + "\n";

  edit_->SlotAppendText2CurTextPage(expire_datetime_format_str_local +
                                    expire_datetime_format_str);
}

void MainWindow::slot_append_keys_fingerprint() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
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

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
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

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
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

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
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

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (key.IsGood()) {
    new KeyDetailsDialog(m_key_list_->GetCurrentGpgContextChannel(), key, this);
  } else {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
  }
}

void MainWindow::slot_add_key_2_favorite() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) return;

  auto key_db_name =
      GetGpgKeyDatabaseName(m_key_list_->GetCurrentGpgContextChannel());

  LOG_D() << "add key" << key.GetId() << "to favorite at key db" << key_db_name;

  CommonUtils::GetInstance()->AddKey2Favorite(key_db_name, key);
  emit SignalUIRefresh();
}

void MainWindow::slot_remove_key_from_favorite() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  assert(key.IsGood());

  auto key_db_name =
      GetGpgKeyDatabaseName(m_key_list_->GetCurrentGpgContextChannel());

  CommonUtils::GetInstance()->RemoveKeyFromFavorite(key_db_name, key);

  emit SignalUIRefresh();
}

void MainWindow::refresh_keys_from_key_server() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;
  CommonUtils::GetInstance()->ImportKeyFromKeyServer(
      m_key_list_->GetCurrentGpgContextChannel(), *key_ids);
}

void MainWindow::slot_set_owner_trust_level_of_key() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto* function = new SetOwnerTrustLevel(this);
  function->Exec(m_key_list_->GetCurrentGpgContextChannel(), key_ids->front());
  function->deleteLater();
}

void MainWindow::upload_key_to_server() {
  auto key_ids = m_key_list_->GetSelected();
  auto* dialog = new KeyUploadDialog(m_key_list_->GetCurrentGpgContextChannel(),
                                     key_ids, this);
  dialog->show();
  dialog->SlotUpload();
}

void MainWindow::SlotOpenFile(const QString& path) {
  edit_->SlotOpenFile(path);
}

void MainWindow::slot_version_upgrade_notify() {
  FLOG_D(
      "slot version upgrade notify called, checking version info from rt...");

  auto is_loading_done = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.loading_done", false);
  if (!is_loading_done) {
    FLOG_W("invalid version info from rt, loading hasn't done yet");
    return;
  }

  auto is_need_upgrade = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.need_upgrade", false);

  auto is_current_a_withdrawn_version = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.current_a_withdrawn_version", false);

  auto is_current_version_released = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.current_version_released", false);

  auto latest_version = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.latest_version", QString{});

  if (is_need_upgrade) {
    statusBar()->showMessage(
        tr("GpgFrontend Upgradeable (New Version: %1).").arg(latest_version),
        30000);
    auto* update_button = new QPushButton("Update GpgFrontend", this);
    connect(update_button, &QPushButton::clicked, [=]() {
      auto* about_dialog = new AboutDialog(tr("Update"), this);
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

void MainWindow::slot_import_key_from_edit() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) return;

  CommonUtils::GetInstance()->SlotImportKeys(
      this, m_key_list_->GetCurrentGpgContextChannel(),
      edit_->CurPlainText().toLatin1());
}

void MainWindow::slot_verify_email_by_eml_data(const QByteArray& buffer) {
  CommonUtils::WaitForOpera(
      this, tr("Verifying"), [this, buffer](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_VERIFY_EML_DATA",
            {
                {"eml_data", QString::fromLatin1(buffer.toBase64())},
                {"channel",
                 QString::number(m_key_list_->GetCurrentGpgContextChannel())},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_VERIFY_EML_DATA callback: " << i << ei;

              // end waiting dialog
              hd();

              // check if error occurred
              if (slot_handle_module_error(p)) return -1;

              if (p.contains("signature") && p.contains("mime")) {
                slot_verify_email_by_eml_data_result_helper(p);
              }

              return 0;
            });
      });
}

void MainWindow::slot_decrypt_email_by_eml_data(const QByteArray& buffer) {
  Module::TriggerEvent(
      "EMAIL_DECRYPT_EML_DATA",
      {
          {"eml_data", QString::fromLatin1(buffer.toBase64())},
          {"channel",
           QString::number(m_key_list_->GetCurrentGpgContextChannel())},
      },
      [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
          Module::Event::Params p) {
        LOG_D() << "EMAIL_DECRYPT_EML_DATA callback: " << i << ei;

        // check if error occurred
        if (slot_handle_module_error(p)) return -1;

        if (p.contains("eml_data")) {
          slot_decrypt_email_by_eml_data_result_helper(p);
        }

        return 0;
      });
}

void MainWindow::SlotVerifyEML() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) return;

  auto buffer = edit_->CurPlainText().toLatin1();
  buffer = buffer.replace("\n", "\r\n");

  // LOG_D() << "EML BUFFER: " << buffer;

  slot_verify_email_by_eml_data(buffer);
}

void MainWindow::slot_verifying_unknown_signature_helper(
    const GpgVerifyResultAnalyse& result_analyse) {
  if (!Module::IsModuleActivate(kKeyServerSyncModuleID)) return;

  LOG_D() << "try to sync missing key info from server: "
          << result_analyse.GetUnknownSignatures();

  QString fingerprint_list;
  for (const auto& fingerprint : result_analyse.GetUnknownSignatures()) {
    fingerprint_list += fingerprint + "\n";
  }

  // Interaction with user
  auto user_response =
      QMessageBox::question(this, tr("Missing Keys"),
                            tr("Some signatures cannot be verified because "
                               "the "
                               "corresponding keys are missing.\n\n"
                               "The following fingerprints are "
                               "missing:\n%1\n\n"
                               "Would you like to fetch these keys from "
                               "the key "
                               "server?")
                                .arg(fingerprint_list),
                            QMessageBox::Yes | QMessageBox::No);

  if (user_response == QMessageBox::Yes) {
    CommonUtils::GetInstance()->ImportKeyByKeyServerSyncModule(
        this, m_key_list_->GetCurrentGpgContextChannel(),
        result_analyse.GetUnknownSignatures());
  } else {
    QMessageBox::information(this, tr("Verification Incomplete"),
                             tr("Verification was incomplete due to "
                                "missing "
                                "keys. You can manually import the keys "
                                "later."));
  }
}

void MainWindow::slot_verify_email_by_eml_data_result_helper(
    const QMap<QString, QString>& p) {
  const auto mime = QByteArray::fromBase64(p["mime"].toLatin1());
  const auto signature = QByteArray::fromBase64(p["signature"].toLatin1());
  const auto part_mime_content_hash = p["mime_hash"];
  const auto prm_micalg_value = p["micalg"];

  auto timestamp = p.value("datetime", "-1").toLongLong();
  auto datetime = tr("None");
  if (timestamp > 0) {
    datetime = QLocale().toString(QDateTime::fromMSecsSinceEpoch(timestamp));
  }

  QString email_info;
  email_info.append("# E-Mail Information\n\n");
  email_info.append(QString("- %1: %2\n")
                        .arg(tr("From"))
                        .arg(p.value("from", tr("Unknown"))));
  email_info.append(
      QString("- %1: %2\n").arg(tr("To")).arg(p.value("to", tr("Unknown"))));
  email_info.append(QString("- %1: %2\n")
                        .arg(tr("Subject"))
                        .arg(p.value("subject", tr("None"))));
  email_info.append(
      QString("- %1: %2\n").arg(tr("CC")).arg(p.value("cc", tr("None"))));
  email_info.append(
      QString("- %1: %2\n").arg(tr("BCC")).arg(p.value("bcc", tr("None"))));
  email_info.append(QString("- %1: %2\n").arg(tr("Date")).arg(datetime));

  email_info.append("\n");
  email_info.append("# OpenPGP Information\n\n");
  email_info.append(QString("- %1: %2\n")
                        .arg(tr("Signed EML Data Hash (SHA1)"))
                        .arg(part_mime_content_hash));
  email_info.append(QString("- %1: %2\n")
                        .arg(tr("Message Integrity Check Algorithm"))
                        .arg(prm_micalg_value));
  email_info.append("\n");

  if (!p["capsule_id"].isEmpty()) {
    auto v =
        UIModuleManager::GetInstance().GetCapsule(p.value("capsule_id", ""));

    try {
      auto sign_result = std::any_cast<GpgVerifyResult>(v);
      auto result_analyse =
          GpgVerifyResultAnalyse(m_key_list_->GetCurrentGpgContextChannel(),
                                 GPG_ERR_NO_ERROR, sign_result);
      result_analyse.Analyse();

      slot_refresh_info_board(result_analyse.GetStatus(),
                              email_info + result_analyse.GetResultReport());

    } catch (const std::bad_any_cast& e) {
      LOG_E() << "capsule" << p["capsule_id"] << "convert to real type failed"
              << e.what();
    }
  }
}

void MainWindow::slot_eml_verify_show_helper(const QString& email_info,
                                             const GpgVerifyResultAnalyse& r) {}

void MainWindow::slot_result_analyse_show_helper(
    const GpgResultAnalyse& result_analyse) {
  slot_refresh_info_board(result_analyse.GetStatus(),
                          result_analyse.GetResultReport());
}

void MainWindow::slot_refresh_info_board(int status, const QString& text) {
  info_board_->SlotReset();

  if (status < 0) {
    info_board_->SlotRefresh(text, INFO_ERROR_CRITICAL);
  } else if (status > 0) {
    info_board_->SlotRefresh(text, INFO_ERROR_OK);
  } else {
    info_board_->SlotRefresh(text, INFO_ERROR_WARN);
  }
}

void MainWindow::slot_result_analyse_show_helper(const GpgResultAnalyse& r_a,
                                                 const GpgResultAnalyse& r_b) {
  info_board_->SlotReset();

  slot_refresh_info_board(std::min(r_a.GetStatus(), r_b.GetStatus()),
                          r_a.GetResultReport() + r_b.GetResultReport());
}

void MainWindow::SlotDecryptEML() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) return;

  auto buffer = edit_->CurPlainText().toLatin1();
  buffer = buffer.replace("\n", "\r\n");

  // LOG_D() << "EML BUFFER: " << buffer;

  slot_decrypt_email_by_eml_data(buffer);
}

void MainWindow::slot_decrypt_email_by_eml_data_result_helper(
    const QMap<QString, QString>& p) {
  auto timestamp = p.value("datetime", "-1").toLongLong();
  auto datetime = tr("None");
  if (timestamp > 0) {
    datetime = QLocale().toString(QDateTime::fromMSecsSinceEpoch(timestamp));
  }

  const auto eml_data = QByteArray::fromBase64(p["eml_data"].toLatin1());
  edit_->SlotFillTextEditWithText(eml_data);

  QString email_info;
  email_info.append("# E-Mail Information\n\n");
  email_info.append(QString("- %1: %2\n")
                        .arg(tr("From"))
                        .arg(p.value("from", tr("Unknown"))));
  email_info.append(
      QString("- %1: %2\n").arg(tr("To")).arg(p.value("to", tr("Unknown"))));
  email_info.append(QString("- %1: %2\n")
                        .arg(tr("Subject"))
                        .arg(p.value("subject", tr("None"))));
  email_info.append(
      QString("- %1: %2\n").arg(tr("CC")).arg(p.value("cc", tr("None"))));
  email_info.append(
      QString("- %1: %2\n").arg(tr("BCC")).arg(p.value("bcc", tr("None"))));
  email_info.append(QString("- %1: %2\n").arg(tr("Date")).arg(datetime));

  email_info.append("\n");

  if (!p["capsule_id"].isEmpty()) {
    auto v =
        UIModuleManager::GetInstance().GetCapsule(p.value("capsule_id", ""));

    try {
      auto sign_result = std::any_cast<GpgDecryptResult>(v);
      auto result_analyse =
          GpgDecryptResultAnalyse(m_key_list_->GetCurrentGpgContextChannel(),
                                  GPG_ERR_NO_ERROR, sign_result);
      result_analyse.Analyse();

      slot_refresh_info_board(result_analyse.GetStatus(),
                              email_info + result_analyse.GetResultReport());

    } catch (const std::bad_any_cast& e) {
      LOG_E() << "capsule" << p["capsule_id"] << "convert to real type failed"
              << e.what();
    }
  }
}

void MainWindow::SlotEncryptEML() {
  if (edit_->TabCount() == 0 || edit_->CurEMailPage() == nullptr) return;
  auto checked_keys = m_key_list_->GetCheckedKeys();

  if (checked_keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for encrypt the EML."));
    return;
  }
  auto buffer = edit_->CurPlainText().toUtf8();

  CommonUtils::WaitForOpera(
      this, tr("Encrypting"),
      [this, buffer, checked_keys](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_ENCRYPT_EML_DATA",
            {
                {"body_data", QString::fromLatin1(buffer.toBase64())},
                {"channel",
                 QString::number(m_key_list_->GetCurrentGpgContextChannel())},
                {"encrypt_keys", checked_keys.join(';')},
            },

            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_ENCRYPT_EML_DATA callback: " << i << ei;

              // close waiting dialog
              hd();

              if (p["ret"] != "0" || !p["err"].isEmpty()) {
                LOG_E() << "An error occurred trying to decrypt email, "
                        << "error message: " << p["err"];

                return;
              }

              if (!p["eml_data"].isEmpty()) {
                edit_->SlotSetText2CurEMailPage(p.value("eml_data", ""));
              }

              if (!p["capsule_id"].isEmpty()) {
                auto v = UIModuleManager::GetInstance().GetCapsule(
                    p.value("capsule_id", ""));

                try {
                  auto encr_result = std::any_cast<GpgEncryptResult>(v);
                  auto result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, encr_result);
                  result_analyse.Analyse();
                  slot_result_analyse_show_helper(result_analyse);

                } catch (const std::bad_any_cast& e) {
                  LOG_E() << "capsule" << p["capsule_id"]
                          << "convert to real type failed" << e.what();
                }
              }

              LOG_E() << "mime or signature data is missing";
            });
      });
}

void MainWindow::SlotSignEML() {
  if (edit_->TabCount() == 0 || edit_->CurEMailPage() == nullptr) return;
  auto checked_keys = m_key_list_->GetCheckedKeys();

  if (checked_keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for signing the EML."));
    return;
  }

  if (checked_keys.size() > 1) {
    QMessageBox::warning(this, tr("Multiple Keys Selected"),
                         tr("Please select only one key to sign the EML."));
    return;
  }

  auto buffer = edit_->CurPlainText().toUtf8();

  CommonUtils::WaitForOpera(
      this, tr("Signing"),
      [this, buffer, checked_keys](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_SIGN_EML_DATA",
            {
                {"body_data", QString::fromLatin1(buffer.toBase64())},
                {"channel",
                 QString::number(m_key_list_->GetCurrentGpgContextChannel())},
                {"sign_key", checked_keys.front()},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_SIGN_EML_DATA callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (slot_handle_module_error(p)) return -1;

              if (!p["eml_data"].isEmpty()) {
                edit_->SlotSetText2CurEMailPage(p.value("eml_data", ""));
              }

              if (!p["capsule_id"].isEmpty()) {
                auto v = UIModuleManager::GetInstance().GetCapsule(
                    p.value("capsule_id", ""));

                try {
                  auto sign_result = std::any_cast<GpgSignResult>(v);
                  auto result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, sign_result);
                  result_analyse.Analyse();
                  slot_result_analyse_show_helper(result_analyse);

                } catch (const std::bad_any_cast& e) {
                  LOG_E() << "capsule" << p["capsule_id"]
                          << "convert to real type failed" << e.what();
                }
              }

              return 0;
            });
      });
}

void MainWindow::SlotEncryptSignEML() {
  if (edit_->TabCount() == 0 || edit_->CurEMailPage() == nullptr) return;
  auto checked_keys = m_key_list_->GetCheckedKeys();

  if (checked_keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for encrypt the EML."));
    return;
  }

  auto* signers_picker =
      new SignersPicker(m_key_list_->GetCurrentGpgContextChannel(), this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_keys = signers_picker->GetCheckedSignerKeyIds();

  if (signer_keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for signing the EML."));
    return;
  }

  if (signer_keys.size() > 1) {
    QMessageBox::warning(this, tr("Multiple Keys Selected"),
                         tr("Please select only one key to sign the EML."));
    return;
  }

  auto buffer = edit_->CurPlainText().toUtf8();

  CommonUtils::WaitForOpera(
      this, tr("Encrypting and Signing"),
      [this, buffer, checked_keys, signer_keys](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_ENCRYPT_SIGN_EML_DATA",
            {
                {"body_data", QString::fromLatin1(buffer.toBase64())},
                {"channel",
                 QString::number(m_key_list_->GetCurrentGpgContextChannel())},
                {"sign_key", signer_keys.front()},
                {"encrypt_keys", checked_keys.front()},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_ENCRYPT_SIGN_EML_DATA callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (slot_handle_module_error(p)) return -1;

              if (!p["sign_capsule_id"].isEmpty() &&
                  !p["encr_capsule_id"].isEmpty()) {
                auto v1 = UIModuleManager::GetInstance().GetCapsule(
                    p.value("sign_capsule_id", ""));
                auto v2 = UIModuleManager::GetInstance().GetCapsule(
                    p.value("encr_capsule_id", ""));

                try {
                  auto sign_result = std::any_cast<GpgSignResult>(v1);
                  auto encr_result = std::any_cast<GpgSignResult>(v1);
                  auto sign_result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, sign_result);
                  auto encr_result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, sign_result);

                  sign_result_analyse.Analyse();
                  encr_result_analyse.Analyse();
                  slot_result_analyse_show_helper(sign_result_analyse,
                                                  encr_result_analyse);

                } catch (const std::bad_any_cast& e) {
                  LOG_E() << "capsule" << p["capsule_id"]
                          << "convert to real type failed" << e.what();
                }
              }

              return 0;
            });
      });
}

void MainWindow::SlotDecryptVerifyEML() {
  if (edit_->TabCount() == 0 || edit_->CurEMailPage() == nullptr) return;

  auto buffer = edit_->CurPlainText().toUtf8();

  CommonUtils::WaitForOpera(
      this, tr("Decrypting and Verifying"),
      [this, buffer](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_DECRYPT_VERIFY_EML_DATA",
            {
                {"eml_data", QString::fromLatin1(buffer.toBase64())},
                {"channel",
                 QString::number(m_key_list_->GetCurrentGpgContextChannel())},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_DECRYPT_VERIFY_EML_DATA callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (slot_handle_module_error(p)) return -1;

              edit_->SlotSetText2CurEMailPage(p.value("eml_data", ""));

              const auto mime = QByteArray::fromBase64(p["mime"].toLatin1());
              const auto signature =
                  QByteArray::fromBase64(p["signature"].toLatin1());
              const auto part_mime_content_hash = p["mime_hash"];
              const auto prm_micalg_value = p["micalg"];

              auto timestamp = p.value("datetime", "-1").toLongLong();
              auto datetime = tr("None");
              if (timestamp > 0) {
                datetime = QLocale().toString(
                    QDateTime::fromMSecsSinceEpoch(timestamp));
              }

              QString email_info;
              email_info.append("# E-Mail Information\n\n");
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("From"))
                                    .arg(p.value("from", tr("Unknown"))));
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("To"))
                                    .arg(p.value("to", tr("Unknown"))));
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("Subject"))
                                    .arg(p.value("subject", tr("None"))));
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("CC"))
                                    .arg(p.value("cc", tr("None"))));
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("BCC"))
                                    .arg(p.value("bcc", tr("None"))));
              email_info.append(
                  QString("- %1: %2\n").arg(tr("Date")).arg(datetime));

              email_info.append("\n");
              email_info.append("# OpenPGP Information\n\n");
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("Signed EML Data Hash (SHA1)"))
                                    .arg(part_mime_content_hash));
              email_info.append(
                  QString("- %1: %2\n")
                      .arg(tr("Message Integrity Check Algorithm"))
                      .arg(prm_micalg_value));
              email_info.append("\n");

              if (!p["decr_capsule_id"].isEmpty() &&
                  !p["verify_capsule_id"].isEmpty()) {
                auto v1 = UIModuleManager::GetInstance().GetCapsule(
                    p.value("decr_capsule_id", ""));
                auto v2 = UIModuleManager::GetInstance().GetCapsule(
                    p.value("verify_capsule_id", ""));

                try {
                  auto decr_result = std::any_cast<GpgDecryptResult>(v1);
                  auto verify_result = std::any_cast<GpgVerifyResult>(v2);
                  auto decr_result_analyse = GpgDecryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, decr_result);
                  auto verify_result_analyse = GpgVerifyResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, verify_result);

                  decr_result_analyse.Analyse();
                  verify_result_analyse.Analyse();
                  slot_refresh_info_board(
                      std::min(decr_result_analyse.GetStatus(),
                               verify_result_analyse.GetStatus()),
                      email_info + decr_result_analyse.GetResultReport() +
                          verify_result_analyse.GetResultReport());

                } catch (const std::bad_any_cast& e) {
                  LOG_E() << "capsule" << p["capsule_id"]
                          << "convert to real type failed" << e.what();
                }
              }

              return 0;
            });
      });
}

auto MainWindow::slot_handle_module_error(const QMap<QString, QString>& p)
    -> bool {
  if (p["ret"] == "-2") {
    QString detailed_error = p["err"];

    QString info =
        tr("# EML Data Error\n\n"
           "The provided EML data does not conform to RFC 3156 standards and "
           "cannot be processed.\n\n"
           "**Details:** %1\n\n"
           "### What is EML Data?\n"
           "EML is a file format for representing email messages, typically "
           "including headers, body text, attachments, and metadata. "
           "Complete and properly structured EML data is required for "
           "validation.\n\n"
           "### Suggested Solutions\n"
           "1. Verify the EML data is complete and matches the structure "
           "outlined in RFC 3156.\n"
           "2. Refer to the official documentation for the EML structure: "
           "%2\n\n"
           "After correcting the EML data, try the operation again.")
            .arg(detailed_error)
            .arg("https://www.rfc-editor.org/rfc/rfc3156.txt");
    slot_refresh_info_board(-2, info);
    return true;
  }

  if (p["ret"] != "0" || !p["err"].isEmpty()) {
    LOG_E() << "An error occurred trying to operate email, "
            << "error message: " << p["err"];

    QString error_message =
        tr("# Email Operation Error\n\n"
           "An error occurred during the email operation. The process "
           "could not be completed.\n\n"
           "**Details:**\n"
           "- **Error Code:** %1\n"
           "- **Error Message:** %2\n\n"
           "### Possible Causes\n"
           "1. The email data may be incomplete or corrupted.\n"
           "2. The selected GPG key does not have the necessary "
           "permissions.\n"
           "3. Issues in the GPG environment or configuration.\n\n"
           "### Suggested Solutions\n"
           "1. Ensure the email data is complete and follows the expected "
           "format.\n"
           "2. Verify the GPG key has the required access permissions.\n"
           "3. Check your GPG environment and configuration settings.\n"
           "4. Review the error details above or application logs for "
           "further troubleshooting.\n\n"
           "If the issue persists, consider seeking technical support or "
           "consulting the documentation.")
            .arg(p["ret"])
            .arg(p["err"]);

    slot_refresh_info_board(-1, error_message);
    return true;
  }

  return false;
}

}  // namespace GpgFrontend::UI
