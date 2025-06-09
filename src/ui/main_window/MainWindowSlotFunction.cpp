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
#include "core/function/GFBufferFactory.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/module/ModuleManager.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
#include "ui/dialog/help/AboutDialog.h"
#include "ui/dialog/import_export/KeyUploadDialog.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/SetOwnerTrustLevel.h"
#include "ui/struct/GpgOperaResult.h"
#include "ui/widgets/FindWidget.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

void MainWindow::slot_find() {
  if (edit_->TabCount() == 0 || edit_->CurTextPage() == nullptr) {
    return;
  }

  // At first close verifynotification, if existing
  edit_->CurPageTextEdit()->CloseNoteByClass("FindWidget");

  auto* fw = new FindWidget(this, edit_->CurTextPage());
  edit_->CurPageTextEdit()->ShowNotificationWidget(fw, "FindWidget");
}

/*
 * Append the selected (not checked!) Key(s) To Textedit
 */
void MainWindow::slot_append_selected_keys() {
  auto keys = m_key_list_->GetSelectedKeys();
  if (keys.empty()) return;

  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance(
                              m_key_list_->GetCurrentGpgContextChannel())
                              .ExportKey(keys.front(), false, true, false);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseMessageBox(this, err);
    return;
  }

  edit_->SlotAppendText2CurTextPage(gf_buffer.ConvertToQByteArray());
}

void MainWindow::slot_append_keys_create_datetime() {
  auto key = m_key_list_->GetSelectedKey();
  if (key == nullptr) return;

  auto create_datetime_format_str_local =
      QLocale().toString(key->CreationTime()) + " (" + tr("Localize") + ") " +
      "\n";
  auto create_datetime_format_str =
      QLocale().toString(key->CreationTime().toUTC()) + " (" + tr("UTC") +
      ") " + "\n ";
  edit_->SlotAppendText2CurTextPage(create_datetime_format_str_local +
                                    create_datetime_format_str);
}

void MainWindow::slot_append_keys_expire_datetime() {
  auto key = m_key_list_->GetSelectedKey();
  if (key == nullptr) return;

  auto expire_datetime_format_str_local =
      QLocale().toString(key->ExpirationTime()) + " (" + tr("Local Time") +
      ") " + "\n";
  auto expire_datetime_format_str =
      QLocale().toString(key->ExpirationTime().toUTC()) + " (UTC) " + "\n";

  edit_->SlotAppendText2CurTextPage(expire_datetime_format_str_local +
                                    expire_datetime_format_str);
}

void MainWindow::slot_append_keys_fingerprint() {
  auto key = m_key_list_->GetSelectedKey();
  if (key == nullptr) return;

  auto fingerprint_format_str = BeautifyFingerprint(key->Fingerprint()) + "\n";

  edit_->SlotAppendText2CurTextPage(fingerprint_format_str);
}

void MainWindow::slot_copy_mail_address_to_clipboard() {
  auto key = m_key_list_->GetSelectedKey();
  if (key == nullptr) return;

  QClipboard* cb = QApplication::clipboard();
  cb->setText(key->Email());
}

void MainWindow::slot_copy_default_uid_to_clipboard() {
  auto key = m_key_list_->GetSelectedKey();
  if (key == nullptr || key->KeyType() != GpgAbstractKeyType::kGPG_KEY) return;

  QClipboard* cb = QApplication::clipboard();
  cb->setText(qSharedPointerDynamicCast<GpgKey>(key)->UIDs().front().GetUID());
}

void MainWindow::slot_copy_key_id_to_clipboard() {
  auto key = m_key_list_->GetSelectedKey();
  if (key == nullptr) return;

  QClipboard* cb = QApplication::clipboard();
  cb->setText(key->ID());
}

void MainWindow::slot_show_key_details() {
  auto keys = m_key_list_->GetSelectedKeys();
  if (keys.isEmpty()) return;

  CommonUtils::OpenDetailsDialogByKey(
      this, m_key_list_->GetCurrentGpgContextChannel(), keys.front());
}

void MainWindow::slot_add_key_2_favorite() {
  auto key = m_key_list_->GetSelectedKey();
  if (key == nullptr) return;

  auto key_db_name =
      GetGpgKeyDatabaseName(m_key_list_->GetCurrentGpgContextChannel());

  LOG_D() << "add key" << key->ID() << "to favorite at key db" << key_db_name;

  CommonUtils::GetInstance()->AddKey2Favorite(key_db_name, key);
  emit SignalUIRefresh();
}

