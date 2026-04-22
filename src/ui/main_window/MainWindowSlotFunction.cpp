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
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
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

  auto [err, gf_buffer] = KeyImportExportOperation::GetInstance(
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

void MainWindow::slot_set_owner_trust_level_of_key() {
  auto keys = m_key_list_->GetSelectedKeys();
  if (keys.empty()) return;

  auto* f = new SetOwnerTrustLevel(this);
  f->Exec(m_key_list_->GetCurrentGpgContextChannel(), keys.front());
  f->deleteLater();
}

void MainWindow::SlotOpenFile(const QString& path) {
  edit_->SlotOpenFile(path);
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

void MainWindow::slot_import_keys_from_key_server(const QStringList& fprs) {
  auto channel = m_key_list_->GetCurrentGpgContextChannel();
  auto all_key_data = SecureCreateSharedObject<GFBuffer>();
  auto remaining_tasks = SecureCreateSharedObject<int>(fprs.size());

  for (const auto& fpr : fprs) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
        ->PostTask(new Thread::Task(
            [=](const DataObjectPtr& data_obj) -> int {
              Module::TriggerEvent(
                  "REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT",
                  {
                      {"fingerprint", GFBuffer{fpr}},
                  },
                  [fpr, all_key_data, remaining_tasks, this, channel](
                      Module::EventIdentifier i,
                      Module::Event::ListenerIdentifier ei,
                      Module::Event::Params p) {
                    if (p["ret"] != "0" || !p["error_msg"].Empty()) {
                      LOG_E()
                          << "An error occurred trying to get data from key:"
                          << fpr << "error message: "
                          << p["error_msg"].ConvertToQString() << "reply data: "
                          << p["reply_data"].ConvertToQString();
                    } else if (p.contains("key_data")) {
                      all_key_data->Append(p["key_data"]);
                    }

                    // it only uses one thread for these operations
                    // so that is no need for locking now
                    (*remaining_tasks)--;

                    // all tasks are done
                    if (*remaining_tasks == 0) {
                      CommonUtils::GetInstance()->ImportKeys(this, channel,
                                                             *all_key_data);
                    }
                  });

              return 0;
            },
            QString("key_%1_import_task").arg(fpr)));
  }
}

void MainWindow::slot_verifying_unknown_signature_helper(
    const QStringList& fprs) {
  // check if event is listened by any module
  if (!Module::IsEventListening("REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT") ||
      fprs.empty()) {
    return;
  }

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
    slot_import_keys_from_key_server(fpr_set.values());
  } else {
    QMessageBox::information(
        this, tr("Verification Incomplete"),
        tr("Verification was incomplete due to "
           "missing keys. You can manually import the keys "
           "later."));
  }
}

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
  summary.append("- " + tr("Total Operations: %1").arg(opera_results.size()) +
                 "\n");
  summary.append("- " + tr("Successful: %1").arg(success_count) + "\n");
  summary.append("- " + tr("Warnings: %1").arg(warn_count) + "\n");
  summary.append("- " + tr("Failures: %1").arg(fail_count) + "\n");

  if (!failed_tags.isEmpty()) {
    summary.append("- " +
                   tr("Failed Objects: %1").arg(failed_tags.join(", ") + "\n"));
  }

  if (!warning_tags.isEmpty()) {
    summary.append(
        "- " + tr("Warning Objects: %1").arg(warning_tags.join(", ") + "\n"));
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

void MainWindow::SlotCustomDecrypt(const QString& type) {
  auto* page = edit_->CurPage();
  if (edit_->TabCount() == 0 || page == nullptr) return;

  auto event_id = QString("EDIT_TAB_TYPE_%1_OP_DECRYPT").arg(type.toUpper());

  if (!Module::IsEventListening(event_id)) {
    QMessageBox::warning(
        this, tr("Unsupported Operation"),
        tr("The decrypt operation for the tab type '%1' is not supported.")
            .arg(type));
    return;
  }

  auto buffer = edit_->CurPlainText().toLatin1();
  buffer = buffer.replace("\n", "\r\n");

  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  Module::TriggerEvent(
      event_id,
      {
          {"data", *sec_buf_base64},
          {"channel", GFBuffer{QString::number(
                          m_key_list_->GetCurrentGpgContextChannel())}},
      },
      [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
          Module::Event::Params p) {
        LOG_D() << event_id << " callback: " << i << ei;

        // check if error occurred
        if (handle_module_error(p)) return -1;

        if (p.contains("data")) {
          edit_->SlotSetGFBuffer2CurTextPage(p["data"]);
        }

        if (p.contains("result") && p.contains("result_status")) {
          slot_refresh_info_board(
              p.value("result_status").ConvertToQString().toInt(),
              p.value("result").ConvertToQString());
        }

        return 0;
      });
}

void MainWindow::SlotCustomVerify(const QString& type) {
  auto* page = edit_->CurPage();
  if (edit_->TabCount() == 0 || page == nullptr) return;

  auto event_id = QString("EDIT_TAB_TYPE_%1_OP_VERIFY").arg(type.toUpper());

  if (!Module::IsEventListening(event_id)) {
    QMessageBox::warning(
        this, tr("Unsupported Operation"),
        tr("The verify operation for the tab type '%1' is not supported.")
            .arg(type));
    return;
  }

  auto buffer = edit_->CurPlainText().toLatin1();
  buffer = buffer.replace("\n", "\r\n");

  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  GpgOperaHelper::WaitForOpera(
      this, tr("Verifying"),
      [this, event_id, sec_buf_base64](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            event_id,
            {
                {"data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << event_id << " callback: " << i << ei;

              // end waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (p.contains("result") && p.contains("result_status")) {
                slot_refresh_info_board(
                    p.value("result_status").ConvertToQString().toInt(),
                    p.value("result").ConvertToQString());
              }

              return 0;
            });
      });
}

