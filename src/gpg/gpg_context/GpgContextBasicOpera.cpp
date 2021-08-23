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

#include "gpg/GpgContext.h"

/**
 * Encrypt data
 * @param keys keys used
 * @param inBuffer input byte array
 * @param outBuffer output byte array
 * @param result opera result
 * @return error information
 */
gpg_error_t GpgME::GpgContext::encrypt(QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer,
                                       gpgme_encrypt_result_t *result) {

    gpgme_data_t dataIn = nullptr, dataOut = nullptr;
    outBuffer->resize(0);

    // gpgme_encrypt_result_t e_result;
    gpgme_key_t recipients[keys.count() + 1];

    int index = 0;
    for (const auto &key : keys) recipients[index++] = key.key_refer;

    // Last entry dataIn array has to be nullptr
    recipients[keys.count()] = nullptr;

    // If the last parameter isnt 0, a private copy of data is made
    if (mCtx) {
        err = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
        checkErr(err);
        if (!err) {
            err = gpgme_data_new(&dataOut);
            checkErr(err);
            if (!err) {
                err = gpgme_op_encrypt(mCtx, recipients, GPGME_ENCRYPT_ALWAYS_TRUST, dataIn, dataOut);
                checkErr(err);
                if (!err) {
                    err = readToBuffer(dataOut, outBuffer);
                    checkErr(err);
                }
            }
        }
    }
    if (dataIn) gpgme_data_release(dataIn);
    if (dataOut) gpgme_data_release(dataOut);

    if (result != nullptr) *result = gpgme_op_encrypt_result(mCtx);
    return err;
}

/**
 * Decrypt data
 * @param keys keys used
 * @param inBuffer input byte array
 * @param outBuffer output byte array
 * @param result opera result
 * @return error information
 */
gpgme_error_t GpgME::GpgContext::decrypt(const QByteArray &inBuffer, QByteArray *outBuffer,
                                         gpgme_decrypt_result_t *result) {
    gpgme_data_t dataIn = nullptr, dataOut = nullptr;
    gpgme_decrypt_result_t m_result = nullptr;

    outBuffer->resize(0);
    if (mCtx != nullptr) {
        err = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
        if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
            err = gpgme_data_new(&dataOut);
            if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                err = gpgme_op_decrypt(mCtx, dataIn, dataOut);
                m_result = gpgme_op_decrypt_result(mCtx);
                if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) err = readToBuffer(dataOut, outBuffer);
            }
        }
    }

    if (!settings.value("general/rememberPassword").toBool()) clearPasswordCache();

    if (dataIn) gpgme_data_release(dataIn);
    if (dataOut) gpgme_data_release(dataOut);

    if (result != nullptr) *result = m_result;

    return err;
}

/**
 * Verify data
 * @param keys keys used
 * @param inBuffer input byte array
 * @param sigBuffer signature byte array (detected by format)
 * @param result opera result
 * @return error information
 */
gpgme_error_t GpgME::GpgContext::verify(QByteArray *inBuffer, QByteArray *sigBuffer, gpgme_verify_result_t *result) {

    gpgme_data_t dataIn;
    gpgme_error_t gpgmeError;
    gpgme_verify_result_t m_result;

    gpgmeError = gpgme_data_new_from_mem(&dataIn, inBuffer->data(), inBuffer->size(), 1);
    checkErr(gpgmeError);

    if (sigBuffer != nullptr) {
        gpgme_data_t sigdata;
        gpgmeError = gpgme_data_new_from_mem(&sigdata, sigBuffer->data(), sigBuffer->size(), 1);
        checkErr(gpgmeError);
        gpgmeError = gpgme_op_verify(mCtx, sigdata, dataIn, nullptr);
    } else {
        gpgmeError = gpgme_op_verify(mCtx, dataIn, nullptr, dataIn);
    }

    checkErr(gpgmeError);

    m_result = gpgme_op_verify_result(mCtx);

    if (result != nullptr) {
        *result = m_result;
    }

    return gpgmeError;
}

/**
 * Sign data
 * @param keys keys used
 * @param inBuffer input byte array
 * @param outBuffer output byte array
 * @param mode sign mode
 * @param result opera result
 * @return
 */
