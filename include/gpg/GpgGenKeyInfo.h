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

#ifndef GPG4USB_GPGGENKEYINFO_H
#define GPG4USB_GPGGENKEYINFO_H

#include "GpgFrontend.h"

class GenKeyInfo {

    bool subKey = true;
    QString userid;
    QString algo;
    int keySize = 2048;
    QDateTime expired = QDateTime::currentDateTime().addYears(2);
    bool nonExpired = false;

    bool noPassPhrase = false;
    bool allowNoPassPhrase = true;

    int suggestMaxKeySize = 1024;
    int suggestSizeAdditionStep = 1024;
    int suggestMinKeySize = 4096;

    QString passPhrase;

public:

    static const QVector<QString> SupportedKeyAlgo;

    static const QVector<QString> SupportedSubkeyAlgo;

    [[nodiscard]] bool isSubKey() const {
        return subKey;
    }

    void setIsSubKey(bool m_sub_key) {
        GenKeyInfo::subKey = m_sub_key;
    }

    [[nodiscard]] const QString &getUserid() const {
        return userid;
    }

    void setUserid(const QString &m_userid) {
        GenKeyInfo::userid = m_userid;
    }

    [[nodiscard]] const QString &getAlgo() const {
        return algo;
    }

    void setAlgo(const QString &m_algo);

    [[nodiscard]] QString getKeySizeStr() const;

    [[nodiscard]] int getKeySize() const {
        return keySize;

    }

    void setKeySize(int m_key_size);

    [[nodiscard]] const QDateTime &getExpired() const {
        return expired;
    }

    void setExpired(const QDateTime &m_expired);

    [[nodiscard]] bool isNonExpired() const {
        return nonExpired;
    }

    void setNonExpired(bool m_non_expired);

    [[nodiscard]] bool isNoPassPhrase() const {
        return this->noPassPhrase;
    }

    void setNonPassPhrase(bool m_non_pass_phrase) {
        GenKeyInfo::noPassPhrase = m_non_pass_phrase;
    }

    [[nodiscard]] bool isAllowSigning() const {
        return allowSigning;
    }

    [[nodiscard]] bool isAllowNoPassPhrase() const {
        return allowNoPassPhrase;
    }

    void setAllowSigning(bool m_allow_signing) {
        if(allowChangeSigning)
            GenKeyInfo::allowSigning = m_allow_signing;
    }

    [[nodiscard]] bool isAllowEncryption() const {
        return allowEncryption;
    }

    void setAllowEncryption(bool m_allow_encryption);

    [[nodiscard]] bool isAllowCertification() const {
        return allowCertification;
    }

    void setAllowCertification(bool m_allow_certification);

    [[nodiscard]] bool isAllowAuthentication() const {
        return allowAuthentication;
    }

    void setAllowAuthentication(bool m_allow_authentication) {
        if(allowChangeAuthentication)
            GenKeyInfo::allowAuthentication = m_allow_authentication;
    }

    [[nodiscard]] const QString &getPassPhrase() const {
        return passPhrase;
    }

    void setPassPhrase(const QString &m_pass_phrase) {
        GenKeyInfo::passPhrase = m_pass_phrase;
    }

    [[nodiscard]] bool isAllowChangeSigning() const {
        return allowChangeSigning;
    }
    [[nodiscard]] bool isAllowChangeEncryption() const {
        return allowChangeEncryption;
    }

    [[nodiscard]] bool isAllowChangeCertification() const {
        return allowChangeCertification;
    }

    [[nodiscard]] bool isAllowChangeAuthentication() const {
        return allowChangeAuthentication;
    }

    [[nodiscard]] int getSuggestMaxKeySize() const {
        return suggestMaxKeySize;
    }

    [[nodiscard]] int getSuggestMinKeySize() const {
        return suggestMinKeySize;
    }

    [[nodiscard]] int getSizeChangeStep() const {
        return suggestSizeAdditionStep;
    }


private:
    bool allowEncryption = true;
    bool allowChangeEncryption = true;

    bool allowCertification = true;
    bool allowChangeCertification = true;

    bool allowAuthentication = true;
    bool allowChangeAuthentication = true;

    bool allowSigning = true;
    bool allowChangeSigning = true;

    void reset_options();

public:

    explicit GenKeyInfo(bool m_is_sub_key = false) : subKey(m_is_sub_key) {
        setAlgo("rsa");
    }


};

#endif //GPG4USB_GPGGENKEYINFO_H
