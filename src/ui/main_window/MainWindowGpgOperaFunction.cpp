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
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/DataObject.h"
#include "core/model/GpgEncryptResult.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

void MainWindow::SlotEncrypt() {
  if (edit_->SlotCurPageTextEdit() == nullptr) return;

  auto key_ids = m_key_list_->GetChecked();

  if (key_ids->empty()) {
    // Symmetric Encrypt
    auto ret = QMessageBox::information(
        this, tr("Symmetric Encryption"),
        tr("No Key Checked. Do you want to encrypt with a "
           "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;

    auto buffer = GFBuffer(edit_->CurPlainText());
    CommonUtils::WaitForOpera(
        this, tr("Symmetrically Encrypting"),
        [this, buffer](const OperaWaitingHd& op_hd) {
          GpgFrontend::GpgBasicOperator::GetInstance(
              m_key_list_->GetCurrentGpgContextChannel())
              .EncryptSymmetric(
                  buffer, true,
                  [this, op_hd](GpgError err, const DataObjectPtr& data_obj) {
                    // stop waiting
                    op_hd();

                    if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                        data_obj == nullptr ||
                        !data_obj->Check<GpgEncryptResult, GFBuffer>()) {
                      QMessageBox::critical(this, tr("Error"),
                                            tr("Unknown error occurred"));
                      return;
                    }

                    auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                    auto buffer = ExtractParams<GFBuffer>(data_obj, 1);

                    auto result_analyse = GpgEncryptResultAnalyse(
                        m_key_list_->GetCurrentGpgContextChannel(), err,
                        result);
                    result_analyse.Analyse();
                    slot_result_analyse_show_helper(result_analyse);

                    if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                      edit_->SlotFillTextEditWithText(
                          buffer.ConvertToQByteArray());
                    }
                    info_board_->ResetOptionActionsMenu();
                  });
        });

    return;
  }

  auto keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  for (const auto& key : *keys) {
    assert(key.IsGood());
  }

  for (const auto& key : *keys) {
    if (!key.IsHasActualEncryptionCapability()) {
      QMessageBox::information(
          this, tr("Invalid Operation"),
          tr("The selected key contains a key that does not actually have a "
             "encrypt usage.") +
              "<br/><br/>" + tr("For example the Following Key:") + " <br/>" +
              key.GetUIDs()->front().GetUID());
      return;
    }
  }

  auto buffer = GFBuffer(edit_->CurPlainText());
  CommonUtils::WaitForOpera(
      this, tr("Encrypting"),
      [this, keys, buffer](const OperaWaitingHd& op_hd) {
        GpgFrontend::GpgBasicOperator::GetInstance(
            m_key_list_->GetCurrentGpgContextChannel())
            .Encrypt(
                {keys->begin(), keys->end()}, buffer, true,
                [this, op_hd](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (data_obj == nullptr ||
                      !data_obj->Check<GpgEncryptResult, GFBuffer>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                  auto buffer = ExtractParams<GFBuffer>(data_obj, 1);

                  auto result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err, result);
                  result_analyse.Analyse();
                  slot_result_analyse_show_helper(result_analyse);

                  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                    edit_->SlotFillTextEditWithText(
                        buffer.ConvertToQByteArray());
                  }
                  info_board_->ResetOptionActionsMenu();
                });
      });
}

void MainWindow::SlotSign() {
  if (edit_->SlotCurPageTextEdit() == nullptr) return;

  auto key_ids = m_key_list_->GetCheckedPrivateKey();
  if (key_ids->empty()) {
    QMessageBox::critical(
        this, tr("No Key Checked"),
        tr("Please check the key in the key toolbox on the right."));
    return;
  }

  auto keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  for (const auto& key : *keys) {
    assert(key.IsGood());
  }

  for (const auto& key : *keys) {
    if (!key.IsHasActualSigningCapability()) {
      QMessageBox::information(
          this, tr("Invalid Operation"),
          tr("The selected key contains a key that does not actually have a "
             "signature usage.") +
              "<br/><br/>" + tr("For example the Following Key:") + "<br/>" +
              key.GetUIDs()->front().GetUID());
      return;
    }
  }

  // set input buffer
  auto buffer = GFBuffer(edit_->CurPlainText());
  CommonUtils::WaitForOpera(
      this, tr("Signing"), [this, keys, buffer](const OperaWaitingHd& hd) {
        GpgFrontend::GpgBasicOperator::GetInstance(
            m_key_list_->GetCurrentGpgContextChannel())
            .Sign(
                {keys->begin(), keys->end()}, buffer, GPGME_SIG_MODE_CLEAR,
                true, [this, hd](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgSignResult, GFBuffer>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }
                  auto sign_result = ExtractParams<GpgSignResult>(data_obj, 0);
                  auto sign_out_buffer = ExtractParams<GFBuffer>(data_obj, 1);
                  auto result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      sign_result);
                  result_analyse.Analyse();
                  slot_result_analyse_show_helper(result_analyse);

                  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                    edit_->SlotFillTextEditWithText(
                        sign_out_buffer.ConvertToQByteArray());
                  }
                });
      });
}

