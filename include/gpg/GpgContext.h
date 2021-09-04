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

#include "GpgFrontend.h"

#include "GpgConstants.h"
#include "GpgGenKeyInfo.h"
#include "GpgModel.h"
#include "GpgInfo.h"
#include "GpgFunctionObject.h"

using GpgKeyList = std::list<GpgFrontend::GpgKey>;

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

namespace GpgFrontend {

    static gpgme_error_t check_gpg_error(gpgme_error_t err);

    static gpgme_error_t check_gpg_error(gpgme_error_t gpgmeError, const std::string &comment);

    /**
     * Custom Encapsulation of GpgME APIs
     */
    class GpgContext : public SingletonFunctionObject<GpgContext> {

    public:

        GpgContext();

        ~GpgContext() override = default;

        [[nodiscard]] bool isGood() const;

        GpgImportInformation importKey(QByteArray inBuffer);

        [[nodiscard]] const GpgKeyList &getKeys() const;

        bool exportKeys(QStringList *uidList, QByteArray *outBuffer);

        bool exportKeys(const QVector<GpgKey> &keys, QByteArray &outBuffer);

        gpgme_error_t generateKey(GenKeyInfo *params);

        gpgme_error_t generateSubkey(const GpgKey &key, GenKeyInfo *params);

        const GpgInfo &getInfo() const { return info; }

        void deleteKeys(QStringList *uidList);

        void clearPasswordCache();

        bool exportSecretKey(const GpgKey &key, QByteArray *outBuffer) const;

        void generateRevokeCert(const GpgKey &key, const QString &outputFileName);

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

        operator gpgme_ctx_t() const { return _ctx_ref.get(); }

    signals:

        void signalKeyDBChanged();

        void signalKeyUpdated(QString key_id);

        void signalKeyInfoChanged();

    private slots:

        void slotRefreshKeyList();

        void slotUpdateKeyList(const QString &key_id);

    private:

        GpgInfo info;

        using CtxRefHandler = std::unique_ptr<struct gpgme_context, std::function<void(gpgme_ctx_t)>>;
        CtxRefHandler _ctx_ref = nullptr;

        gpgme_data_t in{};
        gpgme_error_t err;
        bool debug;
        bool good = true;

        QByteArray mPasswordCache;
        QSettings settings;
        GpgKeyList mKeyList;

        std::map<QString, GpgKey> mKeyMap;

        void fetch_keys();

        static gpgme_error_t passphraseCb(void *hook, const char *uid_hint,
                                          const char *passphrase_info,
                                          int last_was_bad, int fd);

        gpgme_error_t passphrase(const char *uid_hint,
                                 const char *passphrase_info,
                                 int last_was_bad, int fd);

        void executeGpgCommand(const QStringList &arguments, const std::function<void(QProcess *)> &interactFunc);

    };
} // namespace GpgME

#endif // __SGPGMEPP_CONTEXT_H__
