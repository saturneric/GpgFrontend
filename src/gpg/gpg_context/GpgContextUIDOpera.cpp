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
 * create a new uid in certain key pair
 * @param key target key pair
 * @param uid uid args
 * @return if successful
 */
bool GpgME::GpgContext::addUID(const GpgKey &key, const GpgUID &uid) {
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

/**
 * Revoke(Delete) UID from certain key pair
 * @param key target key pair
 * @param uid target uid
 * @return if successful
 */
bool GpgME::GpgContext::revUID(const GpgKey &key, const GpgUID &uid) {
    auto gpgmeError = gpgme_op_revuid(mCtx, key.key_refer, uid.uid.toUtf8().constData(), 0);
    if (gpgmeError == GPG_ERR_NO_ERROR) {
        emit signalKeyUpdated(key.id);
        return true;
    } else {
        checkErr(gpgmeError);
        return false;
    }
}

/**
 * Set one of a uid of a key pair as primary
 * @param key target key pair
 * @param uid target uid
 * @return if successful
 */
bool GpgME::GpgContext::setPrimaryUID(const GpgKey &key, const GpgUID &uid) {
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

