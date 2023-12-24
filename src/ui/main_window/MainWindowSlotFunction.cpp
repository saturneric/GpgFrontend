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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#include "MainWindow.h"
#include "core/GpgConstants.h"
#include "core/GpgModel.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/function/gpg/GpgKeyManager.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/DataObject.h"
#include "core/model/GpgEncryptResult.h"
#include "core/module/ModuleManager.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
#include "ui/dialog/help/AboutDialog.h"
#include "ui/dialog/import_export/KeyUploadDialog.h"
#include "ui/dialog/keypair_details/KeyDetailsDialog.h"
#include "ui/function/SetOwnerTrustLevel.h"
#include "ui/widgets/FindWidget.h"

namespace GpgFrontend::UI {

/**
 * Encrypt Entry(Text & File)
 */
void MainWindow::slot_encrypt() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    if (edit_->SlotCurPageFileTreeView() != nullptr) this->SlotFileEncrypt();
    return;
  }

  auto key_ids = m_key_list_->GetChecked();

  if (key_ids->empty()) {
    // Symmetric Encrypt
    auto ret = QMessageBox::information(
        this, _("Symmetric Encryption"),
        _("No Key Checked. Do you want to encrypt with a "
          "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;

    // encrypt_type = ;
    // encrypt_runner = [](const DataObjectPtr& data_object) -> int {
    //   if (data_object == nullptr || !data_object->Check<std::string>()) {
    //     throw std::runtime_error("data object doesn't pass checking");
    //   }
    //   auto buffer = ExtractParams<std::string>(data_object, 0);
    //   try {
    //     GpgEncrResult result = nullptr;
    //     auto tmp = GpgFrontend::SecureCreateSharedObject<ByteArray>();
    //     GpgError error =
    //         GpgFrontend::GpgBasicOperator::GetInstance().EncryptSymmetric(
    //             buffer, tmp, result);

    //     data_object->Swap(DataObject{error, std::move(result),
    //     std::move(tmp)});
    //   } catch (const std::runtime_error& e) {
    //     return -1;
    //   }
    //   return 0;
    // };

    auto buffer = GFBuffer(edit_->CurTextPage()->GetTextPage()->toPlainText());
    // CommonUtils::WaitForOpera(
    //     this, _("Symmetrically Encrypting"),
    //     [this, buffer](const OperaWaitingHd& hd) {
    //       GpgFrontend::GpgBasicOperator::GetInstance().EncryptSymmetric(
    //           std::move(keys), buffer.toStdString(),
    //           [this, hd](GpgError err, const DataObjectPtr& data_obj) {
    //             auto result = ExtractParams<GpgEncrResult>(data_obj, 0);
    //             auto buffer = ExtractParams<ByteArrayPtr>(data_obj, 1);

    //             auto resultAnalyse =
    //                 GpgEncryptResultAnalyse(err, std::move(result));
    //             resultAnalyse.Analyse();
    //             process_result_analyse(edit_, info_board_, resultAnalyse);

    //             if (CheckGpgError(err) == GPG_ERR_NO_ERROR)
    //               edit_->SlotFillTextEditWithText(
    //                   QString::fromStdString(*buffer));
    //             info_board_->ResetOptionActionsMenu();

    //             // stop waiting
    //             hd();
    //           });
    //     });

    return;
  }

  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);
  for (const auto& key : *keys) {
    if (!key.IsHasActualEncryptionCapability()) {
      QMessageBox::information(
          this, _("Invalid Operation"),
          QString(
              _("The selected key contains a key that does not actually have a "
                "encrypt usage.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  auto buffer = GFBuffer(edit_->CurTextPage()->GetTextPage()->toPlainText());
  CommonUtils::WaitForOpera(
      this, _("Encrypting"), [this, keys, buffer](const OperaWaitingHd& hd) {
        GpgFrontend::GpgBasicOperator::GetInstance().Encrypt(
            {keys->begin(), keys->end()}, buffer, true,
            [this, hd](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              hd();

              auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
              auto buffer = ExtractParams<GFBuffer>(data_obj, 1);

              auto result_analyse = GpgEncryptResultAnalyse(err, result);
              result_analyse.Analyse();
              process_result_analyse(edit_, info_board_, result_analyse);

              if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                edit_->SlotFillTextEditWithText(buffer.ConvertToQByteArray());
              }
              info_board_->ResetOptionActionsMenu();
            });
      });
}

void MainWindow::slot_sign() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    if (edit_->SlotCurPageFileTreeView() != nullptr) this->SlotFileSign();
    return;
  }

  auto key_ids = m_key_list_->GetPrivateChecked();

  if (key_ids->empty()) {
    QMessageBox::critical(
        this, _("No Key Checked"),
        _("Please check the key in the key toolbox on the right."));
    return;
  }

  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);
  for (const auto& key : *keys) {
    if (!key.IsHasActualSigningCapability()) {
      QMessageBox::information(
          this, _("Invalid Operation"),
          QString(
              _("The selected key contains a key that does not actually have a "
                "signature usage.")) +
              "<br/><br/>" + _("For example the Following Key:") + "<br/>" +
              key.GetUIDs()->front().GetUID().c_str());
      return;
    }
  }

  // data to transfer into task
  auto data_object = TransferParams();

  // set input buffer
  auto buffer =
      edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString();
  data_object->AppendObject(std::move(buffer));

  // push the keys into data object
  data_object->AppendObject(std::move(keys));

  auto sign_ruunner = [](DataObjectPtr data_object) -> int {
    // check the size of the data object
    if (data_object == nullptr || data_object->GetObjectSize() != 2)
      throw std::runtime_error("data object doesn't pass checking");

    auto buffer = ExtractParams<std::string>(data_object, 0);
    auto keys = ExtractParams<KeyListPtr>(data_object, 1);

    try {
      GpgSignResult result = nullptr;
      auto tmp = GpgFrontend::SecureCreateSharedObject<ByteArray>();
      GpgError error = GpgFrontend::GpgBasicOperator::GetInstance().Sign(
          std::move(keys), buffer, tmp, GPGME_SIG_MODE_CLEAR, result);

      data_object->Swap({error, result, tmp});
    } catch (const std::runtime_error& e) {
      return -1;
    }
    return 0;
  };

  auto result_callback = [this](int rtn, DataObjectPtr data_object) {
    if (!rtn) {
      if (data_object == nullptr || data_object->GetObjectSize() != 3)
        throw std::runtime_error("data object doesn't pass checking");
      auto error = ExtractParams<GpgError>(data_object, 0);
      auto result = ExtractParams<GpgSignResult>(data_object, 1);
      auto tmp = ExtractParams<ByteArrayPtr>(data_object, 2);
      auto resultAnalyse = GpgSignResultAnalyse(error, std::move(result));
      resultAnalyse.Analyse();
      process_result_analyse(edit_, info_board_, resultAnalyse);

      if (CheckGpgError(error) == GPG_ERR_NO_ERROR)
        edit_->SlotFillTextEditWithText(QString::fromStdString(*tmp));
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("An error occurred during operation."));
      return;
    }
  };

  process_operation(this, _("Signing"), std::move(sign_ruunner),
                    std::move(result_callback), data_object);
}

