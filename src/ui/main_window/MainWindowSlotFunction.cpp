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
#include "core/function/result_analyse/GpgResultAnalyse.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/dialog/SigningKeysPicker.h"
#include "ui/function/FileTypeUtils.h"
#include "ui/function/GpgErrorMessageBox.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/ImportKey.h"
#include "ui/function/InfoBoardCardConverter.h"
#include "ui/function/ProfileController.h"
#include "ui/function/SetOwnerTrustLevel.h"
#include "ui/function/ShowKeyDetails.h"
#include "ui/struct/GpgOperaResult.h"
#include "ui/widgets/FindWidget.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

namespace {

/// The current tab's content, prepared for a module operation.
///
/// Two things used to differ between the six SlotCustom* paths and both were
/// wrong in at least one of them: decrypt and verify encoded the text as
/// Latin-1 while every result came back decoded as UTF-8, so any non-ASCII
/// subject or body was corrupted on the way through; and only those two
/// restored CRLF, even though MIME is CRLF-delimited on every path. Going
/// through one helper is what stops them drifting apart again.
///
/// Reading also flushes a module tab's structured view, so the bytes can never
/// be behind what the user sees.
auto CurrentTabOperationPayload(TextEdit* edit) -> std::optional<GFBuffer> {
  auto text = edit->CurPlainTextForOperation();

  // Normalize to LF first so an already-CRLF document does not become CRCRLF.
  text.replace("\r\n", "\n");
  text.replace("\n", "\r\n");

  GFBuffer buffer(text);
  WipeString(text);

  auto base64 = GFBufferFactory::ToBase64(buffer);
  if (!base64) return std::nullopt;
  return base64;
}

/// The current tab's content for a VERIFY, exactly as the document holds it.
///
/// Deliberately not CurrentTabOperationPayload(): that rewrites every line
/// ending to CRLF, which is right for content about to be signed or encrypted
/// and wrong for content about to be judged. A signature covers exact octets,
/// so repairing them first answers a question about a document the user does
/// not have -- a message whose line endings were rewritten after signing
/// verified cleanly here while the message surface, reading the real bytes,
/// called it a forgery. The bytes are judged as they stand; that they are no
/// longer canonical is something the report says, not something it fixes.
auto CurrentTabVerifyPayload(TextEdit* edit) -> std::optional<GFBuffer> {
  auto bytes = edit->CurDocumentBytesForOperation();

  GFBuffer buffer(bytes);
  WipeByteArray(bytes);

  auto base64 = GFBufferFactory::ToBase64(buffer);
  if (!base64) return std::nullopt;
  return base64;
}

}  // namespace

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
    RaiseMessageBox(this, err);
    return;
  }

  auto key_text = gf_buffer.ConvertToQByteArray();

  // A structured document has a proper place for a key -- an
  // application/pgp-keys part -- so its view is offered the key first. Only a
  // tab that has no opinion about where a key goes gets the armor pasted in.
  const auto name = QString("%1.asc").arg(keys.front()->ID());
  if (edit_->AttachPublicKeyToCurPage(key_text, name) == 0) {
    edit_->SlotAppendText2CurTextPage(key_text);
  }

  WipeByteArray(key_text);
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

  ShowKeyDetails(this, m_key_list_->GetCurrentGpgContextChannel(),
                 keys.front());
}

void MainWindow::slot_set_owner_trust_level_of_key() {
  auto keys = m_key_list_->GetSelectedKeys();
  if (keys.empty()) return;

  auto* f = new SetOwnerTrustLevel(this);
  f->Exec(m_key_list_->GetCurrentGpgContextChannel(), keys.front());
  f->deleteLater();
}

void MainWindow::SlotOpenFile(const QString& path) {
  // A profile file is not a document: showing its bytes in the editor is never
  // what was meant, and today it does not even get that far -- it is refused as
  // binary. Caught here rather than in the file panel because both ways of
  // opening a file there, the double-click and the context menu, arrive here.
  //
  // Not where profiles are not offered: a sandboxed build has no window to open
  // one in, so there a .gfp goes to the editor like any other file.
  if (!IsRunningInAppSandbox() && IsProfilePackageFile(QFileInfo(path))) {
    handle_profile_package_from_file_view(path);
    return;
  }

  edit_->SlotOpenFile(path);
}

void MainWindow::slot_refresh_current_file_view() {
  if (edit_->CurFilePage() != nullptr) {
    edit_->CurFilePage()->update();
  }
}