void MainWindow::slot_remove_key_from_favorite() {
  auto keys = m_key_list_->GetSelectedKeys();
  if (keys.empty()) return;

  auto key_db_name =
      GetGpgKeyDatabaseName(m_key_list_->GetCurrentGpgContextChannel());

  CommonUtils::GetInstance()->RemoveKeyFromFavorite(key_db_name, keys.front());

  emit SignalUIRefresh();
}

void MainWindow::refresh_keys_from_key_server() {
  auto keys = m_key_list_->GetSelectedGpgKeys();
  if (keys.empty()) return;
  CommonUtils::GetInstance()->ImportGpgKeyFromKeyServer(
      m_key_list_->GetCurrentGpgContextChannel(), keys);
}

void MainWindow::slot_set_owner_trust_level_of_key() {
  auto keys = m_key_list_->GetSelectedGpgKeys();
  if (keys.empty()) return;

  auto* function = new SetOwnerTrustLevel(this);
  function->Exec(m_key_list_->GetCurrentGpgContextChannel(), keys.front());
  function->deleteLater();
}

void MainWindow::upload_key_to_server() {
  auto keys = m_key_list_->GetSelectedKeys();
  if (keys.empty()) return;

  auto* dialog = new KeyUploadDialog(m_key_list_->GetCurrentGpgContextChannel(),
                                     keys, this);
  dialog->show();
  dialog->SlotUpload();
}

void MainWindow::SlotOpenFile(const QString& path) {
  edit_->SlotOpenFile(path);
}

void MainWindow::slot_version_upgrade_notify() {
  if (!Module::IsModuleActivate(kVersionCheckingModuleID)) return;

  auto is_loading_done = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.loading_done", false);
  if (!is_loading_done) {
    FLOG_W("invalid version info from rt, loading hasn't done yet");
    return;
  }

  const auto project_build_branch = GetProjectBuildGitBranchName();
  if (project_build_branch.contains("develop") ||
      project_build_branch.contains("dev/")) {
    LOG_I() << "develop or testing version, skip notifying version info.";
    return;
  }

  auto is_need_upgrade = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.need_upgrade", false);

  auto is_current_version_publish_in_remote =
      Module::RetrieveRTValueTypedOrDefault<>(
          kVersionCheckingModuleID, "version.current_version_publish_in_remote",
          false);

  auto latest_version = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.latest_version", QString{});

  auto is_git_commit_hash_mismatch = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.git_commit_hash_mismatch", false);

  auto is_current_commit_hash_publish_in_remote =
      Module::RetrieveRTValueTypedOrDefault<>(
          kVersionCheckingModuleID,
          "version.current_commit_hash_publish_in_remote", false);

  if (is_need_upgrade) {
    statusBar()->showMessage(
        tr("GpgFrontend Upgradeable (New Version: %1).").arg(latest_version),
        30000);

    auto* b = new QToolButton();
    b->setToolButtonStyle(Qt::ToolButtonIconOnly);
    b->setIcon(QIcon(":/icons/upgrade.png"));
    connect(b, &QPushButton::clicked,
            [=]() { (new AboutDialog(tr("Update"), this))->show(); });
    statusBar()->addPermanentWidget(b);
  } else if (!is_current_version_publish_in_remote) {
    auto response = QMessageBox::warning(
        this, tr("Unstable Version"),
        tr("This version (%1) is not an official stable release. It may have "
           "been withdrawn or is a beta build. "
           "Please stop using this version immediately and download the latest "
           "stable version (%2) from the GitHub Releases page.")
            .arg(GetProjectVersion())
            .arg(latest_version),
        QMessageBox::Ok | QMessageBox::Open);

    if (response == QMessageBox::Open) {
      QDesktopServices::openUrl(
          QUrl("https://github.com/saturneric/GpgFrontend/releases/latest"));
    }
  } else if (is_git_commit_hash_mismatch && IsCheckReleaseCommitHash()) {
    QMessageBox::information(
        this, tr("Commit Hash Mismatch"),
        tr("The current version's commit hash does not match the official "
           "release. This may indicate a modified or unofficial build. For "
           "security reasons, please verify your installation or download the "
           "official release from the Github Releases Page."),
        QMessageBox::Ok);
  } else if (!is_current_commit_hash_publish_in_remote) {
    QMessageBox::information(
        this, tr("Unverified Commit Hash"),
        tr("The commit hash for this build was not found in the official "
           "remote repository. This could indicate a modified or unofficial "
           "build. For your security, please verify your installation or "
           "download the official release from the GitHub Releases page."),
        QMessageBox::Ok);
  }
}