void MainWindow::slot_decrypt() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    if (edit_->SlotCurPageFileTreeView() != nullptr) this->SlotFileDecrypt();
    return;
  }

  QByteArray text = edit_->CurTextPage()->GetTextPage()->toPlainText().toUtf8();

  // data to transfer into task
  auto data_object = TransferParams(
      edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString());

  auto decrypt_runner = [](DataObjectPtr data_object) -> int {
    // check the size of the data object
    if (data_object == nullptr || data_object->GetObjectSize() != 1)
      throw std::runtime_error("data object doesn't pass checking");

    auto buffer = ExtractParams<std::string>(data_object, 0);
    try {
      GpgDecrResult result = nullptr;
      auto decrypted = GpgFrontend::SecureCreateSharedObject<ByteArray>();
      // GpgError error = GpgFrontend::GpgBasicOperator::GetInstance().Decrypt(
      //     GFBuffer(buffer), decrypted, result);
      GpgError error;

      data_object->Swap({error, result, decrypted});
    } catch (const std::runtime_error& e) {
      return -1;
    }
    return 0;
  };

  auto result_callback = [this](int rtn, DataObjectPtr data_object) {
    if (!rtn) {
      if (data_object == nullptr ||
          !data_object->Check<GpgError, GpgDecrResult, ByteArrayPtr>())
        throw std::runtime_error("data object doesn't pass checking");
      auto error = ExtractParams<GpgError>(data_object, 0);
      auto result = ExtractParams<GpgDecrResult>(data_object, 1);
      auto decrypted = ExtractParams<ByteArrayPtr>(data_object, 2);
      auto resultAnalyse = GpgDecryptResultAnalyse(error, std::move(result));
      resultAnalyse.Analyse();
      process_result_analyse(edit_, info_board_, resultAnalyse);

      if (CheckGpgError(error) == GPG_ERR_NO_ERROR)
        edit_->SlotFillTextEditWithText(QString::fromStdString(*decrypted));
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("An error occurred during operation."));
      return;
    }
  };

  process_operation(this, _("Decrypting"), std::move(decrypt_runner),
                    std::move(result_callback), data_object);
}

