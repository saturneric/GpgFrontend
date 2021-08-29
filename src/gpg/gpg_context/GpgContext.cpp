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
#include "ui/WaitingDialog.h"

#include <functional>
#include <unistd.h>    /* contains read/write */

#ifdef _WIN32

#include <windows.h>

#endif

#define INT2VOIDP(i) (void*)(uintptr_t)(i)

namespace GpgFrontend {

    /**
     * Constructor
     *  Set up gpgme-context, set paths to app-run path
     */
    GpgContext::GpgContext() {

        gpgme_ctx_t _p_ctx;
        err = gpgme_new(&_p_ctx);
        check_gpg_error(err);
        _ctx_ref = CtxRefHandler(_p_ctx, [&](gpgme_ctx_t ctx) { gpgme_release(ctx); });

        gpgme_engine_info_t engineInfo;
        engineInfo = gpgme_ctx_get_engine_info(*this);

        // Check ENV before running
        bool check_pass = false, find_openpgp = false, find_gpgconf = false, find_assuan = false, find_cms = false;
        while (engineInfo != nullptr) {
            qDebug() << gpgme_get_protocol_name(engineInfo->protocol) << engineInfo->file_name << engineInfo->protocol
                     << engineInfo->home_dir << engineInfo->version;
            if (engineInfo->protocol == GPGME_PROTOCOL_GPGCONF && strcmp(engineInfo->version, "1.0.0") != 0)
                find_gpgconf = true;
            if (engineInfo->protocol == GPGME_PROTOCOL_OpenPGP && strcmp(engineInfo->version, "1.0.0") != 0)
                find_openpgp = true, info.appPath = engineInfo->file_name;
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
        gpgme_set_armor(*this, 1) ;
        // Speed up loading process
        gpgme_set_offline(*this, 1);
        /** passphrase-callback */
        gpgme_set_passphrase_cb(*this, passphraseCb, this);

        gpgme_set_keylist_mode(*this,
                               GPGME_KEYLIST_MODE_LOCAL
                               | GPGME_KEYLIST_MODE_WITH_SECRET
                               | GPGME_KEYLIST_MODE_SIGS
                               | GPGME_KEYLIST_MODE_SIG_NOTATIONS
                               | GPGME_KEYLIST_MODE_WITH_TOFU);

        /** check if app is called with -d from command line */
        if (qApp->arguments().contains("-d")) {
            qDebug() << "gpgme_data_t debug on";
            debug = true;
        } else debug = false;

        slotRefreshKeyList();
    }

    bool GpgContext::isGood() const {
        return good;
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
        WriteFile(hd, "\n", 1, &written, nullptr);

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
    gpgme_error_t check_gpg_error(gpgme_error_t err, const QString &comment) {
        //if (gpgmeError != GPG_ERR_NO_ERROR && gpgmeError != GPG_ERR_CANCELED) {
        if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
            qDebug() << "[Error " << gpg_err_code(err)
                     << "] Source: " << gpgme_strsource(err) << " Description: " << gpgme_strerror(err);
        }
        return err;
    }

    gpgme_error_t check_gpg_error(gpgme_error_t err) {
        //if (gpgmeError != GPG_ERR_NO_ERROR && gpgmeError != GPG_ERR_CANCELED) {
        if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
            qDebug() << "[Error " << gpg_err_code(err)
                     << "] Source: " << gpgme_strsource(err) << " Description: " << gpgme_strerror(err);
        }
        return err;
    }

    /** return type should be gpgme_error_t*/
    void
    GpgContext::executeGpgCommand(const QStringList &arguments, const std::function<void(QProcess *)> &interactFunc) {
        QEventLoop looper;
        auto dialog = new WaitingDialog(tr("Processing"), nullptr);
        dialog->show();
        auto *gpgProcess = new QProcess(&looper);
        gpgProcess->setProcessChannelMode(QProcess::MergedChannels);
        connect(gpgProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &looper, &QEventLoop::quit);
        connect(gpgProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), dialog,
                &WaitingDialog::deleteLater);
        connect(gpgProcess, &QProcess::errorOccurred, []() -> void { qDebug("Error in Process"); });
        connect(gpgProcess, &QProcess::errorOccurred, &looper, &QEventLoop::quit);
        connect(gpgProcess, &QProcess::started, []() -> void { qDebug() << "Gpg Process Started Success"; });
        connect(gpgProcess, &QProcess::readyReadStandardOutput, [interactFunc, gpgProcess]() {
            qDebug() << "Function Called";
            interactFunc(gpgProcess);
        });
        gpgProcess->setProgram(info.appPath);
        gpgProcess->setArguments(arguments);
        gpgProcess->start();
        looper.exec();
        dialog->close();

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

    void GpgContext::slotUpdateKeyList(const QString &key_id) {
        auto it = mKeyMap.find(key_id);
        if (it != mKeyMap.end()) {
            gpgme_key_t new_key_ref;

            auto gpgmeErr = gpgme_get_key(*this, key_id.toUtf8().constData(), &new_key_ref, 0);

            if (gpgme_err_code(gpgmeErr) == GPG_ERR_EOF) {
                gpgmeErr = gpgme_get_key(*this, key_id.toUtf8().constData(), &new_key_ref, 1);

                if (gpgme_err_code(gpgmeErr) == GPG_ERR_EOF)
                    throw std::runtime_error("key_id not found in key database");
            }

            if (new_key_ref != nullptr) {
                it->second = std::move(GpgKey(std::move(new_key_ref)));
                emit signalKeyInfoChanged();
            }

        }
    }

}