void MainWindow::slot_refresh_current_file_view() {
  if (edit_->CurFilePage() != nullptr) {
    edit_->CurFilePage()->update();
  }
}

void MainWindow::slot_import_key_from_edit() {
  if (edit_->TabCount() == 0 || edit_->CurPageTextEdit() == nullptr) return;

  auto plain_text = edit_->CurPlainText().toLatin1();
  CommonUtils::GetInstance()->SlotImportKeys(
      this, m_key_list_->GetCurrentGpgContextChannel(), GFBuffer(plain_text));

  plain_text.fill('X');
  plain_text.clear();
}

void MainWindow::slot_verify_email_by_eml_data(const QByteArray& buffer) {
  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  GpgOperaHelper::WaitForOpera(
      this, tr("Verifying"), [this, sec_buf_base64](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_VERIFY_EML_DATA",
            {
                {"eml_data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_VERIFY_EML_DATA callback: " << i << ei;

              // end waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (p.contains("signature") && p.contains("mime")) {
                slot_verify_email_by_eml_data_result_helper(p);
              }

              return 0;
            });
      });
}

void MainWindow::slot_decrypt_email_by_eml_data(const QByteArray& buffer) {
  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  Module::TriggerEvent(
      "EMAIL_DECRYPT_EML_DATA",
      {
          {"eml_data", *sec_buf_base64},
          {"channel", GFBuffer{QString::number(
                          m_key_list_->GetCurrentGpgContextChannel())}},
      },
      [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
          Module::Event::Params p) {
        LOG_D() << "EMAIL_DECRYPT_EML_DATA callback: " << i << ei;

        // check if error occurred
        if (handle_module_error(p)) return -1;

        if (p.contains("eml_data")) {
          decrypt_email_by_eml_data_result_helper(p);
        }

        return 0;
      });
}

void MainWindow::SlotVerifyEML() {
  if (edit_->TabCount() == 0 || edit_->CurPageTextEdit() == nullptr) return;

  auto buffer = edit_->CurPlainText().toLatin1();
  buffer = buffer.replace("\n", "\r\n");

  // LOG_D() << "EML BUFFER: " << buffer;

  slot_verify_email_by_eml_data(buffer);
}

void MainWindow::slot_verifying_unknown_signature_helper(
    const QStringList& fprs) {
  if (!Module::IsModuleActivate(kKeyServerSyncModuleID) || fprs.empty()) return;

  auto fpr_set = QSet<QString>(fprs.begin(), fprs.end());

  QString fpr_list;
  for (const auto& fpr : fpr_set) {
    fpr_list += fpr + "\n";
  }

  LOG_D() << "try to sync missing key info from server: " << fpr_set;

  // Interaction with user
  auto user_response =
      QMessageBox::question(this, tr("Missing Keys"),
                            tr("Some signatures cannot be verified because "
                               "the corresponding keys are missing.\n\n"
                               "The following fingerprints are "
                               "missing:\n%1\n\n"
                               "Would you like to fetch these keys from "
                               "the key server?")
                                .arg(fpr_list),
                            QMessageBox::Yes | QMessageBox::No);

  if (user_response == QMessageBox::Yes) {
    CommonUtils::GetInstance()->ImportKeyByKeyServerSyncModule(
        this, m_key_list_->GetCurrentGpgContextChannel(), fpr_set.values());
  } else {
    QMessageBox::information(
        this, tr("Verification Incomplete"),
        tr("Verification was incomplete due to "
           "missing keys. You can manually import the keys "
           "later."));
  }
}

