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
#include "core/function/SettingCatalog.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "core/utils/MemoryUtils.h"
#include "ui/dialog/KeysPicker.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/InfoBoardCardConverter.h"
#include "ui/main_window/EncryptBlockReason.h"
#include "ui/main_window/FileOperaOutcome.h"
#include "ui/main_window/RecipientConfirm.h"
#include "ui/struct/GpgOperaResultContext.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

namespace {

// Drop the staging directories once their outputs have been committed or
// cleaned up.
void RemoveSafeOutputWorkDirs(const QList<SafeOutputPath>& outputs) {
  for (const auto& output : outputs) {
    RemoveSafeOutputWorkDir(output.temp_path);
  }
}

// Discard every staged output, files and staged directories (an archive
// extraction) alike, so a failed operation leaves no plaintext behind.
auto CleanupSafeOutputFiles(const QList<SafeOutputPath>& outputs)
    -> QList<bool> {
  QList<bool> removed;
  for (const auto& output : outputs) {
    const bool ok = DiscardSafeOutput(output.temp_path);
    if (!ok) LOG_E() << "cannot discard staged output:" << output.temp_path;
    removed.append(ok);
  }
  return removed;
}
auto PrepareSafeOutputPath(const QString& final_path,
                           QStringList* final_output_paths,
                           QList<SafeOutputPath>* safe_outputs) -> QString {
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

auto MainWindow::commit_safe_output_files(const QList<SafeOutputPath>& outputs,
                                          QStringList& committed,
                                          qsizetype* failed_index,
                                          SafeOutputCommitStep* failed_step)
    -> bool {
  for (qsizetype i = 0; i < outputs.size(); ++i) {
    const auto& output = outputs.at(i);
    if (!CommitSafeOutputFile(output.temp_path, output.final_path,
                              failed_step)) {
      if (failed_index != nullptr) *failed_index = i;
      QMessageBox::critical(
          this, tr("Error"),
          tr("Failed to finalize output file:\n\n%1").arg(output.final_path));
      return false;
    }
    committed.append(output.final_path);
  }

  return true;
}
void MainWindow::exec_file_operas_helper(
    const QString& task,
    const QSharedPointer<GpgOperaContextBasement>& contexts,
    const QList<SafeOutputPath>& safe_outputs) {
  // Every operation must report before its staging directory is touched: one
  // without a result may still be writing into it.
  const auto operation_count = contexts->operas.size();

  GpgOperaHelper::WaitForMultipleOperas(
      this, task, contexts->operas, m_key_list_->GetCurrentGpgContextChannel());

  // Releases the reference cycle the operas hold on this basement.
  contexts->operas.clear();

  // A snapshot: an operation that was still in flight when the wait ended can
  // append one more result, and that must not happen while the list below is
  // being iterated. The report is built from this snapshot only and posted
  // once, after cleanup or commit has settled: a result arriving later is not
  // in it, and its object stays "not reported" with nothing claimed about its
  // output.
  auto opera_results = contexts->opera_results;

  const bool success = IsFileOperaSuccessful(opera_results, operation_count);

  // Each input is the output staged for it; that is the batch's identity.
  QHash<QString, QString> input_of_stage;
  for (const auto& category : contexts->categories) {
    for (qsizetype i = 0; i < category.o_paths.size(); ++i) {
      input_of_stage.insert(category.o_paths.at(i), category.paths.value(i));
    }
  }
  QStringList staged_paths;
  QStringList final_paths;
  QStringList input_paths;
  for (const auto& output : safe_outputs) {
    staged_paths.append(output.temp_path);
    final_paths.append(output.final_path);
    input_paths.append(input_of_stage.value(output.temp_path));
  }
  auto batch =
      AssignFileBatchIdentity(opera_results, staged_paths, input_paths);

  if (!success) {
    LOG_E()
        << "One or more file operations failed. Cleaning up temporary files.";
    const auto removed = CleanupSafeOutputFiles(safe_outputs);
    RemoveSafeOutputWorkDirs(safe_outputs);
    RecordDiscardedOutputs(batch, opera_results, staged_paths, removed);
    slot_result_analyse_show_helper(opera_results, batch);
    return;
  }

  QStringList committed;
  qsizetype failed_index = -1;
  auto failed_step = SafeOutputCommitStep::kMOVE_INTO_PLACE;
  if (!commit_safe_output_files(safe_outputs, committed, &failed_index,
                                &failed_step)) {
    LOG_E() << "Failed to commit output files. Temporary files have been "
               "cleaned up, already committed outputs:"
            << committed.size();
    const auto removed = CleanupSafeOutputFiles(safe_outputs);
    RemoveSafeOutputWorkDirs(safe_outputs);

    // The engine results stay as they were; what the report adds is that
    // the outputs from the failed one on were not written.
    RecordCommittedOutputs(batch, final_paths, failed_index, failed_step,
                           staged_paths, removed);
    slot_result_analyse_show_helper(opera_results, batch);
    return;
  }

  RemoveSafeOutputWorkDirs(safe_outputs);

  LOG_D() << "All file operations completed successfully. Output files have "
             "been finalized.";

  RecordCommittedOutputs(batch, final_paths, -1, failed_step, staged_paths, {});
  slot_result_analyse_show_helper(opera_results, batch);
}

auto MainWindow::encrypt_operation_key_validate(
    const QSharedPointer<GpgOperaContextBasement>& contexts) -> bool {
  auto keys = m_key_list_->GetCheckedKeys();

  // symmetric encryption
  if (keys.isEmpty()) {
    contexts->keys = {};
    return true;
  }

  auto checked = check_encrypt_keys_helper(keys);
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

  contexts->singer_keys = picker->GetKeys();
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

auto MainWindow::check_encrypt_keys_helper(const GpgAbstractKeyPtrList& keys)
    -> GpgAbstractKeyPtrList {
  const auto can_encrypt = [](const GpgAbstractKeyPtr& key) {
    return key->IsHasEncrCap();
  };
  const auto generic = tr("The selected keypair cannot be used for encryption.");

  // Name the recipient and, for an expired key, the date: "cannot be used"
  // alone leaves the user guessing which key and why.
  for (const auto& key : keys) {
    if (key == nullptr || can_encrypt(key)) continue;

    const auto reason = DescribeEncryptBlock(key);
    QMessageBox::critical(this, tr("Cannot Encrypt"),
                          reason.isEmpty() ? generic : reason);
    return {};
  }

  return check_keys_helper(keys, can_encrypt, generic);
}

auto MainWindow::confirm_recipients_helper(const GpgAbstractKeyPtrList& keys)
    -> bool {
  const auto setting_on =
      SettingItems::kConfirmMultipleRecipients.Read(GetSettings());
  if (!NeedsRecipientConfirmation(keys.size(), setting_on)) return true;

  const int count = static_cast<int>(keys.size());

  // Only reached with two or more recipients, so the English source can say
  // "recipients" outright; %n still lets other languages pick their form.
  QMessageBox box(this);
  box.setIcon(QMessageBox::Warning);
  box.setWindowTitle(tr("Confirm Recipients"));
  box.setTextFormat(Qt::RichText);
  box.setText(QString("<b>%1</b><br/>%2")
                  .arg(tr("Encrypt to %n recipients?", nullptr, count),
                       tr("Each recipient will be able to see the key IDs of "
                          "all the others.")));
  box.setInformativeText(BuildRecipientConfirmHtml(
      keys, box.palette().color(QPalette::PlaceholderText)));

  auto* encrypt = box.addButton(tr("Encrypt"), QMessageBox::AcceptRole);
  auto* cancel = box.addButton(QMessageBox::Cancel);
  box.setDefaultButton(cancel);
  box.setEscapeButton(cancel);

  box.exec();
  return box.clickedButton() == encrypt;
}

auto MainWindow::confirm_recipient_mix_helper(const GpgAbstractKeyPtrList& keys)
    -> bool {
  if (!RecipientsMixPostQuantumAndClassical(keys)) return true;

  // Allowed, never silently: every recipient can unwrap the same session key,
  // so the message is only as strong as its weakest recipient.
  QMessageBox box(this);
  box.setIcon(QMessageBox::Warning);
  box.setWindowTitle(tr("Mixed Recipient Types"));
  box.setText(
      tr("Some recipients use post-quantum encryption and others do "
         "not."));
  box.setInformativeText(
      tr("The message key is encrypted to every recipient, so anyone able "
         "to break the classical recipients' encryption can read the whole "
         "message: the post-quantum protection does not hold for it. To keep "
         "that protection, encrypt separately for the post-quantum "
         "recipients."));
  auto* encrypt = box.addButton(tr("Encrypt Anyway"), QMessageBox::AcceptRole);
  auto* cancel = box.addButton(QMessageBox::Cancel);
  box.setDefaultButton(cancel);
  box.setEscapeButton(cancel);

  box.exec();
  return box.clickedButton() == encrypt;
}

auto MainWindow::same_key_database_helper(int channel) -> bool {
  if (m_key_list_->GetCurrentGpgContextChannel() == channel) return true;

  QMessageBox::warning(
      this, tr("Key Databases Changed"),
      tr("The key databases changed while this operation was being prepared, "
         "so it was not started. Please check the selected keys and try "
         "again."));
  return false;
}

auto MainWindow::resolve_encrypt_recipients_helper(
    const GpgAbstractKeyPtrList& keys, bool& canceled)
    -> GpgAbstractKeyPtrList {
  canceled = false;
  if (keys.isEmpty()) return keys;

  if (!confirm_recipients_helper(keys)) {
    canceled = true;
    return {};
  }

  // Judged on the final list, after any subkey the user pinned below.
  const auto confirmed_mix = [&](const GpgAbstractKeyPtrList& resolved) {
    if (confirm_recipient_mix_helper(resolved)) return resolved;
    canceled = true;
    return GpgAbstractKeyPtrList{};
  };

  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  // Per-recipient encryption subkey selection is an rPGP-engine feature; the
  // GnuPG engine picks a valid, non-revoked encryption subkey internally.
  if (OpenPGPContext::GetInstance(channel).Engine() != OpenPGPEngine::kRPGP) {
    return confirmed_mix(keys);
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

  if (!ambiguous) return confirmed_mix(keys);

  auto picker = QSharedPointer<EncryptionKeysPicker>(
      new EncryptionKeysPicker(channel, keys, this),
      [](EncryptionKeysPicker* p) { p->deleteLater(); });
  picker->exec();
  if (picker->result() == QDialog::Rejected) {
    canceled = true;
    return {};
  }
  return confirmed_mix(picker->GetKeys());
}

void MainWindow::SlotEncrypt() {
  // The keys come from this database: the operation runs on it, whatever
  // the key list shows by the time any prompt below has been answered.
  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  auto* text_edit = edit_->CurPageTextEdit();
  if (text_edit == nullptr) return;

  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  if (!encrypt_operation_key_validate(contexts.Base())) return;

  // The document's own bytes, line endings included: what Save would write,
  // not a copy rewritten to LF.
  auto plain_bytes = edit_->CurDocumentBytesForOperation();
  if (plain_bytes.isEmpty()) return;

  GFBuffer secure_plain_text(plain_bytes);

  WipeByteArray(plain_bytes);

  contexts->GetContextBuffer(0).append(secure_plain_text);

  if (!same_key_database_helper(channel)) return;

  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
                              GpgOperaHelper::BuildOperasEncrypt);

  exec_operas_helper(tr("Encrypting"), contexts.Base());
}

void MainWindow::SlotSign() {
  // The keys come from this database: the operation runs on it, whatever
  // the key list shows by the time any prompt below has been answered.
  const int channel = m_key_list_->GetCurrentGpgContextChannel();

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
        new SigningKeysPicker(channel, sign_keys, this),
        [](SigningKeysPicker* p) { p->deleteLater(); });
    picker->exec();
    if (picker->result() == QDialog::Rejected) return;
    contexts->keys = picker->GetKeys();
  } else {
    contexts->keys = sign_keys;
  }

  if (contexts->keys.isEmpty()) return;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  if (!same_key_database_helper(channel)) return;

  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
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
  // The keys come from this database: the operation runs on it, whatever
  // the key list shows by the time any prompt below has been answered.
  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  auto* text_edit = edit_->CurPageTextEdit();
  if (text_edit == nullptr) return;

  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  auto keys = m_key_list_->GetCheckedKeys();

  auto enc_keys = check_encrypt_keys_helper(keys);
  if (enc_keys.empty()) return;

  bool canceled = false;
  contexts->keys = resolve_encrypt_recipients_helper(enc_keys, canceled);
  if (canceled || contexts->keys.empty()) return;

  if (!sign_operation_key_validate(contexts.Base())) return;

  // The document's own bytes, line endings included: what Save would write,
  // not a copy rewritten to LF.
  auto plain_bytes = edit_->CurDocumentBytesForOperation();
  if (plain_bytes.isEmpty()) return;

  GFBuffer secure_plain_text(plain_bytes);

  WipeByteArray(plain_bytes);

  contexts->GetContextBuffer(0).append(secure_plain_text);

  if (!same_key_database_helper(channel)) return;

  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
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
    auto enc_keys = check_encrypt_keys_helper(keys);
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

  auto plain_bytes = edit_->CurDocumentBytesForOperation();
  if (plain_bytes.isEmpty()) return;
  GFBuffer secure_plain_text(plain_bytes);
  WipeByteArray(plain_bytes);

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
  QList<OperaWaitingCb> operas;
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
  // The encoded text replaces the document: that is this operation's
  // output, and the report says whether it got there.
  InfoBoardBatchContext context;
  context.document_target = true;
  if (edit_->CurTextPage() == nullptr) {
    context.fates.insert(result.input_index, OutputFate::kAPPLY_FAILED);
    context.output_errors[result.input_index].append(
        {OutputAction::kAPPLY_TO_DOCUMENT,
         {},
         std::nullopt,
         tr("The current tab has no text document to receive the result."),
         OutputErrorOrigin::kAPPLICATION});
  } else {
    edit_->SlotFillTextEditWithText(text);
    context.fates.insert(result.input_index, OutputFate::kAPPLIED);
  }

  // The encoder's section belongs to the same object as the OpenPGP result.
  auto codec_cards = CodecCards(*encoded).cards;
  for (auto& card : codec_cards) card.kind = InfoBoardCardKind::kCODEC;
  context.extra_cards.insert(result.input_index, codec_cards);

  slot_result_analyse_show_helper({result}, context);
}

auto MainWindow::exec_decoder_stage_helper(const QString& text)
    -> CodecStageResult {
  auto decoded = SecureCreateSharedObject<CodecStageResult>();

  QList<OperaWaitingCb> operas;
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
  // The codec's own section, about the document it worked on; the board adds
  // the Outcome card with the reason. Red because the stage failed: styling,
  // not what the report says happened.
  const auto codec = CodecCards(failed);
  QList<InfoBoardCard> cards = codec.cards;
  for (auto& card : cards) {
    card.status = kINFO_ERROR_CRITICAL;
    card.kind = InfoBoardCardKind::kCODEC;
    card.source = CurrentDocumentSource();
  }

  const auto reason =
      failed.error.isEmpty() ? tr("The message could not be decoded.")
                             : failed.error;
  info_board_->SlotReset();
  info_board_->SetInfoBoardCards(reason, kINFO_ERROR_CRITICAL, cards, operation,
                                 reason);
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

  // A snapshot, as everywhere: a late result does not change this report.
  const auto opera_results = contexts->opera_results;

  // Overall status like the standard path: the minimum across results. A
  // status of 0 is a warning (e.g. Decrypt & Verify on an encrypt-only
  // message, which has no signature) -- not an error.
  int overall = opera_results.empty() ? -1 : 1;
  for (const auto& result : opera_results) {
    overall = std::min(overall, result.op_info.status);
  }

  // Write the recovered plaintext to the editor (skips failed results), and
  // record what became of it.
  InfoBoardBatchContext context;
  context.document_target = true;
  slot_gpg_opera_buffer_show_helper(opera_results, &context);

  // The decoder's section first, in the same object as the OpenPGP result.
  auto codec_cards = CodecCards(decoded).cards;
  for (auto& card : codec_cards) {
    card.kind = InfoBoardCardKind::kCODEC;
    if (overall < 0) card.status = kINFO_ERROR_CRITICAL;
  }

  if (opera_results.empty()) {
    for (auto& card : codec_cards) card.source = CurrentDocumentSource();
    info_board_->SlotReset();
    info_board_->SetInfoBoardCards(
        tr("No operation result available."), kINFO_ERROR_CRITICAL, codec_cards,
        GpgOperationName(GpgOperation::kDECRYPT), {});
    return false;
  }

  context.extra_cards.insert(opera_results.first().input_index, codec_cards);
  slot_result_analyse_show_helper(opera_results, context);

  return overall >= 0;
}

auto MainWindow::stage_file_outputs(const QStringList& paths, GpgOperation op,
                                    GpgOperaContextBasement& contexts,
                                    QList<SafeOutputPath>& safe_outputs)
    -> bool {
  QStringList final_output_paths;
  for (const auto& path : paths) {
    // Category 1 is the archive one: a folder to encrypt, or the .tar.*
    // ciphertext one became.
    const QFileInfo info(path);
    const auto extension = info.completeSuffix();
    const bool archive = op == kDECRYPT
                             ? extension == "tar.gpg" || extension == "tar.asc"
                             : info.isDir();
    const auto final_path =
        archive ? SetExtensionOfOutputFileForArchive(path, op, contexts.ascii)
                : SetExtensionOfOutputFile(path, op, contexts.ascii);
    const auto temp_path =
        PrepareSafeOutputPath(final_path, &final_output_paths, &safe_outputs);

    contexts.GetContextPath(archive ? 1 : 0).append(path);
    contexts.GetContextOutPath(archive ? 1 : 0).append(temp_path);
  }

  if (check_write_file_paths_helper(final_output_paths)) return true;
  RemoveSafeOutputWorkDirs(safe_outputs);
  return false;
}

void MainWindow::SlotFileEncrypt(const QStringList& paths, bool ascii) {
  // The keys come from this database: the operation runs on it, whatever
  // the key list shows by the time any prompt below has been answered.
  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  GpgOperaContextHolder contexts;
  contexts->ascii = ascii;

  if (!encrypt_operation_key_validate(contexts.Base())) return;
  if (!check_read_file_paths_helper(paths)) return;

  QList<SafeOutputPath> safe_outputs;
  if (!stage_file_outputs(paths, kENCRYPT, *contexts.Base(), safe_outputs)) {
    return;
  }

  if (!same_key_database_helper(channel)) return;

  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
                              GpgOperaHelper::BuildOperasFileEncrypt);