void MainWindow::slot_import_key_from_edit() {
  if (edit_->TabCount() == 0 || edit_->CurPageTextEdit() == nullptr) return;

  auto plain_text = edit_->CurPlainText().toUtf8();
  ImportKeys(this, m_key_list_->GetCurrentGpgContextChannel(),
             GFBuffer(plain_text));

  WipeByteArray(plain_text);
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
    if (Module::IsEventListening("REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT")) {
      const auto fpr_values = fpr_set.values();
      Module::TriggerEvent("REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT",
                           {
                               {"fingerprint", GFBuffer(fpr_values.first())},
                           });
    }

  } else {
    QMessageBox::information(
        this, tr("Verification Incomplete"),
        tr("Verification was incomplete due to "
           "missing keys. You can manually import the keys "
           "later."));
  }
}

void MainWindow::slot_result_analyse_show_helper(
    const QList<GpgOperaResult>& opera_results,
    const InfoBoardBatchContext& context) {
  info_board_->SlotReset();

  if (opera_results.empty() && context.expected.isEmpty()) {
    info_board_->SetInfoBoard(tr("No operation result available."),
                              kINFO_ERROR_WARN);
    return;
  }

  info_board_->SetInfoBoardReport(
      BuildReportFromResults(opera_results, context));
}

void MainWindow::slot_refresh_info_board(int status, const QString& text) {
  info_board_->SlotReset();

  if (status < 0) {
    info_board_->SlotRefresh(text, kINFO_ERROR_CRITICAL);
  } else if (status > 0) {
    info_board_->SlotRefresh(text, kINFO_ERROR_OK);
  } else {
    info_board_->SlotRefresh(text, kINFO_ERROR_WARN);
  }
}

void MainWindow::slot_refresh_info_board_from_module(
    const Module::Event::Params& params) {
  if (!params.contains("result") || !params.contains("result_status")) return;

  const int status = params.value("result_status").ConvertToQString().toInt();
  const QString text = params.value("result").ConvertToQString();

  const InfoBoardStatus status_enum =
      status < 0 ? kINFO_ERROR_CRITICAL
                 : (status > 0 ? kINFO_ERROR_OK : kINFO_ERROR_WARN);

  InfoBoardCardsPayload payload;
  if (params.contains("result_cards")) {
    payload = decode_info_board_cards(
        params.value("result_cards").ConvertToQByteArray());
  }

  // The structured payload, cards or not, goes through the same model as
  // everything else: a failure without cards still gets its Outcome card,
  // with the module's own report as its reason. The board never parses the
  // report text.
  if (payload.valid) {
    info_board_->SlotReset();
    info_board_->SetInfoBoardCards(
        text, status_enum, payload.cards, payload.operation,
        payload.description, payload.details_title, payload.details_items);
    return;
  }

  slot_refresh_info_board(status, text);
}

void MainWindow::SlotCustomDecrypt(const QString& type) {
  // A QPointer, not a raw address: the callback below arrives an unbounded
  // time later, by which point this tab may have been switched away from or
  // closed outright. Results are addressed to THIS page rather than to
  // whichever one is current when they land. See TextEdit::SetGFBuffer2Page.
  const QPointer<QWidget> page = edit_->CurPage();
  if (edit_->TabCount() == 0 || page == nullptr) return;

  auto event_id = QString("EDIT_TAB_TYPE_%1_OP_DECRYPT").arg(type.toUpper());

  if (!Module::IsEventListening(event_id)) {
    QMessageBox::warning(
        this, tr("Unsupported Operation"),
        tr("The decrypt operation for the tab type '%1' is not supported.")
            .arg(type));
    return;
  }

  auto sec_buf_base64 = CurrentTabOperationPayload(edit_);
  if (!sec_buf_base64) return;

  // Wrapped like every other long-running crypto operation. Decrypt was the
  // one that never was, so it alone ran with the whole window live underneath
  // it. The dialog is a courtesy, not the correctness mechanism: that is the
  // page-addressed write above, which holds whether or not a modal is up.
  GpgOperaHelper::WaitForOpera(
      this, tr("Decrypting"),
      [this, event_id, sec_buf_base64, page](const OperaWaitingHd& hd) {
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

              if (p.contains("data")) {
                edit_->SetGFBuffer2Page(page, p["data"]);
              }

              slot_refresh_info_board_from_module(p);

              return 0;
            });
      });
}

