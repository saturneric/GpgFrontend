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
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "core/utils/MemoryUtils.h"
#include "ui/dialog/EncryptionKeysPicker.h"
#include "ui/dialog/SigningKeysPicker.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/InfoBoardCardConverter.h"
#include "ui/struct/GpgOperaResultContext.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

namespace {

// Drop the staging directories once their outputs have been committed or
// cleaned up.
void RemoveSafeOutputWorkDirs(const QContainer<SafeOutputPath>& outputs) {
  for (const auto& output : outputs) {
    RemoveSafeOutputWorkDir(output.temp_path);
  }
}

void CleanupSafeOutputFiles(const QContainer<SafeOutputPath>& outputs) {
  for (const auto& output : outputs) {
    if (!output.temp_path.isEmpty() && QFileInfo::exists(output.temp_path)) {
      QFile::remove(output.temp_path);
    }
  }
}

auto IsFileOperaSuccessful(const QContainer<GpgOperaResult>& results) -> bool {
  if (results.empty()) return false;

  return std::all_of(results.cbegin(), results.cend(),
                     [](const GpgOperaResult& result) -> bool {
                       return result.op_info.status > 0;
                     });
}

auto PrepareSafeOutputPath(const QString& final_path,
                           QStringList* final_output_paths,
                           QContainer<SafeOutputPath>* safe_outputs)
    -> QString {
  if (final_output_paths == nullptr || safe_outputs == nullptr) return {};

  const auto temp_path = MakeSafeOutputTempPath(final_path);

  final_output_paths->append(final_path);
  safe_outputs->push_back({
      final_path,
      temp_path,
  });

  return temp_path;
}

/// A codec's cards, from its result_cards JSON; none when it sent none.
auto CodecCards(const CodecStageResult& r) -> InfoBoardCardsPayload {
  if (r.cards.isEmpty()) return {};
  return decode_info_board_cards(r.cards.toUtf8());
}

}  // namespace

auto MainWindow::commit_safe_output_files(
    const QContainer<SafeOutputPath>& outputs) -> bool {
  for (const auto& output : outputs) {
    if (!CommitSafeOutputFile(output.temp_path, output.final_path)) {
      QMessageBox::critical(
          this, tr("Error"),
          tr("Failed to finalize output file:\n\n%1").arg(output.final_path));

      CleanupSafeOutputFiles(outputs);
      return false;
    }
  }

  return true;
}

void MainWindow::exec_file_operas_helper(
    const QString& task,
    const QSharedPointer<GpgOperaContextBasement>& contexts,
    const QContainer<SafeOutputPath>& safe_outputs) {
  GpgOperaHelper::WaitForMultipleOperas(
      this, task, contexts->operas, m_key_list_->GetCurrentGpgContextChannel());

  // Releases the reference cycle the operas hold on this basement.
  contexts->operas.clear();

  // A snapshot: an operation that was still in flight when the wait ended can
  // append one more result, and that must not happen while the list below is
  // being iterated.
  const auto opera_results = contexts->opera_results;

  const bool success = IsFileOperaSuccessful(opera_results);

  if (!success) {
    LOG_E()
        << "One or more file operations failed. Cleaning up temporary files.";
    CleanupSafeOutputFiles(safe_outputs);
    RemoveSafeOutputWorkDirs(safe_outputs);
    slot_result_analyse_show_helper(opera_results);
    return;
  }

  if (!commit_safe_output_files(safe_outputs)) {
    LOG_E() << "Failed to commit output files. Temporary files have been "
               "cleaned up.";
    CleanupSafeOutputFiles(safe_outputs);
    RemoveSafeOutputWorkDirs(safe_outputs);

    slot_refresh_info_board(
        -1,
        tr("The operation succeeded, but GpgFrontend failed to finalize one "
           "or more output files.\n\n"
           "Temporary output files have been cleaned up. Original files were "
           "kept unchanged."));
    return;
  }

  RemoveSafeOutputWorkDirs(safe_outputs);

  LOG_D() << "All file operations completed successfully. Output files have "
             "been finalized.";

  slot_result_analyse_show_helper(opera_results);
}

