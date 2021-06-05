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

#include "gpg/GpgContext.h"
#include "ui/keygen/KeygenThread.h"

#include <unistd.h>    /* contains read/write */

#ifdef _WIN32
#include <windows.h>
#endif

namespace GpgME {

/** Constructor
 *  Set up gpgme-context, set paths to app-run path
 */
    GpgContext::GpgContext() {
        /** get application path */
        QString appPath = qApp->applicationDirPath();

        /** The function `gpgme_check_version' must be called before any other
         *  function in the library, because it initializes the thread support
         *  subsystem in GPGME. (from the info page) */
        gpgme_check_version(nullptr);

        // TODO: Set gpgme_language to config
        /*QSettings qSettings;
        qDebug() << " - " << qSettings.value("int/lang").toLocale().name();
        qDebug() << " - " << QLocale(qSettings.value("int/lang").toString()).name();*/

        // the locale set here is used for the other setlocale calls which have nullptr
        // -> nullptr means use default, which is configured here
        setlocale(LC_ALL, "");

        /** set locale, because tests do also */
        gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
        //qDebug() << "Locale set to" << LC_CTYPE << " - " << setlocale(LC_CTYPE, nullptr);
#ifndef _WIN32
        gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif

        err = gpgme_new(&mCtx);
        checkErr(err);

        QSettings qSettings;
        QString accKeydbPath = qSettings.value("gpgpaths/keydbpath").toString();
        QString qGpgKeys = appPath + "/keydb/" + accKeydbPath;

        if (accKeydbPath != "") {
            if (!QDir(qGpgKeys).exists()) {
                QMessageBox::critical(nullptr, tr("keydb path"),
                                      tr("Didn't find keydb directory. Switching to gpg4usb's default keydb directory for this session."));
                qGpgKeys = appPath + "/keydb";
            }
        }

        gpgme_engine_info_t engineInfo;
        engineInfo = gpgme_ctx_get_engine_info(mCtx);

        while (engineInfo != nullptr) {
            qDebug() << gpgme_get_protocol_name(engineInfo->protocol);
            engineInfo = engineInfo->next;
        }

        /** Setting the output type must be done at the beginning */
        /** think this means ascii-armor --> ? */
        gpgme_set_armor(mCtx, 1);
        /** passphrase-callback */
        gpgme_set_passphrase_cb(mCtx, passphraseCb, this);

        /** check if app is called with -d from command line */
        if (qApp->arguments().contains("-d")) {
            qDebug() << "gpgme_data_t debug on";
            debug = true;
        } else {
            debug = false;
        }

        connect(this, SIGNAL(signalKeyDBChanged()),
                this, SLOT(slotRefreshKeyList()), Qt::DirectConnection);
        connect(this, SIGNAL(signalKeyUpdated(QString)),
                this, SLOT(slotUpdateKeyList(QString)), Qt::DirectConnection);
        slotRefreshKeyList();
    }

/** Destructor
 *  Release gpgme-context
 */
    GpgContext::~GpgContext() {
        if (mCtx) gpgme_release(mCtx);
        mCtx = nullptr;
    }

/** Import Key from QByteArray
 *
 */
    GpgImportInformation GpgContext::importKey(QByteArray inBuffer) {
        auto *importInformation = new GpgImportInformation();
        err = gpgme_data_new_from_mem(&in, inBuffer.data(), inBuffer.size(), 1);
        checkErr(err);
        err = gpgme_op_import(mCtx, in);
        gpgme_import_result_t result;

        result = gpgme_op_import_result(mCtx);
        if (result->unchanged) {
            importInformation->unchanged = result->unchanged;
        }
        if (result->considered) {
            importInformation->considered = result->considered;
        }
        if (result->no_user_id) {
            importInformation->no_user_id = result->no_user_id;
        }
        if (result->imported) {
            importInformation->imported = result->imported;
        }
        if (result->imported_rsa) {
            importInformation->imported_rsa = result->imported_rsa;
        }
        if (result->unchanged) {
            importInformation->unchanged = result->unchanged;
        }
        if (result->new_user_ids) {
            importInformation->new_user_ids = result->new_user_ids;
        }
        if (result->new_sub_keys) {
            importInformation->new_sub_keys = result->new_sub_keys;
        }
        if (result->new_signatures) {
            importInformation->new_signatures = result->new_signatures;
        }
        if (result->new_revocations) {
            importInformation->new_revocations = result->new_revocations;
        }
        if (result->secret_read) {
            importInformation->secret_read = result->secret_read;
        }
        if (result->secret_imported) {
            importInformation->secret_imported = result->secret_imported;
        }
        if (result->secret_unchanged) {
            importInformation->secret_unchanged = result->secret_unchanged;
        }
        if (result->not_imported) {
            importInformation->not_imported = result->not_imported;
        }
        gpgme_import_status_t status = result->imports;
        while (status != nullptr) {
            GpgImportedKey key;
            key.importStatus = static_cast<int>(status->status);
            key.fpr = status->fpr;
            importInformation->importedKeys.append(key);
            status = status->next;
        }
        checkErr(err);
        emit signalKeyDBChanged();
        gpgme_data_release(in);
        return *importInformation;
    }

/** Generate New Key with values params
 *
 */
    bool GpgContext::generateKey(GenKeyInfo *params) {

        auto userid_utf8 = params->getUserid().toUtf8();
        const char *userid = userid_utf8.constData();
        auto algo_utf8 = (params->getAlgo() + params->getKeySizeStr()).toUtf8();
        const char *algo = algo_utf8.constData();
        unsigned long expires = QDateTime::currentDateTime().secsTo(params->getExpired());
        unsigned int flags = 0;

        if (!params->isSubKey()) {
            flags |= GPGME_CREATE_CERT;
        }

        if (params->isAllowEncryption()) {
            flags |= GPGME_CREATE_ENCR;
        }

        if (params->isAllowSigning()) {
            flags |= GPGME_CREATE_SIGN;
        }

        if (params->isAllowAuthentication()) {
            flags |= GPGME_CREATE_AUTH;
        }

        if (params->isNonExpired()) {
            flags |= GPGME_CREATE_NOEXPIRE;
        }

        if (params->isNoPassPhrase()) {
            flags |= GPGME_CREATE_NOPASSWD;
        }

        err = gpgme_op_createkey(mCtx, userid, algo, 0, expires, nullptr, flags);

        if (err != GPG_ERR_NO_ERROR) {
            checkErr(err);
            return false;
        } else {
            emit signalKeyDBChanged();
            return true;
        }
    }

/** Export Key to QByteArray
 *
 */
    bool GpgContext::exportKeys(QStringList *uidList, QByteArray *outBuffer) {
        size_t read_bytes;
        gpgme_data_t dataOut = nullptr;
        outBuffer->resize(0);

        if (uidList->count() == 0) {
            QMessageBox::critical(nullptr, "Export Keys Error", "No Keys Selected");
            return false;
        }

        for (int i = 0; i < uidList->count(); i++) {
            err = gpgme_data_new(&dataOut);
            checkErr(err);

            err = gpgme_op_export(mCtx, uidList->at(i).toUtf8().constData(), 0, dataOut);
            checkErr(err);

            read_bytes = gpgme_data_seek(dataOut, 0, SEEK_END);

            err = readToBuffer(dataOut, outBuffer);
            checkErr(err);
            gpgme_data_release(dataOut);
        }
        return true;
    }

