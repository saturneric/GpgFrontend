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

#ifndef __SGPGMEPP_CONTEXT_H__
#define __SGPGMEPP_CONTEXT_H__

#include <GpgFrontend.h>

#include "GpgConstants.h"
#include "GpgGenKeyInfo.h"
#include "GpgKey.h"

using GpgKeyList = std::list<GpgKey>;

class GpgImportedKey {
public:
    QString fpr;
    int importStatus;
};

typedef std::list<GpgImportedKey> GpgImportedKeyList;

class GpgImportInformation {
public:
    GpgImportInformation() = default;

    int considered = 0;
    int no_user_id = 0;
    int imported = 0;
    int imported_rsa = 0;
    int unchanged = 0;
    int new_user_ids = 0;
    int new_sub_keys = 0;
    int new_signatures = 0;
    int new_revocations = 0;
    int secret_read = 0;
    int secret_imported = 0;
    int secret_unchanged = 0;
    int not_imported = 0;
    GpgImportedKeyList importedKeys;
};

namespace GpgME {

    class GpgContext : public QObject {
    Q_OBJECT

    public:
        GpgContext();

        ~GpgContext() override;

        [[nodiscard]] bool isGood() const;

        GpgImportInformation importKey(QByteArray inBuffer);

        [[nodiscard]] const GpgKeyList &getKeys() const;

        bool exportKeys(QStringList *uidList, QByteArray *outBuffer);

        bool exportKeys(const QVector<GpgKey> &keys, QByteArray &outBuffer);

        bool generateKey(GenKeyInfo *params);

        bool generateSubkey(const GpgKey &key, GenKeyInfo *params);

        void deleteKeys(QStringList *uidList);

        gpg_error_t encrypt(QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer,
                            gpgme_encrypt_result_t *result);

        gpgme_error_t encryptSign(QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer,
                                  gpgme_encrypt_result_t *encr_result, gpgme_sign_result_t *sign_result);

        gpgme_error_t decrypt(const QByteArray &inBuffer, QByteArray *outBuffer, gpgme_decrypt_result_t *result);

        gpgme_error_t
        decryptVerify(const QByteArray &inBuffer, QByteArray *outBuffer, gpgme_decrypt_result_t *decrypt_result,
                      gpgme_verify_result_t *verify_result);

        void clearPasswordCache();

        bool exportSecretKey(const GpgKey &key, QByteArray *outBuffer);

        void getSigners(QVector<GpgKey> &signer);

        void setSigners(const QVector<GpgKey> &keys);

        bool signKey(const GpgKey &target, const QString &uid, const QDateTime *expires);

        bool revSign(const GpgKey &key, const GpgKeySignature &signature);

        gpgme_error_t verify(QByteArray *inBuffer, QByteArray *sigBuffer, gpgme_verify_result_t *result);

        gpg_error_t
        sign(const QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer, bool detached = false,
             gpgme_sign_result_t *result = nullptr);

        bool addUID(const GpgKey &key, const GpgUID &uid);

        bool revUID(const GpgKey &key, const GpgUID &uid);

        bool setPrimaryUID(const GpgKey &key, const GpgUID &uid);

        bool setExpire(const GpgKey &key, const GpgSubKey *subkey, QDateTime *expires);

        QProcess * generateRevokeCert(const GpgKey &key, const QString &outputFileName);

        static bool checkIfKeyCanSign(const GpgKey &key);

        static bool checkIfKeyCanCert(const GpgKey &key);

        static bool checkIfKeyCanAuth(const GpgKey &key);

        static bool checkIfKeyCanEncr(const GpgKey &key);


        /**
         * @details If text contains PGP-message, put a linebreak before the message,
         * so that gpgme can decrypt correctly
         *
         * @param in Pointer to the QBytearray to check.
         */
        static void preventNoDataErr(QByteArray *in);

        GpgKey getKeyByFpr(const QString &fpr);

        const GpgKey &getKeyById(const QString &id);

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
        static int textIsSigned(const QByteArray &text);

        static QString beautifyFingerprint(QString fingerprint);

    signals:

        void signalKeyDBChanged();

        void signalKeyUpdated(QString key_id);

        void signalKeyInfoChanged();

    private slots:

        void slotRefreshKeyList();

        void slotUpdateKeyList(const QString &key_id);

    private:
        gpgme_ctx_t mCtx{};
        gpgme_data_t in{};
        gpgme_error_t err;
        bool debug;
        bool good = true;

        static gpgme_error_t readToBuffer(gpgme_data_t dataIn, QByteArray *outBuffer);

        QByteArray mPasswordCache;
        QSettings settings;
        GpgKeyList mKeyList;

        QMap<QString, GpgKey *> mKeyMap;

        void fetch_keys();

        static void checkErr(gpgme_error_t gpgmeError);

        static void checkErr(gpgme_error_t gpgmeError, const QString &comment);

        static gpgme_error_t passphraseCb(void *hook, const char *uid_hint,
                                          const char *passphrase_info,
                                          int last_was_bad, int fd);

        gpgme_error_t passphrase(const char *uid_hint,
                                 const char *passphrase_info,
                                 int last_was_bad, int fd);

        QProcess * executeGpgCommand(const QStringList &arguments,
                                     QByteArray *stdOut,
                                     QByteArray *stdErr, const std::function<void(QProcess *)> &interactFunc);

        QString gpgExec;
        QString gpgKeys;
    };
} // namespace GpgME

#endif // __SGPGMEPP_CONTEXT_H__
