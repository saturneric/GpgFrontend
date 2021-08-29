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

#include "GpgFrontend.h"
#include "gpg/GpgModel.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgFunctionObject.h"

namespace GpgFrontend {

    using BypeArrayPtr = std::unique_ptr<QByteArray>;
    using BypeArrayRef = QByteArray &;

    using GpgEncrResult = std::unique_ptr<struct _gpgme_op_encrypt_result, std::function<void(
            gpgme_encrypt_result_t)>>;
    using GpgDecrResult = std::unique_ptr<struct _gpgme_op_decrypt_result, std::function<void(
            gpgme_decrypt_result_t)>>;
    using GpgSignResult = std::unique_ptr<struct _gpgme_op_sign_result, std::function<void(
            gpgme_sign_result_t)>>;
    using GpgVerifyResult = std::unique_ptr<struct _gpgme_op_verify_result, std::function<void(
            gpgme_verify_result_t)>>;

    class BasicOperator : public SingletonFunctionObject<BasicOperator> {
    public:

        gpg_error_t encrypt(std::vector<GpgKey> &keys,  BypeArrayRef in_buffer, BypeArrayPtr &out_buffer,
                            GpgEncrResult &result);

        gpgme_error_t encryptSign(std::vector<GpgKey> &keys, std::vector<GpgKey> &signers, BypeArrayRef in_buffer,
                                  BypeArrayPtr &out_buffer, GpgEncrResult &encr_result,
                                  GpgSignResult &sign_result);

        gpgme_error_t decrypt(BypeArrayRef in_buffer, BypeArrayPtr &out_buffer, GpgDecrResult &result);

        gpgme_error_t
        decryptVerify(BypeArrayRef in_buffer, BypeArrayPtr &out_buffer, GpgDecrResult &decrypt_result,
                      GpgVerifyResult &verify_result);

        gpgme_error_t verify(BypeArrayRef in_buffer, BypeArrayRef sig_buffer, GpgVerifyResult &result) const;

        gpg_error_t
        sign(KeyFprArgsList key_fprs, BypeArrayRef in_buffer, BypeArrayPtr &out_buffer, gpgme_sig_mode_t mode,
             GpgSignResult &result);

        void setSigners(KeyArgsList keys);

        std::unique_ptr<std::vector<GpgKey>> getSigners();

    private:

        GpgContext &ctx = GpgContext::getInstance();

    };
}


#endif //GPGFRONTEND_ZH_CN_TS_BASICOPERATOR_H
