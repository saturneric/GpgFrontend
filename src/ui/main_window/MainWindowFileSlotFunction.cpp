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
#include "core/function/gpg/GpgFileOpera.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

void MainWindow::SlotFileEncrypt(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot read from file: %1").arg(QFileInfo(path).fileName()));
    return;
  }

  bool const non_ascii_at_file_operation =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/non_ascii_at_file_operation", true)
          .toBool();
  auto out_path =
      SetExtensionOfOutputFile(path, kENCRYPT, !non_ascii_at_file_operation);

  if (QFile::exists(out_path)) {
    auto out_file_name = tr("The target file %1 already exists, "
                            "do you need to overwrite it?")
                             .arg(out_path);
    auto ret = QMessageBox::warning(this, tr("Warning"), out_file_name,
                                    QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to file: %1").arg(out_path));
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  if (key_ids->empty()) {
    // Symmetric Encrypt
    auto ret = QMessageBox::information(
        this, tr("Symmetric Encryption"),
        tr("No Key Selected. Do you want to encrypt with a "
           "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return;

    CommonUtils::WaitForOpera(
        this, tr("Symmetrically Encrypting"), [=](const OperaWaitingHd& op_hd) {
          GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
              .EncryptFileSymmetric(
                  path, !non_ascii_at_file_operation, out_path,
                  [=](GpgError err, const DataObjectPtr& data_obj) {
                    // stop waiting
                    op_hd();

                    if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                        data_obj == nullptr ||
                        !data_obj->Check<GpgEncryptResult>()) {
                      QMessageBox::critical(this, tr("Error"),
                                            tr("Unknown error occurred"));
                      return;
                    }

                    auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                    auto result_analyse = GpgEncryptResultAnalyse(
                        m_key_list_->GetCurrentGpgContextChannel(), err,
                        result);
                    result_analyse.Analyse();

                    slot_result_analyse_show_helper(result_analyse);
                    this->slot_refresh_current_file_view();
                  });
        });

    return;
  }

  auto p_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  assert(std::all_of(p_keys->begin(), p_keys->end(),
                     [](const auto& key) { return key.IsGood(); }));

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, tr("Invalid KeyPair"),
          tr("The selected keypair cannot be used for encryption.") +
              "<br/><br/>" + tr("For example the Following Key:") + " <br/>" +
              key.GetUIDs()->front().GetUID());
      return;
    }
  }

  CommonUtils::WaitForOpera(
      this, tr("Encrypting"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .EncryptFile(
                {p_keys->begin(), p_keys->end()}, path,
                !non_ascii_at_file_operation, out_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgEncryptResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                  auto result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err, result);
                  result_analyse.Analyse();

                  slot_result_analyse_show_helper(result_analyse);
                  this->slot_refresh_current_file_view();
                });
      });
}

