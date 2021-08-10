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

/**
 * Import key pair
 * @param inBuffer input byte array
 * @return Import information
 */
GpgImportInformation GpgME::GpgContext::importKey(QByteArray inBuffer) {
    auto *importInformation = new GpgImportInformation();
    err = gpgme_data_new_from_mem(&in, inBuffer.data(), inBuffer.size(), 1);
    checkErr(err);
    err = gpgme_op_import(mCtx, in);
    gpgme_import_result_t result;

    result = gpgme_op_import_result(mCtx);

    if (result->unchanged) importInformation->unchanged = result->unchanged;
    if (result->considered) importInformation->considered = result->considered;
    if (result->no_user_id) importInformation->no_user_id = result->no_user_id;
    if (result->imported) importInformation->imported = result->imported;
    if (result->imported_rsa) importInformation->imported_rsa = result->imported_rsa;
    if (result->unchanged) importInformation->unchanged = result->unchanged;
    if (result->new_user_ids) importInformation->new_user_ids = result->new_user_ids;
    if (result->new_sub_keys) importInformation->new_sub_keys = result->new_sub_keys;
    if (result->new_signatures) importInformation->new_signatures = result->new_signatures;
    if (result->new_revocations) importInformation->new_revocations = result->new_revocations;
    if (result->secret_read) importInformation->secret_read = result->secret_read;
    if (result->secret_imported) importInformation->secret_imported = result->secret_imported;
    if (result->secret_unchanged) importInformation->secret_unchanged = result->secret_unchanged;
    if (result->not_imported) importInformation->not_imported = result->not_imported;

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

/**
 * Generate a new key pair
 * @param params key generation args
 * @return error information
 */
gpgme_error_t GpgME::GpgContext::generateKey(GenKeyInfo *params) {

    auto userid_utf8 = params->getUserid().toUtf8();
    const char *userid = userid_utf8.constData();
    auto algo_utf8 = (params->getAlgo() + params->getKeySizeStr()).toUtf8();
    const char *algo = algo_utf8.constData();
    unsigned long expires = QDateTime::currentDateTime().secsTo(params->getExpired());
    unsigned int flags = 0;

    if (!params->isSubKey()) flags |= GPGME_CREATE_CERT;
    if (params->isAllowEncryption()) flags |= GPGME_CREATE_ENCR;
    if (params->isAllowSigning()) flags |= GPGME_CREATE_SIGN;
    if (params->isAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
    if (params->isNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
    if (params->isNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

    err = gpgme_op_createkey(mCtx, userid, algo, 0, expires, nullptr, flags);

    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        checkErr(err);
        return err;
    } else {
        emit signalKeyDBChanged();
        return err;
    }
}

/**
 * Export Key
 * @param uidList key ids
 * @param outBuffer output byte array
 * @return if success
 */
bool GpgME::GpgContext::exportKeys(QStringList *uidList, QByteArray *outBuffer) {
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
 * Get and store all key pairs info
 */
void GpgME::GpgContext::fetch_keys() {

    gpgme_error_t gpgmeError;

    gpgme_key_t key;

    qDebug() << "Clear List and Map";

    mKeyList.clear();
    mKeyMap.clear();

    auto &keys = mKeyList;
    auto &keys_map = mKeyMap;

    qDebug() << "Set Key Listing Mode";

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

/**
 * Delete keys
 * @param uidList key ids
 */
void GpgME::GpgContext::deleteKeys(QStringList *uidList) {

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

/**
 * Export keys
 * @param keys keys used
 * @param outBuffer output byte array
 * @return if success
 */
bool GpgME::GpgContext::exportKeys(const QVector<GpgKey> &keys, QByteArray &outBuffer) {
    size_t read_bytes;
    gpgme_data_t data_out = nullptr;
    outBuffer.resize(0);

    if (keys.empty()) {
        QMessageBox::critical(nullptr, "Export Keys Error", "No Keys Selected");
        return false;
    }

    for (const auto &key : keys) {
        err = gpgme_data_new(&data_out);
        checkErr(err);

        err = gpgme_op_export(mCtx, key.id.toUtf8().constData(), 0, data_out);
        checkErr(err);

        read_bytes = gpgme_data_seek(data_out, 0, SEEK_END);

        err = readToBuffer(data_out, &outBuffer);
        checkErr(err);
        gpgme_data_release(data_out);
    }

    return true;
}

/**
 * Set the expire date and time of a key pair(actually the master key) or subkey
 * @param key target key pair
 * @param subkey null if master key
 * @param expires date and time
 * @return if successful
 */
bool GpgME::GpgContext::setExpire(const GpgKey &key, const GpgSubKey *subkey, QDateTime *expires) {
    unsigned long expires_time = 0;
    if (expires != nullptr) {
        qDebug() << "Expire Datetime" << expires->toString();
        expires_time = QDateTime::currentDateTime().secsTo(*expires);
    }

    const char *subfprs = nullptr;

    if (subkey != nullptr) subfprs = subkey->fpr.toUtf8().constData();

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

/**
 * Export the secret key of a key pair(including subkeys)
 * @param key target key pair
 * @param outBuffer output byte array
 * @return if successful
 */
bool GpgME::GpgContext::exportSecretKey(const GpgKey &key, QByteArray *outBuffer) {
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

/**
 * Sign a key pair(actually a certain uid)
 * @param target target key pair
 * @param uid target
 * @param expires expire date and time of the signature
 * @return if successful
 */
bool GpgME::GpgContext::signKey(const GpgKey &target, const QString &uid, const QDateTime *expires) {

    unsigned int flags = 0;

    unsigned int expires_time_t = 0;
    if (expires == nullptr) flags |= GPGME_KEYSIGN_NOEXPIRE;
    else expires_time_t = QDateTime::currentDateTime().secsTo(*expires);

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

/**
 * Generate revoke cert of a key pair (TODO)
 * @param key target key pair
 * @param outputFileName out file name(path)
 * @return the process doing this job
 */
QProcess *GpgME::GpgContext::generateRevokeCert(const GpgKey &key, const QString &outputFileName) {
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
                                             } else if (line == QLatin1String(
                                                     "[GNUPG:] GET_LINE ask_revocation_reason.code")) {
                                                 proc->write("0\n");
                                             } else if (line == QLatin1String(
                                                     "[GNUPG:] GET_LINE ask_revocation_reason.text")) {
                                                 proc->write("\n");
                                             } else if (line == QLatin1String(
                                                     "[GNUPG:] GET_BOOL openfile.overwrite.okay")) {
                                                 // We asked before
                                                 proc->write("y\n");
                                             } else if (line == QLatin1String(
                                                     "[GNUPG:] GET_BOOL ask_revocation_reason.okay")) {
                                                 proc->write("y\n");
                                             }
                                         }
                                     });

    qDebug() << "GenerateRevokeCert Process" << process;

    return process;
}