void MainWindow::SlotDecrypt() {
  if (edit_->SlotCurPageTextEdit() == nullptr) return;

  // data to transfer into task
  auto buffer = GFBuffer(edit_->CurPlainText());

  CommonUtils::WaitForOpera(
      this, tr("Decrypting"), [this, buffer](const OperaWaitingHd& hd) {
        GpgFrontend::GpgBasicOperator::GetInstance(
            m_key_list_->GetCurrentGpgContextChannel())
            .Decrypt(buffer, [this, hd](GpgError err,
                                        const DataObjectPtr& data_obj) {
              // stop waiting
              hd();

              if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
                  !data_obj->Check<GpgDecryptResult, GFBuffer>()) {
                QMessageBox::critical(this, tr("Error"),
                                      tr("Unknown error occurred"));
                return;
              }
              auto decrypt_result =
                  ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto out_buffer = ExtractParams<GFBuffer>(data_obj, 1);
              auto result_analyse = GpgDecryptResultAnalyse(
                  m_key_list_->GetCurrentGpgContextChannel(), err,
                  decrypt_result);
              result_analyse.Analyse();
              slot_result_analyse_show_helper(result_analyse);

              if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                edit_->SlotFillTextEditWithText(
                    out_buffer.ConvertToQByteArray());
              }
            });
      });
}

void MainWindow::SlotVerify() {
  if (edit_->SlotCurPageTextEdit() == nullptr) return;

  // set input buffer
  auto buffer = GFBuffer(edit_->CurPlainText());

  CommonUtils::WaitForOpera(
      this, tr("Verifying"), [this, buffer](const OperaWaitingHd& hd) {
        GpgFrontend::GpgBasicOperator::GetInstance(
            m_key_list_->GetCurrentGpgContextChannel())
            .Verify(buffer, GFBuffer(),
                    [this, hd](GpgError err, const DataObjectPtr& data_obj) {
                      // stop waiting
                      hd();

                      if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                          data_obj == nullptr ||
                          !data_obj->Check<GpgVerifyResult>()) {
                        QMessageBox::critical(this, tr("Error"),
                                              tr("Unknown error occurred"));
                        return;
                      }
                      auto verify_result =
                          ExtractParams<GpgVerifyResult>(data_obj, 0);

                      // analyse result
                      auto result_analyse = GpgVerifyResultAnalyse(
                          m_key_list_->GetCurrentGpgContextChannel(), err,
                          verify_result);
                      result_analyse.Analyse();
                      slot_result_analyse_show_helper(result_analyse);

                      if (!result_analyse.GetUnknownSignatures().isEmpty() &&
                          Module::IsModuleActivate(kKeyServerSyncModuleID)) {
                        slot_verifying_unknown_signature_helper(result_analyse);
                      }
                    });
      });
}