    /**
     * List all availabe Keys (VERY much like kgpgme)
     */
    void GpgContext::fetch_keys() {

        gpgme_error_t gpgmeError;

        gpgme_key_t key;

        qDebug() << "Clear List and Map";

        mKeyList.clear();
        mKeyMap.clear();

        auto &keys = mKeyList;
        auto &keys_map = mKeyMap;

        qDebug() << "Set Keylist Mode";

        gpgmeError = gpgme_set_keylist_mode(mCtx,
                                            GPGME_KEYLIST_MODE_LOCAL
                                            | GPGME_KEYLIST_MODE_WITH_SECRET
                                            | GPGME_KEYLIST_MODE_SIGS
                                            | GPGME_KEYLIST_MODE_SIG_NOTATIONS
                                            | GPGME_KEYLIST_MODE_WITH_TOFU);
        if (gpg_err_code(gpgmeError) != GPG_ERR_NO_ERROR) {
            checkErr(gpgmeError);
            return;
        }

        qDebug() << "Operate KeyList Start";

        gpgmeError = gpgme_op_keylist_start(mCtx, nullptr, 0);
        if (gpg_err_code(gpgmeError) != GPG_ERR_NO_ERROR) {
            checkErr(gpgmeError);
            return;
        }

        qDebug() << "Start Loop";

        while ((gpgmeError = gpgme_op_keylist_next(mCtx, &key)) == GPG_ERR_NO_ERROR) {
            if (!key->subkeys)
                continue;

            qDebug() << "Append Key" << key->subkeys->keyid;

            keys.append(GpgKey(key));
            keys_map.insert(keys.back().id, &keys.back());
            gpgme_key_unref(key);
        }


        if (gpg_err_code(gpgmeError) != GPG_ERR_EOF) {
            checkErr(gpgmeError);
            return;
        }

        qDebug() << "Operate KeyList End";

        gpgmeError = gpgme_op_keylist_end(mCtx);
        if (gpg_err_code(gpgmeError) != GPG_ERR_NO_ERROR) {
            checkErr(gpgmeError);
            return;
        }

        gpgmeError = gpgme_op_keylist_end(mCtx);
        if (gpg_err_code(gpgmeError) != GPG_ERR_NO_ERROR) {
            checkErr(gpgmeError);
            return;
        }

        mKeyList = keys;
    }

/** Delete keys
 */

