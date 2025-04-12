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
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/struct/GpgOperaResultContext.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

auto MainWindow::encrypt_operation_key_validate(
    const QSharedPointer<GpgOperaContextBasement>& contexts) -> bool {
  auto key_ids = m_key_list_->GetChecked();

  // symmetric encryption
  if (key_ids.isEmpty()) {
    auto ret = QMessageBox::information(
        this, tr("Symmetric Encryption"),
        tr("No Key Selected. Do you want to encrypt with a "
           "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return false;

    contexts->keys = {};
  } else {
    contexts->keys = check_keys_helper(
        key_ids, [](const GpgKey& key) { return key.IsHasActualEncrCap(); },
        tr("The selected keypair cannot be used for encryption."));
    if (contexts->keys.empty()) return false;
  }

  return true;
}

auto MainWindow::sign_operation_key_validate(
    const QSharedPointer<GpgOperaContextBasement>& contexts) -> bool {
  auto* signers_picker =
      new SignersPicker(m_key_list_->GetCurrentGpgContextChannel(), this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return false;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto signer_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(signer_key_ids);
  assert(std::all_of(signer_keys.begin(), signer_keys.end(),
                     [](const auto& key) { return key.IsGood(); }));

  contexts->singer_keys = signer_keys;

  return true;
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
      auto out_file_name = tr("The target file %1 already exists, "
                              "do you need to overwrite it?")
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
    const KeyIdArgsList& key_ids,
    const std::function<bool(const GpgKey&)>& capability_check,
    const QString& capability_err_string) -> GpgKeyList {
  if (key_ids.empty()) {
    QMessageBox::critical(
        this, tr("No Key Checked"),
        tr("Please check the key in the key toolbox on the right."));
    return {};
  }

  auto keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  assert(std::all_of(keys.begin(), keys.end(),
                     [](const auto& key) { return key.IsGood(); }));

  // check key abilities
  for (const auto& key : keys) {
    if (!capability_check(key)) {
      QMessageBox::critical(nullptr, tr("Invalid KeyPair"),
                            capability_err_string + "<br/><br/>" +
                                tr("For example the Following Key:") +
                                " <br/>" + key.UIDs().front().GetUID());
      return {};
    }
  }

  return keys;
}

void MainWindow::SlotEncrypt() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  if (!encrypt_operation_key_validate(contexts)) return;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncrypt);

  exec_operas_helper(tr("Encrypting"), contexts);
}

void MainWindow::SlotSign() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  auto key_ids = m_key_list_->GetChecked();
  contexts->keys = check_keys_helper(
      key_ids, [](const GpgKey& key) { return key.IsHasActualSignCap(); },
      tr("The selected key contains a key that does not actually have a "
         "sign usage."));
  if (contexts->keys.empty()) return;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasSign);

  exec_operas_helper(tr("Signing"), contexts);
}

void MainWindow::SlotDecrypt() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDecrypt);

  exec_operas_helper(tr("Decrypting"), contexts);
}

void MainWindow::SlotVerify() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
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
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  auto key_ids = m_key_list_->GetChecked();
  contexts->keys = check_keys_helper(
      key_ids, [](const GpgKey& key) { return key.IsHasActualEncrCap(); },
      tr("The selected keypair cannot be used for encryption."));
  if (contexts->keys.empty()) return;

  if (!sign_operation_key_validate(contexts)) return;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncryptSign);

  exec_operas_helper(tr("Encrypting and Signing"), contexts);
}

void MainWindow::SlotDecryptVerify() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDecryptVerify);

  exec_operas_helper(tr("Decrypting and Verifying"), contexts);

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::SlotFileEncrypt(const QStringList& paths, bool ascii) {
  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = ascii;

  if (!encrypt_operation_key_validate(contexts)) return;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    if (info.isDir()) {
      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(
          SetExtensionOfOutputFileForArchive(path, kENCRYPT, contexts->ascii));
    } else {
      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(
          SetExtensionOfOutputFile(path, kENCRYPT, contexts->ascii));
    }
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileEncrypt);

  GpgOperaHelper::BuildOperas(contexts, 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDirectoryEncrypt);

  exec_operas_helper(tr("Encrypting"), contexts);
}

void MainWindow::SlotFileDecrypt(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();

  contexts->ascii = true;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    const auto extension = info.completeSuffix();

    if (extension == "tar.gpg" || extension == "tar.asc") {
      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(
          SetExtensionOfOutputFileForArchive(path, kDECRYPT, contexts->ascii));
    } else {
      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(
          SetExtensionOfOutputFile(path, kDECRYPT, contexts->ascii));
    }
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileDecrypt);

  GpgOperaHelper::BuildOperas(contexts, 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasArchiveDecrypt);

  exec_operas_helper(tr("Decrypting"), contexts);
}

void MainWindow::SlotFileSign(const QStringList& paths, bool ascii) {
  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();

  contexts->ascii = ascii;

  auto key_ids = m_key_list_->GetChecked();
  contexts->keys = check_keys_helper(
      key_ids, [](const GpgKey& key) { return key.IsHasActualSignCap(); },
      tr("The selected key contains a key that does not actually have a "
         "sign usage."));
  if (contexts->keys.empty()) return;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    contexts->GetContextPath(0).append(path);
    contexts->GetContextOutPath(0).append(
        SetExtensionOfOutputFile(path, kSIGN, contexts->ascii));
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileSign);

  exec_operas_helper(tr("Signing"), contexts);
}

void MainWindow::SlotFileVerify(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();

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
  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = ascii;

  auto key_ids = m_key_list_->GetChecked();
  contexts->keys = check_keys_helper(
      key_ids, [](const GpgKey& key) { return key.IsHasActualEncrCap(); },
      tr("The selected keypair cannot be used for encryption."));
  if (contexts->keys.empty()) return;

  if (!sign_operation_key_validate(contexts)) return;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    if (info.isDir()) {
      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(
          SetExtensionOfOutputFileForArchive(path, kENCRYPT, contexts->ascii));
    } else {
      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(
          SetExtensionOfOutputFile(path, kENCRYPT, contexts->ascii));
    }
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileEncryptSign);

  GpgOperaHelper::BuildOperas(contexts, 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDirectoryEncryptSign);

  exec_operas_helper(tr("Encrypting and Signing"), contexts);
}

void MainWindow::SlotFileDecryptVerify(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    const auto extension = info.completeSuffix();

    if (extension == "tar.gpg" || extension == "tar.asc") {
      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(
          SetExtensionOfOutputFileForArchive(path, kDECRYPT, contexts->ascii));
    } else {
      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(
          SetExtensionOfOutputFile(path, kDECRYPT, contexts->ascii));
    }
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasFileDecryptVerify);

  GpgOperaHelper::BuildOperas(contexts, 1,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasArchiveDecryptVerify);

  exec_operas_helper(tr("Decrypting and Verifying"), contexts);

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
};

void MainWindow::SlotFileVerifyEML(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from file: %1").arg(path));
    return;
  }

  QFileInfo file_info(path);
  if (file_info.size() > static_cast<qint64>(1024 * 1024 * 32)) {
    QMessageBox::warning(this, tr("EML File Too Large"),
                         tr("The EML file \"%1\" is larger than 32MB and "
                            "will not be opened.")
                             .arg(file_info.fileName()));
    return;
  }

  QFile eml_file(path);
  if (!eml_file.open(QIODevice::ReadOnly)) return;
  auto buffer = eml_file.readAll();

  // LOG_D() << "EML BUFFER (FILE): " << buffer;

  slot_verify_email_by_eml_data(buffer);
}

}  // namespace GpgFrontend::UI