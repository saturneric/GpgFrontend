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

void MainWindow::slotEncrypt() {

    if (edit->tabCount() == 0) return;

    if (edit->slotCurPageTextEdit() != nullptr) {

        QVector<GpgKey> keys;
        mKeyList->getCheckedKeys(keys);

        if (keys.count() == 0) {
            QMessageBox::critical(nullptr, tr("No Key Selected"), tr("No Key Selected"));
            return;
        }

        for (const auto &key : keys) {
            if (!GpgME::GpgContext::checkIfKeyCanEncr(key)) {
                QMessageBox::information(nullptr,
                                         tr("Invalid Operation"),
                                         tr("The selected key contains a key that does not actually have a encrypt usage.<br/>")
                                         + tr("<br/>For example the Following Key: <br/>") + key.uids.first().uid);
                return;

            }
        }

        auto *tmp = new QByteArray();

        gpgme_encrypt_result_t result = nullptr;
        auto error = mCtx->encrypt(keys, edit->curTextPage()->toPlainText().toUtf8(), tmp, &result);

        auto resultAnalyse = new EncryptResultAnalyse(error, result);
        auto &reportText = resultAnalyse->getResultReport();

        auto *tmp2 = new QString(*tmp);
        edit->slotFillTextEditWithText(*tmp2);
        infoBoard->associateTextEdit(edit->curTextPage());

        if (resultAnalyse->getStatus() < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (resultAnalyse->getStatus() > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        delete resultAnalyse;
    } else if (edit->slotCurPageFileTreeView() != nullptr) {
        this->slotFileEncrypt();
    }
}

void MainWindow::slotSign() {

    if (edit->tabCount() == 0) return;

    if (edit->slotCurPageTextEdit() != nullptr) {

        QVector<GpgKey> keys;

        mKeyList->getPrivateCheckedKeys(keys);

        if (keys.isEmpty()) {
            QMessageBox::critical(this, tr("No Key Selected"), tr("No Key Selected"));
            return;
        }

        for (const auto &key : keys) {
            if (!GpgME::GpgContext::checkIfKeyCanSign(key)) {
                QMessageBox::information(this,
                                         tr("Invalid Operation"),
                                         tr("The selected key contains a key that does not actually have a signature usage.<br/>")
                                         + tr("<br/>For example the Following Key: <br/>") + key.uids.first().uid);
                return;
            }
        }

        auto *tmp = new QByteArray();

        gpgme_sign_result_t result = nullptr;

        auto error = mCtx->sign(keys, edit->curTextPage()->toPlainText().toUtf8(), tmp, false, &result);
        infoBoard->associateTextEdit(edit->curTextPage());
        edit->slotFillTextEditWithText(QString::fromUtf8(*tmp));

        auto resultAnalyse = new SignResultAnalyse(error, result);

        auto &reportText = resultAnalyse->getResultReport();
        if (resultAnalyse->getStatus() < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (resultAnalyse->getStatus() > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        delete resultAnalyse;
    } else if (edit->slotCurPageFileTreeView() != nullptr) {
        this->slotFileSign();
    }
}

void MainWindow::slotDecrypt() {
    if (edit->tabCount() == 0) return;

    if (edit->slotCurPageTextEdit() != nullptr) {

        auto *decrypted = new QByteArray();
        QByteArray text = edit->curTextPage()->toPlainText().toUtf8();
        GpgME::GpgContext::preventNoDataErr(&text);

        gpgme_decrypt_result_t result = nullptr;
        // try decrypt, if fail do nothing, especially don't replace text
        auto error = mCtx->decrypt(text, decrypted, &result);
        infoBoard->associateTextEdit(edit->curTextPage());

        if (gpgme_err_code(error) == GPG_ERR_NO_ERROR)
            edit->slotFillTextEditWithText(QString::fromUtf8(*decrypted));

        auto resultAnalyse = new DecryptResultAnalyse(mCtx, error, result);

        auto &reportText = resultAnalyse->getResultReport();
        if (resultAnalyse->getStatus() < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (resultAnalyse->getStatus() > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        delete resultAnalyse;
    } else if (edit->slotCurPageFileTreeView() != nullptr) {
        this->slotFileDecrypt();
    }
}

void MainWindow::slotFind() {
    if (edit->tabCount() == 0 || edit->curTextPage() == nullptr) {
        return;
    }

    // At first close verifynotification, if existing
    edit->slotCurPageTextEdit()->closeNoteByClass("findwidget");

    auto *fw = new FindWidget(this, edit->curTextPage());
    edit->slotCurPageTextEdit()->showNotificationWidget(fw, "findWidget");

}

void MainWindow::slotVerify() {

    if (edit->tabCount() == 0) return;

    if (edit->slotCurPageTextEdit() != nullptr) {

        QByteArray text = edit->curTextPage()->toPlainText().toUtf8();
        GpgME::GpgContext::preventNoDataErr(&text);


        gpgme_verify_result_t result;

        auto error = mCtx->verify(&text, nullptr, &result);

        auto resultAnalyse = new VerifyResultAnalyse(mCtx, error, result);
        infoBoard->associateTextEdit(edit->curTextPage());

        auto &reportText = resultAnalyse->getResultReport();
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
    } else if (edit->slotCurPageFileTreeView() != nullptr) {
        this->slotFileVerify();
    }
}

void MainWindow::slotEncryptSign() {


    if (edit->tabCount() == 0) return;

    if (edit->slotCurPageTextEdit() != nullptr) {

        QVector<GpgKey> keys;
        mKeyList->getCheckedKeys(keys);

        if (keys.empty()) {
            QMessageBox::critical(nullptr, tr("No Key Selected"), tr("No Key Selected"));
            return;
        }

        for (const auto &key : keys) {
            if (!GpgME::GpgContext::checkIfKeyCanSign(key) || !GpgME::GpgContext::checkIfKeyCanEncr(key)) {
                QMessageBox::information(nullptr,
                                         tr("Invalid Operation"),
                                         tr("The selected key cannot be used for signing and encryption at the same time.<br/>")
                                         + tr("<br/>For example the Following Key: <br/>") + key.uids.first().uid);
                return;
            }
        }

        auto *tmp = new QByteArray();
        gpgme_encrypt_result_t encr_result = nullptr;
        gpgme_sign_result_t sign_result = nullptr;

        auto error = mCtx->encryptSign(keys, edit->curTextPage()->toPlainText().toUtf8(), tmp, &encr_result,
                                       &sign_result);
        auto *tmp2 = new QString(*tmp);
        edit->slotFillTextEditWithText(*tmp2);

        auto resultAnalyseEncr = new EncryptResultAnalyse(error, encr_result);
        auto resultAnalyseSign = new SignResultAnalyse(error, sign_result);
        int status = std::min(resultAnalyseEncr->getStatus(), resultAnalyseSign->getStatus());
        auto reportText = resultAnalyseEncr->getResultReport() + resultAnalyseSign->getResultReport();

        infoBoard->associateTextEdit(edit->curTextPage());

        if (status < 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_CRITICAL);
        else if (status > 0)
            infoBoard->slotRefresh(reportText, INFO_ERROR_OK);
        else
            infoBoard->slotRefresh(reportText, INFO_ERROR_WARN);

        delete resultAnalyseEncr;
        delete resultAnalyseSign;
    } else if (edit->slotCurPageFileTreeView() != nullptr) {
        this->slotFileEncryptSign();
    }
}

void MainWindow::slotDecryptVerify() {

    if (edit->tabCount() == 0) return;

    if (edit->slotCurPageTextEdit() != nullptr) {

        auto *decrypted = new QByteArray();
        QByteArray text = edit->curTextPage()->toPlainText().toUtf8();
        GpgME::GpgContext::preventNoDataErr(&text);

        gpgme_decrypt_result_t d_result = nullptr;
        gpgme_verify_result_t v_result = nullptr;
        // try decrypt, if fail do nothing, especially don't replace text
        auto error = mCtx->decryptVerify(text, decrypted, &d_result, &v_result);
        infoBoard->associateTextEdit(edit->curTextPage());

        if (gpgme_err_code(error) == GPG_ERR_NO_ERROR)
            edit->slotFillTextEditWithText(QString::fromUtf8(*decrypted));

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
    } else if (edit->slotCurPageFileTreeView() != nullptr) {
        this->slotFileDecryptVerify();
    }
}

/*
 * Append the selected (not checked!) Key(s) To Textedit
 */
void MainWindow::slotAppendSelectedKeys() {
    if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) {
        return;
    }

    auto *keyArray = new QByteArray();
    mCtx->exportKeys(mKeyList->getSelected(), keyArray);
    edit->curTextPage()->append(*keyArray);
}

void MainWindow::slotCopyMailAddressToClipboard() {
    if (mKeyList->getSelected()->isEmpty()) {
        return;
    }
    auto &key = mCtx->getKeyById(mKeyList->getSelected()->first());
    QClipboard *cb = QApplication::clipboard();
    QString mail = key.email;
    cb->setText(mail);
}

void MainWindow::slotShowKeyDetails() {
    if (mKeyList->getSelected()->isEmpty()) {
        return;
    }
    auto &key = mCtx->getKeyById(mKeyList->getSelected()->first());
    if (key.good) {
        new KeyDetailsDialog(mCtx, key, this);
    }
}

void MainWindow::refreshKeysFromKeyserver() {
    if (mKeyList->getSelected()->isEmpty()) {
        return;
    }

    auto *dialog = new KeyServerImportDialog(mCtx, mKeyList, true, this);
    dialog->show();
    dialog->slotImport(*mKeyList->getSelected());

}

void MainWindow::uploadKeyToServer() {
    QVector<GpgKey> keys;
    keys.append(mKeyList->getSelectedKey());
    auto *dialog = new KeyUploadDialog(mCtx, keys);
}

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

    try {
        gpgme_encrypt_result_t result;
        auto error = GpgFileOpera::encryptFile(mCtx, keys, path, &result);

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

    } catch (std::runtime_error &e) {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
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

    try {
        gpgme_decrypt_result_t result;
        auto error = GpgFileOpera::decryptFile(mCtx, path, &result);

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
    } catch (std::runtime_error &e) {
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

    try {
        gpgme_sign_result_t result;
        auto error = GpgFileOpera::signFile(mCtx, keys, path, &result);

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

    } catch (std::runtime_error &e) {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
    }

    fileTreeView->update();

}

void MainWindow::slotFileVerify() {

    auto fileTreeView = edit->slotCurPageFileTreeView();
    auto path = fileTreeView->getSelected();

    QFileInfo fileInfo(path);

    QString signFilePath, dataFilePath;

    if (fileInfo.suffix() == "sig" || fileInfo.suffix() == "gpg") {
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

    try {
        gpgme_verify_result_t result;
        auto error = GpgFileOpera::verifyFile(mCtx, dataFilePath, &result);

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
    } catch (std::runtime_error &e) {
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

    try {

        gpgme_encrypt_result_t encr_result = nullptr;
        gpgme_sign_result_t sign_result = nullptr;

        auto error = GpgFileOpera::encryptSignFile(mCtx, keys, path, &encr_result, &sign_result);

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

    } catch (std::runtime_error &e) {
        QMessageBox::critical(this, tr("Error"), tr("An error occurred during operation."));
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

    try {

        gpgme_decrypt_result_t d_result = nullptr;
        gpgme_verify_result_t v_result = nullptr;
        // try decrypt, if fail do nothing, especially don't replace text
        auto error = GpgFileOpera::decryptVerifyFile(mCtx, path, &d_result, &v_result);
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
    } catch (std::runtime_error &e) {
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

void MainWindow::slotOpenFile(QString &path) {
    edit->slotOpenFile(path);
}