    void GpgContext::deleteKeys(QStringList *uidList) {

        gpgme_error_t error;
        gpgme_key_t key;

        for (const auto &tmp : *uidList) {

            error = gpgme_op_keylist_start(mCtx, tmp.toUtf8().constData(), 0);
            if (error != GPG_ERR_NO_ERROR) {
                checkErr(error);
                continue;
            }

            error = gpgme_op_keylist_next(mCtx, &key);
            if (error != GPG_ERR_NO_ERROR) {
                checkErr(error);
                continue;
            }

            error = gpgme_op_keylist_end(mCtx);
            if (error != GPG_ERR_NO_ERROR) {
                checkErr(error);
                continue;
            }

            error = gpgme_op_delete(mCtx, key, 1);
            if (error != GPG_ERR_NO_ERROR) {
                checkErr(error);
                continue;
            }

        }
        emit signalKeyDBChanged();
    }

/** Encrypt inBuffer for reciepients-uids, write
 *  result to outBuffer
 */
    bool GpgContext::encrypt(QStringList *uidList, const QByteArray &inBuffer, QByteArray *outBuffer) {

        gpgme_data_t dataIn = nullptr, dataOut = nullptr;
        outBuffer->resize(0);

        if (uidList->count() == 0) {
            QMessageBox::critical(nullptr, tr("No Key Selected"), tr("No Key Selected"));
            return false;
        }

        //gpgme_encrypt_result_t e_result;
        gpgme_key_t recipients[uidList->count() + 1];

        /* get key for user */
        for (int i = 0; i < uidList->count(); i++) {
            // the last 0 is for public keys, 1 would return private keys
            gpgme_op_keylist_start(mCtx, uidList->at(i).toUtf8().constData(), 0);
            gpgme_op_keylist_next(mCtx, &recipients[i]);
            gpgme_op_keylist_end(mCtx);
        }
        //Last entry dataIn array has to be nullptr
        recipients[uidList->count()] = nullptr;

        //If the last parameter isnt 0, a private copy of data is made
        if (mCtx) {
            err = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
            checkErr(err);
            if (!err) {
                err = gpgme_data_new(&dataOut);
                checkErr(err);
                if (!err) {
                    err = gpgme_op_encrypt(mCtx, recipients, GPGME_ENCRYPT_ALWAYS_TRUST, dataIn, dataOut);
                    checkErr(err);
                    if (!err) {
                        err = readToBuffer(dataOut, outBuffer);
                        checkErr(err);
                    }
                }
            }
        }
        /* unref all keys */
        for (int i = 0; i <= uidList->count(); i++) {
            gpgme_key_unref(recipients[i]);
        }
        if (dataIn) {
            gpgme_data_release(dataIn);
        }
        if (dataOut) {
            gpgme_data_release(dataOut);
        }
        return (err == GPG_ERR_NO_ERROR);
    }

/** Decrypt QByteAarray, return QByteArray
 *  mainly from http://basket.kde.org/ (kgpgme.cpp)
 */
    bool GpgContext::decrypt(const QByteArray &inBuffer, QByteArray *outBuffer) {
        gpgme_data_t dataIn = nullptr, dataOut = nullptr;
        gpgme_decrypt_result_t result = nullptr;
        QString errorString;

        outBuffer->resize(0);
        if (mCtx) {
            err = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
            checkErr(err);
            if (!err) {
                err = gpgme_data_new(&dataOut);
                checkErr(err);
                if (!err) {
                    err = gpgme_op_decrypt(mCtx, dataIn, dataOut);
                    checkErr(err);

                    if (gpg_err_code(err) == GPG_ERR_DECRYPT_FAILED) {
                        errorString.append(gpgErrString(err)).append("<br>");
                        result = gpgme_op_decrypt_result(mCtx);
                        checkErr(result->recipients->status);
                        errorString.append(gpgErrString(result->recipients->status)).append("<br>");
                        errorString.append(
                                tr("<br>No private key with id %1 present dataIn keyring").arg(
                                        result->recipients->keyid));
                    } else {
                        errorString.append(gpgErrString(err)).append("<br>");
                    }

                    if (!err) {
                        result = gpgme_op_decrypt_result(mCtx);
                        if (result->unsupported_algorithm) {
                            QMessageBox::critical(0, tr("Unsupported algorithm"), result->unsupported_algorithm);
                        } else {
                            err = readToBuffer(dataOut, outBuffer);
                            checkErr(err);
                        }
                    }
                }
            }
        }
        if (gpg_err_code(err) != GPG_ERR_NO_ERROR && gpg_err_code(err) != GPG_ERR_CANCELED) {
            QMessageBox::critical(nullptr, tr("Error decrypting:"), errorString);
            return false;
        }

        if (!settings.value("general/rememberPassword").toBool()) {
            clearPasswordCache();
        }

        if (dataIn) {
            gpgme_data_release(dataIn);
        }
        if (dataOut) {
            gpgme_data_release(dataOut);
        }
        return (err == GPG_ERR_NO_ERROR);
    }

/**  Read gpgme-Data to QByteArray
 *   mainly from http://basket.kde.org/ (kgpgme.cpp)
 */
#define BUF_SIZE (32 * 1024)

