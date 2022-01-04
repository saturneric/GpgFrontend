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
#include "gpg/function/GpgFileOpera.h"
#include "gpg/function/GpgKeyGetter.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/settings/GlobalSettingStation.h"
#include "ui/widgets/SignersPicker.h"

namespace GpgFrontend::UI {

bool file_pre_check(QWidget* parent, const QString& path) {
  QFileInfo file_info(path);
  QFileInfo path_info(file_info.absolutePath());
  if (!file_info.isFile()) {
    QMessageBox::critical(parent, _("Error"),
                          _("Select a file before doing it."));
    return false;
  }
  if (!file_info.isReadable()) {
    QMessageBox::critical(parent, _("Error"),
                          _("No permission to read this file."));
    return false;
  }
  if (!path_info.isWritable()) {
    QMessageBox::critical(parent, _("Error"),
                          _("No permission to create file."));
    return false;
  }
  return true;
}

void MainWindow::slotFileEncrypt() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path)) return;

  // check selected keys
  auto key_ids = mKeyList->getChecked();
  GpgEncrResult result = nullptr;
  GpgError error;
  bool if_error = false;

  // Detect ascii mode
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  bool non_ascii_when_export = true;
  try {
    non_ascii_when_export = settings.lookup("general.non_ascii_when_export");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("non_ascii_when_export");
  }

  auto _channel = GPGFRONTEND_DEFAULT_CHANNEL;
  auto _extension = ".asc";
  if (non_ascii_when_export) {
    _channel = GPGFRONTEND_NON_ASCII_CHANNEL;
    _extension = ".gpg";
  }

  auto out_path = path + _extension;

  if (QFile::exists(out_path)) {
    boost::filesystem::path _out_path = out_path.toStdString();
    auto out_file_name = boost::format(_("The target file %1% already exists, "
                                         "do you need to overwrite it?")) %
                         _out_path.filename();
    auto ret =
        QMessageBox::warning(this, _("Warning"), out_file_name.str().c_str(),
                             QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  if (key_ids->empty()) {
    // Symmetric Encrypt
    auto ret = QMessageBox::information(
        this, _("Symmetric Encryption"),
        _("No Key Selected. Do you want to encrypt with a "
          "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;

    process_operation(this, _("Symmetrically Encrypting"), [&]() {
      try {
        error = GpgFrontend::GpgFileOpera::EncryptFileSymmetric(
            path.toStdString(), out_path.toStdString(), result, _channel);
      } catch (const std::runtime_error& e) {
        if_error = true;
      }
    });
  } else {
    auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

    // check key abilities
    for (const auto& key : *p_keys) {
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

    process_operation(this, _("Encrypting"), [&]() {
      try {
        error =
            GpgFileOpera::EncryptFile(std::move(p_keys), path.toStdString(),
                                      out_path.toStdString(), result, _channel);
      } catch (const std::runtime_error& e) {
        if_error = true;
      }
    });
  }

  if (!if_error) {
    auto resultAnalyse = EncryptResultAnalyse(error, std::move(result));
    resultAnalyse.analyse();
    process_result_analyse(edit, infoBoard, resultAnalyse);
    fileTreeView->update();
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileDecrypt() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path)) return;

  boost::filesystem::path out_path = path.toStdString();

  if (out_path.extension() == ".asc" || out_path.extension() == ".gpg") {
    out_path = out_path.parent_path() / out_path.stem();
  } else {
    out_path += ".out";
  }

  if (exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  GpgDecrResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;
  process_operation(this, _("Decrypting"), [&]() {
    try {
      error = GpgFileOpera::DecryptFile(path.toStdString(), out_path.string(),
                                        result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto resultAnalyse = DecryptResultAnalyse(error, std::move(result));
    resultAnalyse.analyse();
    process_result_analyse(edit, infoBoard, resultAnalyse);

    fileTreeView->update();
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileSign() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path)) return;

  auto key_ids = mKeyList->getChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (keys->empty()) {
    QMessageBox::critical(
        this, _("No Key Selected"),
        _("Please select the key in the key toolbox on the right."));
    return;
  }

  for (const auto& key : *keys) {
    if (!key.CanSignActual()) {
      QMessageBox::information(
          this, _("Invalid Operation"),
          QString(_("The selected key contains a key that does not actually "
                    "have a sign usage.")) +
              "<br/><br/>" + _("for example the Following Key:") + " <br/>" +
              QString::fromStdString(key.uids()->front().uid()));
      return;
    }
  }

  // Detect ascii mode
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  bool non_ascii_when_export = true;
  try {
    non_ascii_when_export = settings.lookup("general.non_ascii_when_export");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("non_ascii_when_export");
  }

  auto _channel = GPGFRONTEND_DEFAULT_CHANNEL;
  auto _extension = ".asc";
  if (non_ascii_when_export) {
    _channel = GPGFRONTEND_NON_ASCII_CHANNEL;
    _extension = ".sig";
  }

  boost::filesystem::path in_path = path.toStdString();
  auto sig_file_path = in_path;
  sig_file_path += _extension;
  if (exists(sig_file_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        QString(_("The signature file \"%1\" exists, "
                  "do you need to overwrite it?"))
            .arg(sig_file_path.filename().string().c_str()),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  GpgSignResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;

  process_operation(this, _("Signing"), [&]() {
    try {
      error = GpgFileOpera::SignFile(std::move(keys), in_path.string(),
                                     sig_file_path.string(), result, _channel);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto resultAnalyse = SignResultAnalyse(error, std::move(result));
    resultAnalyse.analyse();
    process_result_analyse(edit, infoBoard, resultAnalyse);

    fileTreeView->update();

  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }

  fileTreeView->update();
}

void MainWindow::slotFileVerify() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  boost::filesystem::path in_path = path.toStdString();
  boost::filesystem::path sign_file_path = in_path, data_file_path;

  // Detect ascii mode
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  bool non_ascii_when_export = true;
  try {
    non_ascii_when_export = settings.lookup("general.non_ascii_when_export");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("non_ascii_when_export");
  }

  auto _channel = GPGFRONTEND_DEFAULT_CHANNEL;
  if (non_ascii_when_export) {
    _channel = GPGFRONTEND_NON_ASCII_CHANNEL;
  }

  if (in_path.extension() == ".gpg") {
    swap(data_file_path, sign_file_path);
  } else if (in_path.extension() == ".sig" || in_path.extension() == ".asc") {
    data_file_path = sign_file_path.parent_path() / sign_file_path.stem();
  }

  LOG(INFO) << "sign_file_path" << sign_file_path << sign_file_path.extension();

  if (in_path.extension() != ".gpg") {
    bool ok;
    QString text = QInputDialog::getText(this, _("Origin file to verify"),
                                         _("Filepath"), QLineEdit::Normal,
                                         data_file_path.string().c_str(), &ok);
    if (ok && !text.isEmpty()) {
      data_file_path = text.toStdString();
    } else {
      return;
    }
  }

  if (!is_regular_file(data_file_path) ||
      (!sign_file_path.empty() && !is_regular_file(sign_file_path))) {
    QMessageBox::critical(
        this, _("Error"),
        _("Please select the appropriate origin file or signature file. "
          "Ensure that both are in this directory."));
    return;
  }

  DLOG(INFO) << "data path" << data_file_path;
  DLOG(INFO) << "sign path" << sign_file_path;

  GpgVerifyResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;
  process_operation(this, _("Verifying"), [&]() {
    try {
      error = GpgFileOpera::VerifyFile(
          data_file_path.string(), sign_file_path.string(), result, _channel);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto result_analyse = VerifyResultAnalyse(error, result);
    result_analyse.analyse();
    process_result_analyse(edit, infoBoard, result_analyse);

    if (result_analyse.getStatus() == -2)
      import_unknown_key_from_keyserver(this, result_analyse);

    if (result_analyse.getStatus() >= 0)
      show_verify_details(this, infoBoard, error, result);

    fileTreeView->update();
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileEncryptSign() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path)) return;

  // check selected keys
  auto key_ids = mKeyList->getChecked();
  auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (p_keys->empty()) {
    QMessageBox::critical(
        this, _("No Key Selected"),
        _("Please select the key in the key toolbox on the right."));
    return;
  }

  // check key abilities
  for (const auto& key : *p_keys) {
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

  // Detect ascii mode
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  bool non_ascii_when_export = true;
  try {
    non_ascii_when_export = settings.lookup("general.non_ascii_when_export");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("non_ascii_when_export");
  }

  auto _channel = GPGFRONTEND_DEFAULT_CHANNEL;
  auto _extension = ".asc";
  if (non_ascii_when_export) {
    _channel = GPGFRONTEND_NON_ASCII_CHANNEL;
    _extension = ".gpg";
  }

  boost::filesystem::path out_path = path.toStdString() + _extension;

  if (exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  auto signersPicker = new SignersPicker(this);
  QEventLoop loop;
  connect(signersPicker, SIGNAL(finished(int)), &loop, SLOT(quit()));
  loop.exec();

  auto signer_key_ids = signersPicker->getCheckedSigners();
  auto p_signer_keys = GpgKeyGetter::GetInstance().GetKeys(signer_key_ids);

  GpgEncrResult encr_result = nullptr;
  GpgSignResult sign_result = nullptr;

  gpgme_error_t error;
  bool if_error = false;

  process_operation(this, _("Encrypting and Signing"), [&]() {
    try {
      error = GpgFileOpera::EncryptSignFile(
          std::move(p_keys), std::move(p_signer_keys), path.toStdString(),
          out_path.string(), encr_result, sign_result, _channel);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto encrypt_result = EncryptResultAnalyse(error, std::move(encr_result));
    auto sign_res = SignResultAnalyse(error, std::move(sign_result));
    encrypt_result.analyse();
    sign_res.analyse();
    process_result_analyse(edit, infoBoard, encrypt_result, sign_res);

    fileTreeView->update();

  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileDecryptVerify() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path)) return;

  boost::filesystem::path in_path(path.toStdString());
  boost::filesystem::path out_path = in_path;
  if (in_path.extension() == ".asc" || in_path.extension() == ".gpg") {
    out_path = in_path.parent_path() / out_path.stem();
  } else {
    out_path += ".out";
  }
  LOG(INFO) << "out path" << out_path;

  if (QFile::exists(out_path.string().c_str())) {
    auto ret =
        QMessageBox::warning(this, _("Warning"),
                             QString(_("The output file %1 already exists, do "
                                       "you need to overwrite it?"))
                                 .arg(out_path.filename().string().c_str()),
                             QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  GpgDecrResult d_result = nullptr;
  GpgVerifyResult v_result = nullptr;
  gpgme_error_t error;
  bool if_error = false;
  process_operation(this, _("Decrypting and Verifying"), [&]() {
    try {
      error = GpgFileOpera::DecryptVerifyFile(
          path.toStdString(), out_path.string(), d_result, v_result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto decrypt_res = DecryptResultAnalyse(error, std::move(d_result));
    auto verify_res = VerifyResultAnalyse(error, v_result);
    decrypt_res.analyse();
    verify_res.analyse();
    process_result_analyse(edit, infoBoard, decrypt_res, verify_res);

    if (verify_res.getStatus() == -2)
      import_unknown_key_from_keyserver(this, verify_res);

    if (verify_res.getStatus() >= 0)
      show_verify_details(this, infoBoard, error, v_result);

    fileTreeView->update();
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

}  // namespace GpgFrontend::UI
