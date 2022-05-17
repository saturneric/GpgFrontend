/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "MainWindow.h"
#include "core/GpgConstants.h"
#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/model/GpgKey.h"
#include "core/thread/TaskRunner.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/help/AboutDialog.h"
#include "ui/widgets/SignersPicker.h"

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

  // data to transfer into task
  auto data_object = std::make_shared<Thread::Task::DataObject>();

  // set input buffer
  auto buffer =
      edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString();
  data_object->AppendObject(std::move(buffer));

  // the callback function
  auto result_callback = [this](int rtn,
                                Thread::Task::DataObjectPtr data_object) {
    if (!rtn) {
      if (data_object->GetObjectSize() != 3)
        throw std::runtime_error("Invalid data object size");
      auto error = data_object->PopObject<GpgError>();
      auto result = data_object->PopObject<GpgEncrResult>();
      auto tmp = data_object->PopObject<std::unique_ptr<ByteArray>>();

      auto resultAnalyse = GpgEncryptResultAnalyse(error, std::move(result));
      resultAnalyse.Analyse();
      process_result_analyse(edit_, info_board_, resultAnalyse);

      if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
        edit_->SlotFillTextEditWithText(QString::fromStdString(*tmp));
      info_board_->ResetOptionActionsMenu();
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("An error occurred during operation."));
      return;
    }
  };

  Thread::Task::TaskRunnable encrypt_runner = nullptr;

  std::string encrypt_type = "";

  if (key_ids->empty()) {
    // Symmetric Encrypt
    auto ret = QMessageBox::information(
        this, _("Symmetric Encryption"),
        _("No Key Checked. Do you want to encrypt with a "
          "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;

    encrypt_type = _("Symmetrically Encrypting");
    encrypt_runner = [](Thread::Task::DataObjectPtr data_object) -> int {
      if (data_object == nullptr || data_object->GetObjectSize() != 1)
        throw std::runtime_error("Invalid data object size");
      auto buffer = data_object->PopObject<std::string>();
      try {
        GpgEncrResult result = nullptr;
        auto tmp = std::make_unique<ByteArray>();
        GpgError error =
            GpgFrontend::GpgBasicOperator::GetInstance().EncryptSymmetric(
                buffer, tmp, result);
        data_object->AppendObject(std::move(tmp));
        data_object->AppendObject(std::move(result));
        data_object->AppendObject(std::move(error));
      } catch (const std::runtime_error& e) {
        return -1;
      }
      return 0;
    };

  } else {
    auto& key_getter = GpgFrontend::GpgKeyGetter::GetInstance();
    auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);
    for (const auto& key : *keys) {
      if (!key.IsHasActualEncryptionCapability()) {
        QMessageBox::information(
            this, _("Invalid Operation"),
            QString(_(
                "The selected key contains a key that does not actually have a "
                "encrypt usage.")) +
                "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
                QString::fromStdString(key.GetUIDs()->front().GetUID()));
        return;
      }
    }

    // push the keys into data object
    data_object->AppendObject(std::move(keys));

    // Asymmetric Encrypt
    encrypt_type = _("Encrypting");
    encrypt_runner = [](Thread::Task::DataObjectPtr data_object) -> int {
      // check the size of the data object
      if (data_object == nullptr || data_object->GetObjectSize() != 2)
        throw std::runtime_error("Invalid data object size");

      auto keys = data_object->PopObject<KeyListPtr>();
      auto buffer = data_object->PopObject<std::string>();
      try {
        GpgEncrResult result = nullptr;
        auto tmp = std::make_unique<ByteArray>();
        GpgError error = GpgFrontend::GpgBasicOperator::GetInstance().Encrypt(
            std::move(keys), buffer, tmp, result);

        data_object->AppendObject(std::move(tmp));
        data_object->AppendObject(std::move(result));
        data_object->AppendObject(std::move(error));
      } catch (const std::runtime_error& e) {
        return -1;
      }
      return 0;
    };
  }
  // Do the task
  process_operation(this, _("Encrypting"), std::move(encrypt_runner),
                    std::move(result_callback), data_object);
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
  auto data_object = std::make_shared<Thread::Task::DataObject>();

  // set input buffer
  auto buffer =
      edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString();
  data_object->AppendObject(std::move(buffer));

  // push the keys into data object
  data_object->AppendObject(std::move(keys));

  auto sign_ruunner = [](Thread::Task::DataObjectPtr data_object) -> int {
    // check the size of the data object
    if (data_object == nullptr || data_object->GetObjectSize() != 2)
      throw std::runtime_error("Invalid data object size");

    auto keys = data_object->PopObject<KeyListPtr>();
    auto buffer = data_object->PopObject<std::string>();
    try {
      GpgSignResult result = nullptr;
      auto tmp = std::make_unique<ByteArray>();
      GpgError error = GpgFrontend::GpgBasicOperator::GetInstance().Sign(
          std::move(keys), buffer, tmp, GPGME_SIG_MODE_CLEAR, result);
      data_object->AppendObject(std::move(tmp));
      data_object->AppendObject(std::move(result));
      data_object->AppendObject(std::move(error));
    } catch (const std::runtime_error& e) {
      return -1;
    }
    return 0;
  };

  auto result_callback = [this](int rtn,
                                Thread::Task::DataObjectPtr data_object) {
    if (!rtn) {
      if (data_object == nullptr || data_object->GetObjectSize() != 3)
        throw std::runtime_error("Invalid data object size");
      auto error = data_object->PopObject<GpgError>();
      auto result = data_object->PopObject<GpgSignResult>();
      auto tmp = data_object->PopObject<std::unique_ptr<ByteArray>>();
      auto resultAnalyse = GpgSignResultAnalyse(error, std::move(result));
      resultAnalyse.Analyse();
      process_result_analyse(edit_, info_board_, resultAnalyse);

      if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
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

  if (text.trimmed().startsWith(GpgConstants::GPG_FRONTEND_SHORT_CRYPTO_HEAD)) {
    QMessageBox::critical(
        this, _("Notice"),
        _("Short Crypto Text only supports Decrypt & Verify."));
    return;
  }

  // data to transfer into task
  auto data_object = std::make_shared<Thread::Task::DataObject>();

  // set input buffer
  auto buffer =
      edit_->CurTextPage()->GetTextPage()->toPlainText().toStdString();
  data_object->AppendObject(std::move(buffer));

  auto decrypt_runner = [](Thread::Task::DataObjectPtr data_object) -> int {
    // check the size of the data object
    if (data_object == nullptr || data_object->GetObjectSize() != 1)
      throw std::runtime_error("Invalid data object size");

    auto buffer = data_object->PopObject<std::string>();
    try {
      GpgDecrResult result = nullptr;
      auto decrypted = std::make_unique<ByteArray>();
      GpgError error = GpgFrontend::GpgBasicOperator::GetInstance().Decrypt(
          buffer, decrypted, result);
      data_object->AppendObject(std::move(decrypted));
      data_object->AppendObject(std::move(result));
      data_object->AppendObject(std::move(error));
    } catch (const std::runtime_error& e) {
      return -1;
    }
    return 0;
  };

  auto result_callback = [this](int rtn,
                                Thread::Task::DataObjectPtr data_object) {
    if (!rtn) {
      if (data_object == nullptr || data_object->GetObjectSize() != 3)
        throw std::runtime_error("Invalid data object size");
      auto error = data_object->PopObject<GpgError>();
      auto result = data_object->PopObject<GpgDecrResult>();
      auto decrypted = data_object->PopObject<std::unique_ptr<ByteArray>>();
      auto resultAnalyse = GpgDecryptResultAnalyse(error, std::move(result));
      resultAnalyse.Analyse();
      process_result_analyse(edit_, info_board_, resultAnalyse);

      if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
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

void MainWindow::slot_find() {
  if (edit_->TabCount() == 0 || edit_->CurTextPage() == nullptr) {
    return;
  }

  // At first close verifynotification, if existing
  edit_->SlotCurPageTextEdit()->CloseNoteByClass("findwidget");

  auto* fw = new FindWidget(this, edit_->CurTextPage());
  edit_->SlotCurPageTextEdit()->ShowNotificationWidget(fw, "findWidget");
}

void MainWindow::slot_verify() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    if (edit_->SlotCurPageFileTreeView() != nullptr) this->SlotFileVerify();
    return;
  }

  auto text = edit_->CurTextPage()->GetTextPage()->toPlainText().toUtf8();
  // TODO(Saturneric) PreventNoDataErr

  auto sig_buffer = std::make_unique<ByteArray>();
  sig_buffer.reset();

  GpgVerifyResult result = nullptr;
  GpgError error;
  bool if_error = false;
  process_operation(
      this, _("Verifying"), [&](Thread::Task::DataObjectPtr) -> int {
        try {
          auto buffer = text.toStdString();
          error = GpgFrontend::GpgBasicOperator::GetInstance().Verify(
              buffer, sig_buffer, result);
        } catch (const std::runtime_error& e) {
          if_error = true;
        }
        return 0;
      });

  if (!if_error) {
    auto result_analyse = GpgVerifyResultAnalyse(error, result);
    result_analyse.Analyse();
    process_result_analyse(edit_, info_board_, result_analyse);

    if (result_analyse.GetStatus() == -2)
      import_unknown_key_from_keyserver(this, result_analyse);

    if (result_analyse.GetStatus() >= 0)
      show_verify_details(this, info_board_, error, result);
  }
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

  auto signer_key_ids = signersPicker->GetCheckedSigners();
  auto signer_keys = GpgKeyGetter::GetInstance().GetKeys(signer_key_ids);

  for (const auto& key : *keys) {
    LOG(INFO) << "Keys " << key.GetEmail();
  }

  for (const auto& signer : *signer_keys) {
    LOG(INFO) << "Signers " << signer.GetEmail();
  }

  GpgEncrResult encr_result = nullptr;
  GpgSignResult sign_result = nullptr;
  GpgError error;
  bool if_error = false;

  auto tmp = std::make_unique<ByteArray>();
  process_operation(
      this, _("Encrypting and Signing"),
      [&](Thread::Task::DataObjectPtr) -> int {
        try {
          auto buffer = edit_->CurTextPage()
                            ->GetTextPage()
                            ->toPlainText()
                            .toUtf8()
                            .toStdString();
          error = GpgFrontend::GpgBasicOperator::GetInstance().EncryptSign(
              std::move(keys), std::move(signer_keys), buffer, tmp, encr_result,
              sign_result);
        } catch (const std::runtime_error& e) {
          if_error = true;
        }
        return 0;
      });

  if (!if_error) {
#ifdef ADVANCE_SUPPORT
    if (settings.value("advanced/autoPubkeyExchange").toBool()) {
      PubkeyUploader pubkeyUploader(mCtx, signerKeys);
      pubkeyUploader.start();
      if (!pubkeyUploader.result()) {
        QMessageBox::warning(nullptr, _("Automatic Key Exchange Warning"),
                             _("Part of the automatic key exchange failed, "
                               "which may be related to your key.") +
                                 _("If possible, try to use the RSA algorithm "
                                   "compatible with the server for signing."));
      }
    }
#endif
    LOG(INFO) << "GpgResultAnalyse Started";
    auto encrypt_res = GpgEncryptResultAnalyse(error, std::move(encr_result));
    auto sign_res = GpgSignResultAnalyse(error, std::move(sign_result));
    encrypt_res.Analyse();
    sign_res.Analyse();
    process_result_analyse(edit_, info_board_, encrypt_res, sign_res);
    if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
      edit_->SlotFillTextEditWithText(QString::fromStdString(*tmp));

    info_board_->ResetOptionActionsMenu();
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slot_decrypt_verify() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    if (edit_->SlotCurPageFileTreeView() != nullptr)
      this->SlotFileDecryptVerify();
    return;
  }

  QString plainText = edit_->CurTextPage()->GetTextPage()->toPlainText();

#ifdef ADVANCE_SUPPORT
  if (plainText.trimmed().startsWith(
          GpgConstants::GPG_FRONTEND_SHORT_CRYPTO_HEAD)) {
    auto cryptoText = getCryptText(plainText);
    if (!cryptoText.isEmpty()) {
      plainText = cryptoText;
    }
  }
#endif

  QByteArray text = plainText.toUtf8();

  GpgDecrResult d_result = nullptr;
  GpgVerifyResult v_result = nullptr;
  gpgme_error_t error;
  bool if_error = false;

#ifdef ADVANCE_SUPPORT
  // Automatically import public keys that are not stored locally
  if (settings.value("advanced/autoPubkeyExchange").toBool()) {
    gpgme_verify_result_t tmp_v_result = nullptr;
    auto thread = QThread::create([&](Thread::Task::DataObjectPtr) -> int {
      mCtx->verify(&text, nullptr, &tmp_v_result);
    });
    thread->start();
    while (thread->isRunning()) QApplication::processEvents();
    auto* checker = new UnknownSignersChecker(mCtx, tmp_v_result);
    checker->start();
    checker->deleteLater();
  }
#endif
  auto decrypted = std::make_unique<ByteArray>();
  process_operation(this, _("Decrypting and Verifying"),
                    [&](Thread::Task::DataObjectPtr) -> int {
                      try {
                        auto buffer = text.toStdString();
                        error = GpgBasicOperator::GetInstance().DecryptVerify(
                            buffer, decrypted, d_result, v_result);
                      } catch (const std::runtime_error& e) {
                        if_error = true;
                      }
                      return 0;
                    });

  if (!if_error) {
    auto decrypt_res = GpgDecryptResultAnalyse(error, std::move(d_result));
    auto verify_res = GpgVerifyResultAnalyse(error, v_result);
    decrypt_res.Analyse();
    verify_res.Analyse();
    process_result_analyse(edit_, info_board_, decrypt_res, verify_res);
    if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
      edit_->SlotFillTextEditWithText(QString::fromStdString(*decrypted));

    if (verify_res.GetStatus() == -2)
      import_unknown_key_from_keyserver(this, verify_res);

    if (verify_res.GetStatus() >= 0)
      show_verify_details(this, info_board_, error, v_result);

  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

/*
 * Append the selected (not checked!) Key(s) To Textedit
 */
void MainWindow::slot_append_selected_keys() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) {
    return;
  }

  auto exported = std::make_unique<ByteArray>();
  auto key_ids = m_key_list_->GetSelected();

  GpgKeyImportExporter::GetInstance().ExportKeys(key_ids, exported);
  edit_->CurTextPage()->GetTextPage()->appendPlainText(
      QString::fromStdString(*exported));
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

void MainWindow::refresh_keys_from_key_server() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto* dialog = new KeyServerImportDialog(this);
  dialog->show();
  dialog->SlotImport(key_ids);
}

void MainWindow::upload_key_to_server() {
  auto key_ids = m_key_list_->GetSelected();
  auto* dialog = new KeyUploadDialog(key_ids, this);
  dialog->show();
  dialog->SlotUpload();
}

void MainWindow::SlotOpenFile(QString& path) { edit_->SlotOpenFile(path); }

void MainWindow::slot_version_upgrade(const SoftwareVersion& version) {
  LOG(INFO) << _("called");
  if (version.NeedUpgrade()) {
    statusBar()->showMessage(
        QString(_("GpgFrontend Upgradeable (New Version: %1)."))
            .arg(version.latest_version.c_str()),
        30000);
    auto update_button = new QPushButton("Update GpgFrontend", this);
    connect(update_button, &QPushButton::clicked, [=]() {
      auto* about_dialog = new AboutDialog(2, this);
      about_dialog->show();
    });
    statusBar()->addPermanentWidget(update_button, 0);
  } else if (version.VersionWithDrawn()) {
    QMessageBox::warning(
        this, _("Withdrawn Version"),
        QString(
            _("This version(%1) may have been withdrawn by the developer due "
              "to serious problems. Please stop using this version "
              "immediately and use the latest stable version."))
                .arg(version.current_version.c_str()) +
            "<br/>" +
            QString(_("You can download the latest stable version(%1) on "
                      "Github Releases "
                      "Page.<br/>"))
                .arg(version.latest_version.c_str()));
  } else if (!version.CurrentVersionReleased()) {
    statusBar()->showMessage(
        QString(_("This maybe a BETA Version (Latest Stable Version: %1)."))
            .arg(version.latest_version.c_str()),
        30000);
  }
}

}  // namespace GpgFrontend::UI
