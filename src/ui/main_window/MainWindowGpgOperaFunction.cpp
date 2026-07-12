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
#include "core/function/InstantMessageOperator.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SigningKeysPicker.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/InfoBoardCardConverter.h"
#include "ui/struct/GpgOperaResultContext.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

namespace {
auto MakeSafeTempOutputPath(const QString& final_path) -> QString {
  const QFileInfo info(final_path);
  const auto dir = info.absolutePath();
  const auto base_name = info.fileName();

  QString temp_path;
  int counter = 0;

  do {
    temp_path =
        QDir(dir).absoluteFilePath(QString(".%1.gpgfrontend.tmp.%2.%3")
                                       .arg(base_name)
                                       .arg(QCoreApplication::applicationPid())
                                       .arg(counter++));
  } while (QFileInfo::exists(temp_path));

  return temp_path;
}

auto CommitSafeOutputFile(const QString& temp_path, const QString& final_path)
    -> bool {
  if (!QFileInfo::exists(temp_path)) {
    return false;
  }

  const QFileInfo final_info(final_path);
  const auto final_dir = final_info.absolutePath();
  const auto backup_path = QDir(final_dir).absoluteFilePath(
      QString(".%1.gpgfrontend.backup.%2")
          .arg(final_info.fileName())
          .arg(QCoreApplication::applicationPid()));

  bool has_backup = false;

  if (QFileInfo::exists(final_path)) {
    QFile::remove(backup_path);

    if (!QFile::rename(final_path, backup_path)) {
      return false;
    }

    has_backup = true;
  }

  if (!QFile::rename(temp_path, final_path)) {
    if (has_backup) {
      QFile::rename(backup_path, final_path);
    }
    return false;
  }

  if (has_backup) {
    QFile::remove(backup_path);
  }

  return true;
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

  const auto temp_path = MakeSafeTempOutputPath(final_path);

  final_output_paths->append(final_path);
  safe_outputs->push_back({
      final_path,
      temp_path,
  });

  return temp_path;
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

  const bool success = IsFileOperaSuccessful(contexts->opera_results);

  if (!success) {
    LOG_E()
        << "One or more file operations failed. Cleaning up temporary files.";
    CleanupSafeOutputFiles(safe_outputs);
    slot_result_analyse_show_helper(contexts->opera_results);
    return;
  }

  if (!commit_safe_output_files(safe_outputs)) {
    LOG_E() << "Failed to commit output files. Temporary files have been "
               "cleaned up.";
    CleanupSafeOutputFiles(safe_outputs);

    slot_refresh_info_board(
        -1,
        tr("The operation succeeded, but GpgFrontend failed to finalize one "
           "or more output files.\n\n"
           "Temporary output files have been cleaned up. Original files were "
           "kept unchanged."));
    return;
  }

  LOG_D() << "All file operations completed successfully. Output files have "
             "been finalized.";

  slot_result_analyse_show_helper(contexts->opera_results);
}

auto MainWindow::encrypt_operation_key_validate(
    const QSharedPointer<GpgOperaContextBasement>& contexts) -> bool {
  auto keys = m_key_list_->GetCheckedKeys();

  // symmetric encryption
  if (keys.isEmpty()) {
    contexts->keys = {};
    return true;
  }

  contexts->keys = check_keys_helper(
      keys, [](const GpgAbstractKeyPtr& key) { return key->IsHasEncrCap(); },
      tr("The selected keypair cannot be used for encryption."));

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

void MainWindow::SlotEncrypt() {
  auto* text_edit = edit_->CurPageTextEdit();
  if (text_edit == nullptr) return;

  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
  contexts->ascii = true;

  if (!encrypt_operation_key_validate(contexts)) return;

  auto plain_text = edit_->CurPlainText();
  if (plain_text.isEmpty()) return;

  GFBuffer secure_plain_text(plain_text);

  plain_text.fill(QLatin1Char('X'));
  plain_text.clear();

  contexts->GetContextBuffer(0).append(secure_plain_text);

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncrypt);

  exec_operas_helper(tr("Encrypting"), contexts);
}

void MainWindow::SlotSign() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
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
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasSign);

  exec_operas_helper(tr("Signing"), contexts);
}

void MainWindow::SlotDecrypt() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
  contexts->ascii = true;

  GFBuffer input;
  InstantMessageOperator::ImMessageInfo im_info;
  const bool is_im =
      InstantMessageOperator::Detect(edit_->CurPlainText(), input, &im_info);
  if (!is_im) input = GFBuffer(edit_->CurPlainText());

  contexts->GetContextBuffer(0).append(input);
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDecrypt);

  if (is_im) {
    exec_im_decrypt_helper(tr("Decrypting"), contexts, im_info);
  } else {
    exec_operas_helper(tr("Decrypting"), contexts);
  }
}

