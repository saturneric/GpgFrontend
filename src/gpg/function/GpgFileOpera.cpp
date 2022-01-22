/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "gpg/function/GpgFileOpera.h"

#include <memory>
#include <string>

#include "GpgConstants.h"
#include "gpg/function/BasicOperator.h"

GpgFrontend::GpgError GpgFrontend::GpgFileOpera::EncryptFile(
    KeyListPtr keys, const std::string& in_path, const std::string& out_path,
    GpgEncrResult& result, int _channel) {
  std::string in_buffer = read_all_data_in_file(in_path);
  std::unique_ptr<std::string> out_buffer;

  auto err = BasicOperator::GetInstance(_channel).Encrypt(
      std::move(keys), in_buffer, out_buffer, result);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!write_buffer_to_file(out_path, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}

GpgFrontend::GpgError GpgFrontend::GpgFileOpera::DecryptFile(
    const std::string& in_path, const std::string& out_path,
    GpgDecrResult& result) {
  std::string in_buffer = read_all_data_in_file(in_path);
  std::unique_ptr<std::string> out_buffer;

  auto err =
      BasicOperator::GetInstance().Decrypt(in_buffer, out_buffer, result);

  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!write_buffer_to_file(out_path, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}

gpgme_error_t GpgFrontend::GpgFileOpera::SignFile(KeyListPtr keys,
                                                  const std::string& in_path,
                                                  const std::string& out_path,
                                                  GpgSignResult& result,
                                                  int _channel) {
  auto in_buffer = read_all_data_in_file(in_path);
  std::unique_ptr<std::string> out_buffer;

  auto err = BasicOperator::GetInstance(_channel).Sign(
      std::move(keys), in_buffer, out_buffer, GPGME_SIG_MODE_DETACH, result);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!write_buffer_to_file(out_path, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}

gpgme_error_t GpgFrontend::GpgFileOpera::VerifyFile(
    const std::string& data_path, const std::string& sign_path,
    GpgVerifyResult& result, int _channel) {
  auto in_buffer = read_all_data_in_file(data_path);
  std::unique_ptr<std::string> sign_buffer = nullptr;
  if (!sign_path.empty()) {
    sign_buffer =
        std::make_unique<std::string>(read_all_data_in_file(sign_path));
  }
  auto err = BasicOperator::GetInstance(_channel).Verify(in_buffer, sign_buffer,
                                                         result);
  return err;
}

gpg_error_t GpgFrontend::GpgFileOpera::EncryptSignFile(
    KeyListPtr keys, KeyListPtr signer_keys, const std::string& in_path,
    const std::string& out_path, GpgEncrResult& encr_res,
    GpgSignResult& sign_res, int _channel) {
  auto in_buffer = read_all_data_in_file(in_path);
  std::unique_ptr<std::string> out_buffer = nullptr;

  auto err = BasicOperator::GetInstance(_channel).EncryptSign(
      std::move(keys), std::move(signer_keys), in_buffer, out_buffer, encr_res,
      sign_res);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!write_buffer_to_file(out_path, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}

gpg_error_t GpgFrontend::GpgFileOpera::DecryptVerifyFile(
    const std::string& in_path, const std::string& out_path,
    GpgDecrResult& decr_res, GpgVerifyResult& verify_res) {
  auto in_buffer = read_all_data_in_file(in_path);

  std::unique_ptr<std::string> out_buffer = nullptr;

  auto err = BasicOperator::GetInstance().DecryptVerify(in_buffer, out_buffer,
                                                        decr_res, verify_res);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!write_buffer_to_file(out_path, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}
unsigned int GpgFrontend::GpgFileOpera::EncryptFileSymmetric(
    const std::string& in_path, const std::string& out_path,
    GpgFrontend::GpgEncrResult& result, int _channel) {
  std::string in_buffer = read_all_data_in_file(in_path);
  std::unique_ptr<std::string> out_buffer;

  auto err = BasicOperator::GetInstance(_channel).EncryptSymmetric(
      in_buffer, out_buffer, result);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!write_buffer_to_file(out_path, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}
