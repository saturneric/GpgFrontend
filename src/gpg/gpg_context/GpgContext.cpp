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

    /**  Read gpgme-Data to QByteArray
     *   mainly from http://basket.kde.org/ (kgpgme.cpp)
     */
#define BUF_SIZE (32 * 1024)

    gpgme_error_t GpgContext::readToBuffer(gpgme_data_t dataIn, QByteArray *outBuffer) {
        gpgme_off_t ret;
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

    /**
     * The Passphrase window, if not provided by env-Var GPG_AGENT_INFO
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
        } else result = true;

        if (result) {

#ifndef _WIN32
            if (write(fd, mPasswordCache.data(), mPasswordCache.length()) == -1) qDebug() << "something is terribly broken";
#else
            WriteFile(hd, mPasswordCache.data(), mPasswordCache.length(), &written, 0);
#endif
            returnValue = GPG_ERR_NO_ERROR;
        }

#ifndef _WIN32
        if (write(fd, "\n", 1) == -1) qDebug() << "something is terribly broken";
#else
        WriteFile(hd, "\n", 1, &written, 0);

        /* program will hang on cancel if hd not closed */
        if (!result) CloseHandle(hd);
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

    /** return type should be gpgme_error_t*/
    QProcess *GpgContext::executeGpgCommand(const QStringList &arguments, QByteArray *stdOut, QByteArray *stdErr,
                                            const std::function<void(QProcess *)> &interactFunc) {
        QStringList args;
        args << arguments;

        auto *gpgProcess = new QProcess(this);
        qDebug() << "gpgExec" << gpgExec << args;

        gpgProcess->setReadChannel(QProcess::StandardOutput);
        connect(gpgProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
                gpgProcess, SLOT(deleteLater()));
        connect(gpgProcess, &QProcess::readyReadStandardOutput, this, [gpgProcess, interactFunc]() {
            qDebug() << "Function Called" << &gpgProcess;
            // interactFunc(gpgProcess);
        });

        gpgProcess->start(gpgExec, args);

        if (gpgProcess->waitForStarted()) {
            qDebug() << "Gpg Process Started Success";
        } else {
            qDebug() << "Gpg Process Started Failed";
        }

        return gpgProcess;
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
            text.trimmed().endsWith(GpgConstants::PGP_SIGNED_END))
            return 2;
        else if (text.contains(GpgConstants::PGP_SIGNED_BEGIN) && text.contains(GpgConstants::PGP_SIGNED_END))
            return 1;

        else return 0;
    }

    QString GpgContext::beautifyFingerprint(QString fingerprint) {
        uint len = fingerprint.length();
        if ((len > 0) && (len % 4 == 0))
            for (uint n = 0; 4 * (n + 1) < len; ++n) fingerprint.insert(static_cast<int>(5u * n + 4u), ' ');
        return fingerprint;
    }

    void GpgContext::slotRefreshKeyList() {
        qDebug() << "Refreshing Keys";
        this->fetch_keys();
        emit signalKeyInfoChanged();
    }

    QString GpgContext::getGpgmeVersion() {
        return {gpgme_check_version(nullptr)};
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

}