    gpgme_error_t GpgContext::readToBuffer(gpgme_data_t dataIn, QByteArray *outBuffer) {
        off_t ret;
        gpgme_error_t gpgErrNoError = GPG_ERR_NO_ERROR;

        ret = gpgme_data_seek(dataIn, 0, SEEK_SET);
        if (ret) {
            gpgErrNoError = gpgme_err_code_from_errno(errno);
            checkErr(gpgErrNoError, "failed dataseek dataIn readToBuffer");
        } else {
            char buf[BUF_SIZE + 2];

            while ((ret = gpgme_data_read(dataIn, buf, BUF_SIZE)) > 0) {
                const size_t size = outBuffer->size();
                outBuffer->resize(static_cast<int>(size + ret));
                memcpy(outBuffer->data() + size, buf, ret);
            }
            if (ret < 0) {
                gpgErrNoError = gpgme_err_code_from_errno(errno);
                checkErr(gpgErrNoError, "failed data_read dataIn readToBuffer");
            }
        }
        return gpgErrNoError;
    }

/** The Passphrase window, if not provided by env-Var GPG_AGENT_INFO
 *  originally copied from http://basket.kde.org/ (kgpgme.cpp), but modified
 */
    gpgme_error_t GpgContext::passphraseCb(void *hook, const char *uid_hint,
                                           const char *passphrase_info,
                                           int last_was_bad, int fd) {
        auto *gpg = static_cast<GpgContext *>(hook);
        return gpg->passphrase(uid_hint, passphrase_info, last_was_bad, fd);
    }

    gpgme_error_t GpgContext::passphrase(const char *uid_hint,
                                         const char * /*passphrase_info*/,
                                         int last_was_bad, int fd) {
        gpgme_error_t returnValue = GPG_ERR_CANCELED;
        QString passwordDialogMessage;
        QString gpgHint = QString::fromUtf8(uid_hint);
        bool result;

#ifdef _WIN32
        DWORD written;
        HANDLE hd = (HANDLE)fd;
#endif

        if (last_was_bad) {
            passwordDialogMessage += "<i>" + tr("Wrong password") + ".</i><br><br>\n\n";
            clearPasswordCache();
        }

        /** if uid provided */
        if (!gpgHint.isEmpty()) {
            // remove UID, leave only username & email
            gpgHint.remove(0, gpgHint.indexOf(" "));
            passwordDialogMessage += "<b>" + tr("Enter Password for") + "</b><br>" + gpgHint + "<br>";
        }

        if (mPasswordCache.isEmpty()) {
            QString password = QInputDialog::getText(QApplication::activeWindow(), tr("Enter Password"),
                                                     passwordDialogMessage, QLineEdit::Password,
                                                     "", &result);

            if (result) mPasswordCache = password.toUtf8();
        } else {
            result = true;
        }

        if (result) {

#ifndef _WIN32
            if (write(fd, mPasswordCache.data(), mPasswordCache.length()) == -1) {
                qDebug() << "something is terribly broken";
            }
#else
            WriteFile(hd, mPasswordCache.data(), mPasswordCache.length(), &written, 0);
#endif

            returnValue = GPG_ERR_NO_ERROR;
        }

#ifndef _WIN32
        if (write(fd, "\n", 1) == -1) {
            qDebug() << "something is terribly broken";
        }
#else
        WriteFile(hd, "\n", 1, &written, 0);

        /* program will hang on cancel if hd not closed */
        if(!result) {
                CloseHandle(hd);
        }
#endif

        return returnValue;
    }

/** also from kgpgme.cpp, seems to clear password from mem */
    void GpgContext::clearPasswordCache() {
        if (mPasswordCache.size() > 0) {
            mPasswordCache.fill('\0');
            mPasswordCache.truncate(0);
        }
    }

// error-handling
    void GpgContext::checkErr(gpgme_error_t gpgmeError, const QString &comment) {
        //if (gpgmeError != GPG_ERR_NO_ERROR && gpgmeError != GPG_ERR_CANCELED) {
        if (gpgmeError != GPG_ERR_NO_ERROR) {
            qDebug() << "[Error " << gpg_err_code(gpgmeError)
                     << "] Source: " << gpgme_strsource(gpgmeError) << " Description: " << gpgErrString(gpgmeError);
        }
    }

