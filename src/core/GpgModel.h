/**
 * Copyright (C) 2021 Saturneric
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef GPGFRONTEND_ZH_CN_TS_GPGMODEL_H
#define GPGFRONTEND_ZH_CN_TS_GPGMODEL_H

#include "core/GpgConstants.h"
#include "core/model/GpgData.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSignature.h"

namespace GpgFrontend {

using KeyId = std::string;                                                ///<
using SubkeyId = std::string;                                             ///<
using KeyIdArgsList = std::vector<KeyId>;                                 ///<
using KeyIdArgsListPtr = std::unique_ptr<KeyIdArgsList>;                  ///<
using UIDArgsList = std::vector<std::string>;                             ///<
using UIDArgsListPtr = std::unique_ptr<UIDArgsList>;                      ///<
using SignIdArgsList = std::vector<std::pair<std::string, std::string>>;  ///<
using SignIdArgsListPtr = std::unique_ptr<SignIdArgsList>;                ///<
using KeyFprArgsListPtr = std::unique_ptr<std::vector<std::string>>;      ///<
using KeyArgsList = std::vector<GpgKey>;                                  ///<
using KeyListPtr = std::unique_ptr<KeyArgsList>;                          ///<
using GpgKeyLinkList = std::list<GpgFrontend::GpgKey>;                    ///<
using KeyLinkListPtr = std::unique_ptr<GpgKeyLinkList>;                   ///<
using KeyPtr = std::unique_ptr<GpgKey>;                                   ///<
using KeyPtrArgsList = const std::initializer_list<KeyPtr>;               ///<

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_GPGMODEL_H