void MainWindow::slot_verify() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    if (edit_->SlotCurPageFileTreeView() != nullptr) this->SlotFileVerify();
    return;
  }

  // data to transfer into task
  auto data_object = TransferParams();

  // set input buffer
  auto buffer =
      edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString();
  data_object->AppendObject(std::move(buffer));

  auto verify_runner = [](DataObjectPtr data_object) -> int {
    // check the size of the data object
    if (data_object == nullptr || !data_object->Check<std::string>())
      throw std::runtime_error("data object doesn't pass checking");

    auto buffer = ExtractParams<std::string>(data_object, 0);

    SPDLOG_DEBUG("verify buffer size: {}", buffer.size());

    try {
      GpgVerifyResult verify_result = nullptr;
      auto sig_buffer = ByteArrayPtr(nullptr);
      GpgError error = GpgFrontend::GpgBasicOperator::GetInstance().Verify(
          buffer, sig_buffer, verify_result);

      data_object->Swap({error, verify_result});
    } catch (const std::runtime_error& e) {
      return -1;
    }
    return 0;
  };

  auto result_callback = [this](int rtn, DataObjectPtr data_object) {
    if (!rtn) {
      if (data_object == nullptr ||
          !data_object->Check<GpgError, GpgVerifyResult>())
        throw std::runtime_error("data object doesn't pass checking");
      auto error = ExtractParams<GpgError>(data_object, 0);
      auto verify_result = ExtractParams<GpgVerifyResult>(data_object, 1);

      auto result_analyse = GpgVerifyResultAnalyse(error, verify_result);
      result_analyse.Analyse();
      process_result_analyse(edit_, info_board_, result_analyse);

      if (CheckGpgError(error) == GPG_ERR_NO_ERROR) {
        if (result_analyse.GetStatus() == -2)
          import_unknown_key_from_keyserver(this, result_analyse);

        if (result_analyse.GetStatus() >= 0)
          show_verify_details(this, info_board_, error, verify_result);
      }
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("An error occurred during operation."));
      return;
    }
  };

  process_operation(this, _("Verifying"), verify_runner, result_callback,
                    data_object);
}