    void GpgContext::checkErr(gpgme_error_t gpgmeError) {
        //if (gpgmeError != GPG_ERR_NO_ERROR && gpgmeError != GPG_ERR_CANCELED) {
        if (gpg_err_code(gpgmeError) != GPG_ERR_NO_ERROR) {
            qDebug() << "[Error " << gpg_err_code(gpgmeError)
                     << "] Source: " << gpgme_strsource(gpgmeError) << " Description: " << gpgErrString(gpgmeError);
        }
    }

    QString GpgContext::gpgErrString(gpgme_error_t err) {
        return QString::fromUtf8(gpgme_strerror(err));
    }

/** export private key, TODO errohandling, e.g. like in seahorse (seahorse-gpg-op.c) **/

    void GpgContext::exportSecretKey(const QString &uid, QByteArray *outBuffer) {
        qDebug() << *outBuffer;
        // export private key to outBuffer
        QStringList arguments;
        arguments << "--armor" << "--export-secret-key" << uid;
        auto *p_errArray = new QByteArray();
        executeGpgCommand(arguments, outBuffer, p_errArray);

        // append public key to outBuffer
        auto *pubKey = new QByteArray();
        QStringList keyList;
        keyList.append(uid);
        exportKeys(&keyList, pubKey);
        outBuffer->append(*pubKey);
    }

/** return type should be gpgme_error_t*/
    void GpgContext::executeGpgCommand(const QStringList &arguments, QByteArray *stdOut, QByteArray *stdErr) {
        QStringList args;
        args << "--homedir" << gpgKeys << "--batch" << arguments;

        qDebug() << args;
        QProcess gpg;
        //  qDebug() << "engine->file_name" << engine->file_name;

        gpg.start(gpgBin, args);
        gpg.waitForFinished();

        *stdOut = gpg.readAllStandardOutput();
        *stdErr = gpg.readAllStandardError();
        qDebug() << *stdOut;
    }

/***
  * if sigbuffer not set, the inbuffer should contain signed text
  *
  * TODO: return type should contain:
  * -> list of sigs
  * -> valid
  * -> errors
  */
    gpgme_signature_t GpgContext::verify(QByteArray *inBuffer, QByteArray *sigBuffer) {

        gpgme_data_t dataIn;
        gpgme_error_t gpgmeError;
        gpgme_signature_t sign;
        gpgme_verify_result_t result;

        gpgmeError = gpgme_data_new_from_mem(&dataIn, inBuffer->data(), inBuffer->size(), 1);
        checkErr(gpgmeError);

        if (sigBuffer != nullptr) {
            gpgme_data_t sigdata;
            gpgmeError = gpgme_data_new_from_mem(&sigdata, sigBuffer->data(), sigBuffer->size(), 1);
            checkErr(gpgmeError);
            gpgmeError = gpgme_op_verify(mCtx, sigdata, dataIn, nullptr);
        } else {
            gpgmeError = gpgme_op_verify(mCtx, dataIn, nullptr, dataIn);
        }

        checkErr(gpgmeError);

        if (gpgmeError != 0) {
            return nullptr;
        }

        result = gpgme_op_verify_result(mCtx);
        sign = result->signatures;
        return sign;
    }

    /***
      * return type should contain:
      * -> list of sigs
      * -> valid
      * -> decrypted message
      */
    //void GpgContext::decryptVerify(QByteArray in) {