auto MainWindow::encrypt_operation_key_validate(
    const QSharedPointer<GpgOperaContextBasement>& contexts) -> bool {
  auto keys = m_key_list_->GetCheckedKeys();

  // symmetric encryption
  if (keys.isEmpty()) {
    contexts->keys = {};
    return true;
  }

  auto checked = check_keys_helper(
      keys, [](const GpgAbstractKeyPtr& key) { return key->IsHasEncrCap(); },
      tr("The selected keypair cannot be used for encryption."));
  if (checked.empty()) {
    contexts->keys = {};
    return false;
  }

  bool canceled = false;
  contexts->keys = resolve_encrypt_recipients_helper(checked, canceled);
  if (canceled) return false;

  return !contexts->keys.empty();
}

auto MainWindow::sign_operation_key_validate(
    const QSharedPointer<GpgOperaContextBasement>& contexts) -> bool {
  auto picker = QSharedPointer<SigningKeysPicker>(
      new SigningKeysPicker(m_key_list_->GetCurrentGpgContextChannel(), this),
      [](SigningKeysPicker* p) { p->deleteLater(); });

  picker->exec();

  if (picker->result() == QDialog::Rejected) return false;

  contexts->singer_keys = picker->GetSigningKeys();
  return !contexts->singer_keys.isEmpty();
}

auto MainWindow::check_read_file_paths_helper(const QStringList& paths)
    -> bool {
  QStringList invalid_files;
  for (const auto& path : paths) {
    auto result = TargetFilePreCheck(path, true);
    if (!std::get<0>(result)) {
      invalid_files.append(path);
    }
  }

  if (!invalid_files.empty()) {
    QString error_file_names;
    for (const auto& file_path : invalid_files) {
      error_file_names += QFileInfo(file_path).fileName() + "\n";
    }

    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from the following files:\n\n%1")
                              .arg(error_file_names.trimmed()));
    return false;
  }

  return true;
}

auto MainWindow::check_write_file_paths_helper(const QStringList& o_paths)
    -> bool {
  for (const auto& o_path : o_paths) {
    if (QFile::exists(o_path)) {
      auto out_file_name =
          tr("The target file \"%1\" already exists.\n\n"
             "It will only be replaced after the operation succeeds.\n"
             "Do you want to continue?")
              .arg(QFileInfo(o_path).fileName());
      auto ret = QMessageBox::warning(this, tr("Warning"), out_file_name,
                                      QMessageBox::Ok | QMessageBox::Cancel);

      if (ret == QMessageBox::Cancel) return false;
    }
  }

  QStringList invalid_output_files;
  for (const auto& path : o_paths) {
    auto result = TargetFilePreCheck(path, false);
    if (!std::get<0>(result)) {
      invalid_output_files.append(path);
    }
  }

  if (!invalid_output_files.empty()) {
    QString error_file_names;
    for (const auto& file_path : invalid_output_files) {
      error_file_names += QFileInfo(file_path).fileName() + "\n";
    }

    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to the following files:\n\n%1")
                              .arg(error_file_names.trimmed()));
    return false;
  }

  return true;
}

auto MainWindow::check_keys_helper(
    const GpgAbstractKeyPtrList& keys,
    const std::function<bool(const GpgAbstractKeyPtr&)>& capability_check,
    const QString& capability_err_string) -> GpgAbstractKeyPtrList {
  if (keys.isEmpty()) {
    QMessageBox::critical(
        this, tr("No Key Checked"),
        tr("Please check the key in the key toolbox on the right."));
    return {};
  }

  assert(std::all_of(keys.begin(), keys.end(),
                     [](const auto& key) { return key->IsGood(); }));

  // check key abilities
  for (const auto& key : keys) {
    if (!capability_check(key)) {
      QMessageBox::critical(nullptr, tr("Invalid KeyPair"),
                            capability_err_string + "<br/><br/>" +
                                tr("For example the Following Key:") +
                                " <br/>" + key->Email());
      return {};
    }
  }

  return keys;
}

