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

#ifndef GPGFRONTEND_ZH_CN_TS_UIDOPERATOR_H
#define GPGFRONTEND_ZH_CN_TS_UIDOPERATOR_H

#include "GpgFrontend.h"
#include "gpg/GpgModel.h"
#include "gpg/GpgContext.h"

namespace GpgFrontend {

    class UidOperator {
    public:

        /**
         * create a new uid in certain key pair
         * @param key target key pair
         * @param uid uid args
         * @return if successful
         */
        bool addUID(const GpgKey &key, const GpgUID &uid);

        /**
         * Revoke(Delete) UID from certain key pair
         * @param key target key pair
         * @param uid target uid
         * @return if successful
         */
        bool revUID(const GpgKey &key, const GpgUID &uid);


        /**
         * Set one of a uid of a key pair as primary
         * @param key target key pair
         * @param uid target uid
         * @return if successful
         */
        bool setPrimaryUID(const GpgKey &key, const GpgUID &uid);

    private:

        GpgContext &ctx = GpgContext::getInstance();

    };

}


#endif //GPGFRONTEND_ZH_CN_TS_UIDOPERATOR_H