    /*    gpgme_error_t err;
        gpgme_data_t in, out;

        gpgme_decrypt_result_t decrypt_result;
        gpgme_verify_result_t verify_result;

        err = gpgme_op_decrypt_verify (mCtx, in, out);
        decrypt_result = gpgme_op_decrypt_result (mCtx);

        verify_result = gpgme_op_verify_result (mCtx);
     */
    //}
    bool GpgContext::sign(QVector<GpgKey> keys, const QByteArray &inBuffer, QByteArray *outBuffer, bool detached) {

        gpgme_error_t gpgmeError;
        gpgme_data_t dataIn, dataOut;
        gpgme_sign_result_t result;
        gpgme_sig_mode_t mode;

        if (keys.isEmpty()) {
            QMessageBox::critical(nullptr, tr("Key Selection"), tr("No Private Key Selected"));
            return false;
        }

        // at start or end?

        setSigners(keys);

        gpgmeError = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
        checkErr(gpgmeError);
        gpgmeError = gpgme_data_new(&dataOut);
        checkErr(gpgmeError);

        /*
            `GPGME_SIG_MODE_NORMAL'
                  A normal signature is made, the output includes the plaintext
                  and the signature.

            `GPGME_SIG_MODE_DETACH'
                  A detached signature is made.

            `GPGME_SIG_MODE_CLEAR'
                  A clear text signature is made.  The ASCII armor and text
                  mode settings of the context are ignored.
        */

        if (detached) {
            mode = GPGME_SIG_MODE_DETACH;
        } else {
            mode = GPGME_SIG_MODE_CLEAR;
        }

        gpgmeError = gpgme_op_sign(mCtx, dataIn, dataOut, mode);
        checkErr(gpgmeError);

        if (gpgmeError == GPG_ERR_CANCELED) {
            return false;
        }

        if (gpgmeError != GPG_ERR_NO_ERROR) {
            QMessageBox::critical(nullptr, tr("Error in signing:"), QString::fromUtf8(gpgme_strerror(gpgmeError)));
            return false;
        }

        result = gpgme_op_sign_result(mCtx);

        // TODO Handle the result

        gpgmeError = readToBuffer(dataOut, outBuffer);
        checkErr(gpgmeError);

        gpgme_data_release(dataIn);
        gpgme_data_release(dataOut);

        if (!settings.value("general/rememberPassword").toBool()) {
            clearPasswordCache();
        }

        return (gpgmeError == GPG_ERR_NO_ERROR);
    }

    /*
     * if there is no '\n' before the PGP-Begin-Block, but for example a whitespace,
     * GPGME doesn't recognise the Message as encrypted. This function adds '\n'
     * before the PGP-Begin-Block, if missing.
     */
    void GpgContext::preventNoDataErr(QByteArray *in) {
        int block_start = in->indexOf(GpgConstants::PGP_CRYPT_BEGIN);
        if (block_start > 0 && in->at(block_start - 1) != '\n') {
            in->insert(block_start, '\n');
        }
        block_start = in->indexOf(GpgConstants::PGP_SIGNED_BEGIN);
        if (block_start > 0 && in->at(block_start - 1) != '\n') {
            in->insert(block_start, '\n');
        }
    }

    /*
      * isSigned returns:
      * - 0, if text isn't signed at all
      * - 1, if text is partially signed
      * - 2, if text is completly signed
      */
    int GpgContext::textIsSigned(const QByteArray &text) {
        if (text.trimmed().startsWith(GpgConstants::PGP_SIGNED_BEGIN) &&
            text.trimmed().endsWith(GpgConstants::PGP_SIGNED_END)) {
            return 2;
        }
        if (text.contains(GpgConstants::PGP_SIGNED_BEGIN) && text.contains(GpgConstants::PGP_SIGNED_END)) {
            return 1;
        }
        return 0;
    }

    QString GpgContext::beautifyFingerprint(QString fingerprint) {
        uint len = fingerprint.length();
        if ((len > 0) && (len % 4 == 0))
            for (uint n = 0; 4 * (n + 1) < len; ++n)
                fingerprint.insert(static_cast<int>(5u * n + 4u), ' ');
        return fingerprint;
    }

    void GpgContext::slotRefreshKeyList() {
        qDebug() << "Refreshing Keys";
        this->fetch_keys();
        emit signalKeyInfoChanged();
    }

/**
 * note: is_private_key status is not returned
 */
    GpgKey GpgContext::getKeyByFpr(const QString &fpr) {
        for (const auto &key : mKeyList) {
            if (key.fpr == fpr) {
                return key;
            } else {
                for(auto &subkey : key.subKeys) {
                    if(subkey.fpr == fpr) {
                        return key;
                    }
                }
            }
        }
        return GpgKey(nullptr);
    }