void MainWindow::SlotVerify(const QByteArray& raw_data,
                            const QByteArray& signature) {
  // set input buffer
  auto raw_data_buffer = GFBuffer(raw_data);
  auto signature_buffer = GFBuffer(signature);

  CommonUtils::WaitForOpera(
      this, tr("Verifying"),
      [this, raw_data_buffer, signature_buffer](const OperaWaitingHd& hd) {
        GpgFrontend::GpgBasicOperator::GetInstance(
            m_key_list_->GetCurrentGpgContextChannel())
            .Verify(
                raw_data_buffer, signature_buffer,
                [this, hd](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgVerifyResult>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }
                  auto verify_result =
                      ExtractParams<GpgVerifyResult>(data_obj, 0);

                  // analyse result
                  auto result_analyse = GpgVerifyResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      verify_result);
                  result_analyse.Analyse();
                  slot_result_analyse_show_helper(result_analyse);

                  if (!result_analyse.GetUnknownSignatures().isEmpty() &&
                      Module::IsModuleActivate(kKeyServerSyncModuleID)) {
                    LOG_D() << "try to sync missing key info from server"
                            << result_analyse.GetUnknownSignatures();

                    QString fingerprint_list;
                    for (const auto& fingerprint :
                         result_analyse.GetUnknownSignatures()) {
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
                              result_analyse.GetUnknownSignatures());
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

void MainWindow::SlotEncryptSign() {
  if (edit_->SlotCurPageTextEdit() == nullptr) return;

  auto key_ids = m_key_list_->GetChecked();

  if (key_ids->empty()) {
    QMessageBox::critical(
        this, tr("No Key Checked"),
        tr("Please check some key in the key toolbox on the right."));
    return;
  }

  auto keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  for (const auto& key : *keys) {
    assert(key.IsGood());
  }

  for (const auto& key : *keys) {
    bool key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          this, tr("Invalid KeyPair"),
          tr("The selected keypair cannot be used for encryption.") +
              "<br/><br/>" + tr("For example the Following Key:") + " <br/>" +
              key.GetUIDs()->front().GetUID());
      return;
    }
  }

  auto* signers_picker =
      new SignersPicker(m_key_list_->GetCurrentGpgContextChannel(), this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto signer_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(signer_key_ids);
  for (const auto& key : *signer_keys) {
    assert(key.IsGood());
  }

  // data to transfer into task
  auto buffer = GFBuffer(edit_->CurPlainText());

  CommonUtils::WaitForOpera(
      this, tr("Encrypting and Signing"),
      [this, keys, signer_keys, buffer](const OperaWaitingHd& hd) {
        GpgFrontend::GpgBasicOperator::GetInstance(
            m_key_list_->GetCurrentGpgContextChannel())
            .EncryptSign(
                {keys->begin(), keys->end()},
                {signer_keys->begin(), signer_keys->end()}, buffer, true,
                [this, hd](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                      data_obj == nullptr ||
                      !data_obj->Check<GpgEncryptResult, GpgSignResult,
                                       GFBuffer>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }
                  auto encrypt_result =
                      ExtractParams<GpgEncryptResult>(data_obj, 0);
                  auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);
                  auto out_buffer = ExtractParams<GFBuffer>(data_obj, 2);

                  // analyse result
                  auto encrypt_result_analyse = GpgEncryptResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      encrypt_result);
                  encrypt_result_analyse.Analyse();

                  auto sign_result_analyse = GpgSignResultAnalyse(
                      m_key_list_->GetCurrentGpgContextChannel(), err,
                      sign_result);
                  sign_result_analyse.Analyse();

                  // show analyse result
                  slot_result_analyse_show_helper(encrypt_result_analyse,
                                                  sign_result_analyse);

                  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                    edit_->SlotFillTextEditWithText(
                        out_buffer.ConvertToQByteArray());
                  }
                });
      });
}

void MainWindow::SlotDecryptVerify() {
  if (edit_->SlotCurPageTextEdit() == nullptr) return;

  // data to transfer into task
  auto buffer = GFBuffer(edit_->CurPlainText());

  CommonUtils::WaitForOpera(
      this, tr("Decrypting and Verifying"),
      [this, buffer](const OperaWaitingHd& hd) {
        GpgFrontend::GpgBasicOperator::GetInstance(
            m_key_list_->GetCurrentGpgContextChannel())
            .DecryptVerify(buffer, [this, hd](GpgError err,
                                              const DataObjectPtr& data_obj) {
              // stop waiting
              hd();

              if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
                  !data_obj
                       ->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()) {
                QMessageBox::critical(this, tr("Error"),
                                      tr("Unknown error occurred"));
                return;
              }
              auto decrypt_result =
                  ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto verify_result = ExtractParams<GpgVerifyResult>(data_obj, 1);
              auto out_buffer = ExtractParams<GFBuffer>(data_obj, 2);

              // analyse result
              auto decrypt_result_analyse = GpgDecryptResultAnalyse(
                  m_key_list_->GetCurrentGpgContextChannel(), err,
                  decrypt_result);
              decrypt_result_analyse.Analyse();

              auto verify_result_analyse = GpgVerifyResultAnalyse(
                  m_key_list_->GetCurrentGpgContextChannel(), err,
                  verify_result);
              verify_result_analyse.Analyse();

              // show analyse result
              slot_result_analyse_show_helper(decrypt_result_analyse,
                                              verify_result_analyse);

              if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                edit_->SlotFillTextEditWithText(
                    out_buffer.ConvertToQByteArray());
              }

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
                  CommonUtils::GetInstance()->ImportKeyByKeyServerSyncModule(
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

}  // namespace GpgFrontend::UI