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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgImportInformation.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Singleton for importing and exporting OpenPGP key material.
 *
 * Provides synchronous import and both synchronous and asynchronous export
 * operations. All operations use the engine-dispatched OpTraits machinery.
 */
class GF_CORE_EXPORT KeyImportExportOperation
    : public SingletonFunctionObject<KeyImportExportOperation> {
 public:
  /**
   * @brief Construct the operation handler for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit KeyImportExportOperation(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Import key material from an armored or binary buffer.
   *
   * @param buffer key data buffer to import
   * @return import information describing what was imported; nullptr on failure
   */
  auto ImportKey(const GFBuffer& buffer)
      -> QSharedPointer<GpgImportInformation>;

  /**
   * @brief Import a revocation certificate from a buffer.
   *
   * @param buffer revocation certificate data to import
   * @return import information; nullptr on failure
   */
  auto ImportRevCert(const GFBuffer& buffer)
      -> QSharedPointer<GpgImportInformation>;

  /**
   * @brief Export a single key to a buffer.
   *
   * @param key key to export
   * @param secret if true, include secret key material
   * @param ascii if true, produce ASCII-armored output
   * @param shortest if true, export the minimal representation
   * @param ssh_mode if true, export in OpenSSH format (GnuPG only)
   * @return tuple of (GpgError, GFBuffer containing the exported key data)
   */
  [[nodiscard]] auto ExportKey(const GpgAbstractKeyPtr& key, bool secret,
                               bool ascii, bool shortest,
                               bool ssh_mode = false) const
      -> std::tuple<GpgError, GFBuffer>;

  /**
   * @brief Export a subkey by its fingerprint.
   *
   * @param fpr subkey fingerprint
   * @param ascii if true, produce ASCII-armored output
   * @return tuple of (GpgError, GFBuffer containing the subkey data)
   */
  [[nodiscard]] auto ExportSubkey(const QString& fpr, bool ascii) const
      -> std::tuple<GpgError, GFBuffer>;

  /**
   * @brief Export a list of keys to a buffer (async).
   *
   * @param keys list of keys to export
   * @param secret if true, include secret key material
   * @param ascii if true, produce ASCII-armored output
   * @param shortest if true, export minimal representations
   * @param ssh_mode if true, export in OpenSSH format
   * @param cb callback invoked on completion with (GpgError, DataObjectPtr)
   */
  void ExportKeys(const GpgAbstractKeyPtrList& keys, bool secret, bool ascii,
                  bool shortest, bool ssh_mode,
                  const GpgOperationCallback& cb) const;

  /**
   * @brief Export all key material (public and optionally secret) for a list of
   * keys (async).
   *
   * Unlike ExportKeys(), this always exports the full key including all subkeys
   * and user IDs.
   *
   * @param keys list of keys to export
   * @param secret if true, include secret key material
   * @param ascii if true, produce ASCII-armored output
   * @param cb callback invoked on completion
   */
  void ExportAllKeys(const GpgAbstractKeyPtrList& keys, bool secret, bool ascii,
                     const GpgOperationCallback& cb) const;

 private:
  // OpenPGP context for this channel.
  OpenPGPContext& ctx_;
};

}  // namespace GpgFrontend