auto MainWindow::resolve_encrypt_recipients_helper(
    const GpgAbstractKeyPtrList& keys, bool& canceled)
    -> GpgAbstractKeyPtrList {
  canceled = false;
  if (keys.isEmpty()) return keys;

  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  // Per-recipient encryption subkey selection is an rPGP-engine feature; the
  // GnuPG engine picks a valid, non-revoked encryption subkey internally.
  if (OpenPGPContext::GetInstance(channel).Engine() != OpenPGPEngine::kRPGP) {
    return keys;
  }

  // Only prompt when a recipient exposes more than one usable (non-revoked)
  // encryption subkey — otherwise the auto-selection is unambiguous.
  bool ambiguous = false;
  for (const auto& key : keys) {
    if (key == nullptr || key->KeyType() != GpgAbstractKeyType::kGPG_KEY) {
      continue;
    }
    auto gpg_key = qSharedPointerDynamicCast<GpgKey>(key);
    if (gpg_key == nullptr) continue;

    int encr_subkey_count = 0;
    for (const auto& s_key : gpg_key->SubKeys()) {
      if (s_key.IsHasEncrCap() && !s_key.IsRevoked()) encr_subkey_count++;
    }
    if (encr_subkey_count > 1) {
      ambiguous = true;
      break;
    }
  }

  if (!ambiguous) return keys;

  auto picker = QSharedPointer<EncryptionKeysPicker>(
      new EncryptionKeysPicker(channel, keys, this),
      [](EncryptionKeysPicker* p) { p->deleteLater(); });
  picker->exec();
  if (picker->result() == QDialog::Rejected) {
    canceled = true;
    return {};
  }
  return picker->GetEncryptionKeys();
}

void MainWindow::SlotEncrypt() {
  auto* text_edit = edit_->CurPageTextEdit();
  if (text_edit == nullptr) return;

  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  if (!encrypt_operation_key_validate(contexts.Base())) return;

  auto plain_text = edit_->CurPlainText();
  if (plain_text.isEmpty()) return;

  GFBuffer secure_plain_text(plain_text);

  WipeString(plain_text);

  contexts->GetContextBuffer(0).append(secure_plain_text);

  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncrypt);

  exec_operas_helper(tr("Encrypting"), contexts.Base());
}

void MainWindow::SlotSign() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  auto keys = m_key_list_->GetCheckedKeys();

  GpgAbstractKeyPtrList sign_keys;
  for (const auto& k : keys) {
    if (k->IsHasSignCap()) sign_keys.push_back(k);
  }

  bool ambiguous = false;
  for (const auto& key : sign_keys) {
    if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) continue;
    auto gpg_key = qSharedPointerDynamicCast<GpgKey>(key);
    if (gpg_key == nullptr) continue;
    int sign_subkey_count = 0;
    for (const auto& s_key : gpg_key->SubKeys()) {
      if (s_key.IsHasSignCap()) sign_subkey_count++;
    }
    if (sign_subkey_count > 1) {
      ambiguous = true;
      break;
    }
  }

  if (sign_keys.isEmpty() || ambiguous) {
    auto picker = QSharedPointer<SigningKeysPicker>(
        new SigningKeysPicker(m_key_list_->GetCurrentGpgContextChannel(),
                              sign_keys, this),
        [](SigningKeysPicker* p) { p->deleteLater(); });
    picker->exec();
    if (picker->result() == QDialog::Rejected) return;
    contexts->keys = picker->GetSigningKeys();
  } else {
    contexts->keys = sign_keys;
  }

  if (contexts->keys.isEmpty()) return;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasSign);

  exec_operas_helper(tr("Signing"), contexts.Base());
}

void MainWindow::SlotDecrypt() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  // A module may recognise the text as its own encoding of an OpenPGP
  // message and hand that message back; one that fails to decode it ends the
  // operation. Unclaimed, the text is decrypted as it is, below.
  const auto decoded = exec_decoder_stage_helper(edit_->CurPlainText());
  if (decoded.kind == CodecStageResult::Kind::kFailed) {
    show_codec_failure_helper(tr("Decrypt"), decoded);
    return;
  }
  if (decoded.kind == CodecStageResult::Kind::kHandled) {
    GpgOperaContextHolder contexts;
    contexts->ascii = true;
    contexts->GetContextBuffer(0).append(decoded.output);
    GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
                                GpgOperaHelper::BuildOperasDecrypt);
    exec_decoded_decrypt_helper(tr("Decrypting"), contexts.Base(), decoded);
    return;
  }

  GpgOperaContextHolder contexts;
  contexts->ascii = true;
  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
                              GpgOperaHelper::BuildOperasDecrypt);
  exec_operas_helper(tr("Decrypting"), contexts.Base());
}

