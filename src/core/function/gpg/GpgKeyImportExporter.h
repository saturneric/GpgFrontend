/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#ifndef _GPGKEYIMPORTEXPORTOR_H
#define _GPGKEYIMPORTEXPORTOR_H

#include <string>

#include "core/GpgConstants.h"
#include "core/GpgContext.h"
#include "core/GpgFunctionObject.h"
#include "core/GpgModel.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GpgImportedKey {
 public:
  std::string fpr;    ///<
  int import_status;  ///<
};

typedef std::list<GpgImportedKey> GpgImportedKeyList;  ///<

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgImportInformation {
 public:
  GpgImportInformation();

  /**
   * @brief Construct a new Gpg Import Information object
   *
   * @param result
   */
  explicit GpgImportInformation(gpgme_import_result_t result);

  int considered = 0;        ///<
  int no_user_id = 0;        ///<
  int imported = 0;          ///<
  int imported_rsa = 0;      ///<
  int unchanged = 0;         ///<
  int new_user_ids = 0;      ///<
  int new_sub_keys = 0;      ///<
  int new_signatures = 0;    ///<
  int new_revocations = 0;   ///<
  int secret_read = 0;       ///<
  int secret_imported = 0;   ///<
  int secret_unchanged = 0;  ///<
  int not_imported = 0;      ///<

  GpgImportedKeyList importedKeys;  ///<
};

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKeyImportExporter
    : public SingletonFunctionObject<GpgKeyImportExporter> {
 public:
  /**
   * @brief Construct a new Gpg Key Import Exporter object
   *
   * @param channel
   */
  explicit GpgKeyImportExporter(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   * @param inBuffer
   * @return GpgImportInformation
   */
  GpgImportInformation ImportKey(StdBypeArrayPtr inBuffer);

  /**
   * @brief
   *
   * @param uid_list
   * @param out_buffer
   * @param secret
   * @return true
   * @return false
   */
  bool ExportKeys(KeyIdArgsListPtr& uid_list, ByteArrayPtr& out_buffer,
                  bool secret = false) const;

  /**
   * @brief
   *
   * @param keys
   * @param outBuffer
   * @param secret
   * @return true
   * @return false
   */
  bool ExportKeys(const KeyArgsList& keys, ByteArrayPtr& outBuffer,
                  bool secret = false) const;

  /**
   * @brief
   *
   * @param keys
   * @param outBuffer
   * @param secret
   * @return true
   * @return false
   */
  bool ExportAllKeys(KeyIdArgsListPtr& uid_list, ByteArrayPtr& out_buffer,
                     bool secret) const;

  /**
   * @brief
   *
   * @param key
   * @param out_buffer
   * @return true
   * @return false
   */
  bool ExportKey(const GpgKey& key, ByteArrayPtr& out_buffer) const;

  /**
   * @brief
   *
   * @param key
   * @param out_buffer
   * @return true
   * @return false
   */
  bool ExportKeyOpenSSH(const GpgKey& key, ByteArrayPtr& out_buffer) const;

  /**
   * @brief
   *
   * @param key
   * @param outBuffer
   * @return true
   * @return false
   */
  bool ExportSecretKey(const GpgKey& key, ByteArrayPtr& outBuffer) const;

  /**
   * @brief
   *
   * @param key
   * @param outBuffer
   * @return true
   * @return false
   */
  bool ExportSecretKeyShortest(const GpgKey& key,
                               ByteArrayPtr& outBuffer) const;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};

}  // namespace GpgFrontend

#endif  // _GPGKEYIMPORTEXPORTOR_H