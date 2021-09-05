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

#ifndef GPGFRONTEND_ZH_CN_TS_GPGMODEL_H
#define GPGFRONTEND_ZH_CN_TS_GPGMODEL_H

#include "gpg/model/GpgData.h"
#include "gpg/model/GpgKey.h"

#include <list>

namespace GpgFrontend {

using KeyIdArgsListPtr = std::unique_ptr<std::vector<std::string>>;

using KeyFprArgsListPtr = std::unique_ptr<std::vector<std::string>>;

using KeyArgsList = std::vector<GpgKey>;

using KeyListPtr = std::unique_ptr<std::vector<GpgKey>>;

using GpgKeyLinkList = std::list<GpgFrontend::GpgKey>;

using KeyPtr = std::unique_ptr<GpgKey>;

using KeyPtrArgsList = std::initializer_list<KeyPtr>;
} // namespace GpgFrontend

#endif // GPGFRONTEND_ZH_CN_TS_GPGMODEL_H