  GpgOperaHelper::BuildOperas(contexts.Base(), 1, channel,
                              GpgOperaHelper::BuildOperasDirectoryEncrypt);

  exec_file_operas_helper(tr("Encrypting"), contexts.Base(), safe_outputs);
}

void MainWindow::SlotFileDecrypt(const QStringList& paths) {
  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  if (!check_read_file_paths_helper(paths)) return;

  QList<SafeOutputPath> safe_outputs;
  if (!stage_file_outputs(paths, kDECRYPT, *contexts.Base(), safe_outputs)) {
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
  // The keys come from this database: the operation runs on it, whatever
  // the key list shows by the time any prompt below has been answered.
  const int channel = m_key_list_->GetCurrentGpgContextChannel();

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
  QList<SafeOutputPath> safe_outputs;

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

  if (!same_key_database_helper(channel)) return;

  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
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

      if (text.isEmpty()) {
        // Combined: the file holds both the data and its signature, verified
        // like a .gpg/.pgp file, with no separate signature.
        data_file_path = sign_file_path;
        sign_file_path.clear();
      } else {
        data_file_path = text;
      }
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
  // The keys come from this database: the operation runs on it, whatever
  // the key list shows by the time any prompt below has been answered.
  const int channel = m_key_list_->GetCurrentGpgContextChannel();

  GpgOperaContextHolder contexts;
  contexts->ascii = ascii;

  auto keys = m_key_list_->GetCheckedKeys();
  auto enc_keys = check_encrypt_keys_helper(keys);
  if (enc_keys.empty()) return;

  bool canceled = false;
  contexts->keys = resolve_encrypt_recipients_helper(enc_keys, canceled);
  if (canceled || contexts->keys.empty()) return;

  if (!sign_operation_key_validate(contexts.Base())) return;
  if (!check_read_file_paths_helper(paths)) return;

  QList<SafeOutputPath> safe_outputs;
  if (!stage_file_outputs(paths, kENCRYPT, *contexts.Base(), safe_outputs)) {
    return;
  }

  if (!same_key_database_helper(channel)) return;

  GpgOperaHelper::BuildOperas(contexts.Base(), 0, channel,
                              GpgOperaHelper::BuildOperasFileEncryptSign);

  GpgOperaHelper::BuildOperas(contexts.Base(), 1, channel,
                              GpgOperaHelper::BuildOperasDirectoryEncryptSign);

  exec_file_operas_helper(tr("Encrypting and Signing"), contexts.Base(),
                          safe_outputs);
}

void MainWindow::SlotFileDecryptVerify(const QStringList& paths) {
  GpgOperaContextHolder contexts;
  contexts->ascii = true;

  if (!check_read_file_paths_helper(paths)) return;

  QList<SafeOutputPath> safe_outputs;
  if (!stage_file_outputs(paths, kDECRYPT, *contexts.Base(), safe_outputs)) {
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