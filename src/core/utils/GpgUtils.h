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

#include "core/GpgCoreRust.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GpgAbstractKey.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/typedef/CoreTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Log a warning if @p err is non-zero and return the error code.
 *
 * Strips the gpgme error source and returns only the error code portion.
 *
 * @param err gpgme error value
 * @return gpg_err_code_t error code (GPG_ERR_NO_ERROR on success)
 */
auto GF_CORE_EXPORT CheckGpgError(GpgError err) -> GpgError;

/**
 * @brief Log a warning with the error and an additional comment if @p err is non-zero.
 *
 * @param gpgmeError gpgme error value
 * @param comment additional context string included in the log message
 * @return the original @p gpgmeError value unchanged
 */
auto GF_CORE_EXPORT CheckGpgError(GpgError gpgmeError, const QString& comment)
    -> GpgError;

/**
 * @brief Compare @p err against @p predict and log a warning if they differ.
 *
 * @param err gpgme error value to check
 * @param predict expected error code (default: GPG_ERR_NO_ERROR)
 * @return error code portion of @p err
 */
auto GF_CORE_EXPORT CheckGpgError2ErrCode(
    gpgme_error_t err, gpgme_error_t predict = GPG_ERR_NO_ERROR)
    -> gpg_err_code_t;

/**
 * @brief Return a (source, description) pair describing the given error.
 *
 * @param err gpgme error value
 * @return GpgErrorDesc with the error source name and human-readable description
 */
auto GF_CORE_EXPORT DescribeGpgErrCode(GpgError err) -> GpgErrorDesc;

/**
 * @brief Return whether the given text contains PGP signature markers.
 *
 * @param text byte array reference to inspect
 * @return non-zero if a PGP signature is detected, 0 otherwise
 */
auto GF_CORE_EXPORT TextIsSigned(BypeArrayRef text) -> int;

/**
 * @brief Return the output file path with an extension appropriate for the given operation.
 *
 * @param path input file path to derive the output path from
 * @param opera GPG operation type (encrypt, sign, etc.)
 * @param ascii true if ASCII-armored output is requested
 * @return output file path with the correct extension
 */
auto GF_CORE_EXPORT SetExtensionOfOutputFile(const QString& path,
                                             GpgOperation opera, bool ascii)
    -> QString;

/**
 * @brief Return the output file path with an archive-operation extension.
 *
 * @param path input file path
 * @param opera GPG operation type
 * @param ascii true if ASCII-armored output is requested
 * @return output file path with the correct archive extension
 */
auto GF_CORE_EXPORT SetExtensionOfOutputFileForArchive(const QString& path,
                                                       GpgOperation opera,
                                                       bool ascii) -> QString;

/**
 * @brief Resolve and canonicalize a key database path relative to @p app_path.
 *
 * @param app_path base application directory
 * @param path raw path from configuration
 * @return canonical absolute path to the key database directory
 */
auto GF_CORE_EXPORT GetCanonicalKeyDatabasePath(const QDir& app_path,
                                                const QString& path) -> QString;

/**
 * @brief Return all key database infos (both GPG and custom) from settings.
 *
 * @return list of KeyDatabaseInfo for every configured database
 */
auto GF_CORE_EXPORT GetAllKeyDatabaseInfoBySettings()
    -> QContainer<KeyDatabaseInfo>;

/**
 * @brief Return the raw settings objects for all configured key databases.
 *
 * @return list of KeyDatabaseItemSO settings objects
 */
auto GF_CORE_EXPORT GetKeyDatabasesBySettings()
    -> QContainer<KeyDatabaseItemSO>;

/**
 * @brief Return key database infos for custom (non-GPG) databases from settings.
 *
 * @return list of KeyDatabaseInfo for custom databases
 */
auto GF_CORE_EXPORT GetKeyDatabaseInfoBySettings()
    -> QContainer<KeyDatabaseInfo>;

/**
 * @brief Return key database infos for all GPG key databases.
 *
 * @return list of KeyDatabaseInfo for GPG databases
 */