void MainWindow::SlotDirectoryEncrypt(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot read from file: %1").arg(QFileInfo(path).fileName()));
    return;
  }

  bool const non_ascii_at_file_operation =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/non_ascii_at_file_operation", true)
          .toBool();
  auto out_path = SetExtensionOfOutputFileForArchive(
      path, kENCRYPT, !non_ascii_at_file_operation);

  if (QFile::exists(out_path)) {
    auto out_file_name = tr("The target file %1 already exists, "
                            "do you need to overwrite it?")
                             .arg(out_path);
    auto ret = QMessageBox::warning(this, tr("Warning"), out_file_name,
                                    QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to file: %1").arg(out_path));
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  // symmetric encrypt
  if (key_ids->empty()) {
    auto ret = QMessageBox::information(
        this, tr("Symmetric Encryption"),
        tr("No Key Selected. Do you want to encrypt with a "
           "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return;

    CommonUtils::WaitForOpera(
        this, tr("Archiving & Symmetrically Encrypting"),
        [=](const OperaWaitingHd& op_hd) {
          GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
              .EncryptDerectorySymmetric(
                  path, !non_ascii_at_file_operation, out_path,
                  [=](GpgError err, const DataObjectPtr& data_obj) {
                    // stop waiting
                    op_hd();

                    if (data_obj == nullptr ||
                        !data_obj->Check<GpgEncryptResult>()) {
                      QMessageBox::critical(this, tr("Error"),
                                            tr("Unknown error occurred"));
                      return;
                    }

                    auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                    auto result_analyse = GpgEncryptResultAnalyse(
                        m_key_list_->GetCurrentGpgContextChannel(), err,
                        result);
                    result_analyse.Analyse();

                    slot_result_analyse_show_helper(result_analyse);
                    this->slot_refresh_current_file_view();
                  });
        });

    return;
  }

  auto p_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  assert(std::all_of(p_keys->begin(), p_keys->end(),
                     [](const auto& key) { return key.IsGood(); }));

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, tr("Invalid KeyPair"),
          tr("The selected keypair cannot be used for encryption.") +
              "<br/><br/>" + tr("For example the Following Key:") + " <br/>" +
              key.GetUIDs()->front().GetUID());
      return;
    }
  }

  CommonUtils::WaitForOpera(
      this, tr("Archiving & Encrypting"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .EncryptDirectory(
                {p_keys->begin(), p_keys->end()}, path,
                !non_ascii_at_file_operation, out_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgEncryptResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                  auto result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err, result);
                  result_analyse.Analyse();

                  slot_result_analyse_show_helper(result_analyse);
                  this->slot_refresh_current_file_view();
                });
      });
}

void MainWindow::SlotFileDecrypt(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot read from file: %1").arg(QFileInfo(path).fileName()));
    return;
  }

  auto out_path = SetExtensionOfOutputFile(path, kDECRYPT, true);
  if (QFileInfo(out_path).exists()) {
    auto ret = QMessageBox::warning(
        this, tr("Warning"),
        tr("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to file: %1").arg(out_path));
    return;
  }

  CommonUtils::WaitForOpera(
      this, tr("Decrypting"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .DecryptFile(
                path, out_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgDecryptResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto result = ExtractParams<GpgDecryptResult>(data_obj, 0);
                  auto result_analyse = GpgDecryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err, result);
                  result_analyse.Analyse();

                  slot_result_analyse_show_helper(result_analyse);
                  this->slot_refresh_current_file_view();
                });
      });
}

void MainWindow::SlotArchiveDecrypt(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from file: %1").arg(path));
    return;
  }

  auto out_path = SetExtensionOfOutputFileForArchive(path, kDECRYPT, true);
  if (QFileInfo(out_path).exists()) {
    auto ret = QMessageBox::warning(
        this, tr("Warning"),
        tr("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to file: %1").arg(out_path));
    return;
  }

  CommonUtils::WaitForOpera(
      this, tr("Decrypting & Extrating"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .DecryptArchive(
                path, out_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgDecryptResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto result = ExtractParams<GpgDecryptResult>(data_obj, 0);
                  auto result_analyse = GpgDecryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err, result);
                  result_analyse.Analyse();

                  slot_result_analyse_show_helper(result_analyse);
                  this->slot_refresh_current_file_view();
                });
      });
}

