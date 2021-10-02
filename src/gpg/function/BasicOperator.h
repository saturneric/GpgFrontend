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

#ifndef GPGFRONTEND_ZH_CN_TS_BASICOPERATOR_H
#define GPGFRONTEND_ZH_CN_TS_BASICOPERATOR_H

#include "gpg/GpgConstants.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgFunctionObject.h"
#include "gpg/GpgModel.h"

namespace GpgFrontend {

class BasicOperator : public SingletonFunctionObject<BasicOperator> {
 public:
  gpg_error_t Encrypt(KeyArgsList&& keys,
                      BypeArrayRef in_buffer,
                      ByteArrayPtr& out_buffer,
                      GpgEncrResult& result);

  gpgme_error_t EncryptSign(KeyArgsList&& keys,
                            KeyArgsList&& signers,
                            BypeArrayRef in_buffer,
                            ByteArrayPtr& out_buffer,
                            GpgEncrResult& encr_result,
                            GpgSignResult& sign_result);

  gpgme_error_t Decrypt(BypeArrayRef in_buffer,
                        ByteArrayPtr& out_buffer,
                        GpgDecrResult& result);

  gpgme_error_t DecryptVerify(BypeArrayRef in_buffer,
                              ByteArrayPtr& out_buffer,
                              GpgDecrResult& decrypt_result,
                              GpgVerifyResult& verify_result);

  gpgme_error_t Verify(BypeArrayRef in_buffer,
                       ByteArrayPtr& sig_buffer,
                       GpgVerifyResult& result) const;

  gpg_error_t Sign(KeyArgsList&& key_fprs,
                   BypeArrayRef in_buffer,
                   ByteArrayPtr& out_buffer,
                   gpgme_sig_mode_t mode,
                   GpgSignResult& result);

  void SetSigners(KeyArgsList& keys);

  std::unique_ptr<KeyArgsList> GetSigners();

 private:
  GpgContext& ctx =
      GpgContext::GetInstance(SingletonFunctionObject::GetDefaultChannel());
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_BASICOPERATOR_H
