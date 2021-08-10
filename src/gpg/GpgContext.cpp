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

#include <functional>
#include <unistd.h>    /* contains read/write */

#ifdef _WIN32

#include <windows.h>

#endif

#define INT2VOIDP(i) (void*)(uintptr_t)(i)

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

        // the locale set here is used for the other setlocale calls which have nullptr
        // -> nullptr means use default, which is configured here
        setlocale(LC_ALL, settings.value("int/lang").toLocale().name().toUtf8().constData());

        /** set locale, because tests do also */
        gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
        //qDebug() << "Locale set to" << LC_CTYPE << " - " << setlocale(LC_CTYPE, nullptr);
#ifndef _WIN32
        gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif

        err = gpgme_new(&mCtx);
        checkErr(err);

        gpgme_engine_info_t engineInfo;
        engineInfo = gpgme_ctx_get_engine_info(mCtx);

        // Check ENV before running
        bool check_pass = false, find_openpgp = false, find_gpgconf = false, find_assuan = false, find_cms = false;
        while (engineInfo != nullptr) {
            qDebug() << gpgme_get_protocol_name(engineInfo->protocol) << engineInfo->file_name << engineInfo->protocol
                     << engineInfo->home_dir << engineInfo->version;
            if (engineInfo->protocol == GPGME_PROTOCOL_GPGCONF && strcmp(engineInfo->version, "1.0.0") != 0)
                find_gpgconf = true;
            if (engineInfo->protocol == GPGME_PROTOCOL_OpenPGP && strcmp(engineInfo->version, "1.0.0") != 0) {
                gpgExec = engineInfo->file_name;
                find_openpgp = true;
            }
            if (engineInfo->protocol == GPGME_PROTOCOL_CMS && strcmp(engineInfo->version, "1.0.0") != 0)
                find_cms = true;
            if (engineInfo->protocol == GPGME_PROTOCOL_ASSUAN)
                find_assuan = true;

            engineInfo = engineInfo->next;
        }

        if (find_gpgconf && find_openpgp && find_cms && find_assuan)
            check_pass = true;

        if (!check_pass) {
            good = false;
            return;
        } else good = true;


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

    bool GpgContext::isGood() const {
        return good;
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
            importInformation->importedKeys.emplace_back(key);
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
    gpgme_error_t GpgContext::generateKey(GenKeyInfo *params) {

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

        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
            checkErr(err);
            return err;
        } else {
            emit signalKeyDBChanged();
            return err;
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

            keys.emplace_back(key);
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
    gpg_error_t GpgContext::encrypt(QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer,
                                    gpgme_encrypt_result_t *result) {

        gpgme_data_t dataIn = nullptr, dataOut = nullptr;
        outBuffer->resize(0);

        //gpgme_encrypt_result_t e_result;
        gpgme_key_t recipients[keys.count() + 1];

        int index = 0;
        for (const auto &key : keys) {
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
                    err = gpgme_op_encrypt(mCtx, recipients, GPGME_ENCRYPT_ALWAYS_TRUST, dataIn, dataOut);
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

        if (result != nullptr) {
            *result = gpgme_op_encrypt_result(mCtx);
        }
        return err;
    }

/** Decrypt QByteAarray, return QByteArray
 *  mainly from http://basket.kde.org/ (kgpgme.cpp)
 */
    gpgme_error_t
    GpgContext::decrypt(const QByteArray &inBuffer, QByteArray *outBuffer, gpgme_decrypt_result_t *result) {
        gpgme_data_t dataIn = nullptr, dataOut = nullptr;
        gpgme_decrypt_result_t m_result = nullptr;

        outBuffer->resize(0);
        if (mCtx != nullptr) {
            err = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
            if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                err = gpgme_data_new(&dataOut);
                if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                    err = gpgme_op_decrypt(mCtx, dataIn, dataOut);
                    m_result = gpgme_op_decrypt_result(mCtx);
                    if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                        err = readToBuffer(dataOut, outBuffer);
                    }
                }
            }
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

        if (result != nullptr) {
            *result = m_result;
        }
        return err;
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
        auto hd = INT2VOIDP(fd);
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
        if (!result) {
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

    bool GpgContext::exportSecretKey(const GpgKey &key, QByteArray *outBuffer) {
        qDebug() << "Export Secret Key" << key.id;
        gpgme_key_t target_key[2] = {
                key.key_refer,
                nullptr
        };

        gpgme_data_t dataOut;
        gpgme_data_new(&dataOut);
        // export private key to outBuffer
        gpgme_error_t error = gpgme_op_export_keys(mCtx, target_key, GPGME_EXPORT_MODE_SECRET, dataOut);

        if (gpgme_err_code(error) != GPG_ERR_NO_ERROR) {
            checkErr(error);
            gpgme_data_release(dataOut);
            return false;
        }

        readToBuffer(dataOut, outBuffer);
        gpgme_data_release(dataOut);
        return true;
    }

/** return type should be gpgme_error_t*/
    QProcess * GpgContext::executeGpgCommand(const QStringList &arguments, QByteArray *stdOut, QByteArray *stdErr,
                                       const std::function<void(QProcess *)> &interactFunc) {
        QStringList args;
        args << arguments;

        auto *gpgProcess = new QProcess(this);
        qDebug() << "gpgExec" << gpgExec << args;

        gpgProcess->setReadChannel(QProcess::StandardOutput);
        connect(gpgProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                gpgProcess, SLOT(deleteLater()));
        connect(gpgProcess, &QProcess::readyReadStandardOutput, this, [gpgProcess, interactFunc]() {
            qDebug() << "Function Called" << &gpgProcess;
            // interactFunc(gpgProcess);
        });

        gpgProcess->start(gpgExec, args);

        if (gpgProcess->waitForStarted()){
            qDebug() << "Gpg Process Started Success";
        } else {
            qDebug() << "Gpg Process Started Failed";
        }

        return gpgProcess;
    }

/***
  * if sigbuffer not set, the inbuffer should contain signed text
  *
  * TODO: return type should contain:
  * -> list of sigs
  * -> valid
  * -> errors
  */
    gpgme_error_t GpgContext::verify(QByteArray *inBuffer, QByteArray *sigBuffer, gpgme_verify_result_t *result) {

        gpgme_data_t dataIn;
        gpgme_error_t gpgmeError;
        gpgme_signature_t sign;
        gpgme_verify_result_t m_result;

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

        m_result = gpgme_op_verify_result(mCtx);

        if (result != nullptr) {
            *result = m_result;
        }

        return gpgmeError;
    }

    gpg_error_t
    GpgContext::sign(const QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer, gpgme_sig_mode_t mode,
                     gpgme_sign_result_t *result) {

        gpgme_error_t gpgmeError;
        gpgme_data_t dataIn, dataOut;
        gpgme_sign_result_t m_result;

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

        gpgmeError = gpgme_op_sign(mCtx, dataIn, dataOut, mode);
        checkErr(gpgmeError);

        if (gpgmeError == GPG_ERR_CANCELED) {
            return false;
        }

        if (gpgmeError != GPG_ERR_NO_ERROR) {
            QMessageBox::critical(nullptr, tr("Error in signing:"), QString::fromUtf8(gpgme_strerror(gpgmeError)));
            return false;
        }

        m_result = gpgme_op_sign_result(mCtx);

        if (result != nullptr) {
            *result = m_result;
        }

        gpgmeError = readToBuffer(dataOut, outBuffer);
        checkErr(gpgmeError);

        gpgme_data_release(dataIn);
        gpgme_data_release(dataOut);

        if (!settings.value("general/rememberPassword").toBool()) {
            clearPasswordCache();
        }

        return gpgmeError;
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
                for (auto &subkey : key.subKeys) {
                    if (subkey.fpr == fpr) {
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

        for (const auto &key : mKeyList) {
            if (key.id == id) {
                return key;
            } else {
                auto subkeys = key.subKeys;
                for (const auto &subkey : subkeys) {
                    if (subkey.id == id) {
                        return key;
                    }
                }
            }
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

        if (gpgmeError == GPG_ERR_NO_ERROR) {
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
            if (checkIfKeyCanSign(key)) {
                auto gpgmeError = gpgme_signers_add(mCtx, key.key_refer);
                checkErr(gpgmeError);
            }
        }
        if (keys.length() != gpgme_signers_count(mCtx)) {
            qDebug() << "No All Keys Added";
        }
    }

    void GpgContext::slotUpdateKeyList(const QString &key_id) {
        auto it = mKeyMap.find(key_id);
        if (it != mKeyMap.end()) {
            gpgme_key_t new_key_refer;
            auto gpgmeErr = gpgme_get_key(mCtx, key_id.toUtf8().constData(), &new_key_refer, 0);

            if (gpgme_err_code(gpgmeErr) == GPG_ERR_EOF) {
                gpgmeErr = gpgme_get_key(mCtx, key_id.toUtf8().constData(), &new_key_refer, 1);

                if (gpgme_err_code(gpgmeErr) == GPG_ERR_EOF) {
                    throw std::runtime_error("key_id not found in key database");
                }

            }

            if (new_key_refer != nullptr) {
                it.value()->swapKeyRefer(new_key_refer);
                emit signalKeyInfoChanged();
            }

        }
    }

    bool GpgContext::addUID(const GpgKey &key, const GpgUID &uid) {
        QString userid = QString("%1 (%3) <%2>").arg(uid.name, uid.email, uid.comment);
        auto gpgmeError = gpgme_op_adduid(mCtx, key.key_refer, userid.toUtf8().constData(), 0);
        if (gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        } else {
            checkErr(gpgmeError);
            return false;
        }

    }

    bool GpgContext::revUID(const GpgKey &key, const GpgUID &uid) {
        auto gpgmeError = gpgme_op_revuid(mCtx, key.key_refer, uid.uid.toUtf8().constData(), 0);
        if (gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        } else {
            checkErr(gpgmeError);
            return false;
        }
    }

    bool GpgContext::setPrimaryUID(const GpgKey &key, const GpgUID &uid) {
        auto gpgmeError = gpgme_op_set_uid_flag(mCtx, key.key_refer,
                                                uid.uid.toUtf8().constData(), "primary", nullptr);
        if (gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        } else {
            checkErr(gpgmeError);
            return false;
        }
    }

    bool GpgContext::revSign(const GpgKey &key, const GpgKeySignature &signature) {

        auto signing_key = getKeyById(signature.keyid);

        auto gpgmeError = gpgme_op_revsig(mCtx, key.key_refer,
                                          signing_key.key_refer,
                                          signature.uid.toUtf8().constData(), 0);
        if (gpg_err_code(gpgmeError) == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        } else {
            checkErr(gpgmeError);
            return false;
        }
    }

    gpgme_error_t GpgContext::generateSubkey(const GpgKey &key, GenKeyInfo *params) {

        if (!params->isSubKey()) {
            return GPG_ERR_CANCELED;
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
        if (gpgme_err_code(gpgmeError) == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return gpgmeError;
        } else {
            checkErr(gpgmeError);
            return gpgmeError;
        }
    }

    bool GpgContext::setExpire(const GpgKey &key, const GpgSubKey *subkey, QDateTime *expires) {
        unsigned long expires_time = 0;
        if (expires != nullptr) {
            qDebug() << "Expire Datetime" << expires->toString();
            expires_time = QDateTime::currentDateTime().secsTo(*expires);
        }

        const char *subfprs = nullptr;

        if (subkey != nullptr) {
            subfprs = subkey->fpr.toUtf8().constData();
        }

        auto gpgmeError = gpgme_op_setexpire(mCtx, key.key_refer,
                                             expires_time, subfprs, 0);
        if (gpgmeError == GPG_ERR_NO_ERROR) {
            emit signalKeyUpdated(key.id);
            return true;
        } else {
            checkErr(gpgmeError);
            return false;
        }
    }

    bool GpgContext::checkIfKeyCanSign(const GpgKey &key) {
        if (std::any_of(key.subKeys.begin(), key.subKeys.end(), [](const GpgSubKey &subkey) -> bool {
            return subkey.secret && subkey.can_sign && !subkey.disabled && !subkey.revoked && !subkey.expired;
        }))
            return true;
        return false;
    }

    bool GpgContext::checkIfKeyCanCert(const GpgKey &key) {
        return key.has_master_key && !key.expired && !key.revoked && !key.disabled;
    }

    bool GpgContext::checkIfKeyCanAuth(const GpgKey &key) {
        if (std::any_of(key.subKeys.begin(), key.subKeys.end(), [](const GpgSubKey &subkey) -> bool {
            return subkey.secret && subkey.can_authenticate && !subkey.disabled && !subkey.revoked && !subkey.expired;
        }))
            return true;
        return false;
    }

    bool GpgContext::checkIfKeyCanEncr(const GpgKey &key) {
        if (std::any_of(key.subKeys.begin(), key.subKeys.end(), [](const GpgSubKey &subkey) -> bool {
            return subkey.can_encrypt && !subkey.disabled && !subkey.revoked && !subkey.expired;
        }))
            return true;
        return false;
    }

    gpgme_error_t GpgContext::encryptSign(QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer,
                                          gpgme_encrypt_result_t *encr_result, gpgme_sign_result_t *sign_result) {
        gpgme_data_t dataIn = nullptr, dataOut = nullptr;
        outBuffer->resize(0);

        setSigners(keys);

        //gpgme_encrypt_result_t e_result;
        gpgme_key_t recipients[keys.count() + 1];

        /* set key for user */
        int index = 0;
        for (const auto &key : keys) {
            recipients[index++] = key.key_refer;
        }
        //Last entry dataIn array has to be nullptr
        recipients[keys.count()] = nullptr;

        //If the last parameter isnt 0, a private copy of data is made
        if (mCtx != nullptr) {
            err = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
            if (gpg_err_code(err) == GPG_ERR_NO_ERROR) {
                err = gpgme_data_new(&dataOut);
                if (gpg_err_code(err) == GPG_ERR_NO_ERROR) {
                    err = gpgme_op_encrypt_sign(mCtx, recipients, GPGME_ENCRYPT_ALWAYS_TRUST, dataIn, dataOut);
                    if (encr_result != nullptr)
                        *encr_result = gpgme_op_encrypt_result(mCtx);
                    if (sign_result != nullptr)
                        *sign_result = gpgme_op_sign_result(mCtx);
                    if (gpg_err_code(err) == GPG_ERR_NO_ERROR) {
                        err = readToBuffer(dataOut, outBuffer);
                    }
                }
            }
        }

        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR)
            checkErr(err);

        if (dataIn) {
            gpgme_data_release(dataIn);
        }
        if (dataOut) {
            gpgme_data_release(dataOut);
        }

        return err;
    }

    gpgme_error_t
    GpgContext::decryptVerify(const QByteArray &inBuffer, QByteArray *outBuffer, gpgme_decrypt_result_t *decrypt_result,
                              gpgme_verify_result_t *verify_result) {
        gpgme_data_t dataIn = nullptr, dataOut = nullptr;

        outBuffer->resize(0);
        if (mCtx != nullptr) {
            err = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
            if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                err = gpgme_data_new(&dataOut);
                if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                    err = gpgme_op_decrypt_verify(mCtx, dataIn, dataOut);
                    if (decrypt_result != nullptr)
                        *decrypt_result = gpgme_op_decrypt_result(mCtx);
                    if (verify_result != nullptr)
                        *verify_result = gpgme_op_verify_result(mCtx);
                    if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                        err = readToBuffer(dataOut, outBuffer);
                    }
                }
            }
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

        return err;
    }

    bool GpgContext::exportKeys(const QVector<GpgKey> &keys, QByteArray &outBuffer) {
        size_t read_bytes;
        gpgme_data_t dataOut = nullptr;
        outBuffer.resize(0);

        if (keys.count() == 0) {
            QMessageBox::critical(nullptr, "Export Keys Error", "No Keys Selected");
            return false;
        }

        for (const auto &key : keys) {
            err = gpgme_data_new(&dataOut);
            checkErr(err);

            err = gpgme_op_export(mCtx, key.id.toUtf8().constData(), 0, dataOut);
            checkErr(err);

            read_bytes = gpgme_data_seek(dataOut, 0, SEEK_END);

            err = readToBuffer(dataOut, &outBuffer);
            checkErr(err);
            gpgme_data_release(dataOut);
        }
        return true;
    }

    QProcess * GpgContext::generateRevokeCert(const GpgKey &key, const QString &outputFileName) {
        QByteArray out, stdErr;
        auto process = executeGpgCommand({
                                  "--command-fd",
                                  "0",
                                  "--status-fd", "1",
                                  "-o",
                                  outputFileName,
                                  "--gen-revoke",
                                  key.fpr
                          }, &out, &stdErr,
                          [](QProcess *proc) {
                              qDebug() << "Function Called" << proc;
                              while (proc->canReadLine()) {
                                  const QString line = QString::fromUtf8(proc->readLine()).trimmed();
                                  // Command-fd is a stable interface, while this is all kind of hacky we
                                  // are on a deadline :-/
                                  if (line == QLatin1String("[GNUPG:] GET_BOOL gen_revoke.okay")) {
                                      proc->write("y\n");
                                  } else if (line == QLatin1String("[GNUPG:] GET_LINE ask_revocation_reason.code")) {
                                      proc->write("0\n");
                                  } else if (line == QLatin1String("[GNUPG:] GET_LINE ask_revocation_reason.text")) {
                                      proc->write("\n");
                                  } else if (line == QLatin1String("[GNUPG:] GET_BOOL openfile.overwrite.okay")) {
                                      // We asked before
                                      proc->write("y\n");
                                  } else if (line == QLatin1String("[GNUPG:] GET_BOOL ask_revocation_reason.okay")) {
                                      proc->write("y\n");
                                  }
                              }
                          });

        qDebug() << "GenerateRevokeCert Process" << process;

        return process;
    }
}
