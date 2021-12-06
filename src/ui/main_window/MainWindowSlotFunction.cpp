/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "MainWindow.h"

#ifdef ADVANCE_SUPPORT
#include "advance/UnknownSignersChecker.h"
#endif
#ifdef SERVER_SUPPORT
#include "server/api/PubkeyUploader.h"
#endif

#ifdef SMTP_SUPPORT
#include "ui/smtp/SendMailDialog.h"
#endif

#include "gpg/function/BasicOperator.h"
#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExportor.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/help/AboutDialog.h"
#include "ui/widgets/SignersPicker.h"

namespace GpgFrontend::UI {
/**
 * Encrypt Entry(Text & File)
 */
void MainWindow::slotEncrypt() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) return;

  auto key_ids = mKeyList->getChecked();

  if (key_ids->empty()) {
    QMessageBox::critical(nullptr, _("No Key Selected"), _("No Key Selected"));
    return;
  }

  auto key_getter = GpgFrontend::GpgKeyGetter::GetInstance();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);
  for (const auto& key : *keys) {
    if (!key.CanEncrActual()) {
      QMessageBox::information(
          nullptr, _("Invalid Operation"),
          QString(
              _("The selected key contains a key that does not actually have a "
                "encrypt usage.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.uids()->front().uid()));
      return;
    }
  }

  auto tmp = std::make_unique<ByteArray>();

  GpgEncrResult result = nullptr;
  GpgError error;
  bool if_error = false;
  process_operation(this, _("Encrypting"), [&]() {
    try {
      auto buffer = edit->curTextPage()->toPlainText().toUtf8().toStdString();
      error = GpgFrontend::BasicOperator::GetInstance().Encrypt(
          std::move(keys), buffer, tmp, result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto resultAnalyse = EncryptResultAnalyse(error, std::move(result));
    resultAnalyse.analyse();
    process_result_analyse(edit, infoBoard, resultAnalyse);

    if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
      edit->slotFillTextEditWithText(QString::fromStdString(*tmp));
#ifdef SMTP_SUPPORT
    // set optional actions
    if (resultAnalyse.getStatus() >= 0) {
      infoBoard->resetOptionActionsMenu();
      infoBoard->addOptionalAction("Send Mail", [this]() {
        if (settings.value("sendMail/enable", false).toBool())
          new SendMailDialog(edit->curTextPage()->toPlainText(), this);
        else {
          QMessageBox::warning(nullptr, _("Function Disabled"),
                               _("Please go to the settings interface to "
                                 "enable and configure this function."));
        }
      });
    }
#endif

  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotSign() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) return;

  auto key_ids = mKeyList->getPrivateChecked();

  if (key_ids->empty()) {
    QMessageBox::critical(this, _("No Key Selected"), _("No Key Selected"));
    return;
  }

  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);
  for (const auto& key : *keys) {
    if (!key.CanSignActual()) {
      QMessageBox::information(
          this, _("Invalid Operation"),
          QString(
              _("The selected key contains a key that does not actually have a "
                "signature usage.")) +
              "<br/><br/>" + _("For example the Following Key:") + "<br/>" +
              key.uids()->front().uid().c_str());
      return;
    }
  }

  auto tmp = std::make_unique<ByteArray>();

  GpgSignResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;

  process_operation(this, _("Signing"), [&]() {
    try {
      auto buffer = edit->curTextPage()->toPlainText().toUtf8().toStdString();
      error = GpgFrontend::BasicOperator::GetInstance().Sign(
          std::move(keys), buffer, tmp, GPGME_SIG_MODE_CLEAR, result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto resultAnalyse = SignResultAnalyse(error, std::move(result));
    resultAnalyse.analyse();
    process_result_analyse(edit, infoBoard, resultAnalyse);

    if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
      edit->slotFillTextEditWithText(QString::fromStdString(*tmp));
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotDecrypt() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) return;

  auto decrypted = std::make_unique<ByteArray>();
  QByteArray text = edit->curTextPage()->toPlainText().toUtf8();

  if (text.trimmed().startsWith(GpgConstants::GPG_FRONTEND_SHORT_CRYPTO_HEAD)) {
    QMessageBox::critical(
        this, _("Notice"),
        _("Short Crypto Text only supports Decrypt & Verify."));
    return;
  }

  GpgDecrResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;
  process_operation(this, _("Decrypting"), [&]() {
    try {
      auto buffer = text.toStdString();
      error = GpgFrontend::BasicOperator::GetInstance().Decrypt(
          buffer, decrypted, result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto resultAnalyse = DecryptResultAnalyse(error, std::move(result));
    resultAnalyse.analyse();
    process_result_analyse(edit, infoBoard, resultAnalyse);

    if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
      edit->slotFillTextEditWithText(QString::fromStdString(*decrypted));
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFind() {
  if (edit->tabCount() == 0 || edit->curTextPage() == nullptr) {
    return;
  }

  // At first close verifynotification, if existing
  edit->slotCurPageTextEdit()->closeNoteByClass("findwidget");

  auto* fw = new FindWidget(this, edit->curTextPage());
  edit->slotCurPageTextEdit()->showNotificationWidget(fw, "findWidget");
}

void MainWindow::slotVerify() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) return;

  auto text = edit->curTextPage()->toPlainText().toUtf8();
  // TODO(Saturneric) PreventNoDataErr

  auto sig_buffer = std::make_unique<ByteArray>();
  sig_buffer.reset();

  GpgVerifyResult result = nullptr;
  GpgError error;
  bool if_error = false;
  process_operation(this, _("Verifying"), [&]() {
    try {
      auto buffer = text.toStdString();
      error = GpgFrontend::BasicOperator::GetInstance().Verify(
          buffer, sig_buffer, result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto result_analyse = VerifyResultAnalyse(error, std::move(result));
    result_analyse.analyse();
    process_result_analyse(edit, infoBoard, result_analyse);

    if (result_analyse.getStatus() >= 0) {
      // take out result
      auto _result = result_analyse.TakeChargeOfResult();
      auto _result_ptr = _result.get();
      _result.reset(nullptr);
      infoBoard->resetOptionActionsMenu();
      infoBoard->addOptionalAction(
          "Show Verify Details", [this, error, _result_ptr]() {
            VerifyDetailsDialog(this, error, GpgVerifyResult(_result_ptr));
          });
    }
  }
}

void MainWindow::slotEncryptSign() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) return;

  auto key_ids = mKeyList->getChecked();

  if (key_ids->empty()) {
    QMessageBox::critical(nullptr, _("No Key Selected"), _("No Key Selected"));
    return;
  }

  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  for (const auto& key : *keys) {
    bool key_can_encrypt = key.CanEncrActual();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, _("Invalid KeyPair"),
          QString(_("The selected keypair cannot be used for encryption.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.uids()->front().uid()));
      return;
    }
  }

  auto signersPicker = new SignersPicker(this);
  QEventLoop loop;
  connect(signersPicker, SIGNAL(finished(int)), &loop, SLOT(quit()));
  loop.exec();

  auto signer_key_ids = signersPicker->getCheckedSigners();
  auto signer_keys = GpgKeyGetter::GetInstance().GetKeys(signer_key_ids);

  for (const auto& key : *keys) {
    LOG(INFO) << "Keys " << key.email();
  }

  for (const auto& signer : *signer_keys) {
    LOG(INFO) << "Signers " << signer.email();
  }

  GpgEncrResult encr_result = nullptr;
  GpgSignResult sign_result = nullptr;
  GpgError error;
  bool if_error = false;

  auto tmp = std::make_unique<ByteArray>();
  process_operation(this, _("Encrypting and Signing"), [&]() {
    try {
      auto buffer = edit->curTextPage()->toPlainText().toUtf8().toStdString();
      error = GpgFrontend::BasicOperator::GetInstance().EncryptSign(
          std::move(keys), std::move(signer_keys), buffer, tmp, encr_result,
          sign_result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
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
    LOG(INFO) << "ResultAnalyse Started";
    auto encrypt_res = EncryptResultAnalyse(error, std::move(encr_result));
    auto sign_res = SignResultAnalyse(error, std::move(sign_result));
    encrypt_res.analyse();
    sign_res.analyse();
    process_result_analyse(edit, infoBoard, encrypt_res, sign_res);
    if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
      edit->slotFillTextEditWithText(QString::fromStdString(*tmp));

#ifdef SMTP_SUPPORT
    infoBoard->resetOptionActionsMenu();
    infoBoard->addOptionalAction("Send Mail", [this]() {
      if (settings.value("sendMail/enable", false).toBool())
        new SendMailDialog(edit->curTextPage()->toPlainText(), this);
      else {
        QMessageBox::warning(nullptr, _("Function Disabled"),
                             _("Please go to the settings interface to "
                               "enable and configure this function."));
      }
    });
#endif

#ifdef ADVANCE_SUPPORT
    infoBoard->addOptionalAction("Shorten Ciphertext", [this]() {
      if (settings.value("general/serviceToken").toString().isEmpty())
        QMessageBox::warning(nullptr, _("Service Token Empty"),
                             _("Please go to the settings interface to set "
                               "Own Key and get Service Token."));
      else {
        shortenCryptText();
      }
    });
#endif

  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotDecryptVerify() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) return;

  QString plainText = edit->curTextPage()->toPlainText();

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
    auto thread =
        QThread::create([&]() { mCtx->verify(&text, nullptr, &tmp_v_result); });
    thread->start();
    while (thread->isRunning()) QApplication::processEvents();
    auto* checker = new UnknownSignersChecker(mCtx, tmp_v_result);
    checker->start();
    checker->deleteLater();
  }
#endif
  auto decrypted = std::make_unique<ByteArray>();
  process_operation(this, _("Decrypting and Verifying"), [&]() {
    try {
      auto buffer = text.toStdString();
      error = BasicOperator::GetInstance().DecryptVerify(buffer, decrypted,
                                                         d_result, v_result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto decrypt_res = DecryptResultAnalyse(error, std::move(d_result));
    auto verify_res = VerifyResultAnalyse(error, std::move(v_result));
    decrypt_res.analyse();
    verify_res.analyse();
    process_result_analyse(edit, infoBoard, decrypt_res, verify_res);
    if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR)
      edit->slotFillTextEditWithText(QString::fromStdString(*decrypted));

    //          if (verify_res.getStatus() >= 0) {
    //            infoBoard->resetOptionActionsMenu();
    //            infoBoard->addOptionalAction(
    //                "Show Verify Details", [this, error, v_result]() {
    //                  VerifyDetailsDialog(this, mCtx, mKeyList, error,
    //                  v_result);
    //                });
    //          }
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

/*
 * Append the selected (not checked!) Key(s) To Textedit
 */
void MainWindow::slotAppendSelectedKeys() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) {
    return;
  }

  auto exported = std::make_unique<ByteArray>();
  auto key_ids = mKeyList->getSelected();

  GpgKeyImportExportor::GetInstance().ExportKeys(key_ids, exported);
  edit->curTextPage()->append(QString::fromStdString(*exported));
}

void MainWindow::slotCopyMailAddressToClipboard() {
  auto key_ids = mKeyList->getSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (!key.good()) {
    QMessageBox::critical(nullptr, _("Error"), _("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(QString::fromStdString(key.email()));
}

void MainWindow::slotShowKeyDetails() {
  auto key_ids = mKeyList->getSelected();
  if (key_ids->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(key_ids->front());
  if (key.good()) {
    new KeyDetailsDialog(key, this);
  } else {
    QMessageBox::critical(nullptr, _("Error"), _("Key Not Found."));
  }
}

void MainWindow::refreshKeysFromKeyserver() {
  auto key_ids = mKeyList->getSelected();
  if (key_ids->empty()) return;

  auto* dialog = new KeyServerImportDialog(true, this);
  dialog->show();
  dialog->slotImport(key_ids);
}

void MainWindow::uploadKeyToServer() {
  auto key_ids = mKeyList->getSelected();
  auto* dialog = new KeyUploadDialog(key_ids, this);
  dialog->show();
  dialog->slotUpload();
}

void MainWindow::slotOpenFile(QString& path) { edit->slotOpenFile(path); }

void MainWindow::slotVersionUpgrade(const QString& currentVersion,
                                    const QString& latestVersion) {
  LOG(INFO) << _("called");
  if (currentVersion < latestVersion) {
    statusBar()->showMessage(
        QString(_("GpgFrontend Upgradeable (New Version: %1)."))
            .arg(latestVersion),
        30000);
    auto update_button = new QPushButton("Update GpgFrontend", this);
    connect(update_button, &QPushButton::clicked, [=]() {
      auto* about_dialog = new AboutDialog(2, this);
      about_dialog->show();
    });
    statusBar()->addPermanentWidget(update_button, 0);
  } else if (currentVersion > latestVersion) {
    QMessageBox::warning(
        this, _("Unreleased Version"),
        QString(
            _("This version(%1) has not been officially released and is not "
              "recommended for use in a production environment. <br/>"))
                .arg(currentVersion) +
            QString(
                _("You can download the latest version(%1) on Github Releases "
                  "Page.<br/>"))
                .arg(latestVersion));
  }
}

}  // namespace GpgFrontend::UI