void MainWindow::SlotCustomVerify(const QString& type) {
  // A QPointer, not a raw address: the callback below arrives an unbounded
  // time later, by which point this tab may have been switched away from or
  // closed outright. Results are addressed to THIS page rather than to
  // whichever one is current when they land. See TextEdit::SetGFBuffer2Page.
  const QPointer<QWidget> page = edit_->CurPage();
  if (edit_->TabCount() == 0 || page == nullptr) return;

  auto event_id = QString("EDIT_TAB_TYPE_%1_OP_VERIFY").arg(type.toUpper());

  if (!Module::IsEventListening(event_id)) {
    QMessageBox::warning(
        this, tr("Unsupported Operation"),
        tr("The verify operation for the tab type '%1' is not supported.")
            .arg(type));
    return;
  }

  // The document as it stands, not a canonicalized copy of it: see
  // CurrentTabVerifyPayload.
  auto sec_buf_base64 = CurrentTabVerifyPayload(edit_);
  if (!sec_buf_base64) return;

  GpgOperaHelper::WaitForOpera(
      this, tr("Verifying"),
      [this, event_id, sec_buf_base64, page](const OperaWaitingHd& hd) {
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
              if (handle_module_error(p)) {
                // The page asked for this and is waiting on it. Told that it
                // ended, even though it ended with nothing: a request whose
                // answer never arrives at all would leave the message surface
                // unable to ask again.
                edit_->ApplyVerificationToPage(page, {});
                return -1;
              }

              slot_refresh_info_board_from_module(p);

              // The same answer the board above was just given, to the message
              // surface that shows the badge and the attachment list. One
              // verification, one result, every surface quoting it: the tab
              // used to run a second verification of its own and the two
              // regularly disagreed about the same message.
              edit_->ApplyVerificationToPage(
                  page, p.contains("verification")
                            ? p.value("verification").ConvertToQByteArray()
                            : QByteArray{});

              return 0;
            });
      });
}

void MainWindow::SlotCustomEncrypt(const QString& type) {
  // A QPointer, not a raw address: the callback below arrives an unbounded
  // time later, by which point this tab may have been switched away from or
  // closed outright. Results are addressed to THIS page rather than to
  // whichever one is current when they land. See TextEdit::SetGFBuffer2Page.
  const QPointer<QWidget> page = edit_->CurPage();
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

  if (!confirm_recipients_helper(keys)) return;
  if (!confirm_recipient_mix_helper(keys)) return;

  auto key_ids =
      ConvertKey2GpgKeyIdList(m_key_list_->GetCurrentGpgContextChannel(), keys);

  auto sec_buf_base64 = CurrentTabOperationPayload(edit_);
  if (!sec_buf_base64) return;

  GpgOperaHelper::WaitForOpera(
      this, tr("Encrypting"),
      [this, event_id, sec_buf_base64, key_ids,
       page](const OperaWaitingHd& hd) {
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
                edit_->SetGFBuffer2Page(page, p.value("data"));
              }

              slot_refresh_info_board_from_module(p);

              return 0;
            });
      });
}

void MainWindow::SlotCustomSign(const QString& type) {
  // A QPointer, not a raw address: the callback below arrives an unbounded
  // time later, by which point this tab may have been switched away from or
  // closed outright. Results are addressed to THIS page rather than to
  // whichever one is current when they land. See TextEdit::SetGFBuffer2Page.
  const QPointer<QWidget> page = edit_->CurPage();
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
                         tr("Please select a key for signing this document."));
    return;
  }

  if (keys.size() > 1) {
    QMessageBox::warning(this, tr("Multiple Keys Selected"),
                         tr("Please select only one key to sign this document."));
    return;
  }

  auto key_ids =
      ConvertKey2GpgKeyIdList(m_key_list_->GetCurrentGpgContextChannel(), keys);

  auto sec_buf_base64 = CurrentTabOperationPayload(edit_);
  if (!sec_buf_base64) return;

  GpgOperaHelper::WaitForOpera(
      this, tr("Signing"),
      [this, event_id, sec_buf_base64, key_ids,
       page](const OperaWaitingHd& hd) {
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
                edit_->SetGFBuffer2Page(page, p.value("data"));
              }

              slot_refresh_info_board_from_module(p);

              return 0;
            });
      });
}

void MainWindow::SlotCustomEncryptSign(const QString& type) {
  // A QPointer, not a raw address: the callback below arrives an unbounded
  // time later, by which point this tab may have been switched away from or
  // closed outright. Results are addressed to THIS page rather than to
  // whichever one is current when they land. See TextEdit::SetGFBuffer2Page.
  const QPointer<QWidget> page = edit_->CurPage();
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
                         tr("Please select a key for encrypting this document."));
    return;
  }

  if (!confirm_recipients_helper(keys)) return;
  if (!confirm_recipient_mix_helper(keys)) return;

  auto picker = QSharedPointer<SigningKeysPicker>(
      new SigningKeysPicker(m_key_list_->GetCurrentGpgContextChannel(), this),
      [](SigningKeysPicker* picker) { picker->deleteLater(); });

  picker->exec();

  // return when canceled
  if (picker->result() == QDialog::Rejected) return;

  auto signer_keys = picker->GetSigningKeys();
  assert(std::all_of(signer_keys.begin(), signer_keys.end(),
                     [](const auto& key) { return key->IsGood(); }));

  if (signer_keys.isEmpty()) {
    QMessageBox::warning(this, tr("No Key Selected"),
                         tr("Please select a key for signing this document."));
    return;
  }

  if (signer_keys.size() > 1) {
    QMessageBox::warning(this, tr("Multiple Keys Selected"),
                         tr("Please select only one key to sign this document."));
    return;
  }

  auto key_ids =
      ConvertKey2GpgKeyIdList(m_key_list_->GetCurrentGpgContextChannel(), keys);

  auto sec_buf_base64 = CurrentTabOperationPayload(edit_);
  if (!sec_buf_base64) return;

  GpgOperaHelper::WaitForOpera(
      this, tr("Encrypting and Signing"),
      [this, event_id, sec_buf_base64, key_ids, signer_keys,
       page](const OperaWaitingHd& hd) {
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
                edit_->SetGFBuffer2Page(page, p.value("data"));
              }

              slot_refresh_info_board_from_module(p);

              return 0;
            });
      });
}

