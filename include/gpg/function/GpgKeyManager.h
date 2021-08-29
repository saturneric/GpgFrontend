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


#ifndef GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H
#define GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H

#include "GpgFrontend.h"
#include "gpg/GpgModel.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgFunctionObject.h"

namespace GpgFrontend {

    class GpgKeyManager : public SingletonFunctionObject<GpgKeyManager> {
    public:

        /**
         * Sign a key pair(actually a certain uid)
         * @param target target key pair
         * @param uid target
         * @param expires expire date and time of the signature
         * @return if successful
         */
        bool signKey(const GpgKey &target, KeyArgsList &keys, const QString &uid,
                     std::unique_ptr<QDateTime> &expires);

        bool revSign(const GpgKey &key, const GpgKeySignature &signature);

        bool setExpire(const GpgKey &key, std::unique_ptr<GpgSubKey> &subkey, std::unique_ptr<QDateTime> &expires);

    private:

        GpgContext &ctx = GpgContext::getInstance();

    };

}


#endif //GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H