gpg_error_t GpgME::GpgContext::sign(const QVector<GpgKey> &keys, const QByteArray &inBuffer, QByteArray *outBuffer,
                                    gpgme_sig_mode_t mode, gpgme_sign_result_t *result, bool default_ctx) {

    gpgme_error_t gpgmeError;
    gpgme_data_t dataIn, dataOut;
    gpgme_sign_result_t m_result;

    auto _ctx = mCtx;

    if(!default_ctx)
        _ctx = create_ctx();

    if (keys.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Key Selection"), tr("No Private Key Selected"));
        return false;
    }

    // Set Singers of this opera
    setSigners(keys, _ctx);

    gpgmeError = gpgme_data_new_from_mem(&dataIn, inBuffer.data(), inBuffer.size(), 1);
    checkErr(gpgmeError);
    gpgmeError = gpgme_data_new(&dataOut);
    checkErr(gpgmeError);

    /**
        `GPGME_SIG_MODE_NORMAL'
              A normal signature is made, the output includes the plaintext
              and the signature.

        `GPGME_SIG_MODE_DETACH'
              A detached signature is made.

        `GPGME_SIG_MODE_CLEAR'
              A clear text signature is made.  The ASCII armor and text
              mode settings of the context are ignored.
    */

    gpgmeError = gpgme_op_sign(_ctx, dataIn, dataOut, mode);
    checkErr(gpgmeError);

    if (gpgmeError == GPG_ERR_CANCELED) return false;

    if (gpgmeError != GPG_ERR_NO_ERROR) {
        QMessageBox::critical(nullptr, tr("Error in signing:"), QString::fromUtf8(gpgme_strerror(gpgmeError)));
        return false;
    }

    if(default_ctx)
        m_result = gpgme_op_sign_result(_ctx);
    else m_result = nullptr;

    if (result != nullptr) *result = m_result;

    if(!default_ctx) gpgme_release(_ctx);

    gpgmeError = readToBuffer(dataOut, outBuffer);
    checkErr(gpgmeError);

    gpgme_data_release(dataIn);
    gpgme_data_release(dataOut);

    // Of no use yet
    if (!settings.value("general/rememberPassword").toBool()) clearPasswordCache();

    return gpgmeError;
}

/**
 * Encrypt and sign data
 * @param keys keys used
 * @param inBuffer input byte array
 * @param outBuffer output byte array
 * @param encr_result encrypt opera result
 * @param sign_result sign opera result
 * @return
 */
gpgme_error_t
GpgME::GpgContext::encryptSign(QVector<GpgKey> &keys, QVector<GpgKey> &signers, const QByteArray &inBuffer,
                               QByteArray *outBuffer, gpgme_encrypt_result_t *encr_result,
                               gpgme_sign_result_t *sign_result) {
    gpgme_data_t data_in = nullptr, data_out = nullptr;
    outBuffer->resize(0);

    setSigners(signers, mCtx);

    //gpgme_encrypt_result_t e_result;
    gpgme_key_t recipients[keys.count() + 1];

    // set key for user
    int index = 0;
    for (const auto &key : keys) recipients[index++] = key.key_refer;

    // Last entry dataIn array has to be nullptr
    recipients[keys.count()] = nullptr;

    // If the last parameter isnt 0, a private copy of data is made
    if (mCtx != nullptr) {
        err = gpgme_data_new_from_mem(&data_in, inBuffer.data(), inBuffer.size(), 1);
        if (gpg_err_code(err) == GPG_ERR_NO_ERROR) {
            err = gpgme_data_new(&data_out);
            if (gpg_err_code(err) == GPG_ERR_NO_ERROR) {
                err = gpgme_op_encrypt_sign(mCtx, recipients, GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out);
                if (encr_result != nullptr)
                    *encr_result = gpgme_op_encrypt_result(mCtx);
                if (sign_result != nullptr)
                    *sign_result = gpgme_op_sign_result(mCtx);
                if (gpg_err_code(err) == GPG_ERR_NO_ERROR) {
                    err = readToBuffer(data_out, outBuffer);
                }
            }
        }
    }

    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) checkErr(err);

    if (data_in) gpgme_data_release(data_in);
    if (data_out) gpgme_data_release(data_out);

    return err;
}

/**
 * Decrypt and verify data
 * @param inBuffer input byte array
 * @param outBuffer output byte array
 * @param decrypt_result decrypt opera result
 * @param verify_result verify opera result
 * @return error info
 */
gpgme_error_t GpgME::GpgContext::decryptVerify(const QByteArray &inBuffer, QByteArray *outBuffer,
                                               gpgme_decrypt_result_t *decrypt_result,
                                               gpgme_verify_result_t *verify_result) {
    gpgme_data_t data_in = nullptr, data_out = nullptr;

    outBuffer->resize(0);
    if (mCtx != nullptr) {
        err = gpgme_data_new_from_mem(&data_in, inBuffer.data(), inBuffer.size(), 1);
        if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
            err = gpgme_data_new(&data_out);
            if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                err = gpgme_op_decrypt_verify(mCtx, data_in, data_out);
                if (decrypt_result != nullptr)
                    *decrypt_result = gpgme_op_decrypt_result(mCtx);
                if (verify_result != nullptr)
                    *verify_result = gpgme_op_verify_result(mCtx);
                if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
                    err = readToBuffer(data_out, outBuffer);
                }
            }
        }
    }

    if (!settings.value("general/rememberPassword").toBool()) clearPasswordCache();

    if (data_in) gpgme_data_release(data_in);
    if (data_out) gpgme_data_release(data_out);

    return err;
}