void MainWindow::SlotFileSign(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot read from file: %1").arg(QFileInfo(path).fileName()));
    return;
  }

  auto key_ids = m_key_list_->GetChecked();
  auto keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  assert(std::all_of(keys->begin(), keys->end(),
                     [](const auto& key) { return key.IsGood(); }));

  if (keys->empty()) {
    QMessageBox::critical(
        this, tr("No Key Checked"),
        tr("Please check the key in the key toolbox on the right."));
    return;
  }

  for (const auto& key : *keys) {
    if (!key.IsHasActualSigningCapability()) {
      QMessageBox::information(
          this, tr("Invalid Operation"),
          tr("The selected key contains a key that does not actually "
             "have a sign usage.") +
              "<br/><br/>" + tr("for example the Following Key:") + " <br/>" +
              key.GetUIDs()->front().GetUID());
      return;
    }
  }

  bool const non_ascii_at_file_operation =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/non_ascii_at_file_operation", true)
          .toBool();
  auto sig_file_path =
      SetExtensionOfOutputFile(path, kSIGN, !non_ascii_at_file_operation);

  if (QFileInfo(sig_file_path).exists()) {
    auto ret = QMessageBox::warning(this, tr("Warning"),
                                    tr("The signature file \"%1\" exists, "
                                       "do you need to overwrite it?")
                                        .arg(sig_file_path),
                                    QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, tr("Signing"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .SignFile({keys->begin(), keys->end()}, path,
                      !non_ascii_at_file_operation, sig_file_path,
                      [=](GpgError err, const DataObjectPtr& data_obj) {
                        // stop waiting
                        op_hd();

                        if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                            data_obj == nullptr ||
                            !data_obj->Check<GpgSignResult>()) {
                          QMessageBox::critical(this, tr("Error"),
                                                tr("Unknown error occurred"));
                          return;
                        }

                        auto result = ExtractParams<GpgSignResult>(data_obj, 0);
                        auto result_analyse = GpgSignResultAnalyse(
                            m_key_list_->GetCurrentGpgContextChannel(), err,
                            result);
                        result_analyse.Analyse();

                        slot_result_analyse_show_helper(result_analyse);
                        this->slot_refresh_current_file_view();
                      });
      });
}

void MainWindow::SlotFileVerify(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot read from file: %1").arg(QFileInfo(path).fileName()));
    return;
  }

  auto file_info = QFileInfo(path);
  QString sign_file_path = path;
  QString data_file_path;

  bool const prossible_singleton_target =
      file_info.suffix() == "gpg" || file_info.suffix() == "pgp";
  if (prossible_singleton_target) {
    swap(data_file_path, sign_file_path);
  } else {
    data_file_path = file_info.path() + "/" + file_info.completeBaseName();
  }

  auto data_file_info = QFileInfo(data_file_path);
  if (!prossible_singleton_target && !data_file_info.exists()) {
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

  if (!data_file_info.isFile() ||
      (!sign_file_path.isEmpty() && !QFileInfo(sign_file_path).isFile())) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Please select the appropriate origin file or signature file. "
           "Ensure that both are in this directory."));
    return;
  }

  CommonUtils::WaitForOpera(
      this, tr("Verifying"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .VerifyFile(
                data_file_path, sign_file_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgVerifyResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto result = ExtractParams<GpgVerifyResult>(data_obj, 0);
                  auto result_analyse = GpgVerifyResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err, result);
                  result_analyse.Analyse();

                  slot_result_analyse_show_helper(result_analyse);

                  if (!result_analyse.GetUnknownSignatures().isEmpty() &&
                      Module::IsModuleActivate(kKeyServerSyncModuleID)) {
                    slot_verifying_unknown_signature_helper(result_analyse);
                  }

                  this->slot_refresh_current_file_view();
                });
      });
}