void MainWindow::SlotVerify() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasVerify);

  exec_operas_helper(tr("Verifying"), contexts.Base());

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::SlotEncryptSign() {
  auto* text_edit = edit_->CurPageTextEdit();
  if (text_edit == nullptr) return;

  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  auto keys = m_key_list_->GetCheckedKeys();

  auto enc_keys = check_keys_helper(
      keys, [](const GpgAbstractKeyPtr& key) { return key->IsHasEncrCap(); },
      tr("The selected keypair cannot be used for encryption."));
  if (enc_keys.empty()) return;

  bool canceled = false;
  contexts->keys = resolve_encrypt_recipients_helper(enc_keys, canceled);
  if (canceled || contexts->keys.empty()) return;

  if (!sign_operation_key_validate(contexts.Base())) return;

  auto plain_text = edit_->CurPlainText();
  if (plain_text.isEmpty()) return;

  GFBuffer secure_plain_text(plain_text);

  WipeString(plain_text);

  contexts->GetContextBuffer(0).append(secure_plain_text);

  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncryptSign);

  exec_operas_helper(tr("Encrypting and Signing"), contexts.Base());
}

void MainWindow::SlotDecryptVerify() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  const auto decoded = exec_decoder_stage_helper(edit_->CurPlainText());
  if (decoded.kind == CodecStageResult::Kind::kFailed) {
    show_codec_failure_helper(tr("Decrypt Verify"), decoded);
    return;
  }
  if (decoded.kind == CodecStageResult::Kind::kHandled) {
    GpgOperaContextHolder contexts;
    contexts->ascii = true;
    contexts->GetContextBuffer(0).append(decoded.output);
    GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
                                GpgOperaHelper::BuildOperasDecryptVerify);
    exec_decoded_decrypt_helper(tr("Decrypting and Verifying"),
                                contexts.Base(), decoded);
    if (!contexts->unknown_fprs.isEmpty()) {
      slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
    }
    return;
  }

  GpgOperaContextHolder contexts;
  contexts->ascii = true;
  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
                              GpgOperaHelper::BuildOperasDecryptVerify);
  exec_operas_helper(tr("Decrypting and Verifying"), contexts.Base());

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::exec_encoded_encrypt_helper(const QString& encoder,
                                             bool sign) {
  auto* text_edit = edit_->CurPageTextEdit();
  if (text_edit == nullptr) return;

  // Replacing the editor's text with an encoding only means anything when
  // that text IS the document.
  if (!edit_->CurPageIsPlainText()) return;

  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  GpgOperaContextHolder contexts;
  contexts->ascii = false;

  if (sign) {
    // Signing needs a named recipient, so -- unlike the encrypt-only path --
    // there is no symmetric fallback here. Same rule as the standard
    // Encrypt & Sign.
    auto keys = m_key_list_->GetCheckedKeys();
    auto enc_keys = check_keys_helper(
        keys, [](const GpgAbstractKeyPtr& key) { return key->IsHasEncrCap(); },
        tr("The selected keypair cannot be used for encryption."));
    if (enc_keys.empty()) return;

    bool canceled = false;
    contexts->keys = resolve_encrypt_recipients_helper(enc_keys, canceled);
    if (canceled || contexts->keys.empty()) return;

    if (!sign_operation_key_validate(contexts.Base())) return;
  } else {
    // Encrypt to the checked recipient key(s), or symmetrically (passphrase)
    // when none are checked -- same rule as the standard Encrypt.
    if (!encrypt_operation_key_validate(contexts.Base())) return;
  }

  auto plain_text = edit_->CurPlainText();
  if (plain_text.isEmpty()) return;
  GFBuffer secure_plain_text(plain_text);
  WipeString(plain_text);

  contexts->GetContextBuffer(0).append(secure_plain_text);
  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
                              sign ? GpgOperaHelper::BuildOperasEncryptSign
                                   : GpgOperaHelper::BuildOperasEncrypt);
  GpgOperaHelper::WaitForMultipleOperas(
      this, sign ? tr("Encrypting and Signing") : tr("Encrypting"),
      contexts->operas, channel);

  // Releases the reference cycle the operas hold on this basement.
  contexts->operas.clear();

  if (contexts->opera_results.empty()) return;
  for (const auto& result : contexts->opera_results) {
    if (result.op_info.status <= 0) {
      slot_result_analyse_show_helper(contexts->opera_results);
      return;
    }
  }

  const auto& result = contexts->opera_results.first();
  const QString op_name = !result.op_info.operation.isEmpty()
                              ? result.op_info.operation
                              : (sign ? tr("Encrypt Sign") : tr("Encrypt"));

  // The encoder may run a memory-hard derivation: off the GUI thread, behind
  // the deferred waiting dialog.
  auto encoded = SecureCreateSharedObject<CodecStageResult>();
  QContainer<OperaWaitingCb> operas;
  operas.append([encoder, input = result.o_buffer,
                 encoded](const OperaWaitingHd& hd) {
    CodecStage::RunEncoder(encoder, input, [encoded, hd](CodecStageResult r) {
      *encoded = std::move(r);
      hd();
    });
  });
  GpgOperaHelper::WaitForMultipleOperas(this, tr("Encoding"), operas);

  if (encoded->kind != CodecStageResult::Kind::kHandled) {
    show_codec_failure_helper(op_name, *encoded);
    return;
  }

  // What replaces the document is text; an encoder that answers otherwise
  // does not get to put bytes in the editor.
  const auto text = QString::fromUtf8(encoded->output.ConvertToQByteArray());
  if (text.toUtf8() != encoded->output.ConvertToQByteArray()) {
    CodecStageResult not_text = *encoded;
    not_text.kind = CodecStageResult::Kind::kFailed;
    not_text.error = tr("The encoder did not return text.");
    show_codec_failure_helper(op_name, not_text);
    return;
  }
  edit_->SlotFillTextEditWithText(text);

  const auto codec = CodecCards(*encoded);
  QContainer<InfoBoardCard> cards = codec.cards;
  cards.append(convert_op_info_to_cards(result.op_info));

  info_board_->SetInfoBoardCards(
      sign ? tr("Message encrypted, signed and encoded.")
           : tr("Message encrypted and encoded."),
      kINFO_ERROR_OK, cards, op_name, codec.description);
}