    /**
     * note: is_private_key status is not returned
     */
    const GpgKey &GpgContext::getKeyById(const QString &id) {

        auto it = mKeyMap.find(id);

        if(it != mKeyMap.end()) {
            return *(it.value());
        }

        throw std::runtime_error("key not found");
    }

    QString GpgContext::getGpgmeVersion() {
        return QString(gpgme_check_version(nullptr));
    }

    bool GpgContext::signKey(const GpgKey &target, const QString &uid, const QDateTime *expires) {

        unsigned int flags = 0;

        unsigned int expires_time_t = 0;
        if (expires == nullptr) {
            flags |= GPGME_KEYSIGN_NOEXPIRE;
        } else {
            expires_time_t = QDateTime::currentDateTime().secsTo(*expires);
        }

        auto gpgmeError =
                gpgme_op_keysign(mCtx, target.key_refer, uid.toUtf8().constData(), expires_time_t, flags);

        if(gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(target.id);
            return true;
        } else {
            checkErr(gpgmeError);
            return false;
        }
    }

    const GpgKeyList &GpgContext::getKeys() const {
        return mKeyList;
    }

    void GpgContext::getSigners(QVector<GpgKey> &signer) {
        auto count = gpgme_signers_count(mCtx);
        signer.clear();
        for (auto i = 0; i < count; i++) {
            auto key = gpgme_signers_enum(mCtx, i);
            auto it = mKeyMap.find(key->subkeys->keyid);
            if (it == mKeyMap.end()) {
                qDebug() << "Inconsistent state";
                signer.push_back(GpgKey(key));
            } else {
                signer.push_back(*it.value());
            }
        }
    }

    void GpgContext::setSigners(const QVector<GpgKey> &keys) {
        gpgme_signers_clear(mCtx);
        for (const auto &key : keys) {
            auto gpgmeError = gpgme_signers_add(mCtx, key.key_refer);
            checkErr(gpgmeError);
        }
        if (keys.length() != gpgme_signers_count(mCtx)) {
            qDebug() << "No All Keys Added";
        }
    }

    void GpgContext::slotUpdateKeyList(QString key_id) {
        auto it = mKeyMap.find(key_id);
        if (it != mKeyMap.end()) {
            gpgme_key_t new_key_refer;
            auto gpgmeErr = gpgme_get_key(mCtx, key_id.toUtf8().constData(), &new_key_refer, 0);

            if(gpgme_err_code(gpgmeErr) == GPG_ERR_EOF) {
                gpgmeErr = gpgme_get_key(mCtx, key_id.toUtf8().constData(), &new_key_refer, 1);

                if(gpgme_err_code(gpgmeErr) == GPG_ERR_EOF) {
                    throw std::runtime_error("key_id not found in key database");
                }

            }

            if(new_key_refer != nullptr) {
                it.value()->swapKeyRefer(new_key_refer);
                emit signalKeyInfoChanged();
            }

        }
    }

    bool GpgContext::addUID(const GpgKey &key, const UID &uid) {
        QString userid = QString("%1 (%3) <%2>").arg(uid.name, uid.email, uid.comment);
        auto gpgmeError = gpgme_op_adduid(mCtx, key.key_refer, userid.toUtf8().constData(), 0);
        if(gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        }
        else {
            checkErr(gpgmeError);
            return false;
        }

    }

    bool GpgContext::revUID(const GpgKey &key, const UID &uid) {
        auto gpgmeError = gpgme_op_revuid(mCtx, key.key_refer, uid.uid.toUtf8().constData(), 0);
        if(gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        }
        else {
            checkErr(gpgmeError);
            return false;
        }
    }

    bool GpgContext::setPrimaryUID(const GpgKey &key, const UID &uid) {
        auto gpgmeError = gpgme_op_set_uid_flag(mCtx, key.key_refer,
                                                uid.uid.toUtf8().constData(), "primary", nullptr);
        if(gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        }
        else {
            checkErr(gpgmeError);
            return false;
        }
    }

    bool GpgContext::revSign(const GpgKey &key, const Signature &signature) {

        auto signing_key = getKeyById(signature.keyid);

        auto gpgmeError = gpgme_op_revsig(mCtx, key.key_refer,
                                                signing_key.key_refer,
                                                signature.uid.toUtf8().constData(), 0);
        if(gpg_err_code(gpgmeError) == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        }
        else {
            checkErr(gpgmeError);
            return false;
        }
    }