void MainWindow::slot_verify_email_by_eml_data_result_helper(
    const Module::Event::Params& p) {
  const auto part_mime_content_hash = p["mime_hash"];
  const auto prm_micalg_value = p["micalg"];

  auto timestamp =
      p.value("datetime", GFBuffer{"-1"}).ConvertToQString().toLongLong();
  auto datetime = tr("None");
  if (timestamp > 0) {
    datetime = QLocale().toString(QDateTime::fromMSecsSinceEpoch(timestamp));
  }

  QString email_info;
  email_info.append("# E-Mail Information\n\n");
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("From"))
          .arg(p.value("from", GFBuffer{tr("Unknown")}).ConvertToQString()));
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("To"))
          .arg(p.value("to", GFBuffer{tr("Unknown")}).ConvertToQString()));
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("Subject"))
          .arg(p.value("subject", GFBuffer{tr("None")}).ConvertToQString()));
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("CC"))
          .arg(p.value("cc", GFBuffer{tr("None")}).ConvertToQString()));
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("BCC"))
          .arg(p.value("bcc", GFBuffer{tr("None")}).ConvertToQString()));
  email_info.append(QString("- %1: %2\n").arg(tr("Date")).arg(datetime));

  email_info.append("\n");
  email_info.append("# OpenPGP Information\n\n");
  email_info.append(QString("- %1: %2\n")
                        .arg(tr("Signed EML Data Hash (SHA1)"))
                        .arg(part_mime_content_hash.ConvertToQString()));
  email_info.append(QString("- %1: %2\n")
                        .arg(tr("Message Integrity Check Algorithm"))
                        .arg(prm_micalg_value.ConvertToQString()));
  email_info.append("\n");

  if (!p["capsule_id"].Empty()) {
    auto v = UIModuleManager::GetInstance().GetCapsule(
        p.value("capsule_id").ConvertToQString());

    try {
      auto sign_result = std::any_cast<GpgVerifyResult>(v);
      auto result_analyse =
          GpgVerifyResultAnalyse(m_key_list_->GetCurrentGpgContextChannel(),
                                 GPG_ERR_NO_ERROR, sign_result);
      result_analyse.Analyse();

      slot_refresh_info_board(result_analyse.GetStatus(),
                              email_info + result_analyse.GetResultReport());

    } catch (const std::bad_any_cast& e) {
      LOG_E() << "capsule" << p["capsule_id"].ConvertToQString()
              << "convert to real type failed" << e.what();
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

void MainWindow::slot_result_analyse_show_helper(
    const QContainer<GpgOperaResult>& opera_results) {
  if (opera_results.empty()) {
    slot_refresh_info_board(0, "");
    return;
  }

  int overall_status = 1;  // Initialize to OK
  QStringList report;
  QStringList summary;

  QStringList failed_tags;
  QStringList warning_tags;

  int success_count = 0;
  int fail_count = 0;
  int warn_count = 0;

  for (const auto& opera_result : opera_results) {
    // Update overall status
    overall_status = std::min(overall_status, opera_result.status);

    QString status_text;
    if (opera_result.status < 0) {
      status_text = tr("FAIL");
      failed_tags << opera_result.tag;
      fail_count++;
    } else if (opera_result.status > 0) {
      status_text = tr("OK");
      success_count++;
    } else {
      status_text = tr("WARN");
      warning_tags << opera_result.tag;
      warn_count++;
    }

    // Append detailed report for each operation
    report.append(QString("[ %1 ] %2\n\n%3\n")
                      .arg(status_text, opera_result.tag, opera_result.report));
  }

  // Prepare summary section
  summary.append("# " + tr("Summary Report") + "\n\n");
  summary.append("- " + tr("Total Operations: %1\n").arg(opera_results.size()));
  summary.append("- " + tr("Successful: %1\n").arg(success_count));
  summary.append("- " + tr("Warnings: %1\n").arg(warn_count));
  summary.append("- " + tr("Failures: %1\n").arg(fail_count));

  if (!failed_tags.isEmpty()) {
    summary.append("- " +
                   tr("Failed Objects: %1\n").arg(failed_tags.join(", ")));
  }

  if (!warning_tags.isEmpty()) {
    summary.append("- " +
                   tr("Warning Objects: %1\n").arg(warning_tags.join(", ")));
  }

  // Display the final report in the info board
  if (opera_results.size() == 1) {
    slot_refresh_info_board(overall_status, report.join(""));

  } else {
    slot_refresh_info_board(overall_status,
                            summary.join("") + "\n\n" + report.join(""));
  }
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
  if (edit_->TabCount() == 0 || edit_->CurPageTextEdit() == nullptr) return;

  auto buffer = edit_->CurPlainText().toLatin1();
  buffer = buffer.replace("\n", "\r\n");

  // LOG_D() << "EML BUFFER: " << buffer;

  slot_decrypt_email_by_eml_data(buffer);
}

void MainWindow::decrypt_email_by_eml_data_result_helper(
    Module::Event::Params p) {
  auto timestamp =
      p.value("datetime", GFBuffer{"-1"}).ConvertToQString().toLongLong();
  auto datetime = tr("None");
  if (timestamp > 0) {
    datetime = QLocale().toString(QDateTime::fromMSecsSinceEpoch(timestamp));
  }

  const auto eml_data = GFBufferFactory::FromBase64(p["eml_data"]);
  if (eml_data) edit_->SlotSetGFBuffer2CurEMailPage(*eml_data);

  QString email_info;
  email_info.append("# E-Mail Information\n\n");
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("From"))
          .arg(p.value("from", GFBuffer{tr("Unknown")}).ConvertToQString()));
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("To"))
          .arg(p.value("to", GFBuffer{tr("Unknown")}).ConvertToQString()));
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("Subject"))
          .arg(p.value("subject", GFBuffer{tr("None")}).ConvertToQString()));
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("CC"))
          .arg(p.value("cc", GFBuffer{tr("None")}).ConvertToQString()));
  email_info.append(
      QString("- %1: %2\n")
          .arg(tr("BCC"))
          .arg(p.value("bcc", GFBuffer{tr("None")}).ConvertToQString()));
  email_info.append(QString("- %1: %2\n").arg(tr("Date")).arg(datetime));

  email_info.append("\n");

  if (!p["capsule_id"].Empty()) {
    auto v = UIModuleManager::GetInstance().GetCapsule(
        p.value("capsule_id").ConvertToQString());

    try {
      auto sign_result = std::any_cast<GpgDecryptResult>(v);
      auto result_analyse =
          GpgDecryptResultAnalyse(m_key_list_->GetCurrentGpgContextChannel(),
                                  GPG_ERR_NO_ERROR, sign_result);
      result_analyse.Analyse();

      slot_refresh_info_board(result_analyse.GetStatus(),
                              email_info + result_analyse.GetResultReport());

    } catch (const std::bad_any_cast& e) {
      LOG_E() << "capsule" << p["capsule_id"].ConvertToQString()
              << "convert to real type failed" << e.what();
    }
  }
}