void MainWindow::SlotCustomDecryptVerify(const QString& type) {
  // A QPointer, not a raw address: the callback below arrives an unbounded
  // time later, by which point this tab may have been switched away from or
  // closed outright. Results are addressed to THIS page rather than to
  // whichever one is current when they land. See TextEdit::SetGFBuffer2Page.
  const QPointer<QWidget> page = edit_->CurPage();
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

  auto sec_buf_base64 = CurrentTabOperationPayload(edit_);
  if (!sec_buf_base64) return;

  GpgOperaHelper::WaitForOpera(
      this, tr("Decrypting and Verifying"),
      [this, event_id, sec_buf_base64, page](const OperaWaitingHd& hd) {
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
                edit_->SetGFBuffer2Page(page, p.value("data"));
              }

              slot_refresh_info_board_from_module(p);

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
    LOG_E() << "An error occurred trying to run the module operation, "
            << "error message: " << p["err"].ConvertToQString();
    return true;
  }

  return false;
}

void MainWindow::slot_gpg_opera_buffer_show_helper(
    const QList<GpgOperaResult>& results, InfoBoardBatchContext* context) {
  const auto record = [context](const GpgOperaResult& result, OutputFate fate,
                                const QString& error = {}) {
    if (context == nullptr) return;
    context->fates.insert(result.input_index, fate);
    if (!error.isEmpty()) {
      context->output_errors[result.input_index].append(
          {OutputAction::kAPPLY_TO_DOCUMENT,
           {},
           std::nullopt,
           error,
           OutputErrorOrigin::kAPPLICATION});
    }
  };

  bool applied = false;
  for (const auto& result : results) {
    // Never replace the editor content with the output of a failed operation.
    // A failed decrypt can still carry a non-empty buffer (e.g. gpg extracts
    // the cleartext payload of a clear-signed message), and writing it back
    // would silently overwrite the user's input with leaked plaintext.
    if (result.o_buffer.Empty()) {
      // A verification has nothing to put back by design.
      record(result, result.op_info.op == GpgOperation::kVERIFY
                         ? OutputFate::kNOT_APPLICABLE
                         : OutputFate::kNOT_PRODUCED);
      continue;
    }
    if (result.op_info.status < 0) {
      record(result, OutputFate::kWITHHELD);
      continue;
    }
    // Only the first usable output goes in; nothing is claimed of the rest.
    if (applied) continue;
    applied = true;

    // The current tab can have changed during the wait to one with no text
    // to replace; filling it then does nothing, and that is a failure.
    if (edit_->CurTextPage() == nullptr) {
      record(result, OutputFate::kAPPLY_FAILED,
             tr("The current tab has no text document to receive the "
                "result."));
      continue;
    }
    edit_->SlotFillTextEditWithText(result.o_buffer);
    record(result, OutputFate::kAPPLIED);
  }
}

auto MainWindow::exec_operas_helper(
    const QString& task,
    const QSharedPointer<GpgOperaContextBasement>& contexts) -> bool {
  GpgOperaHelper::WaitForMultipleOperas(
      this, task, contexts->operas, m_key_list_->GetCurrentGpgContextChannel());

  // Releases the reference cycle the operas hold on this basement.
  contexts->operas.clear();

  // A snapshot: an operation that was still in flight when the wait ended can
  // append one more result, and that must not happen while the list below is
  // being iterated.
  const auto opera_results = contexts->opera_results;

  bool all_success = !opera_results.empty();

  for (const auto& result : opera_results) {
    if (result.op_info.status <= 0) {
      all_success = false;
      break;
    }
  }

  InfoBoardBatchContext context;
  // A buffer operation works on the current document; a file one here (file
  // Verify) produces nothing to put anywhere.
  context.document_target = std::any_of(
      opera_results.cbegin(), opera_results.cend(),
      [](const GpgOperaResult& r) { return r.source_path.isEmpty(); });
  slot_gpg_opera_buffer_show_helper(opera_results, &context);

  slot_result_analyse_show_helper(opera_results, context);
  return all_success;
}

}  // namespace GpgFrontend::UI
