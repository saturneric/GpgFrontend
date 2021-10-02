/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

namespace GpgFrontend::UI {

void refresh_info_board(InfoBoardWidget* info_board,
                        int status,
                        const std::string& report_text) {
  if (status < 0)
    info_board->slotRefresh(QString::fromStdString(report_text),
                            INFO_ERROR_CRITICAL);
  else if (status > 0)
    info_board->slotRefresh(QString::fromStdString(report_text), INFO_ERROR_OK);
  else
    info_board->slotRefresh(QString::fromStdString(report_text),
                            INFO_ERROR_WARN);
}

void process_result_analyse(TextEdit* edit,
                            InfoBoardWidget* info_board,
                            const ResultAnalyse& result_analyse) {
  info_board->associateTabWidget(edit->tabWidget);
  info_board->associateFileTreeView(edit->curFilePage());
  refresh_info_board(info_board, result_analyse.getStatus(),
                     result_analyse.getResultReport());
}

void process_result_analyse(TextEdit* edit,
                            InfoBoardWidget* info_board,
                            const ResultAnalyse& result_analyse_a,
                            const ResultAnalyse& result_analyse_b) {
  info_board->associateTabWidget(edit->tabWidget);
  info_board->associateFileTreeView(edit->curFilePage());

  refresh_info_board(
      info_board,
      std::min(result_analyse_a.getStatus(), result_analyse_a.getStatus()),
      result_analyse_a.getResultReport() + result_analyse_a.getResultReport());
}

bool file_pre_check(QWidget* parent, const QString& path) {
  QFileInfo file_info(path);
  QFileInfo path_info(file_info.absolutePath());
  if (!file_info.isFile()) {
    QMessageBox::critical(parent, QApplication::tr("Error"),
                          QApplication::tr("Select a file before doing it."));
    return false;
  }
  if (!file_info.isReadable()) {
    QMessageBox::critical(parent, QApplication::tr("Error"),
                          QApplication::tr("No permission to read this file."));
    return false;
  }
  if (!path_info.isWritable()) {
    QMessageBox::critical(parent, QApplication::tr("Error"),
                          QApplication::tr("No permission to create file."));
    return false;
  }
  return true;
}

void process_operation(QWidget* parent,
                       std::string waiting_title,
                       std::function<void()> func) {
  GpgEncrResult result = nullptr;

  gpgme_error_t error;
  bool if_error = false;
  auto thread = QThread::create(func);
  QApplication::connect(thread, SIGNAL(finished()), thread,
                        SLOT(deleteLater()));
  thread->start();

  auto* dialog =
      new WaitingDialog(QString::fromStdString(waiting_title), parent);
  while (thread->isRunning()) {
    QApplication::processEvents();
  }
  dialog->close();
}

void MainWindow::slotFileEncrypt() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path))
    return;

  if (QFile::exists(path + ".asc")) {
    auto ret = QMessageBox::warning(
        this, tr("Warning"),
        tr("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel)
      return;
  }

  auto key_ids = mKeyList->getChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);
  if (keys->empty()) {
    QMessageBox::critical(this, tr("No Key Selected"), tr("No Key Selected"));
    return;
  }

  for (const auto& key : *keys) {
    if (!key.CanEncrActual()) {
      QMessageBox::information(
          this, tr("Invalid Operation"),
          tr("The selected key contains a key that does not actually have a "
             "encrypt usage.<br/>") +
              tr("<br/>For example the Following Key: <br/>") +
              QString::fromStdString(key.uids()->front().uid()));
      return;
    }
  }

  GpgEncrResult result = nullptr;
  GpgError error;
  bool if_error = false;
  process_operation(this, tr("Encrypting").toStdString(), [&]() {
    try {
      error = GpgFileOpera::GetInstance().EncryptFile(
          std::move(*keys), path.toStdString(), result);
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
    QMessageBox::critical(this, tr("Error"),
                          tr("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileDecrypt() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path))
    return;

  QString outFileName, fileExtension = QFileInfo(path).suffix();

  if (fileExtension == "asc" || fileExtension == "gpg") {
    int pos = path.lastIndexOf(QChar('.'));
    outFileName = path.left(pos);
  } else {
    outFileName = path + ".out";
  }

  if (QFile::exists(outFileName)) {
    auto ret = QMessageBox::warning(
        this, tr("Warning"),
        tr("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel)
      return;
  }

  GpgDecrResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;
  process_operation(this, tr("Decrypting").toStdString(), [&]() {
    try {
      error =
          GpgFileOpera::GetInstance().DecryptFile(path.toStdString(), result);
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
    QMessageBox::critical(this, tr("Error"),
                          tr("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileSign() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path))
    return;

  if (QFile::exists(path + ".sig")) {
    auto ret = QMessageBox::warning(
        this, tr("Warning"),
        tr("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel)
      return;
  }

  auto key_ids = mKeyList->getChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (keys->empty()) {
    QMessageBox::critical(this, tr("No Key Selected"), tr("No Key Selected"));
    return;
  }

  for (const auto& key : *keys) {
    if (!key.CanSignActual()) {
      QMessageBox::information(
          this, tr("Invalid Operation"),
          tr("The selected key contains a key that does not actually have a "
             "sign usage.<br/>") +
              tr("<br/>For example the Following Key: <br/>") +
              QString::fromStdString(key.uids()->front().uid()));
      return;
    }
  }

  GpgSignResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;

  process_operation(this, tr("Signing").toStdString(), [&]() {
    try {
      error = GpgFileOpera::GetInstance().SignFile(std::move(*keys),
                                                   path.toStdString(), result);
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
    QMessageBox::critical(this, tr("Error"),
                          tr("An error occurred during operation."));
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

  QFileInfo dataFileInfo(dataFilePath), signFileInfo(signFilePath);

  if (!dataFileInfo.isFile() || !signFileInfo.isFile()) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Please select the appropriate target file or signature file. "
           "Ensure that both are in this directory."));
    return;
  }
  if (!dataFileInfo.isReadable()) {
    QMessageBox::critical(this, tr("Error"),
                          tr("No permission to read target file."));
    return;
  }
  if (!fileInfo.isReadable()) {
    QMessageBox::critical(this, tr("Error"),
                          tr("No permission to read signature file."));
    return;
  }

  GpgVerifyResult result = nullptr;
  gpgme_error_t error;
  bool if_error = false;
  process_operation(this, tr("Verifying").toStdString(), [&]() {
    try {
      error = GpgFileOpera::GetInstance().VerifyFile(dataFilePath.toStdString(),
                                                     result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    auto resultAnalyse = VerifyResultAnalyse(error, std::move(result));
    resultAnalyse.analyse();
    process_result_analyse(edit, infoBoard, resultAnalyse);

    //    if (resultAnalyse->getStatus() >= 0) {
    //      infoBoard->resetOptionActionsMenu();
    //      infoBoard->addOptionalAction(
    //          "Show Verify Details", [this, error, result]() {
    //            VerifyDetailsDialog(this, mCtx, mKeyList, error, result);
    //          });
    //    }

    fileTreeView->update();
  } else {
    QMessageBox::critical(this, tr("Error"),
                          tr("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileEncryptSign() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path))
    return;

  if (QFile::exists(path + ".gpg")) {
    auto ret = QMessageBox::warning(
        this, tr("Warning"),
        tr("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel)
      return;
  }

  auto key_ids = mKeyList->getChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (keys->empty()) {
    QMessageBox::critical(this, tr("No Key Selected"), tr("No Key Selected"));
    return;
  }

  bool can_sign = false, can_encr = false;

  for (const auto& key : *keys) {
    bool key_can_sign = key.CanSignActual();
    bool key_can_encr = key.CanEncrActual();

    if (!key_can_sign && !key_can_encr) {
      QMessageBox::critical(
          nullptr, tr("Invalid KeyPair"),
          tr("The selected keypair cannot be used for signing and encryption "
             "at the same time.<br/>") +
              tr("<br/>For example the Following Key: <br/>") +
              QString::fromStdString(key.uids()->front().uid()));
      return;
    }

    if (key_can_sign)
      can_sign = true;
    if (key_can_encr)
      can_encr = true;
  }

  if (!can_encr) {
    QMessageBox::critical(nullptr, tr("Incomplete Operation"),
                          tr("None of the selected key pairs can provide the "
                             "encryption function."));
    return;
  }

  if (!can_sign) {
    QMessageBox::warning(nullptr, tr("Incomplete Operation"),
                         tr("None of the selected key pairs can provide the "
                            "signature function."));
  }

  GpgEncrResult encr_result = nullptr;
  GpgSignResult sign_result = nullptr;

  gpgme_error_t error;
  bool if_error = false;

  process_operation(this, tr("Encrypting and Signing").toStdString(), [&]() {
    try {
      error = GpgFileOpera::GetInstance().EncryptSignFile(
          std::move(*keys), path.toStdString(), encr_result, sign_result);
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
    QMessageBox::critical(this, tr("Error"),
                          tr("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileDecryptVerify() {
  auto fileTreeView = edit->slotCurPageFileTreeView();
  auto path = fileTreeView->getSelected();

  if (!file_pre_check(this, path))
    return;

  QString outFileName, fileExtension = QFileInfo(path).suffix();

  if (fileExtension == "asc" || fileExtension == "gpg") {
    int pos = path.lastIndexOf(QChar('.'));
    outFileName = path.left(pos);
  } else {
    outFileName = path + ".out";
  }

  GpgDecrResult d_result = nullptr;
  GpgVerifyResult v_result = nullptr;
  gpgme_error_t error;
  bool if_error = false;
  process_operation(this, tr("Decrypting and Verifying").toStdString(), [&]() {
    try {
      error = GpgFileOpera::GetInstance().DecryptVerifyFile(path.toStdString(),
                                                            d_result, v_result);
    } catch (const std::runtime_error& e) {
      if_error = true;
    }
  });

  if (!if_error) {
    infoBoard->associateFileTreeView(edit->curFilePage());

    auto decrypt_res = DecryptResultAnalyse(error, std::move(d_result));
    auto verify_res = VerifyResultAnalyse(error, std::move(v_result));
    decrypt_res.analyse();
    verify_res.analyse();
    process_result_analyse(edit, infoBoard, decrypt_res, verify_res);

    //    if (verify_res.getStatus() >= 0) {
    //      infoBoard->resetOptionActionsMenu();
    //      infoBoard->addOptionalAction(
    //          "Show Verify Details", [this, error, v_result]() {
    //            VerifyDetailsDialog(this, mCtx, mKeyList, error, v_result);
    //          });
    //    }

    fileTreeView->update();
  } else {
    QMessageBox::critical(this, tr("Error"),
                          tr("An error occurred during operation."));
    return;
  }
}

void MainWindow::slotFileEncryptCustom() {
  new FileEncryptionDialog(mKeyList->getChecked(),
                           FileEncryptionDialog::Encrypt, this);
}

void MainWindow::slotFileDecryptCustom() {
  auto key_ids = mKeyList->getChecked();
  new FileEncryptionDialog(mKeyList->getChecked(),
                           FileEncryptionDialog::Decrypt, this);
}

void MainWindow::slotFileSignCustom() {
  auto key_ids = mKeyList->getChecked();
  new FileEncryptionDialog(mKeyList->getChecked(), FileEncryptionDialog::Sign,
                           this);
}

void MainWindow::slotFileVerifyCustom() {
  auto key_ids = mKeyList->getChecked();
  new FileEncryptionDialog(mKeyList->getChecked(), FileEncryptionDialog::Verify,
                           this);
}

}  // namespace GpgFrontend::UI
