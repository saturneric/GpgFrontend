/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
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

#ifndef GPGFRONTEND_GPGFILEOPERA_H
#define GPGFRONTEND_GPGFILEOPERA_H

#include "gpg/GpgConstants.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgModel.h"

namespace GpgFrontend {

class GpgFileOpera : public SingletonFunctionObject<GpgFileOpera> {
 public:
  explicit GpgFileOpera(
      int channel = SingletonFunctionObject::GetDefaultChannel())
      : SingletonFunctionObject<GpgFileOpera>(channel) {}

  static GpgError EncryptFile(KeyListPtr keys, const std::string& path,
                              GpgEncrResult& result);

  static GpgError DecryptFile(const std::string& path, GpgDecrResult& result);

  static GpgError SignFile(KeyListPtr keys, const std::string& path,
                           GpgSignResult& result);

  static GpgError VerifyFile(const std::string& path, GpgVerifyResult& result);

  static GpgError EncryptSignFile(KeyListPtr keys, KeyListPtr signer_keys,
                                  const std::string& path,
                                  GpgEncrResult& encr_res,
                                  GpgSignResult& sign_res);

  static GpgError DecryptVerifyFile(const std::string& path,
                                    GpgDecrResult& decr_res,
                                    GpgVerifyResult& verify_res);
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGFILEOPERA_H
