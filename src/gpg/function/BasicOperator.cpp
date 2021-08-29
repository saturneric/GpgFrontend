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

#include "gpg/function/BasicOperator.h"
#include "gpg/function/GpgKeyGetter.h"

/**
 * Read gpgme-Data to QByteArray
 *   mainly from http://basket.kde.org/ (kgpgme.cpp)
 */
#define BUF_SIZE (32 * 1024)

gpgme_error_t read_2_buffer(gpgme_data_t data_in, GpgFrontend::BypeArrayPtr &out_buffer) {
    gpgme_off_t ret = gpgme_data_seek(data_in, 0, SEEK_SET);
    gpgme_error_t err = gpg_error(GPG_ERR_NO_ERROR);

    if (ret) {
        err = gpgme_err_code_from_errno(errno);
        GpgFrontend::check_gpg_error(err, "failed data_seek data_in read_2_buffer");
    } else {
        char buf[BUF_SIZE + 2];

        while ((ret = gpgme_data_read(data_in, buf, BUF_SIZE)) > 0) {
            const size_t size = out_buffer->size();
            out_buffer->resize(static_cast<int>(size + ret));
            memcpy(out_buffer->data() + size, buf, ret);
        }
        if (ret < 0) {
            err = gpgme_err_code_from_errno(errno);
            GpgFrontend::check_gpg_error(err, "failed data_read data_in read_2_buffer");
        }
    }
    return err;
}

gpg_error_t GpgFrontend::BasicOperator::encrypt(std::vector<GpgKey> &keys, GpgFrontend::BypeArrayRef in_buffer,
                                                GpgFrontend::BypeArrayPtr &out_buffer,
                                                GpgFrontend::GpgEncrResult &result) {

    gpgme_data_t data_in = nullptr, data_out = nullptr;
    out_buffer->resize(0);

    // gpgme_encrypt_result_t e_result;
    gpgme_key_t recipients[keys.size() + 1];

    int index = 0;
    for (const auto &key : keys) recipients[index++] = gpgme_key_t(key);

    // Last entry data_in array has to be nullptr
    recipients[keys.size()] = nullptr;

    gpgme_error_t err;

    // If the last parameter isnt 0, a private copy of data is made
    check_gpg_error(gpgme_data_new_from_mem(&data_in, in_buffer.data(), in_buffer.size(), 1));
    check_gpg_error(gpgme_data_new(&data_out));
    err = check_gpg_error(gpgme_op_encrypt(ctx, recipients, GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));
    check_gpg_error(read_2_buffer(data_out, out_buffer));

    if (data_in) gpgme_data_release(data_in);
    if (data_out) gpgme_data_release(data_out);

    result = GpgEncrResult(gpgme_op_encrypt_result(ctx), [&](gpgme_encrypt_result_t res) { gpgme_result_unref(res); });
    return err;
}

gpgme_error_t GpgFrontend::BasicOperator::decrypt(BypeArrayRef in_buffer, GpgFrontend::BypeArrayPtr &out_buffer,
                                                  GpgFrontend::GpgDecrResult &result) {
    gpgme_data_t data_in = nullptr, data_out = nullptr;
    out_buffer->resize(0);

    gpgme_error_t err;

    check_gpg_error(gpgme_data_new_from_mem(&data_in, in_buffer.data(), in_buffer.size(), 1));
    check_gpg_error(gpgme_data_new(&data_out));
    err = check_gpg_error(gpgme_op_decrypt(ctx, data_in, data_out));
    check_gpg_error(read_2_buffer(data_out, out_buffer));

    if (data_in) gpgme_data_release(data_in);
    if (data_out) gpgme_data_release(data_out);

    result = GpgDecrResult(gpgme_op_decrypt_result(ctx), [&](gpgme_decrypt_result_t res) { gpgme_result_unref(res); });

    return err;
}

gpgme_error_t GpgFrontend::BasicOperator::verify(QByteArray &in_buffer, QByteArray &sig_buffer,
                                                 GpgFrontend::GpgVerifyResult &result) const {
    gpgme_data_t data_in;
    gpgme_error_t err;
    gpgme_verify_result_t m_result;

    check_gpg_error(gpgme_data_new_from_mem(&data_in, in_buffer.data(), in_buffer.size(), 1));

    if (sig_buffer != nullptr) {
        gpgme_data_t sig_data;
        check_gpg_error(gpgme_data_new_from_mem(&sig_data, sig_buffer.data(), sig_buffer.size(), 1));
        err = check_gpg_error(gpgme_op_verify(ctx, sig_data, data_in, nullptr));
    } else
        err = check_gpg_error(gpgme_op_verify(ctx, data_in, nullptr, data_in));

    result = GpgVerifyResult(gpgme_op_verify_result(ctx), [&](gpgme_verify_result_t res) { gpgme_result_unref(res); });

    return err;
}