auto MainWindow::exec_decoder_stage_helper(const QString& text)
    -> CodecStageResult {
  auto decoded = SecureCreateSharedObject<CodecStageResult>();

  QContainer<OperaWaitingCb> operas;
  operas.append([input = GFBuffer(text), decoded](const OperaWaitingHd& hd) {
    CodecStage::RunDecoders(input, [decoded, hd](CodecStageResult r) {
      *decoded = std::move(r);
      hd();
    });
  });

  // No cancel channel: a decoder's derivation is not interruptible, and the
  // stage bounds each decoder itself. The deferred-show timer usually means
  // no dialog is presented at all.
  GpgOperaHelper::WaitForMultipleOperas(this, tr("Checking Message"), operas);
  return *decoded;
}

void MainWindow::show_codec_failure_helper(const QString& operation,
                                           const CodecStageResult& failed) {
  const auto codec = CodecCards(failed);
  QContainer<InfoBoardCard> cards = codec.cards;
  for (auto& card : cards) card.status = kINFO_ERROR_CRITICAL;

  const auto reason =
      failed.error.isEmpty() ? tr("The message could not be decoded.")
                             : failed.error;
  info_board_->SlotReset();
  info_board_->SetInfoBoardCards(reason, kINFO_ERROR_CRITICAL, cards,
                                 operation, codec.description);
  QMessageBox::warning(this, operation, reason);
}

auto MainWindow::exec_decoded_decrypt_helper(
    const QString& task,
    const QSharedPointer<GpgOperaContextBasement>& contexts,
    const CodecStageResult& decoded) -> bool {
  GpgOperaHelper::WaitForMultipleOperas(
      this, task, contexts->operas, m_key_list_->GetCurrentGpgContextChannel());

  // Releases the reference cycle the operas hold on this basement, the same as
  // the other exec helpers. Without it every decoded message decrypted in
  // this session keeps its plaintext buffer alive until the process exits.
  contexts->operas.clear();

  // Overall status like the standard path: the minimum across results. A
  // status of 0 is a warning (e.g. Decrypt & Verify on an encrypt-only
  // message, which has no signature) -- not an error.
  int overall = contexts->opera_results.empty() ? -1 : 1;
  for (const auto& result : contexts->opera_results) {
    overall = std::min(overall, result.op_info.status);
  }

  // Write the recovered plaintext to the editor (skips failed results).
  slot_gpg_opera_buffer_show_helper(contexts->opera_results);

  InfoBoardStatus status = kINFO_ERROR_OK;
  if (overall < 0) {
    status = kINFO_ERROR_CRITICAL;
  } else if (overall == 0) {
    status = kINFO_ERROR_WARN;
  }

  // The decoder's section first, then the OpenPGP result.
  const auto codec = CodecCards(decoded);
  QContainer<InfoBoardCard> cards = codec.cards;
  if (status == kINFO_ERROR_CRITICAL) {
    for (auto& card : cards) card.status = kINFO_ERROR_CRITICAL;
  }

  QString op_name = tr("Decrypt");
  if (!contexts->opera_results.empty()) {
    const auto& op_info = contexts->opera_results.first().op_info;
    if (!op_info.operation.isEmpty()) op_name = op_info.operation;
    cards.append(convert_op_info_to_cards(op_info));
  }

  QString summary;
  if (overall < 0) {
    summary = tr("Failed to decrypt the decoded message.");
  } else if (overall == 0) {
    summary = tr("Decoded message decrypted (not signed).");
  } else {
    summary = tr("Decoded message decrypted.");
  }

  info_board_->SetInfoBoardCards(summary, status, cards, op_name,
                                 codec.description);

  return overall >= 0;
}

