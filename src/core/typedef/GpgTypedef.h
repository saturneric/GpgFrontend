/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <gpgme.h>

#include <future>

#include "core/model/DataObject.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

class GpgKey;  ///< forward declaration
class GpgSubKey;
class GpgSignature;
class GpgTOFUInfo;

using GpgError = gpgme_error_t;  ///< gpgme error
using GpgErrorCode = gpg_err_code_t;
using GpgErrorDesc = QPair<QString, QString>;

using KeyId = QString;
using SubkeyId = QString;
using KeyIdArgsList = QStringList;                           ///<
using UIDArgsList = QStringList;                             ///<
using SignIdArgsList = QContainer<QPair<QString, QString>>;  ///<
using KeyArgsList = QContainer<GpgKey>;                      ///<
using GpgKeyLinkList = QContainer<GpgKey>;                   ///<
using GpgKeyList = QContainer<GpgKey>;                       ///<

using GpgSignMode = gpgme_sig_mode_t;

using GpgOperaRunnable = std::function<GpgError(DataObjectPtr)>;
using GpgOperationCallback = std::function<void(GpgError, DataObjectPtr)>;
using GpgOperationFuture = std::future<std::tuple<GpgError, DataObjectPtr>>;

enum GpgOperation {
  kNONE = 0,
  kENCRYPT = 1 << 0,
  kDECRYPT = 1 << 1,
  kSIGN = 1 << 2,
  kVERIFY = 1 << 3,
  kENCRYPT_SIGN = 1 << 4,
  kDECRYPT_VERIFY = 1 << 5,
  kAUTH = 1 << 6,
  kCERT = 1 << 7,
};
}  // namespace GpgFrontend