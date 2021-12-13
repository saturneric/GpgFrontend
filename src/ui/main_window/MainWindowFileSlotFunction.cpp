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

  if (QFile::exists(path + ".asc")) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  auto key_ids = mKeyList->getChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);
  if (keys->empty()) {
    QMessageBox::critical(this, _("No Key Selected"), _("No Key Selected"));
    return;
  }

  for (const auto& key : *keys) {
    if (!key.CanEncrActual()) {
      QMessageBox::information(
          this, _("Invalid Operation"),
          QString(
              _("The selected key contains a key that does not actually have a "
                "encrypt usage.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.uids()->front().uid()));
      return;
    }
  }

  GpgEncrResult result = nullptr;
  GpgError error;
  bool if_error = false;
  process_operation(this, _("Encrypting"), [&]() {
    try {
      error = GpgFileOpera::EncryptFile(std::move(keys), path.toStdString(),
                                        result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

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

  QString outFileName, fileExtension = QFileInfo(path).suffix();

  if (fileExtension == "asc" || fileExtension == "gpg") {
    int pos = path.lastIndexOf(QChar('.'));
    outFileName = path.left(pos);
  } else {
    outFileName = path + ".out";
  }

  if (QFile::exists(outFileName)) {
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
      error = GpgFileOpera::DecryptFile(path.toStdString(), result);
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
    QMessageBox::critical(this, _("No Key Selected"), _("No Key Selected"));
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

  auto sig_file_path = boost::filesystem::path(path.toStdString() + ".sig");
  if (QFile::exists(sig_file_path.string().c_str())) {
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
      error =
          GpgFileOpera::SignFile(std::move(keys), path.toStdString(), result);
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

  QFileInfo fileInfo(path);

  QString signFilePath, dataFilePath;

  if (fileInfo.suffix() == "gpg") {
    dataFilePath = path;
    signFilePath = path;
  } else if (fileInfo.suffix() == "sig") {
    int pos = path.lastIndexOf(QChar('.'));
    dataFilePath = path.left(pos);
    signFilePath = path;
  } else {
    dataFilePath = path;
    signFilePath = path + ".sig";
  }

  if (fileInfo.suffix() != "gpg") {
    bool ok;
    QString text =
        QInputDialog::getText(this, _("Origin file to verify"), _("Filepath"),
                              QLineEdit::Normal, dataFilePath, &ok);
    if (ok && !text.isEmpty()) {
      dataFilePath = text;
    } else {
      return;
    }
  }

  QFileInfo dataFileInfo(dataFilePath), signFileInfo(signFilePath);

  if (!dataFileInfo.isFile() || !signFileInfo.isFile()) {
    QMessageBox::critical(
        this, _("Error"),
        _("Please select the appropriate origin file or signature file. "
          "Ensure that both are in this directory."));
    return;
  }
  if (!dataFileInfo.isReadable()) {
    QMessageBox::critical(this, _("Error"),
                          _("No permission to read target file."));
    return;
  }
  if (!fileInfo.isReadable()) {
    QMessageBox::critical(this, _("Error"),
                          _("No permission to read signature file."));
    return;
  }

  GpgVerifyResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;
  process_operation(this, _("Verifying"), [&]() {
    try {
      error = GpgFileOpera::VerifyFile(dataFilePath.toStdString(), result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto result_analyse = VerifyResultAnalyse(error, std::move(result));
    result_analyse.analyse();
    process_result_analyse(edit, infoBoard, result_analyse);

    if (result_analyse.getStatus() == -2)
      import_unknown_key_from_keyserver(this, result_analyse);

    if (result_analyse.getStatus() >= 0)
      show_verify_details(this, infoBoard, error, result_analyse);

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

  if (QFile::exists(path + ".gpg")) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  auto key_ids = mKeyList->getChecked();
  auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (p_keys->empty()) {
    QMessageBox::critical(this, _("No Key Selected"), _("No Key Selected"));
    return;
  }

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

  auto signersPicker = new SignersPicker(this);
  QEventLoop loop;
  connect(signersPicker, SIGNAL(finished(int)), &loop, SLOT(quit()));
  loop.exec();

  auto signer_key_ids = signersPicker->getCheckedSigners();
  auto p_signer_keys = GpgKeyGetter::GetInstance().GetKeys(signer_key_ids);

  for (const auto& key : *p_keys) {
    LOG(INFO) << "Keys " << key.email();
  }

  for (const auto& signer : *p_signer_keys) {
    LOG(INFO) << "Signers " << signer.email();
  }

  GpgEncrResult encr_result = nullptr;
  GpgSignResult sign_result = nullptr;

  gpgme_error_t error;
  bool if_error = false;

  process_operation(this, _("Encrypting and Signing"), [&]() {
    try {
      error = GpgFileOpera::EncryptSignFile(
          std::move(p_keys), std::move(p_signer_keys), path.toStdString(),
          encr_result, sign_result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto encrypt_res = EncryptResultAnalyse(error, std::move(encr_result));
    auto sign_res = SignResultAnalyse(error, std::move(sign_result));
    encrypt_res.analyse();
    sign_res.analyse();
    process_result_analyse(edit, infoBoard, encrypt_res, sign_res);

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

  boost::filesystem::path out_path(path.toStdString());
  if (out_path.extension() == ".asc" || out_path.extension() == ".gpg") {
    out_path = out_path.parent_path() / out_path.filename();
  } else {
    out_path = out_path.replace_extension(".out").string();
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
      error = GpgFileOpera::DecryptVerifyFile(path.toStdString(), d_result,
                                              v_result);
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

    if (verify_res.getStatus() == -2)
      import_unknown_key_from_keyserver(this, verify_res);

    if (verify_res.getStatus() >= 0)
      show_verify_details(this, infoBoard, error, verify_res);

    fileTreeView->update();
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during operation."));
    return;
  }
}

}  // namespace GpgFrontend::UI