void MainWindow::SlotFileEncrypt(const QStringList& paths, bool ascii) {
  GpgOperaContextHolder contexts;
  contexts->ascii = ascii;

  if (!encrypt_operation_key_validate(contexts.Base())) return;
  if (!check_read_file_paths_helper(paths)) return;

  QStringList final_output_paths;
  QContainer<SafeOutputPath> safe_outputs;

  for (const auto& path : paths) {
    const QFileInfo info(path);

    if (info.isDir()) {
      const auto final_path =
          SetExtensionOfOutputFileForArchive(path, kENCRYPT, contexts->ascii);
      const auto temp_path =
          PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(temp_path);
    } else {
      const auto final_path =
          SetExtensionOfOutputFile(path, kENCRYPT, contexts->ascii);
      const auto temp_path =
          PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(temp_path);
    }
  }

  if (!check_write_file_paths_helper(final_output_paths)) {
    RemoveSafeOutputWorkDirs(safe_outputs);
    return;
  }

  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileEncrypt);

  GpgOperaHelper::BuildOperas(contexts.Base(), 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDirectoryEncrypt);

  exec_file_operas_helper(tr("Encrypting"), contexts.Base(), safe_outputs);
}

void MainWindow::SlotFileDecrypt(const QStringList& paths) {
  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  if (!check_read_file_paths_helper(paths)) return;

  QStringList final_output_paths;
  QContainer<SafeOutputPath> safe_outputs;

  for (const auto& path : paths) {
    const QFileInfo info(path);
    const auto extension = info.completeSuffix();

    if (extension == "tar.gpg" || extension == "tar.asc") {
      const auto final_path =
          SetExtensionOfOutputFileForArchive(path, kDECRYPT, contexts->ascii);
      const auto temp_path =
          PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(temp_path);
    } else {
      const auto final_path =
          SetExtensionOfOutputFile(path, kDECRYPT, contexts->ascii);
      const auto temp_path =
          PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(temp_path);
    }
  }

  if (!check_write_file_paths_helper(final_output_paths)) {
    RemoveSafeOutputWorkDirs(safe_outputs);
    return;
  }

  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileDecrypt);

  GpgOperaHelper::BuildOperas(contexts.Base(), 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasArchiveDecrypt);

  exec_file_operas_helper(tr("Decrypting"), contexts.Base(), safe_outputs);
}

void MainWindow::SlotFileSign(const QStringList& paths, bool ascii) {
  GpgOperaContextHolder contexts;
  contexts->ascii = ascii;

  auto keys = m_key_list_->GetCheckedKeys();

  contexts->keys = check_keys_helper(
      keys, [](const GpgAbstractKeyPtr& key) { return key->IsHasSignCap(); },
      tr("The selected key contains a key that does not actually have a "
         "sign usage."));
  if (contexts->keys.empty()) return;

  if (!check_read_file_paths_helper(paths)) return;

  QStringList final_output_paths;
  QContainer<SafeOutputPath> safe_outputs;

  for (const auto& path : paths) {
    const auto final_path =
        SetExtensionOfOutputFile(path, kSIGN, contexts->ascii);
    const auto temp_path =
        PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

    contexts->GetContextPath(0).append(path);
    contexts->GetContextOutPath(0).append(temp_path);
  }

  if (!check_write_file_paths_helper(final_output_paths)) {
    RemoveSafeOutputWorkDirs(safe_outputs);
    return;
  }

  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileSign);

  exec_file_operas_helper(tr("Signing"), contexts.Base(), safe_outputs);
}