void MainWindow::SlotVerify() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
  contexts->ascii = true;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasVerify);

  exec_operas_helper(tr("Verifying"), contexts);

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::SlotEncryptSign() {
  auto* text_edit = edit_->CurPageTextEdit();
  if (text_edit == nullptr) return;

  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
  contexts->ascii = true;

  auto keys = m_key_list_->GetCheckedKeys();

  contexts->keys = check_keys_helper(
      keys, [](const GpgAbstractKeyPtr& key) { return key->IsHasEncrCap(); },
      tr("The selected keypair cannot be used for encryption."));
  if (contexts->keys.empty()) return;

  if (!sign_operation_key_validate(contexts)) return;

  auto plain_text = edit_->CurPlainText();
  if (plain_text.isEmpty()) return;

  GFBuffer secure_plain_text(plain_text);

  plain_text.fill(QLatin1Char('X'));
  plain_text.clear();

  contexts->GetContextBuffer(0).append(secure_plain_text);

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncryptSign);

  exec_operas_helper(tr("Encrypting and Signing"), contexts);
}

void MainWindow::SlotDecryptVerify() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
  contexts->ascii = true;

  GFBuffer input;
  InstantMessageOperator::ImMessageInfo im_info;
  const bool is_im =
      InstantMessageOperator::Detect(edit_->CurPlainText(), input, &im_info);
  if (!is_im) input = GFBuffer(edit_->CurPlainText());

  contexts->GetContextBuffer(0).append(input);
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDecryptVerify);

  if (is_im) {
    exec_im_decrypt_helper(tr("Decrypting and Verifying"), contexts, im_info);
  } else {
    exec_operas_helper(tr("Decrypting and Verifying"), contexts);
  }

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::slot_im_encrypt_message() {
  auto* text_edit = edit_->CurPageTextEdit();
  if (text_edit == nullptr) return;

  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
  // Binary output: skip the verbose ASCII armor so the message can be encoded
  // into a compact single-line token below.
  contexts->ascii = false;

  if (!encrypt_operation_key_validate(contexts)) return;

  auto plain_text = edit_->CurPlainText();
  if (plain_text.isEmpty()) return;

  GFBuffer secure_plain_text(plain_text);
  plain_text.fill(QLatin1Char('X'));
  plain_text.clear();

  contexts->GetContextBuffer(0).append(secure_plain_text);

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncrypt);

  // Run the encryption modally, then post-process the ciphertext ourselves
  // (unlike exec_operas_helper, which would write the raw bytes to the editor).
  GpgOperaHelper::WaitForMultipleOperas(
      this, tr("Encrypting"), contexts->operas,
      m_key_list_->GetCurrentGpgContextChannel());

  if (contexts->opera_results.empty()) return;
  for (const auto& result : contexts->opera_results) {
    if (result.op_info.status <= 0) {
      // Encryption failed; surface the standard error UI and stop.
      slot_result_analyse_show_helper(contexts->opera_results);
      return;
    }
  }

  const auto& result = contexts->opera_results.first();
  const auto& binary = result.o_buffer;
  const auto token = InstantMessageOperator::Encode(binary);
  edit_->SlotFillTextEditWithText(token);

  // Instant-messaging section — kept separate from the OpenPGP section and
  // rendered first. Recipient details are left to the OpenPGP section below.
  InfoBoardCard im_card;
  im_card.title = tr("Instant Messaging");
  im_card.status = kINFO_ERROR_OK;
  im_card.fields.append({tr("Encoding"), QStringLiteral("Base58")});
  im_card.fields.append(
      {tr("Version"),
       QString::number(InstantMessageOperator::FormatVersion())});
  im_card.fields.append(
      {tr("Password Book"),
       QString::fromLatin1(InstantMessageOperator::DefaultBookId().toHex())});

  QContainer<InfoBoardCard> cards;
  cards.append(im_card);  // IM section first

  // OpenPGP section — the standard encrypt result cards (including the
  // "Encryption Recipient" section), appended after.
  QString op_name = tr("Encrypt");
  if (!result.op_info.operation.isEmpty()) op_name = result.op_info.operation;
  cards.append(convert_op_info_to_cards(result.op_info));

  info_board_->SetInfoBoardCards(
      tr("Message encrypted for instant messaging."), kINFO_ERROR_OK, cards,
      op_name,
      tr("An Instant Messaging section followed by the OpenPGP result."));
}