void MainWindow::SlotEncryptEML() {
  if (edit_->TabCount() == 0 || edit_->CurEMailPage() == nullptr) return;
  auto keys = m_key_list_->GetCheckedKeys();

  if (keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for encrypt the EML."));
    return;
  }
  auto buffer = edit_->CurPlainText();

  auto key_ids =
      ConvertKey2GpgKeyIdList(m_key_list_->GetCurrentGpgContextChannel(), keys);

  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  buffer.fill('X');
  buffer.clear();

  GpgOperaHelper::WaitForOpera(
      this, tr("Encrypting"),
      [this, sec_buf_base64, key_ids](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_ENCRYPT_EML_DATA",
            {
                {"body_data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
                {"encrypt_keys", GFBuffer{key_ids.join(';')}},
            },

            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_ENCRYPT_EML_DATA callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (!p["eml_data"].Empty()) {
                edit_->SlotSetGFBuffer2CurEMailPage(p.value("eml_data"));
              }

              if (!p["capsule_id"].Empty()) {
                auto v = UIModuleManager::GetInstance().GetCapsule(
                    p.value("capsule_id").ConvertToQString());

                try {
                  auto encr_result = std::any_cast<GpgEncryptResult>(v);
                  auto result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, encr_result);
                  result_analyse.Analyse();
                  slot_result_analyse_show_helper(result_analyse);

                } catch (const std::bad_any_cast& e) {
                  LOG_E() << "capsule" << p["capsule_id"].ConvertToQString()
                          << "convert to real type failed" << e.what();
                }
              }

              return 0;
            });
      });
}