void MainWindow::SlotFileEncryptSign(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from file: %1").arg(path));
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  auto p_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  assert(std::all_of(p_keys->begin(), p_keys->end(),
                     [](const auto& key) { return key.IsGood(); }));

  if (p_keys->empty()) {
    QMessageBox::critical(
        this, tr("No Key Checked"),
        tr("Please check the key in the key toolbox on the right."));
    return;
  }

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, tr("Invalid KeyPair"),
          tr("The selected keypair cannot be used for encryption.") +
              "<br/><br/>" + tr("For example the Following Key:") + " <br/>" +
              key.GetUIDs()->front().GetUID());
      return;
    }
  }

  bool const non_ascii_at_file_operation =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/non_ascii_at_file_operation", true)
          .toBool();
  auto out_path = SetExtensionOfOutputFile(path, kENCRYPT_SIGN,
                                           !non_ascii_at_file_operation);

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to file: %1").arg(out_path));
    return;
  }

  if (QFile::exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, tr("Warning"),
        tr("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  auto* signers_picker =
      new SignersPicker(m_key_list_->GetCurrentGpgContextChannel(), this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto p_signer_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(signer_key_ids);
  assert(std::all_of(p_signer_keys->begin(), p_signer_keys->end(),
                     [](const auto& key) { return key.IsGood(); }));

  CommonUtils::WaitForOpera(
      this, tr("Encrypting and Signing"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .EncryptSignFile(
                {p_keys->begin(), p_keys->end()},
                {p_signer_keys->begin(), p_signer_keys->end()}, path,
                !non_ascii_at_file_operation, out_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgEncryptResult, GpgSignResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }
                  auto encrypt_result =
                      ExtractParams<GpgEncryptResult>(data_obj, 0);
                  auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);

                  auto encrypt_result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      encrypt_result);
                  encrypt_result_analyse.Analyse();

                  auto sign_result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      sign_result);
                  sign_result_analyse.Analyse();

                  slot_result_analyse_show_helper(encrypt_result_analyse,
                                                  sign_result_analyse);

                  this->slot_refresh_current_file_view();
                });
      });
}

void MainWindow::SlotDirectoryEncryptSign(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from file: %1").arg(path));
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  auto p_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  assert(std::all_of(p_keys->begin(), p_keys->end(),
                     [](const auto& key) { return key.IsGood(); }));

  if (p_keys->empty()) {
    QMessageBox::critical(
        this, tr("No Key Checked"),
        tr("Please check the key in the key toolbox on the right."));
    return;
  }

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, tr("Invalid KeyPair"),
          tr("The selected keypair cannot be used for encryption.") +
              "<br/><br/>" + tr("For example the Following Key:") + " <br/>" +
              key.GetUIDs()->front().GetUID());
      return;
    }
  }

  bool const non_ascii_at_file_operation =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/non_ascii_at_file_operation", true)
          .toBool();
  auto out_path = SetExtensionOfOutputFileForArchive(
      path, kENCRYPT_SIGN, !non_ascii_at_file_operation);

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to file: %1").arg(out_path));
    return;
  }

  if (QFile::exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, tr("Warning"),
        tr("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  auto* signers_picker =
      new SignersPicker(m_key_list_->GetCurrentGpgContextChannel(), this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto p_signer_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(signer_key_ids);
#ifndef NDEBUG
  for (const auto& key : *p_signer_keys) {
    assert(key.IsGood());
  }
#endif

  CommonUtils::WaitForOpera(
      this, tr("Archiving & Encrypting & Signing"),
      [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .EncryptSignDirectory(
                {p_keys->begin(), p_keys->end()},
                {p_signer_keys->begin(), p_signer_keys->end()}, path,
                !non_ascii_at_file_operation, out_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgEncryptResult, GpgSignResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }
                  auto encrypt_result =
                      ExtractParams<GpgEncryptResult>(data_obj, 0);
                  auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);

                  auto encrypt_result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      encrypt_result);
                  encrypt_result_analyse.Analyse();

                  auto sign_result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      sign_result);
                  sign_result_analyse.Analyse();

                  slot_result_analyse_show_helper(encrypt_result_analyse,
                                                  sign_result_analyse);

                  this->slot_refresh_current_file_view();
                });
      });
}