void MainWindow::slot_encrypt_sign() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    if (edit_->SlotCurPageFileTreeView() != nullptr)
      this->SlotFileEncryptSign();
    return;
  }

  auto key_ids = m_key_list_->GetChecked();

  if (key_ids->empty()) {
    QMessageBox::critical(
        this, _("No Key Checked"),
        _("Please check some key in the key toolbox on the right."));
    return;
  }

  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  for (const auto& key : *keys) {
    bool key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          this, _("Invalid KeyPair"),
          QString(_("The selected keypair cannot be used for encryption.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  auto signersPicker = new SignersPicker(this);
  QEventLoop loop;
  connect(signersPicker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signersPicker->GetStatus()) return;

  auto signer_key_ids = signersPicker->GetCheckedSigners();
  auto signer_keys = GpgKeyGetter::GetInstance().GetKeys(signer_key_ids);

  for (const auto& key : *keys) {
    SPDLOG_DEBUG("keys {}", key.GetEmail());
  }

  for (const auto& signer : *signer_keys) {
    SPDLOG_DEBUG("signers {}", signer.GetEmail());
  }

  // data to transfer into task
  auto data_object = TransferParams(
      std::move(signer_keys), std::move(keys),
      edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString());

  auto encrypt_sign_runner = [](DataObjectPtr data_object) -> int {
    // check the size of the data object
    if (data_object == nullptr ||
        !data_object->Check<KeyListPtr, KeyListPtr, std::string>())
      throw std::runtime_error("data object doesn't pass checking");

    auto signer_keys = ExtractParams<KeyListPtr>(data_object, 0);
    auto keys = ExtractParams<KeyListPtr>(data_object, 1);
    auto buffer = ExtractParams<std::string>(data_object, 2);
    try {
      GpgEncrResult encr_result = nullptr;
      GpgSignResult sign_result = nullptr;
      auto tmp = GpgFrontend::SecureCreateSharedObject<ByteArray>();
      GpgError error = GpgFrontend::GpgBasicOperator::GetInstance().EncryptSign(
          std::move(keys), std::move(signer_keys), buffer, tmp, encr_result,
          sign_result);

      data_object->Swap({error, encr_result, sign_result, tmp});

    } catch (const std::runtime_error& e) {
      return -1;
    }
    return 0;
  };

  auto result_callback = [this](int rtn, DataObjectPtr data_object) {
    if (!rtn) {
      if (data_object == nullptr ||
          !data_object
               ->Check<GpgError, GpgEncrResult, GpgSignResult, ByteArrayPtr>())
        throw std::runtime_error("data object doesn't pass checking");
      auto error = ExtractParams<GpgError>(data_object, 0);
      auto encrypt_result = ExtractParams<GpgEncrResult>(data_object, 1);
      auto sign_result = ExtractParams<GpgSignResult>(data_object, 2);
      auto tmp = ExtractParams<ByteArrayPtr>(data_object, 3);

      auto encrypt_result_analyse = GpgEncryptResultAnalyse(
          error, GpgEncryptResult(encrypt_result.get()));
      auto sign_result_analyse =
          GpgSignResultAnalyse(error, std::move(sign_result));
      encrypt_result_analyse.Analyse();
      sign_result_analyse.Analyse();
      process_result_analyse(edit_, info_board_, encrypt_result_analyse,
                             sign_result_analyse);
      if (CheckGpgError(error) == GPG_ERR_NO_ERROR)
        edit_->SlotFillTextEditWithText(QString::fromStdString(*tmp));

      info_board_->ResetOptionActionsMenu();
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("An error occurred during operation."));
      return;
    }
  };

  process_operation(this, _("Encrypting and Signing"), encrypt_sign_runner,
                    result_callback, data_object);
}

void MainWindow::slot_decrypt_verify() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    if (edit_->SlotCurPageFileTreeView() != nullptr)
      this->SlotFileDecryptVerify();
    return;
  }

  // data to transfer into task
  auto data_object = TransferParams(
      edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString());

  auto decrypt_verify_runner = [](DataObjectPtr data_object) -> int {
    // check the size of the data object
    if (data_object == nullptr || !data_object->Check<std::string>())
      throw std::runtime_error("data object doesn't pass checking");

    auto buffer = ExtractParams<std::string>(data_object, 0);
    try {
      GpgDecrResult decrypt_result = nullptr;
      GpgVerifyResult verify_result = nullptr;
      auto decrypted_buffer =
          GpgFrontend::SecureCreateSharedObject<ByteArray>();
      GpgError error = GpgBasicOperator::GetInstance().DecryptVerify(
          buffer, decrypted_buffer, decrypt_result, verify_result);

      data_object->Swap(
          {error, decrypt_result, verify_result, decrypted_buffer});
    } catch (const std::runtime_error& e) {
      SPDLOG_ERROR(e.what());
      return -1;
    }
    return 0;
  };

  auto result_callback = [this](int rtn, DataObjectPtr data_object) {
    if (!rtn) {
      if (data_object == nullptr ||
          !data_object->Check<GpgError, GpgDecrResult, GpgVerifyResult,
                              ByteArrayPtr>())
        throw std::runtime_error("data object doesn't pass checking");

      auto error = ExtractParams<GpgError>(data_object, 0);
      auto decrypt_result = ExtractParams<GpgDecrResult>(data_object, 1);
      auto verify_result = ExtractParams<GpgVerifyResult>(data_object, 2);
      auto decrypted = ExtractParams<ByteArrayPtr>(data_object, 3);

      auto decrypt_result_analyse =
          GpgDecryptResultAnalyse(error, std::move(decrypt_result));
      auto verify_result_analyse = GpgVerifyResultAnalyse(error, verify_result);
      decrypt_result_analyse.Analyse();
      verify_result_analyse.Analyse();
      process_result_analyse(edit_, info_board_, decrypt_result_analyse,
                             verify_result_analyse);
      if (CheckGpgError(error) == GPG_ERR_NO_ERROR)
        edit_->SlotFillTextEditWithText(QString::fromStdString(*decrypted));

      if (verify_result_analyse.GetStatus() == -2)
        import_unknown_key_from_keyserver(this, verify_result_analyse);

      if (verify_result_analyse.GetStatus() >= 0)
        show_verify_details(this, info_board_, error, verify_result);

    } else {
      QMessageBox::critical(this, _("Error"),
                            _("An error occurred during operation."));
      return;
    }
  };

  process_operation(this, _("Decrypting and Verifying"), decrypt_verify_runner,
                    result_callback, data_object);
}