void MainWindow::SlotSignEML() {
  if (edit_->TabCount() == 0 || edit_->CurEMailPage() == nullptr) return;
  auto keys = m_key_list_->GetCheckedKeys();

  if (keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for signing the EML."));
    return;
  }

  if (keys.size() > 1) {
    QMessageBox::warning(this, tr("Multiple Keys Selected"),
                         tr("Please select only one key to sign the EML."));
    return;
  }

  auto buffer = edit_->CurPlainText();
  auto key_ids =
      ConvertKey2GpgKeyIdList(m_key_list_->GetCurrentGpgContextChannel(), keys);

  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  buffer.fill('X');
  buffer.clear();

  GpgOperaHelper::WaitForOpera(
      this, tr("Signing"),
      [this, sec_buf_base64, key_ids](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_SIGN_EML_DATA",
            {
                {"body_data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
                {"sign_key", GFBuffer{key_ids.front()}},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_SIGN_EML_DATA callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (!p["eml_data"].Empty()) {
                edit_->SlotSetGFBuffer2CurEMailPage(p.value("eml_data"));
              }

              if (!p["capsule_id"].Empty()) {
                auto v = UIModuleManager::GetInstance().GetCapsule(
                    p.value("capsule_id").ConvertToQString());

                try {
                  auto sign_result = std::any_cast<GpgSignResult>(v);
                  auto result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, sign_result);
                  result_analyse.Analyse();
                  slot_result_analyse_show_helper(result_analyse);

                } catch (const std::bad_any_cast& e) {
                  LOG_E() << "capsule" << p["capsule_id"].ConvertToQString()
                          << "convert to real type failed" << e.what();
                }
              }

              return 0;
            });
      });
}

void MainWindow::SlotEncryptSignEML() {
  if (edit_->TabCount() == 0 || edit_->CurEMailPage() == nullptr) return;
  auto keys = m_key_list_->GetCheckedKeys();

  if (keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for encrypt the EML."));
    return;
  }

  auto picker = QSharedPointer<SignersPicker>(
      new SignersPicker(m_key_list_->GetCurrentGpgContextChannel(), this),
      [](SignersPicker* picker) { picker->deleteLater(); });

  picker->exec();

  // return when canceled
  if (picker->result() == QDialog::Rejected) return;

  auto signer_keys = picker->GetCheckedSigners();
  assert(std::all_of(signer_keys.begin(), signer_keys.end(),
                     [](const auto& key) { return key->IsGood(); }));

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

  auto buffer = edit_->CurPlainText();
  auto key_ids =
      ConvertKey2GpgKeyIdList(m_key_list_->GetCurrentGpgContextChannel(), keys);

  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  buffer.fill('X');
  buffer.clear();

  GpgOperaHelper::WaitForOpera(
      this, tr("Encrypting and Signing"),
      [this, sec_buf_base64, key_ids, signer_keys](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_ENCRYPT_SIGN_EML_DATA",
            {
                {"body_data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
                {"sign_key", GFBuffer{signer_keys.front()->ID()}},
                {"encrypt_keys", GFBuffer{key_ids.front()}},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_ENCRYPT_SIGN_EML_DATA callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (!p["eml_data"].Empty()) {
                edit_->SlotSetGFBuffer2CurEMailPage(p.value("eml_data"));
              }

              if (!p["sign_capsule_id"].Empty() &&
                  !p["encr_capsule_id"].Empty()) {
                auto v1 = UIModuleManager::GetInstance().GetCapsule(
                    p.value("sign_capsule_id").ConvertToQString());
                auto v2 = UIModuleManager::GetInstance().GetCapsule(
                    p.value("encr_capsule_id").ConvertToQString());

                try {
                  auto sign_result = std::any_cast<GpgSignResult>(v1);
                  auto encr_result = std::any_cast<GpgEncryptResult>(v2);
                  auto sign_result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, sign_result);
                  auto encr_result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(),
                      GPG_ERR_NO_ERROR, encr_result);

                  sign_result_analyse.Analyse();
                  encr_result_analyse.Analyse();
                  slot_result_analyse_show_helper(sign_result_analyse,
                                                  encr_result_analyse);

                } catch (const std::bad_any_cast& e) {
                  LOG_E() << "capsule" << p["capsule_id"].ConvertToQString()
                          << "convert to real type failed" << e.what();
                }
              }

              return 0;
            });
      });
}

