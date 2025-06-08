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

#include "core/model/GpgAbstractKey.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/typedef/CoreTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

// Error Info Printer

/**
 * @brief
 *
 * @param err
 * @return GpgError
 */
auto GF_CORE_EXPORT CheckGpgError(GpgError err) -> GpgError;

/**
 * @brief
 *
 * @param gpgmeError
 * @param comment
 * @return GpgError
 */
auto GF_CORE_EXPORT CheckGpgError(GpgError gpgmeError, const QString& comment)
    -> GpgError;

/**
 * @brief
 *
 * @param err
 * @param predict
 * @return gpg_err_code_t
 */
auto GF_CORE_EXPORT CheckGpgError2ErrCode(
    gpgme_error_t err, gpgme_error_t predict = GPG_ERR_NO_ERROR)
    -> gpg_err_code_t;

/**
 * @brief
 *
 * @param err
 * @return GpgErrorDesc
 */
auto GF_CORE_EXPORT DescribeGpgErrCode(GpgError err) -> GpgErrorDesc;

// Check

/**
 * @brief
 *
 * @param text
 * @return int
 */
auto GF_CORE_EXPORT TextIsSigned(BypeArrayRef text) -> int;

/**
 * @brief
 *
 * @param opera
 * @param ascii
 * @return QString
 */
auto GF_CORE_EXPORT SetExtensionOfOutputFile(const QString& path,
                                             GpgOperation opera, bool ascii)
    -> QString;

/**
 * @brief
 *
 * @param path
 * @param opera
 * @param ascii
 * @return QString
 */
auto GF_CORE_EXPORT SetExtensionOfOutputFileForArchive(const QString& path,
                                                       GpgOperation opera,
                                                       bool ascii) -> QString;

/**
 * @brief
 *
 * @param app_path
 * @param path
 * @return QString
 */
auto GF_CORE_EXPORT GetCanonicalKeyDatabasePath(const QDir& app_path,
                                                const QString& path) -> QString;

/**
 * @brief Get the Key Databases By Settings object
 *
 * @return QContainer<KeyDatabaseItemSO>
 */
auto GF_CORE_EXPORT GetKeyDatabasesBySettings()
    -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief
 *
 * @return QContainer<KeyDatabaseInfo>
 */
auto GF_CORE_EXPORT GetKeyDatabaseInfoBySettings()
    -> QContainer<KeyDatabaseInfo>;

/**
 * @brief
 *
 * @return QContainer<KeyDatabaseItemSO>
 */
auto GF_CORE_EXPORT GetGpgKeyDatabaseInfos() -> QContainer<KeyDatabaseInfo>;

/**
 * @brief
 *
 * @return QContainer<KeyDatabaseItemSO>
 */
auto GF_CORE_EXPORT GetGpgKeyDatabaseName(int channel) -> QString;

/**
 * @brief
 *
 * @param channel
 * @param keys
 * @return KeyIdArgsList
 */
auto GF_CORE_EXPORT ConvertKey2GpgKeyIdList(int channel,
                                            const GpgAbstractKeyPtrList& keys)
    -> KeyIdArgsList;

/**
 * @brief
 *
 * @param channel
 * @param keys
 * @return GpgKeyPtrList
 */
auto GF_CORE_EXPORT ConvertKey2GpgKeyList(int channel,
                                          const GpgAbstractKeyPtrList& keys)
    -> GpgKeyPtrList;

/**
 * @brief
 *
 * @param keys
 * @return QContainer<gpgme_key_t>
 */
auto GF_CORE_EXPORT Convert2RawGpgMEKeyList(int channel,
                                            const GpgAbstractKeyPtrList& keys)
    -> QContainer<gpgme_key_t>;

/**
 * @brief
 *
 * @param key
 * @return QString
 */
auto GF_CORE_EXPORT GetUsagesByAbstractKey(const GpgAbstractKey* key)
    -> QString;

/**
 * @brief
 *
 * @return GpgKey
 */
auto GF_CORE_EXPORT GetGpgKeyByGpgAbstractKey(GpgAbstractKey*) -> GpgKey;

/**
 * @brief
 *
 * @param id
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT IsKeyGroupID(const KeyId& id) -> bool;

/**
 * @brief
 *
 * @return bool
 */
auto GF_CORE_EXPORT GpgAgentVersionGreaterThan(int channel, const QString&)
    -> bool;

/**
 * @brief
 *
 * @param channel
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT CheckGpgVersion(int channel, const QString&) -> bool;

/**
 * @brief
 *
 * @return QString
 */
auto GF_CORE_EXPORT DecidePinentry() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GF_CORE_EXPORT GnuPGVersion() -> QString;
}  // namespace GpgFrontend