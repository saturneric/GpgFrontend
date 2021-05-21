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

#include "gpg/GpgGenKeyInfo.h"

const QVector<QString> GenKeyInfo::SupportedKeyAlgo = {
        "RSA",
        "DSA",
        "ED25519"
};

void GenKeyInfo::setAlgo(const QString &m_algo) {

    qDebug() << "set algo " << m_algo;

    reset_options();

    if (!this->subKey) {
        this->setAllowCertification(true);
        this->allowChangeCertification = false;
    }

    auto lower_algo = m_algo.toLower();

    if(lower_algo == "rsa") {
        /**
         * RSA is the world’s premier asymmetric cryptographic algorithm,
         * and is built on the difficulty of factoring extremely large composites.
         * GnuPG supports RSA with key sizes of between 1024 and 4096 bits.
         */
        suggestMinKeySize = 1024;
        suggestMaxKeySize = 4096;
        suggestSizeAdditionStep = 1024;
        setKeySize(2048);

    } else if (lower_algo == "dsa") {
        /**
         * Algorithm (DSA) as a government standard for digital signatures.
         * Originally, it supported key lengths between 512 and 1024 bits.
         * Recently, NIST has declared 512-bit keys obsolete:
         * now, DSA is available in 1024, 2048 and 3072-bit lengths.
         */
        setAllowEncryption(false);
        allowChangeEncryption = false;

        suggestMinKeySize = 1024;
        suggestMaxKeySize = 3072;
        suggestSizeAdditionStep = 1024;
        setKeySize(2048);

    } else if (lower_algo == "ed25519") {
        /**
         * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths ranging from 1024 to 4096 bits.
         */

        setAllowEncryption(false);
        allowChangeEncryption = false;

        suggestMinKeySize = -1;
        suggestMaxKeySize = -1;
        suggestSizeAdditionStep = -1;
        setKeySize(-1);
    }
    GenKeyInfo::algo = lower_algo;
}

void GenKeyInfo::reset_options() {

    allowChangeEncryption = true;
    setAllowEncryption(true);

    allowChangeCertification = true;
    setAllowCertification(true);

    allowChangeSigning = true;
    setAllowSigning(true);

    allowChangeAuthentication = true;
    setAllowAuthentication(true);


    passPhrase.clear();

}

QString GenKeyInfo::getKeySizeStr() const {
    if(keySize > 0) {
        return QString::number(keySize);
    }
    else {
        return QString();
    }

}

void GenKeyInfo::setKeySize(int m_key_size) {
    if (m_key_size < suggestMinKeySize || m_key_size > suggestMaxKeySize) {
        return;
    }
    GenKeyInfo::keySize = m_key_size;
}

void GenKeyInfo::setExpired(const QDateTime &m_expired) {
    auto current = QDateTime::currentDateTime();
    if (isNonExpired() && m_expired < current.addYears(2)) {
        GenKeyInfo::expired = m_expired;
    }
}

void GenKeyInfo::setNonExpired(bool m_non_expired) {
    if (!m_non_expired) {
        this->expired = QDateTime(QDateTime::fromTime_t(0));
    }
    GenKeyInfo::nonExpired = m_non_expired;
}

void GenKeyInfo::setAllowEncryption(bool m_allow_encryption) {
    if(allowChangeEncryption)
        GenKeyInfo::allowEncryption = m_allow_encryption;
}

void GenKeyInfo::setAllowCertification(bool m_allow_certification) {
    if(allowChangeCertification)
        GenKeyInfo::allowCertification = m_allow_certification;
}