auto GF_CORE_EXPORT GetGpgKeyDatabaseInfos() -> QContainer<KeyDatabaseInfo>;

/**
 * @brief Return the name of the key database associated with @p channel.
 *
 * @param channel OpenPGP context channel
 * @return database name string
 */
auto GF_CORE_EXPORT GetGpgKeyDatabaseName(int channel) -> QString;

/**
 * @brief Extract key ID strings from a list of abstract keys for the given channel.
 *
 * @param channel OpenPGP context channel
 * @param keys list of abstract key pointers
 * @return list of key ID strings
 */
auto GF_CORE_EXPORT ConvertKey2GpgKeyIdList(int channel,
                                            const GpgAbstractKeyPtrList& keys)
    -> KeyIdArgsList;

/**
 * @brief Convert a list of abstract keys to their underlying GpgKey objects.
 *
 * @param channel OpenPGP context channel
 * @param keys list of abstract key pointers
 * @return list of GpgKey pointers
 */
auto GF_CORE_EXPORT ConvertKey2GpgKeyList(int channel,
                                          const GpgAbstractKeyPtrList& keys)
    -> GpgKeyPtrList;

/**
 * @brief Convert a list of abstract keys to a container of GpgKey values.
 *
 * @param channel OpenPGP context channel
 * @param keys list of abstract key pointers
 * @return container of GpgKey values
 */
auto GF_CORE_EXPORT Convert2GpgKeyList(int channel,
                                       const GpgAbstractKeyPtrList& keys)
    -> QContainer<GpgKey>;

/**
 * @brief Return a string describing the usage flags of an abstract key (e.g. "ESCA").
 *
 * @param key abstract key to inspect
 * @return usage string composed of capability letters
 */
auto GF_CORE_EXPORT GetUsagesByAbstractKey(const GpgAbstractKey* key)
    -> QString;

/**
 * @brief Look up the underlying GpgKey for the given abstract key.
 *
 * @param key abstract key pointer
 * @return corresponding GpgKey value
 */
auto GF_CORE_EXPORT GetGpgKeyByGpgAbstractKey(GpgAbstractKey* key) -> GpgKey;

/**
 * @brief Return whether the given key ID refers to a key group rather than a single key.
 *
 * @param id key identifier to test
 * @return true if the ID refers to a key group
 */
auto GF_CORE_EXPORT IsKeyGroupID(const KeyId& id) -> bool;

/**
 * @brief Return whether the gpg-agent version on @p channel is greater than @p version.
 *
 * @param channel OpenPGP context channel
 * @param version version string to compare against
 * @return true if the agent version is strictly greater
 */
auto GF_CORE_EXPORT GpgAgentVersionGreaterThan(int channel,
                                               const QString& version) -> bool;

/**
 * @brief Return the path to the appropriate pinentry program for the platform.
 *
 * @return absolute path to the pinentry binary, or empty if not found
 */
auto GF_CORE_EXPORT DecidePinentry() -> QString;

/**
 * @brief Return the GnuPG version string from the active context.
 *
 * @return GnuPG version string (e.g. "2.4.3")
 */
auto GF_CORE_EXPORT GnuPGVersion() -> QString;

/**
 * @brief Parse a raw user ID string (e.g. "Name <email>") into a GFUserId structure.
 *
 * @param raw_id raw user ID string
 * @return parsed GFUserId with name, comment, and email fields
 */
auto GF_CORE_EXPORT ParseUserId(const QString& raw_id) -> GFUserId;

/**
 * @brief Convert an OpenPGPEngine enum value to its string name.
 *
 * @param type engine enum value
 * @return engine name string (e.g. "GPG")
 */
auto GF_CORE_EXPORT ConvertOpenPGPEngine2String(OpenPGPEngine type) -> QString;

/**
 * @brief Convert a GpgComponentType enum value to its string name.
 *
 * @param type component type enum value
 * @return component name string
 */
auto GF_CORE_EXPORT ConvertComponentType2String(GpgComponentType type)
    -> QString;

}  // namespace GpgFrontend