void MainWindow::slot_find() {
  if (edit_->TabCount() == 0 || edit_->CurTextPage() == nullptr) {
    return;
  }

  // At first close verifynotification, if existing
  edit_->SlotCurPageTextEdit()->CloseNoteByClass("findwidget");

  auto* fw = new FindWidget(this, edit_->CurTextPage());
  edit_->SlotCurPageTextEdit()->ShowNotificationWidget(fw, "findWidget");
}

/*
 * Append the selected (not checked!) Key(s) To Textedit
 */
void MainWindow::slot_append_selected_keys() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    return;
  }

  auto exported = GpgFrontend::SecureCreateSharedObject<ByteArray>();
  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    SPDLOG_ERROR("no key is selected");
    return;
  }

  if (!GpgKeyImportExporter::GetInstance().ExportKeys(key_ids, exported)) {
    QMessageBox::critical(this, _("Error"), _("Key Export Operation Failed."));
    return;
  }
  edit_->CurTextPage()->GetTextPage()->appendPlainText(
      QString::fromStdString(*exported));
}

void MainWindow::slot_append_keys_create_datetime() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    return;
  }

  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    SPDLOG_ERROR("no key is selected");
    return;
  }

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }

  auto create_datetime_format_str =
      boost::posix_time::to_iso_extended_string(key.GetCreateTime()) +
      " (UTC) " + "\n";

  edit_->CurTextPage()->GetTextPage()->appendPlainText(
      QString::fromStdString(create_datetime_format_str));
}

void MainWindow::slot_append_keys_expire_datetime() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    return;
  }

  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    SPDLOG_ERROR("no key is selected");
    return;
  }

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }

  auto create_datetime_format_str =
      boost::posix_time::to_iso_extended_string(key.GetCreateTime()) +
      " (UTC) " + "\n";

  edit_->CurTextPage()->GetTextPage()->appendPlainText(
      QString::fromStdString(create_datetime_format_str));
}