gpg_error_t
GpgFrontend::BasicOperator::sign(KeyFprArgsList key_fprs, BypeArrayRef in_buffer, BypeArrayPtr &out_buffer,
                                 gpgme_sig_mode_t mode, GpgSignResult &result) {
    gpgme_error_t err;
    gpgme_data_t data_in, data_out;

    out_buffer->resize(0);

    std::vector<GpgKey> keys;
    auto &key_getter = GpgKeyGetter::getInstance();

    for (const auto &key_fpr : key_fprs)
        keys.push_back(key_getter.getKey(key_fpr));

    // Set Singers of this opera
    setSigners(keys);

    check_gpg_error(gpgme_data_new_from_mem(&data_in, in_buffer.data(), in_buffer.size(), 1));
    check_gpg_error(gpgme_data_new(&data_out));

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

    err = check_gpg_error(gpgme_op_sign(ctx, data_in, data_out, mode));
    check_gpg_error(read_2_buffer(data_out, out_buffer));

    gpgme_data_release(data_in);
    gpgme_data_release(data_out);

    result = GpgSignResult(gpgme_op_sign_result(ctx), [&](gpgme_sign_result_t res) { gpgme_result_unref(res); });

    return err;
}

gpgme_error_t
GpgFrontend::BasicOperator::decryptVerify(const BypeArrayRef in_buffer, BypeArrayPtr &out_buffer,
                                          GpgDecrResult &decrypt_result, GpgVerifyResult &verify_result) {
    gpgme_error_t err;
    gpgme_data_t data_in, data_out;
    out_buffer->resize(0);

    check_gpg_error(gpgme_data_new_from_mem(&data_in, in_buffer.data(), in_buffer.size(), 1));
    check_gpg_error(gpgme_data_new(&data_out));
    err = check_gpg_error(gpgme_op_decrypt_verify(ctx, data_in, data_out));

    check_gpg_error(read_2_buffer(data_out, out_buffer));

    if (data_in) gpgme_data_release(data_in);
    if (data_out) gpgme_data_release(data_out);

    decrypt_result = GpgDecrResult(gpgme_op_decrypt_result(ctx),
                                   [&](gpgme_decrypt_result_t res) { gpgme_result_unref(res); });
    verify_result = GpgVerifyResult(gpgme_op_verify_result(ctx),
                                    [&](gpgme_verify_result_t res) { gpgme_result_unref(res); });


    return err;
}

gpgme_error_t GpgFrontend::BasicOperator::encryptSign(std::vector<GpgKey> &keys, std::vector<GpgKey> &signers,
                                                      BypeArrayRef in_buffer,
                                                      BypeArrayPtr &out_buffer, GpgEncrResult &encr_result,
                                                      GpgSignResult &sign_result) {
    gpgme_error_t err;
    gpgme_data_t data_in, data_out;
    out_buffer->resize(0);

    setSigners(signers);

    //gpgme_encrypt_result_t e_result;
    gpgme_key_t recipients[keys.size() + 1];

    // set key for user
    int index = 0;
    for (const auto &key : keys) recipients[index++] = gpgme_key_t(key);

    // Last entry dataIn array has to be nullptr
    recipients[keys.size()] = nullptr;

    // If the last parameter isnt 0, a private copy of data is made
    check_gpg_error(gpgme_data_new_from_mem(&data_in, in_buffer.data(), in_buffer.size(), 1));
    check_gpg_error(gpgme_data_new(&data_out));
    err = check_gpg_error(gpgme_op_encrypt_sign(ctx, recipients, GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));

    check_gpg_error(read_2_buffer(data_out, out_buffer));

    if (data_in) gpgme_data_release(data_in);
    if (data_out) gpgme_data_release(data_out);

    encr_result = GpgEncrResult(gpgme_op_encrypt_result(ctx),
                                [&](gpgme_encrypt_result_t res) { gpgme_result_unref(res); });
    sign_result = GpgSignResult(gpgme_op_sign_result(ctx), [&](gpgme_sign_result_t res) { gpgme_result_unref(res); });

    return err;
}

void GpgFrontend::BasicOperator::setSigners(KeyArgsList keys) {
    gpgme_signers_clear(ctx);
    for (const GpgKey &key : keys) {
        if (key.canSignActual()) {
            auto gpgmeError = gpgme_signers_add(ctx, gpgme_key_t(key));
            check_gpg_error(gpgmeError);
        }
    }
    if (keys.size() != gpgme_signers_count(ctx))
        qDebug() << "No All Signers Added";
}

std::unique_ptr<std::vector<GpgFrontend::GpgKey>> GpgFrontend::BasicOperator::getSigners() {
    auto count = gpgme_signers_count(ctx);
    auto signers = std::make_unique<std::vector<GpgKey>>();
    for (auto i = 0; i < count; i++) {
        auto key = GpgKey(gpgme_signers_enum(ctx, i));
        signers->push_back(std::move(GpgKey(std::move(key))));
    }
    return signers;
}