auto MainWindow::exec_im_decrypt_helper(
    const QString& task,
    const QSharedPointer<GpgOperaContextBasement>& contexts,
    const InstantMessageOperator::ImMessageInfo& info) -> bool {
  GpgOperaHelper::WaitForMultipleOperas(
      this, task, contexts->operas, m_key_list_->GetCurrentGpgContextChannel());

  bool all_success = !contexts->opera_results.empty();
  for (const auto& result : contexts->opera_results) {
    if (result.op_info.status <= 0) {
      all_success = false;
      break;
    }
  }

  // Write the recovered plaintext to the editor (skips failed results).
  slot_gpg_opera_buffer_show_helper(contexts->opera_results);

  const auto status = all_success ? kINFO_ERROR_OK : kINFO_ERROR_CRITICAL;

  // Instant-messaging section — kept separate from the OpenPGP section and
  // rendered first.
  InfoBoardCard im_card;
  im_card.title = tr("Instant Messaging");
  im_card.status = status;
  im_card.fields.append({tr("Encoding"), QStringLiteral("Base58")});
  im_card.fields.append({tr("Version"), QString::number(info.version)});
  im_card.fields.append(
      {tr("Password Book"), QString::fromLatin1(info.book_id.toHex())});

  QContainer<InfoBoardCard> cards;
  cards.append(im_card);  // IM section first

  // OpenPGP section — the standard decrypt result cards, appended after.
  QString op_name = tr("Decrypt");
  if (!contexts->opera_results.empty()) {
    const auto& op_info = contexts->opera_results.first().op_info;
    if (!op_info.operation.isEmpty()) op_name = op_info.operation;
    cards.append(convert_op_info_to_cards(op_info));
  }

  info_board_->SetInfoBoardCards(
      all_success ? tr("Instant message decrypted.")
                  : tr("Failed to decrypt instant message."),
      status, cards, op_name,
      tr("An Instant Messaging section followed by the OpenPGP result."));

  return all_success;
}

void MainWindow::SlotFileEncrypt(const QStringList& paths, bool ascii) {
  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
  contexts->ascii = ascii;

  if (!encrypt_operation_key_validate(contexts)) return;
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

  if (!check_write_file_paths_helper(final_output_paths)) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileEncrypt);

  GpgOperaHelper::BuildOperas(contexts, 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDirectoryEncrypt);

  exec_file_operas_helper(tr("Encrypting"), contexts, safe_outputs);
}

void MainWindow::SlotFileDecrypt(const QStringList& paths) {
  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
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

  if (!check_write_file_paths_helper(final_output_paths)) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileDecrypt);

  GpgOperaHelper::BuildOperas(contexts, 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasArchiveDecrypt);

  exec_file_operas_helper(tr("Decrypting"), contexts, safe_outputs);
}

void MainWindow::SlotFileSign(const QStringList& paths, bool ascii) {
  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
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

  if (!check_write_file_paths_helper(final_output_paths)) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileSign);

  exec_file_operas_helper(tr("Signing"), contexts, safe_outputs);
}

void MainWindow::SlotFileVerify(const QStringList& paths) {
  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();

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

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileVerify);

  exec_operas_helper(tr("Verifying"), contexts);

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::SlotFileEncryptSign(const QStringList& paths, bool ascii) {
  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
  contexts->ascii = ascii;

  auto keys = m_key_list_->GetCheckedKeys();
  contexts->keys = check_keys_helper(
      keys, [](const GpgAbstractKeyPtr& key) { return key->IsHasEncrCap(); },
      tr("The selected keypair cannot be used for encryption."));
  if (contexts->keys.empty()) return;

  if (!sign_operation_key_validate(contexts)) return;
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

  if (!check_write_file_paths_helper(final_output_paths)) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileEncryptSign);

  GpgOperaHelper::BuildOperas(contexts, 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDirectoryEncryptSign);

  exec_file_operas_helper(tr("Encrypting and Signing"), contexts, safe_outputs);
}

void MainWindow::SlotFileDecryptVerify(const QStringList& paths) {
  auto contexts = SecureCreateSharedObject<GpgOperaContextBasement>();
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

  if (!check_write_file_paths_helper(final_output_paths)) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileDecryptVerify);

  GpgOperaHelper::BuildOperas(contexts, 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasArchiveDecryptVerify);

  exec_file_operas_helper(tr("Decrypting and Verifying"), contexts,
                          safe_outputs);

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

}  // namespace GpgFrontend::UI