void MainWindow::SlotDecryptVerifyEML() {
  if (edit_->TabCount() == 0 || edit_->CurEMailPage() == nullptr) return;

  auto buffer = edit_->CurPlainText();

  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  buffer.fill('X');
  buffer.clear();

  GpgOperaHelper::WaitForOpera(
      this, tr("Decrypting and Verifying"),
      [this, sec_buf_base64](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            "EMAIL_DECRYPT_VERIFY_EML_DATA",
            {
                {"eml_data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << "EMAIL_DECRYPT_VERIFY_EML_DATA callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (!p["eml_data"].Empty()) {
                edit_->SlotSetGFBuffer2CurEMailPage(p.value("eml_data"));
              }

              const auto part_mime_content_hash = p["mime_hash"];
              const auto prm_micalg_value = p["micalg"];

              auto timestamp = p.value("datetime", GFBuffer{"-1"})
                                   .ConvertToQString()
                                   .toLongLong();
              auto datetime = tr("None");
              if (timestamp > 0) {
                datetime = QLocale().toString(
                    QDateTime::fromMSecsSinceEpoch(timestamp));
              }

              QString email_info;
              email_info.append("# E-Mail Information\n\n");
              email_info.append(
                  QString("- %1: %2\n")
                      .arg(tr("From"))
                      .arg(p.value("from", GFBuffer{tr("Unknown")})
                               .ConvertToQString()));
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("To"))
                                    .arg(p.value("to", GFBuffer{tr("Unknown")})
                                             .ConvertToQString()));
              email_info.append(
                  QString("- %1: %2\n")
                      .arg(tr("Subject"))
                      .arg(p.value("subject", GFBuffer{tr("None")})
                               .ConvertToQString()));
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("CC"))
                                    .arg(p.value("cc", GFBuffer{tr("None")})
                                             .ConvertToQString()));
              email_info.append(QString("- %1: %2\n")
                                    .arg(tr("BCC"))
                                    .arg(p.value("bcc", GFBuffer{tr("None")})
                                             .ConvertToQString()));
              email_info.append(
                  QString("- %1: %2\n").arg(tr("Date")).arg(datetime));

              email_info.append("\n");
              email_info.append("# OpenPGP Information\n\n");
              email_info.append(
                  QString("- %1: %2\n")
                      .arg(tr("Signed EML Data Hash (SHA1)"))
                      .arg(part_mime_content_hash.ConvertToQString()));
              email_info.append(
                  QString("- %1: %2\n")
                      .arg(tr("Message Integrity Check Algorithm"))
                      .arg(prm_micalg_value.ConvertToQString()));
              email_info.append("\n");

              if (!p["decr_capsule_id"].Empty() &&
                  !p["verify_capsule_id"].Empty()) {
                auto v1 = UIModuleManager::GetInstance().GetCapsule(
                    p.value("decr_capsule_id").ConvertToQString());
                auto v2 = UIModuleManager::GetInstance().GetCapsule(
                    p.value("verify_capsule_id").ConvertToQString());

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
                  LOG_E() << "capsule" << p["capsule_id"].ConvertToQString()
                          << "convert to real type failed" << e.what();
                }
              }

              return 0;
            });
      });
}

auto MainWindow::handle_module_error(QMap<QString, GFBuffer> p) -> bool {
  if (p["ret"] == "-2") {
    auto detailed_error = p["err"];

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
            .arg(detailed_error.ConvertToQString())
            .arg("https://www.rfc-editor.org/rfc/rfc3156.txt");
    slot_refresh_info_board(-2, info);
    return true;
  }

  if (p["ret"] != "0" || !p["err"].Empty()) {
    LOG_E() << "An error occurred trying to operate email, "
            << "error message: " << p["err"].ConvertToQString();

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
            .arg(p["ret"].ConvertToQString())
            .arg(p["err"].ConvertToQString());

    slot_refresh_info_board(-1, error_message);
    return true;
  }

  return false;
}

void MainWindow::slot_gpg_opera_buffer_show_helper(
    const QContainer<GpgOperaResult>& results) {
  for (const auto& result : results) {
    if (result.o_buffer.Empty()) continue;
    edit_->SlotFillTextEditWithText(result.o_buffer);
  }
}

void MainWindow::exec_operas_helper(
    const QString& task,
    const QSharedPointer<GpgOperaContextBasement>& contexts) {
  GpgOperaHelper::WaitForMultipleOperas(this, task, contexts->operas);
  slot_gpg_opera_buffer_show_helper(contexts->opera_results);
  slot_result_analyse_show_helper(contexts->opera_results);
}

}  // namespace GpgFrontend::UI
