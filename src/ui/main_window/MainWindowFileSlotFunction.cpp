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

#include <boost/format.hpp>

#include "MainWindow.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgFileOpera.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/thread/Task.h"
#include "core/utils/GpgUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"

namespace GpgFrontend::UI {

void MainWindow::SlotFileEncrypt(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  bool const non_ascii_when_export =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.non_ascii_when_export", true);
  auto out_path =
      SetExtensionOfOutputFile(path, kENCRYPT, !non_ascii_when_export);

  if (QFile::exists(out_path)) {
    auto out_file_name = boost::format(_("The target file %1% already exists, "
                                         "do you need to overwrite it?")) %
                         out_path.filename();
    auto ret =
        QMessageBox::warning(this, _("Warning"), out_file_name.str().c_str(),
                             QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, _("Error"),
                          QString(_("Cannot write to file: %1"))
                              .arg(out_path.filename().c_str()));
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  if (key_ids->empty()) {
    // Symmetric Encrypt
    auto ret = QMessageBox::information(
        this, _("Symmetric Encryption"),
        _("No Key Selected. Do you want to encrypt with a "
          "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return;

    CommonUtils::WaitForOpera(
        this, _("Symmetrically Encrypting"), [=](const OperaWaitingHd& op_hd) {
          GpgFileOpera::GetInstance().EncryptFileSymmetric(
              path, !non_ascii_when_export, out_path,
              [=](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult>()) {
                  throw std::runtime_error("data object doesn't pass checking");
                }

                auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto result_analyse = GpgEncryptResultAnalyse(err, result);
                result_analyse.Analyse();

                process_result_analyse(edit_, info_board_, result_analyse);
                this->slot_refresh_current_file_view();
              });
        });

    return;
  }

  auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, _("Invalid KeyPair"),
          QString(_("The selected keypair cannot be used for encryption.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  CommonUtils::WaitForOpera(
      this, _("Encrypting"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().EncryptFile(
            {p_keys->begin(), p_keys->end()}, path, !non_ascii_when_export,
            out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgEncryptResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
              auto result_analyse = GpgEncryptResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotDirectoryEncrypt(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  bool const non_ascii_when_export =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.non_ascii_when_export", true);
  auto out_path = SetExtensionOfOutputFileForArchive(path, kENCRYPT,
                                                     !non_ascii_when_export);

  if (QFile::exists(out_path)) {
    auto out_file_name = boost::format(_("The target file %1% already exists, "
                                         "do you need to overwrite it?")) %
                         out_path.filename();
    auto ret =
        QMessageBox::warning(this, _("Warning"), out_file_name.str().c_str(),
                             QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, _("Error"),
                          QString(_("Cannot write to file: %1"))
                              .arg(out_path.filename().c_str()));
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  // symmetric encrypt
  if (key_ids->empty()) {
    auto ret = QMessageBox::information(
        this, _("Symmetric Encryption"),
        _("No Key Selected. Do you want to encrypt with a "
          "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return;

    CommonUtils::WaitForOpera(
        this, _("Archiving & Symmetrically Encrypting"),
        [=](const OperaWaitingHd& op_hd) {
          GpgFileOpera::GetInstance().EncryptDerectorySymmetric(
              path, !non_ascii_when_export, out_path,
              [=](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult>()) {
                  throw std::runtime_error("data object doesn't pass checking");
                }

                auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto result_analyse = GpgEncryptResultAnalyse(err, result);
                result_analyse.Analyse();

                process_result_analyse(edit_, info_board_, result_analyse);
                this->slot_refresh_current_file_view();
              });
        });

    return;
  }

  auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, _("Invalid KeyPair"),
          QString(_("The selected keypair cannot be used for encryption.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  CommonUtils::WaitForOpera(
      this, _("Archiving & Encrypting"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().EncryptDirectory(
            {p_keys->begin(), p_keys->end()}, path, !non_ascii_when_export,
            out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgEncryptResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
              auto result_analyse = GpgEncryptResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotFileDecrypt(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  auto out_path = SetExtensionOfOutputFile(path, kDECRYPT, true);
  if (exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, _("Error"),
                          QString(_("Cannot write to file: %1"))
                              .arg(out_path.filename().c_str()));
    return;
  }

  CommonUtils::WaitForOpera(
      this, _("Decrypting"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().DecryptFile(
            path, out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgDecryptResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto result_analyse = GpgDecryptResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotArchiveDecrypt(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  auto out_path = SetExtensionOfOutputFileForArchive(path, kDECRYPT, true);
  if (exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, _("Error"),
                          QString(_("Cannot write to file: %1"))
                              .arg(out_path.filename().c_str()));
    return;
  }

  CommonUtils::WaitForOpera(
      this, _("Decrypting & Extrating"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().DecryptArchive(
            path, out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgDecryptResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto result_analyse = GpgDecryptResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotFileSign(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  auto key_ids = m_key_list_->GetChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (keys->empty()) {
    QMessageBox::critical(
        this, _("No Key Checked"),
        _("Please check the key in the key toolbox on the right."));
    return;
  }

  for (const auto& key : *keys) {
    if (!key.IsHasActualSigningCapability()) {
      QMessageBox::information(
          this, _("Invalid Operation"),
          QString(_("The selected key contains a key that does not actually "
                    "have a sign usage.")) +
              "<br/><br/>" + _("for example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  bool const non_ascii_when_export =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.non_ascii_when_export", true);
  auto sig_file_path =
      SetExtensionOfOutputFile(path, kSIGN, !non_ascii_when_export);

  if (exists(sig_file_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        QString(_("The signature file \"%1\" exists, "
                  "do you need to overwrite it?"))
            .arg(sig_file_path.filename().u8string().c_str()),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, _("Signing"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().SignFile(
            {keys->begin(), keys->end()}, path, !non_ascii_when_export,
            sig_file_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgSignResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgSignResult>(data_obj, 0);
              auto result_analyse = GpgSignResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotFileVerify(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  std::filesystem::path sign_file_path = path;
  std::filesystem::path data_file_path;

  bool const prossible_singleton_target =
      path.extension() == ".gpg" || path.extension() == ".pgp";
  if (prossible_singleton_target) {
    swap(data_file_path, sign_file_path);
  } else {
    data_file_path = sign_file_path.parent_path() / sign_file_path.stem();
  }

  if (!prossible_singleton_target && !std::filesystem::exists(data_file_path)) {
    bool ok;
    QString const text = QInputDialog::getText(
        this, _("File to be Verified"),
        _("Please provide An ABSOLUTE Path \n"
          "If Data And Signature is COMBINED within a single file, "
          "KEEP THIS EMPTY: "),
        QLineEdit::Normal, data_file_path.u8string().c_str(), &ok);

    if (!ok) return;

    data_file_path =
        text.isEmpty() ? path : std::filesystem::path{text.toStdString()};
  }

  if (!is_regular_file(data_file_path) ||
      (!sign_file_path.empty() && !is_regular_file(sign_file_path))) {
    QMessageBox::critical(
        this, _("Error"),
        _("Please select the appropriate origin file or signature file. "
          "Ensure that both are in this directory."));
    return;
  }

  GF_UI_LOG_DEBUG("verification data file path: {}", data_file_path.u8string());
  GF_UI_LOG_DEBUG("verification signature file path: {}",
                  sign_file_path.u8string());

  CommonUtils::WaitForOpera(
      this, _("Verifying"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().VerifyFile(
            data_file_path.u8string(), sign_file_path.u8string(),
            [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgVerifyResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgVerifyResult>(data_obj, 0);
              auto result_analyse = GpgVerifyResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              if (result_analyse.GetStatus() == -2) {
                import_unknown_key_from_keyserver(this, result_analyse);
              }
              if (result_analyse.GetStatus() >= 0) {
                show_verify_details(this, info_board_, err, result);
              }

              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotFileEncryptSign(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (p_keys->empty()) {
    QMessageBox::critical(
        this, _("No Key Checked"),
        _("Please check the key in the key toolbox on the right."));
    return;
  }

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, _("Invalid KeyPair"),
          QString(_("The selected keypair cannot be used for encryption.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  bool const non_ascii_when_export =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.non_ascii_when_export", true);
  auto out_path =
      SetExtensionOfOutputFile(path, kENCRYPT_SIGN, !non_ascii_when_export);

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, _("Error"),
                          QString(_("Cannot write to file: %1"))
                              .arg(out_path.filename().c_str()));
    return;
  }

  if (QFile::exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  auto* signers_picker = new SignersPicker(this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto p_signer_keys = GpgKeyGetter::GetInstance().GetKeys(signer_key_ids);

  CommonUtils::WaitForOpera(
      this, _("Encrypting and Signing"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().EncryptSignFile(
            {p_keys->begin(), p_keys->end()},
            {p_signer_keys->begin(), p_signer_keys->end()}, path,
            !non_ascii_when_export, out_path,
            [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr ||
                  !data_obj->Check<GpgEncryptResult, GpgSignResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }
              auto encrypt_result =
                  ExtractParams<GpgEncryptResult>(data_obj, 0);
              auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);

              auto encrypt_result_analyse =
                  GpgEncryptResultAnalyse(err, encrypt_result);
              encrypt_result_analyse.Analyse();

              auto sign_result_analyse = GpgSignResultAnalyse(err, sign_result);
              sign_result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, encrypt_result_analyse,
                                     sign_result_analyse);

              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotDirectoryEncryptSign(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (p_keys->empty()) {
    QMessageBox::critical(
        this, _("No Key Checked"),
        _("Please check the key in the key toolbox on the right."));
    return;
  }

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, _("Invalid KeyPair"),
          QString(_("The selected keypair cannot be used for encryption.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  bool const non_ascii_when_export =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.non_ascii_when_export", true);
  auto out_path = SetExtensionOfOutputFileForArchive(path, kENCRYPT_SIGN,
                                                     !non_ascii_when_export);

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, _("Error"),
                          QString(_("Cannot write to file: %1"))
                              .arg(out_path.filename().c_str()));
    return;
  }

  if (QFile::exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  auto* signers_picker = new SignersPicker(this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto p_signer_keys = GpgKeyGetter::GetInstance().GetKeys(signer_key_ids);

  CommonUtils::WaitForOpera(
      this, _("Archiving & Encrypting & Signing"),
      [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().EncryptSignDirectory(
            {p_keys->begin(), p_keys->end()},
            {p_signer_keys->begin(), p_signer_keys->end()}, path,
            !non_ascii_when_export, out_path,
            [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr ||
                  !data_obj->Check<GpgEncryptResult, GpgSignResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }
              auto encrypt_result =
                  ExtractParams<GpgEncryptResult>(data_obj, 0);
              auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);

              auto encrypt_result_analyse =
                  GpgEncryptResultAnalyse(err, encrypt_result);
              encrypt_result_analyse.Analyse();

              auto sign_result_analyse = GpgSignResultAnalyse(err, sign_result);
              sign_result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, encrypt_result_analyse,
                                     sign_result_analyse);

              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotFileDecryptVerify(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  auto out_path = SetExtensionOfOutputFile(path, kDECRYPT_VERIFY, true);

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, _("Error"),
                          QString(_("Cannot write to file: %1"))
                              .arg(out_path.filename().c_str()));
    return;
  }

  if (QFile::exists(out_path.u8string().c_str())) {
    auto ret =
        QMessageBox::warning(this, _("Warning"),
                             QString(_("The output file %1 already exists, do "
                                       "you need to overwrite it?"))
                                 .arg(out_path.filename().u8string().c_str()),
                             QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, _("Decrypting and Verifying"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().DecryptVerifyFile(
            path, out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr ||
                  !data_obj->Check<GpgDecryptResult, GpgVerifyResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }
              auto decrypt_result =
                  ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto verify_result = ExtractParams<GpgVerifyResult>(data_obj, 1);

              auto decrypt_result_analyse =
                  GpgDecryptResultAnalyse(err, decrypt_result);
              decrypt_result_analyse.Analyse();

              auto verify_result_analyse =
                  GpgVerifyResultAnalyse(err, verify_result);
              verify_result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, decrypt_result_analyse,
                                     verify_result_analyse);
              if (verify_result_analyse.GetStatus() == -2) {
                import_unknown_key_from_keyserver(this, verify_result_analyse);
              }
              if (verify_result_analyse.GetStatus() >= 0) {
                show_verify_details(this, info_board_, err, verify_result);
              }

              this->slot_refresh_current_file_view();
            });
      });
}

void MainWindow::SlotArchiveDecryptVerify(std::filesystem::path path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(
        this, _("Error"),
        QString(_("Cannot read from file: %1")).arg(path.filename().c_str()));
    return;
  }

  auto out_path =
      SetExtensionOfOutputFileForArchive(path, kDECRYPT_VERIFY, true);

  check_result = TargetFilePreCheck(out_path, false);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, _("Error"),
                          QString(_("Cannot write to file: %1"))
                              .arg(out_path.filename().c_str()));
    return;
  }

  if (QFile::exists(out_path.u8string().c_str())) {
    auto ret =
        QMessageBox::warning(this, _("Warning"),
                             QString(_("The output file %1 already exists, do "
                                       "you need to overwrite it?"))
                                 .arg(out_path.filename().u8string().c_str()),
                             QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, _("Decrypting & Verifying & Extracting"),
      [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::GetInstance().DecryptVerifyArchive(
            path, out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr ||
                  !data_obj->Check<GpgDecryptResult, GpgVerifyResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }
              auto decrypt_result =
                  ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto verify_result = ExtractParams<GpgVerifyResult>(data_obj, 1);

              auto decrypt_result_analyse =
                  GpgDecryptResultAnalyse(err, decrypt_result);
              decrypt_result_analyse.Analyse();

              auto verify_result_analyse =
                  GpgVerifyResultAnalyse(err, verify_result);
              verify_result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, decrypt_result_analyse,
                                     verify_result_analyse);
              if (verify_result_analyse.GetStatus() == -2) {
                import_unknown_key_from_keyserver(this, verify_result_analyse);
              }
              if (verify_result_analyse.GetStatus() >= 0) {
                show_verify_details(this, info_board_, err, verify_result);
              }

              this->slot_refresh_current_file_view();
            });
      });
}

}  // namespace GpgFrontend::UI