void MainWindow::SlotCustomEncrypt(const QString& type) {
  auto* page = edit_->CurPage();
  if (edit_->TabCount() == 0 || page == nullptr) return;

  auto event_id = QString("EDIT_TAB_TYPE_%1_OP_ENCRYPT").arg(type.toUpper());

  if (!Module::IsEventListening(event_id)) {
    QMessageBox::warning(
        this, tr("Unsupported Operation"),
        tr("The encryption operation for the tab type '%1' is not supported.")
            .arg(type));
    return;
  }

  auto keys = m_key_list_->GetCheckedKeys();
  if (keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for encryption."));
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
      [this, event_id, sec_buf_base64, key_ids](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            event_id,
            {
                {"body_data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
                {"encrypt_keys", GFBuffer{key_ids.join(';')}},
            },

            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << event_id << " callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (!p["data"].Empty()) {
                edit_->SlotSetGFBuffer2CurTextPage(p.value("data"));
              }

              if (p.contains("result") && p.contains("result_status")) {
                slot_refresh_info_board(
                    p.value("result_status").ConvertToQString().toInt(),
                    p.value("result").ConvertToQString());
              }

              return 0;
            });
      });
}

void MainWindow::SlotCustomSign(const QString& type) {
  auto* page = edit_->CurPage();
  if (edit_->TabCount() == 0 || page == nullptr) return;

  auto event_id = QString("EDIT_TAB_TYPE_%1_OP_SIGN").arg(type.toUpper());

  if (!Module::IsEventListening(event_id)) {
    QMessageBox::warning(this, tr("Unsupported Operation"),
                         tr("The sign operation for the tab type "
                            "'%1' is not supported.")
                             .arg(type));
    return;
  }

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
      [this, event_id, sec_buf_base64, key_ids](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            event_id,
            {
                {"body_data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
                {"sign_key", GFBuffer{key_ids.front()}},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << event_id << " callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (!p["data"].Empty()) {
                edit_->SlotSetGFBuffer2CurTextPage(p.value("data"));
              }

              if (p.contains("result") && p.contains("result_status")) {
                slot_refresh_info_board(
                    p.value("result_status").ConvertToQString().toInt(),
                    p.value("result").ConvertToQString());
              }

              return 0;
            });
      });
}

void MainWindow::SlotCustomEncryptSign(const QString& type) {
  auto* page = edit_->CurPage();
  if (edit_->TabCount() == 0 || page == nullptr) return;

  auto event_id =
      QString("EDIT_TAB_TYPE_%1_OP_ENCRYPT_SIGN").arg(type.toUpper());

  if (!Module::IsEventListening(event_id)) {
    QMessageBox::warning(this, tr("Unsupported Operation"),
                         tr("The encrypt and sign operation for the tab type "
                            "'%1' is not supported.")
                             .arg(type));
    return;
  }

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
      [this, event_id, sec_buf_base64, key_ids,
       signer_keys](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            event_id,
            {
                {"body_data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
                {"sign_key", GFBuffer{signer_keys.front()->ID()}},
                {"encrypt_keys", GFBuffer{key_ids.front()}},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << event_id << " callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (!p["data"].Empty()) {
                edit_->SlotSetGFBuffer2CurTextPage(p.value("data"));
              }

              if (p.contains("result") && p.contains("result_status")) {
                slot_refresh_info_board(
                    p.value("result_status").ConvertToQString().toInt(),
                    p.value("result").ConvertToQString());
              }

              return 0;
            });
      });
}

void MainWindow::SlotCustomDecryptVerify(const QString& type) {
  auto* page = edit_->CurPage();
  if (edit_->TabCount() == 0 || page == nullptr) return;

  auto event_id =
      QString("EDIT_TAB_TYPE_%1_OP_DECRYPT_VERIFY").arg(type.toUpper());

  if (!Module::IsEventListening(event_id)) {
    QMessageBox::warning(this, tr("Unsupported Operation"),
                         tr("The decrypt and verify operation for the tab type "
                            "'%1' is not supported.")
                             .arg(type));
    return;
  }

  auto buffer = edit_->CurPlainText();

  GFBuffer sec_buf(buffer);
  auto sec_buf_base64 = GFBufferFactory::ToBase64(sec_buf);
  if (!sec_buf_base64) return;

  buffer.fill('X');
  buffer.clear();

  GpgOperaHelper::WaitForOpera(
      this, tr("Decrypting and Verifying"),
      [this, event_id, sec_buf_base64](const OperaWaitingHd& hd) {
        Module::TriggerEvent(
            event_id,
            {
                {"data", *sec_buf_base64},
                {"channel", GFBuffer{QString::number(
                                m_key_list_->GetCurrentGpgContextChannel())}},
            },
            [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
                Module::Event::Params p) {
              LOG_D() << event_id << " callback: " << i << ei;

              // close waiting dialog
              hd();

              // check if error occurred
              if (handle_module_error(p)) return -1;

              if (!p["data"].Empty()) {
                edit_->SlotSetGFBuffer2CurTextPage(p.value("data"));
              }

              if (p.contains("result") && p.contains("result_status")) {
                slot_refresh_info_board(
                    p.value("result_status").ConvertToQString().toInt(),
                    p.value("result").ConvertToQString());
              }

              return 0;
            });
      });
}

auto MainWindow::handle_module_error(QMap<QString, GFBuffer> p) -> bool {
  if (p["ret"] == "-2") {
    auto detailed_error = p["err"];

    return true;
  }

  if (p["ret"] != "0" || !p["err"].Empty()) {
    LOG_E() << "An error occurred trying to operate email, "
            << "error message: " << p["err"].ConvertToQString();
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
