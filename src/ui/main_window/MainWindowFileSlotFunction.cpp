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

void MainWindow::slotFileEncrypt() {

    auto fileTreeView = edit->slotCurPageFileTreeView();
    auto path = fileTreeView->getSelected();

    QFileInfo fileInfo(path);
    QFileInfo pathInfo(fileInfo.absolutePath());

    if (!fileInfo.isFile()) {
        QMessageBox::critical(this, tr("Error"), tr("Select a file before doing it."));
        return;
    }
    if (!fileInfo.isReadable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to read this file."));
        return;
    }
    if (!pathInfo.isWritable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to create file."));
        return;
    }
    if (QFile::exists(path + ".asc")) {
        auto ret = QMessageBox::warning(this,
                                        tr("Warning"),
                                        tr("The target file already exists, do you need to overwrite it?"),
                                        QMessageBox::Ok | QMessageBox::Cancel);

        if (ret == QMessageBox::Cancel)
            return;
    }

    QVector<GpgKey> keys;

    mKeyList->getCheckedKeys(keys);

    if (keys.empty()) {
        QMessageBox::critical(this, tr("No Key Selected"), tr("No Key Selected"));
        return;
    }

    for (const auto &key : keys) {
        if (!GpgME::GpgContext::checkIfKeyCanEncr(key)) {
            QMessageBox::information(this,
                                     tr("Invalid Operation"),
                                     tr("The selected key contains a key that does not actually have a encrypt usage.<br/>")
                                     + tr("<br/>For example the Following Key: <br/>") + key.uids.first().uid);
            return;

        }
    }

    gpgme_encrypt_result_t result;

    gpgme_error_t error;
    bool if_error = false;
    auto thread = QThread::create([&]() {
        try {
            error = GpgFileOpera::encryptFile(mCtx, keys, path, &result);
        } catch (const std::runtime_error &e) {
            if_error = true;
        }
    });
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

    auto *dialog = new WaitingDialog(tr("Encrypting"), this);
    while (thread->isRunning()) {
        QApplication::processEvents();
    }

    dialog->close();
    if (!if_error) {
        auto resultAnalyse = new EncryptResultAnalyse(error, result);
        auto &reportText = resultAnalyse->getResultReport();
        infoBoard->associateTabWidget(edit->tabWidget);
        infoBoard->associateFileTreeView(edit->curFilePage());

        if (resultAnalyse->getStatus() < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (resultAnalyse->getStatus() > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        delete resultAnalyse;

        fileTreeView->update();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
        return;
    }
}

void MainWindow::slotFileDecrypt() {

    auto fileTreeView = edit->slotCurPageFileTreeView();
    auto path = fileTreeView->getSelected();

    QFileInfo fileInfo(path);
    QFileInfo pathInfo(fileInfo.absolutePath());
    if (!fileInfo.isFile()) {
        QMessageBox::critical(this, tr("Error"), tr("Select a file before doing it."));
        return;
    }
    if (!fileInfo.isReadable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to read this file."));
        return;
    }
    if (!pathInfo.isWritable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to create file."));
        return;
    }

    QString outFileName, fileExtension = fileInfo.suffix();

    if (fileExtension == "asc" || fileExtension == "gpg") {
        int pos = path.lastIndexOf(QChar('.'));
        outFileName = path.left(pos);
    } else {
        outFileName = path + ".out";
    }

    if (QFile::exists(outFileName)) {
        auto ret = QMessageBox::warning(this,
                                        tr("Warning"),
                                        tr("The target file already exists, do you need to overwrite it?"),
                                        QMessageBox::Ok | QMessageBox::Cancel);

        if (ret == QMessageBox::Cancel)
            return;
    }

    gpgme_decrypt_result_t result;
    gpgme_error_t error;
    bool if_error = false;

    auto thread = QThread::create([&]() {
        try {
            error = GpgFileOpera::decryptFile(mCtx, path, &result);
        } catch (const std::runtime_error &e) {
            if_error = true;
        }
    });
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

    auto *dialog = new WaitingDialog("Decrypting", this);
    while (thread->isRunning()) {
        QApplication::processEvents();
    }

    dialog->close();

    if (!if_error) {
        auto resultAnalyse = new DecryptResultAnalyse(mCtx, error, result);
        auto &reportText = resultAnalyse->getResultReport();
        infoBoard->associateTabWidget(edit->tabWidget);
        infoBoard->associateFileTreeView(edit->curFilePage());

        if (resultAnalyse->getStatus() < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (resultAnalyse->getStatus() > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        delete resultAnalyse;

        fileTreeView->update();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
        return;
    }


}

void MainWindow::slotFileSign() {

    auto fileTreeView = edit->slotCurPageFileTreeView();
    auto path = fileTreeView->getSelected();

    QFileInfo fileInfo(path);
    QFileInfo pathInfo(fileInfo.absolutePath());

    if (!fileInfo.isFile()) {
        QMessageBox::critical(this, tr("Error"), tr("Select a file before doing it."));
        return;
    }
    if (!fileInfo.isReadable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to read this file."));
        return;
    }
    if (!pathInfo.isWritable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to create file."));
        return;
    }

    if (QFile::exists(path + ".sig")) {
        auto ret = QMessageBox::warning(this,
                                        tr("Warning"),
                                        tr("The target file already exists, do you need to overwrite it?"),
                                        QMessageBox::Ok | QMessageBox::Cancel);

        if (ret == QMessageBox::Cancel)
            return;
    }

    QVector<GpgKey> keys;

    mKeyList->getCheckedKeys(keys);

    if (keys.empty()) {
        QMessageBox::critical(this, tr("No Key Selected"), tr("No Key Selected"));
        return;
    }

    for (const auto &key : keys) {
        if (!GpgME::GpgContext::checkIfKeyCanEncr(key)) {
            QMessageBox::information(this,
                                     tr("Invalid Operation"),
                                     tr("The selected key contains a key that does not actually have a encrypt usage.<br/>")
                                     + tr("<br/>For example the Following Key: <br/>") + key.uids.first().uid);
            return;

        }
    }

    gpgme_sign_result_t result;
    gpgme_error_t error;
    bool if_error = false;

    auto thread = QThread::create([&]() {
        try {
            error = GpgFileOpera::signFile(mCtx, keys, path, &result);
        } catch (const std::runtime_error &e) {
            if_error = true;
        }
    });
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

    auto *dialog = new WaitingDialog(tr("Signing"), this);
    while (thread->isRunning()) {
        QApplication::processEvents();
    }

    dialog->close();

    if (!if_error) {

        auto resultAnalyse = new SignResultAnalyse(error, result);
        auto &reportText = resultAnalyse->getResultReport();
        infoBoard->associateTabWidget(edit->tabWidget);
        infoBoard->associateFileTreeView(edit->curFilePage());

        if (resultAnalyse->getStatus() < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (resultAnalyse->getStatus() > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        delete resultAnalyse;

        fileTreeView->update();

    } else {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
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
        QMessageBox::critical(this, tr("Error"),
                              tr("Please select the appropriate target file or signature file. Ensure that both are in this directory."));
        return;
    }
    if (!dataFileInfo.isReadable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to read target file."));
        return;
    }
    if (!fileInfo.isReadable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to read signature file."));
        return;
    }

    gpgme_verify_result_t result;

    gpgme_error_t error;
    bool if_error = false;
    auto thread = QThread::create([&]() {
        try {
            error = GpgFileOpera::verifyFile(mCtx, dataFilePath, &result);
        } catch (const std::runtime_error &e) {
            if_error = true;
        }
    });
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

    auto *dialog = new WaitingDialog(tr("Verifying"), this);
    while (thread->isRunning()) {
        QApplication::processEvents();
    }
    dialog->close();

    if (!if_error) {
        auto resultAnalyse = new VerifyResultAnalyse(mCtx, error, result);
        auto &reportText = resultAnalyse->getResultReport();
        infoBoard->associateTabWidget(edit->tabWidget);
        infoBoard->associateFileTreeView(edit->curFilePage());

        if (resultAnalyse->getStatus() < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (resultAnalyse->getStatus() > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        if (resultAnalyse->getStatus() >= 0) {
            infoBoard->resetOptionActionsMenu();
            infoBoard->addOptionalAction("Show Verify Details", [this, error, result]() {
                VerifyDetailsDialog(this, mCtx, mKeyList, error, result);
            });
        }

        delete resultAnalyse;

        fileTreeView->update();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
        return;
    }
}

void MainWindow::slotFileEncryptSign() {
    auto fileTreeView = edit->slotCurPageFileTreeView();
    auto path = fileTreeView->getSelected();

    QFileInfo fileInfo(path);
    QFileInfo pathInfo(fileInfo.absolutePath());

    if (!fileInfo.isFile()) {
        QMessageBox::critical(this, tr("Error"), tr("Select a file before doing it."));
        return;
    }
    if (!fileInfo.isReadable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to read this file."));
        return;
    }
    if (!pathInfo.isWritable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to create file."));
        return;
    }
    if (QFile::exists(path + ".gpg")) {
        auto ret = QMessageBox::warning(this,
                                        tr("Warning"),
                                        tr("The target file already exists, do you need to overwrite it?"),
                                        QMessageBox::Ok | QMessageBox::Cancel);

        if (ret == QMessageBox::Cancel)
            return;
    }

    QVector<GpgKey> keys;

    mKeyList->getCheckedKeys(keys);

    if (keys.empty()) {
        QMessageBox::critical(this, tr("No Key Selected"), tr("No Key Selected"));
        return;
    }

    bool can_sign = false, can_encr = false;

    for (const auto &key : keys) {
        bool key_can_sign = GpgME::GpgContext::checkIfKeyCanSign(key);
        bool key_can_encr = GpgME::GpgContext::checkIfKeyCanEncr(key);

        if (!key_can_sign && !key_can_encr) {
            QMessageBox::critical(nullptr,
                                  tr("Invalid KeyPair"),
                                  tr("The selected keypair cannot be used for signing and encryption at the same time.<br/>")
                                  + tr("<br/>For example the Following Key: <br/>") + key.uids.first().uid);
            return;
        }

        if (key_can_sign) can_sign = true;
        if (key_can_encr) can_encr = true;
    }

    if (!can_encr) {
        QMessageBox::critical(nullptr,
                              tr("Incomplete Operation"),
                              tr("None of the selected key pairs can provide the encryption function."));
        return;
    }

    if (!can_sign) {
        QMessageBox::warning(nullptr,
                             tr("Incomplete Operation"),
                             tr("None of the selected key pairs can provide the signature function."));
    }

    gpgme_encrypt_result_t encr_result = nullptr;
    gpgme_sign_result_t sign_result = nullptr;

    gpgme_error_t error;
    bool if_error = false;

    auto thread = QThread::create([&]() {
        try {
            error = GpgFileOpera::encryptSignFile(mCtx, keys, path, &encr_result, &sign_result);
        } catch (const std::runtime_error &e) {
            if_error = true;
        }
    });
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

    auto *dialog = new WaitingDialog(tr("Encrypting and Signing"), this);
    while (thread->isRunning()) {
        QApplication::processEvents();
    }
    dialog->close();

    if (!if_error) {

        auto resultAnalyseEncr = new EncryptResultAnalyse(error, encr_result);
        auto resultAnalyseSign = new SignResultAnalyse(error, sign_result);
        int status = std::min(resultAnalyseEncr->getStatus(), resultAnalyseSign->getStatus());
        auto reportText = resultAnalyseEncr->getResultReport() + resultAnalyseSign->getResultReport();

        infoBoard->associateFileTreeView(edit->curFilePage());

        if (status < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (status > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        delete resultAnalyseEncr;
        delete resultAnalyseSign;

        fileTreeView->update();

    } else {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
        return;
    }
}

void MainWindow::slotFileDecryptVerify() {
    auto fileTreeView = edit->slotCurPageFileTreeView();
    auto path = fileTreeView->getSelected();

    QFileInfo fileInfo(path);
    QFileInfo pathInfo(fileInfo.absolutePath());
    if (!fileInfo.isFile()) {
        QMessageBox::critical(this, tr("Error"), tr("Select a file(.gpg/.asc) before doing it."));
        return;
    }
    if (!fileInfo.isReadable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to read this file."));
        return;
    }
    if (!pathInfo.isWritable()) {
        QMessageBox::critical(this, tr("Error"), tr("No permission to create file."));
        return;
    }

    QString outFileName, fileExtension = fileInfo.suffix();

    if (fileExtension == "asc" || fileExtension == "gpg") {
        int pos = path.lastIndexOf(QChar('.'));
        outFileName = path.left(pos);
    } else {
        outFileName = path + ".out";
    }

    gpgme_decrypt_result_t d_result = nullptr;
    gpgme_verify_result_t v_result = nullptr;

    gpgme_error_t error;
    bool if_error = false;

    auto thread = QThread::create([&]() {
        try {
            error = GpgFileOpera::decryptVerifyFile(mCtx, path, &d_result, &v_result);
        } catch (const std::runtime_error &e) {
            if_error = true;
        }
    });
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();


    auto *dialog = new WaitingDialog(tr("Decrypting and Verifying"), this);
    while (thread->isRunning()) {
        QApplication::processEvents();
    }
    dialog->close();

    if (!if_error) {
        infoBoard->associateFileTreeView(edit->curFilePage());

        auto resultAnalyseDecrypt = new DecryptResultAnalyse(mCtx, error, d_result);
        auto resultAnalyseVerify = new VerifyResultAnalyse(mCtx, error, v_result);

        int status = std::min(resultAnalyseDecrypt->getStatus(), resultAnalyseVerify->getStatus());
        auto &reportText = resultAnalyseDecrypt->getResultReport() + resultAnalyseVerify->getResultReport();
        if (status < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (status > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        if (resultAnalyseVerify->getStatus() >= 0) {
            infoBoard->resetOptionActionsMenu();
            infoBoard->addOptionalAction("Show Verify Details", [this, error, v_result]() {
                VerifyDetailsDialog(this, mCtx, mKeyList, error, v_result);
            });
        }
        delete resultAnalyseDecrypt;
        delete resultAnalyseVerify;

        fileTreeView->update();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
        return;
    }
}

void MainWindow::slotFileEncryptCustom() {
    QStringList *keyList;
    keyList = mKeyList->getChecked();
    new FileEncryptionDialog(mCtx, *keyList, FileEncryptionDialog::Encrypt, this);
}

void MainWindow::slotFileDecryptCustom() {
    QStringList *keyList;
    keyList = mKeyList->getChecked();
    new FileEncryptionDialog(mCtx, *keyList, FileEncryptionDialog::Decrypt, this);
}

void MainWindow::slotFileSignCustom() {
    QStringList *keyList;
    keyList = mKeyList->getChecked();
    new FileEncryptionDialog(mCtx, *keyList, FileEncryptionDialog::Sign, this);
}

void MainWindow::slotFileVerifyCustom() {
    QStringList *keyList;
    keyList = mKeyList->getChecked();
    new FileEncryptionDialog(mCtx, *keyList, FileEncryptionDialog::Verify, this);
}