    bool GpgContext::generateSubkey(const GpgKey &key, GenKeyInfo *params) {

        if(!params->isSubKey()) {
            return false;
        }

        auto algo_utf8 = (params->getAlgo() + params->getKeySizeStr()).toUtf8();
        const char *algo = algo_utf8.constData();
        unsigned long expires = QDateTime::currentDateTime().secsTo(params->getExpired());
        unsigned int flags = 0;

        if (!params->isSubKey()) {
            flags |= GPGME_CREATE_CERT;
        }

        if (params->isAllowEncryption()) {
            flags |= GPGME_CREATE_ENCR;
        }

        if (params->isAllowSigning()) {
            flags |= GPGME_CREATE_SIGN;
        }

        if (params->isAllowAuthentication()) {
            flags |= GPGME_CREATE_AUTH;
        }

        if (params->isNonExpired()) {
            flags |= GPGME_CREATE_NOEXPIRE;
        }

        flags |= GPGME_CREATE_NOPASSWD;


        auto gpgmeError = gpgme_op_createsubkey(mCtx, key.key_refer,
                                                algo, 0, expires, flags);
        if(gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        }
        else {
            checkErr(gpgmeError);
            return false;
        }
    }

    bool GpgContext::setExpire(const GpgKey &key, const GpgSubKey *subkey, QDateTime *expires) {
        unsigned long expires_time = 0;
        if(expires != nullptr) {
            qDebug() << "Expire Datetime" << expires->toString();
            expires_time = QDateTime::currentDateTime().secsTo(*expires);
        }

        const char *subfprs = nullptr;

        if(subkey != nullptr) {
            subfprs = subkey->fpr.toUtf8().constData();
        }

        auto gpgmeError = gpgme_op_setexpire(mCtx, key.key_refer,
                                             expires_time, subfprs, 0);
        if(gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        }
        else {
            checkErr(gpgmeError);
            return false;
        }
    }

    bool GpgContext::checkIfKeyCanSign(const GpgKey &key) {
        if(std::any_of(key.subKeys.begin(), key.subKeys.end(), [] (const GpgSubKey &subkey) -> bool {
            return subkey.secret && subkey.can_sign && !subkey.disabled && !subkey.revoked && !subkey.expired;
        })) return true;
        return false;
    }

    bool GpgContext::checkIfKeyCanCert(const GpgKey &key) {
        return key.has_master_key && !key.expired && !key.revoked && !key.disabled;
    }

    bool GpgContext::checkIfKeyCanAuth(const GpgKey &key) {
        if(std::any_of(key.subKeys.begin(), key.subKeys.end(), [] (const GpgSubKey &subkey) -> bool {
            return subkey.secret && subkey.can_authenticate && !subkey.disabled && !subkey.revoked && !subkey.expired;
        })) return true;
        return false;
    }

    bool GpgContext::checkIfKeyCanEncr(const GpgKey &key) {
        if(std::any_of(key.subKeys.begin(), key.subKeys.end(), [] (const GpgSubKey &subkey) -> bool {
            return subkey.can_encrypt && !subkey.disabled && !subkey.revoked && !subkey.expired;
        })) return true;
        return false;
    }

    bool GpgContext::encryptSign(QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer) {
        gpgme_data_t dataIn = nullptr, dataOut = nullptr;
        outBuffer->resize(0);

        if (keys.count() == 0) {
            QMessageBox::critical(nullptr, tr("No Key Selected"), tr("No Key Selected"));
            return false;
        }

        setSigners(keys);

        //gpgme_encrypt_result_t e_result;
        gpgme_key_t recipients[keys.count() + 1];

        /* set key for user */
        int index = 0;
        for(const auto &key : keys) {
            recipients[index++] = key.key_refer;
        }
        //Last entry dataIn array has to be nullptr
        recipients[keys.count()] = nullptr;

        //If the last parameter isnt 0, a private copy of data is made
        if (mCtx) {
            err = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
            checkErr(err);
            if (!err) {
                err = gpgme_data_new(&dataOut);
                checkErr(err);
                if (!err) {
                    err = gpgme_op_encrypt_sign(mCtx, recipients, GPGME_ENCRYPT_ALWAYS_TRUST, dataIn, dataOut);
                    checkErr(err);
                    if (!err) {
                        err = readToBuffer(dataOut, outBuffer);
                        checkErr(err);
                    }
                }
            }
        }
        if (dataIn) {
            gpgme_data_release(dataIn);
        }
        if (dataOut) {
            gpgme_data_release(dataOut);
        }
        return (err == GPG_ERR_NO_ERROR);
    }

}