void MainWindow::SlotFileVerify(const QStringList& paths) {
  GpgOperaContextHolder contexts;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);

    QString sign_file_path = path;
    QString data_file_path;

    bool const possible_singleton_target =
        info.suffix() == "gpg" || info.suffix() == "pgp";
    if (possible_singleton_target) {
      swap(data_file_path, sign_file_path);
    } else {
      data_file_path = info.path() + "/" + info.completeBaseName();
    }

    auto data_file_info = QFileInfo(data_file_path);
    if (!possible_singleton_target && !data_file_info.exists()) {
      bool ok;
      QString const text = QInputDialog::getText(
          this, tr("File to be Verified"),
          tr("Please provide An ABSOLUTE Path \n"
             "If Data And Signature is COMBINED within a single file, "
             "KEEP THIS EMPTY: "),
          QLineEdit::Normal, data_file_path, &ok);

      if (!ok) return;

      data_file_path = text.isEmpty() ? data_file_path : text;
      data_file_info = QFileInfo(data_file_path);
    }

    contexts->GetContextPath(0).append(sign_file_path);
    contexts->GetContextOutPath(0).append(data_file_path);
  }

  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileVerify);

  exec_operas_helper(tr("Verifying"), contexts.Base());

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::SlotFileEncryptSign(const QStringList& paths, bool ascii) {
  GpgOperaContextHolder contexts;
  contexts->ascii = ascii;

  auto keys = m_key_list_->GetCheckedKeys();
  auto enc_keys = check_keys_helper(
      keys, [](const GpgAbstractKeyPtr& key) { return key->IsHasEncrCap(); },
      tr("The selected keypair cannot be used for encryption."));
  if (enc_keys.empty()) return;

  bool canceled = false;
  contexts->keys = resolve_encrypt_recipients_helper(enc_keys, canceled);
  if (canceled || contexts->keys.empty()) return;

  if (!sign_operation_key_validate(contexts.Base())) return;
  if (!check_read_file_paths_helper(paths)) return;

  QStringList final_output_paths;
  QContainer<SafeOutputPath> safe_outputs;

  for (const auto& path : paths) {
    const QFileInfo info(path);

    if (info.isDir()) {
      const auto final_path =
          SetExtensionOfOutputFileForArchive(path, kENCRYPT, contexts->ascii);
      const auto temp_path =
          PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(temp_path);
    } else {
      const auto final_path =
          SetExtensionOfOutputFile(path, kENCRYPT, contexts->ascii);
      const auto temp_path =
          PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(temp_path);
    }
  }

  if (!check_write_file_paths_helper(final_output_paths)) {
    RemoveSafeOutputWorkDirs(safe_outputs);
    return;
  }

  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileEncryptSign);

  GpgOperaHelper::BuildOperas(contexts.Base(), 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDirectoryEncryptSign);

  exec_file_operas_helper(tr("Encrypting and Signing"), contexts.Base(),
                          safe_outputs);
}

void MainWindow::SlotFileDecryptVerify(const QStringList& paths) {
  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  if (!check_read_file_paths_helper(paths)) return;

  QStringList final_output_paths;
  QContainer<SafeOutputPath> safe_outputs;

  for (const auto& path : paths) {
    const QFileInfo info(path);
    const auto extension = info.completeSuffix();

    if (extension == "tar.gpg" || extension == "tar.asc") {
      const auto final_path =
          SetExtensionOfOutputFileForArchive(path, kDECRYPT, contexts->ascii);
      const auto temp_path =
          PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(temp_path);
    } else {
      const auto final_path =
          SetExtensionOfOutputFile(path, kDECRYPT, contexts->ascii);
      const auto temp_path =
          PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(temp_path);
    }
  }

  if (!check_write_file_paths_helper(final_output_paths)) {
    RemoveSafeOutputWorkDirs(safe_outputs);
    return;
  }

  GpgOperaHelper::BuildOperas(contexts.Base(), 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileDecryptVerify);

  GpgOperaHelper::BuildOperas(contexts.Base(), 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasArchiveDecryptVerify);

  exec_file_operas_helper(tr("Decrypting and Verifying"), contexts.Base(),
                          safe_outputs);

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

}  // namespace GpgFrontend::UI