void MainWindow::SlotFileDecryptVerify(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from file: %1").arg(path));
    return;
  }

  auto out_path = SetExtensionOfOutputFile(path, kDECRYPT_VERIFY, true);
  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to file: %1").arg(out_path));
    return;
  }

  if (QFile::exists(out_path)) {
    auto ret = QMessageBox::warning(this, tr("Warning"),
                                    tr("The output file %1 already exists, do "
                                       "you need to overwrite it?")
                                        .arg(out_path),
                                    QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, tr("Decrypting and Verifying"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .DecryptVerifyFile(
                path, out_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgDecryptResult, GpgVerifyResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }
                  auto decrypt_result =
                      ExtractParams<GpgDecryptResult>(data_obj, 0);
                  auto verify_result =
                      ExtractParams<GpgVerifyResult>(data_obj, 1);

                  auto decrypt_result_analyse = GpgDecryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      decrypt_result);
                  decrypt_result_analyse.Analyse();

                  auto verify_result_analyse = GpgVerifyResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      verify_result);
                  verify_result_analyse.Analyse();

                  slot_result_analyse_show_helper(decrypt_result_analyse,
                                                  verify_result_analyse);

                  this->slot_refresh_current_file_view();

                  if (!verify_result_analyse.GetUnknownSignatures().isEmpty() &&
                      Module::IsModuleActivate(kKeyServerSyncModuleID)) {
                    LOG_D() << "try to sync missing key info from server"
                            << verify_result_analyse.GetUnknownSignatures();

                    QString fingerprint_list;
                    for (const auto& fingerprint :
                         verify_result_analyse.GetUnknownSignatures()) {
                      fingerprint_list += fingerprint + "\n";
                    }

                    // Interaction with user
                    auto user_response = QMessageBox::question(
                        this, tr("Missing Keys"),
                        tr("Some signatures cannot be verified because the "
                           "corresponding keys are missing.\n\n"
                           "The following fingerprints are missing:\n%1\n\n"
                           "Would you like to fetch these keys from the key "
                           "server?")
                            .arg(fingerprint_list),
                        QMessageBox::Yes | QMessageBox::No);

                    if (user_response == QMessageBox::Yes) {
                      CommonUtils::GetInstance()
                          ->ImportKeyByKeyServerSyncModule(
                              this, m_key_list_->GetCurrentGpgContextChannel(),
                              verify_result_analyse.GetUnknownSignatures());
                    } else {
                      QMessageBox::information(
                          this, tr("Verification Incomplete"),
                          tr("Verification was incomplete due to missing "
                             "keys. You can manually import the keys later."));
                    }
                  }
                });
      });
}

void MainWindow::SlotArchiveDecryptVerify(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from file: %1").arg(path));
    return;
  }

  auto out_path =
      SetExtensionOfOutputFileForArchive(path, kDECRYPT_VERIFY, true);
  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to file: %1").arg(out_path));
    return;
  }

  if (QFile::exists(out_path)) {
    auto ret = QMessageBox::warning(this, tr("Warning"),
                                    tr("The output file %1 already exists, do "
                                       "you need to overwrite it?")
                                        .arg(out_path),
                                    QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, tr("Decrypting & Verifying & Extracting"),
      [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
            .DecryptVerifyArchive(
                path, out_path,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgDecryptResult, GpgVerifyResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }
                  auto decrypt_result =
                      ExtractParams<GpgDecryptResult>(data_obj, 0);
                  auto verify_result =
                      ExtractParams<GpgVerifyResult>(data_obj, 1);

                  auto decrypt_result_analyse = GpgDecryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      decrypt_result);
                  decrypt_result_analyse.Analyse();

                  auto verify_result_analyse = GpgVerifyResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      verify_result);
                  verify_result_analyse.Analyse();

                  slot_result_analyse_show_helper(decrypt_result_analyse,
                                                  verify_result_analyse);

                  // pause this feature
                  // if (verify_result_analyse.GetStatus() == -2) {
                  //   import_unknown_key_from_keyserver(this,
                  //   verify_result_analyse);
                  // }
                  // pause this feature
                  // if (verify_result_analyse.GetStatus() >= 0) {
                  //   show_verify_details(this, info_board_, err,
                  //   verify_result);
                  // }

                  this->slot_refresh_current_file_view();
                });
      });
}

void MainWindow::SlotFileVerifyEML(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from file: %1").arg(path));
    return;
  }

  QFileInfo file_info(path);
  if (file_info.size() > static_cast<qint64>(1024 * 1024 * 32)) {
    QMessageBox::warning(
        this, tr("EML File Too Large"),
        tr("The EML file \"%1\" is larger than 32MB and will not be opened.")
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