void MainWindow::slot_append_keys_fingerprint() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }

  auto fingerprint_format_str =
      BeautifyFingerprint(key.GetFingerprint()) + "\n";

  edit_->CurTextPage()->GetTextPage()->appendPlainText(
      QString::fromStdString(fingerprint_format_str));
}

void MainWindow::slot_copy_mail_address_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(QString::fromStdString(key.GetEmail()));
}

void MainWindow::slot_copy_default_uid_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(QString::fromStdString(key.GetUIDs()->front().GetUID()));
}

void MainWindow::slot_copy_key_id_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(QString::fromStdString(key.GetId()));
}

void MainWindow::slot_show_key_details() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (key.IsGood()) {
    new KeyDetailsDialog(key, this);
  } else {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
  }
}

void MainWindow::slot_add_key_2_favourite() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  CommonUtils::GetInstance()->AddKey2Favourtie(key);

  emit SignalUIRefresh();
}

void MainWindow::slot_remove_key_from_favourite() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  CommonUtils::GetInstance()->RemoveKeyFromFavourite(key);

  emit SignalUIRefresh();
}

void MainWindow::refresh_keys_from_key_server() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto* dialog = new KeyServerImportDialog(this);
  dialog->show();
  dialog->SlotImport(key_ids);
}

void MainWindow::slot_set_owner_trust_level_of_key() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto* function = new SetOwnerTrustLevel(this);
  function->Exec(key_ids->front());
  function->deleteLater();
}

void MainWindow::upload_key_to_server() {
  auto key_ids = m_key_list_->GetSelected();
  auto* dialog = new KeyUploadDialog(key_ids, this);
  dialog->show();
  dialog->SlotUpload();
}

void MainWindow::SlotOpenFile(QString& path) { edit_->SlotOpenFile(path); }

void MainWindow::slot_version_upgrade_nofity() {
  SPDLOG_DEBUG(
      "slot version upgrade notify called, checking version info from rt...");
  auto is_loading_done = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.loading_done", false);

  SPDLOG_DEBUG("checking version info from rt, is loading done state: {}",
               is_loading_done);
  if (!is_loading_done) {
    SPDLOG_ERROR("invalid version info from rt, loading hasn't done yet");
    return;
  }

  auto is_need_upgrade = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.need_upgrade", false);

  auto is_current_a_withdrawn_version = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.current_a_withdrawn_version", false);

  auto is_current_version_released = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.current_version_released", false);

  auto latest_version = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.version-checking",
      "version.latest_version", std::string{});

  SPDLOG_DEBUG(
      "got version info from rt, need upgrade: {}, with drawn: {}, current "
      "version "
      "released: {}",
      is_need_upgrade, is_current_a_withdrawn_version,
      is_current_version_released);

  if (is_need_upgrade) {
    statusBar()->showMessage(
        QString(_("GpgFrontend Upgradeable (New Version: %1)."))
            .arg(latest_version.c_str()),
        30000);
    auto update_button = new QPushButton("Update GpgFrontend", this);
    connect(update_button, &QPushButton::clicked, [=]() {
      auto* about_dialog = new AboutDialog(2, this);
      about_dialog->show();
    });
    statusBar()->addPermanentWidget(update_button, 0);
  } else if (is_current_a_withdrawn_version) {
    QMessageBox::warning(
        this, _("Withdrawn Version"),
        QString(
            _("This version(%1) may have been withdrawn by the developer due "
              "to serious problems. Please stop using this version "
              "immediately and use the latest stable version."))
                .arg(latest_version.c_str()) +
            "<br/>" +
            QString(_("You can download the latest stable version(%1) on "
                      "Github Releases "
                      "Page.<br/>"))
                .arg(latest_version.c_str()));
  } else if (!is_current_version_released) {
    statusBar()->showMessage(
        QString(_("This maybe a BETA Version (Latest Stable Version: %1)."))
            .arg(latest_version.c_str()),
        30000);
  }
}

}  // namespace GpgFrontend::UI
