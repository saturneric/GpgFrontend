/*
 *      gpgcontext.h
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef __SGPGMEPP_CONTEXT_H__
#define __SGPGMEPP_CONTEXT_H__

#include <GPG4USB.h>

#include "gpgconstants.h"


class GpgKey {
public:
    GpgKey() {
        privkey = false;
    }

    QString id;
    QString name;
    QString email;
    QString fpr;
    bool privkey;
    bool expired{};
    bool revoked{};
};

typedef QLinkedList<GpgKey> GpgKeyList;

class GpgImportedKey {
public:
    QString fpr;
    int importStatus;
};

typedef QLinkedList<GpgImportedKey> GpgImportedKeyList;

class GpgImportInformation {
public:
    GpgImportInformation() {
        considered = 0;
        no_user_id = 0;
        imported = 0;
        imported_rsa = 0;
        unchanged = 0;
        new_user_ids = 0;
        new_sub_keys = 0;
        new_signatures = 0;
        new_revocations = 0;
        secret_read = 0;
        secret_imported = 0;
        secret_unchanged = 0;
        not_imported = 0;
    }

    int considered;
    [[maybe_unused]] int no_user_id;
    int imported;
    [[maybe_unused]] int imported_rsa;
    int unchanged;
    [[maybe_unused]] int new_user_ids;
    [[maybe_unused]] int new_sub_keys;
    [[maybe_unused]] int new_signatures;
    [[maybe_unused]] int new_revocations;
    int secret_read;
    int secret_imported;
    int secret_unchanged;
    int not_imported;
    GpgImportedKeyList importedKeys;
};

namespace GpgME {

    class GpgContext : public QObject {
    Q_OBJECT

    public:
        GpgContext(); // Constructor
        ~GpgContext() override; // Destructor
        GpgImportInformation importKey(QByteArray inBuffer);

        bool exportKeys(QStringList *uidList, QByteArray *outBuffer);

        void generateKey(QString *params);

        GpgKeyList listKeys();

        void deleteKeys(QStringList *uidList);

        bool encrypt(QStringList *uidList, const QByteArray &inBuffer,
                     QByteArray *outBuffer);

        bool decrypt(const QByteArray &inBuffer, QByteArray *outBuffer);

        void clearPasswordCache();

        void exportSecretKey(QString uid, QByteArray *outBuffer);

        gpgme_key_t getKeyDetails(QString uid);

        gpgme_signature_t verify(QByteArray *inBuffer, QByteArray *sigBuffer = nullptr);

//    void decryptVerify(QByteArray in);
        bool sign(QStringList *uidList, const QByteArray &inBuffer, QByteArray *outBuffer, bool detached = false);

        /**
         * @details If text contains PGP-message, put a linebreak before the message,
         * so that gpgme can decrypt correctly
         *
         * @param in Pointer to the QBytearray to check.
         */
        void preventNoDataErr(QByteArray *in);

        GpgKey getKeyByFpr(QString fpr);

        GpgKey getKeyById(QString id);

        static QString gpgErrString(gpgme_error_t err);

        static QString getGpgmeVersion();

        /**
         * @brief
         *
         * @param text
         * @return \li 2, if the text is completly signed,
         *          \li 1, if the text is partially signed,
         *          \li 0, if the text is not signed at all.
         */
        int textIsSigned(const QByteArray &text);

        QString beautifyFingerprint(QString fingerprint);

    signals:

        void signalKeyDBChanged();

    private slots:

        void slotRefreshKeyList();

    private:
        gpgme_ctx_t mCtx;
        gpgme_data_t in;
        [[maybe_unused]] gpgme_data_t out;
        gpgme_error_t err;

        gpgme_error_t readToBuffer(gpgme_data_t in, QByteArray *outBuffer);

        QByteArray mPasswordCache;
        QSettings settings;
        [[maybe_unused]] bool debug;
        GpgKeyList mKeyList;

        [[nodiscard]] int checkErr(gpgme_error_t err) const;

        [[nodiscard]] int checkErr(gpgme_error_t err, QString comment) const;

        static gpgme_error_t passphraseCb(void *hook, const char *uid_hint,
                                          const char *passphrase_info,
                                          int last_was_bad, int fd);

        gpgme_error_t passphrase(const char *uid_hint,
                                 const char *passphrase_info,
                                 int last_was_bad, int fd);

        void executeGpgCommand(QStringList arguments,
                               QByteArray *stdOut,
                               QByteArray *stdErr);

        QString gpgBin;
        QString gpgKeys;
    };
} // namespace GpgME

#endif // __SGPGMEPP_CONTEXT_H__
