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
#include "gpg/function/GpgFileOpera.h"
#include "GpgConstants.h"
#include "gpg/function/BasicOperator.h"

#include <boost/process/detail/config.hpp>

#include <iterator>
#include <memory>
#include <string>

GpgFrontend::GpgError GpgFrontend::GpgFileOpera::EncryptFile(
    KeyArgsList& keys,
    const std::string& path,
    GpgEncrResult& result) {
  std::string in_buffer = read_all_data_in_file(path);
  std::unique_ptr<std::string> out_buffer;

  auto err =
      BasicOperator::GetInstance().Encrypt(keys, in_buffer, out_buffer, result);

  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  write_buffer_to_file(path + ".asc", *out_buffer);
  return err;
}

GpgFrontend::GpgError GpgFrontend::GpgFileOpera::DecryptFile(
    const std::string& path,
    GpgDecrResult& result) {
  std::string in_buffer = read_all_data_in_file(path);
  std::unique_ptr<std::string> out_buffer;

  auto err =
      BasicOperator::GetInstance().Decrypt(in_buffer, out_buffer, result);

  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  std::string out_file_name = get_file_name_with_path(path),
              file_extension = get_file_extension(path);

  if (!(file_extension == ".asc" || file_extension == ".gpg"))
    out_file_name += ".out";

  write_buffer_to_file(out_file_name, *out_buffer);

  return err;
}

gpgme_error_t GpgFrontend::GpgFileOpera::SignFile(KeyArgsList& keys,
                                                  const std::string& path,
                                                  GpgSignResult& result) {
  auto in_buffer = read_all_data_in_file(path);
  std::unique_ptr<std::string> out_buffer;

  auto err = BasicOperator::GetInstance().Sign(keys, in_buffer, out_buffer,
                                               GPGME_SIG_MODE_DETACH, result);

  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  write_buffer_to_file(path + ".sig", *out_buffer);

  return err;
}

gpgme_error_t GpgFrontend::GpgFileOpera::VerifyFile(const std::string& path,
                                                    GpgVerifyResult& result) {
  auto in_buffer = read_all_data_in_file(path);
  std::unique_ptr<std::string> sign_buffer = nullptr;

  if (get_file_extension(path) == ".gpg") {
    auto err =
        BasicOperator::GetInstance().Verify(in_buffer, sign_buffer, result);
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
    return err;
  } else {
    auto sign_buffer =
        std::make_unique<std::string>(read_all_data_in_file(path + ".sig"));

    auto err =
        BasicOperator::GetInstance().Verify(in_buffer, sign_buffer, result);
    return err;
  }
}

// TODO

gpg_error_t GpgFrontend::GpgFileOpera::EncryptSignFile(
    KeyArgsList& keys,
    const std::string& path,
    GpgEncrResult& encr_res,
    GpgSignResult& sign_res) {
  auto in_buffer = read_all_data_in_file(path);
  std::unique_ptr<std::string> out_buffer = nullptr;

  // TODO Fill the vector
  std::vector<GpgKey> signerKeys;

  // TODO dealing with signer keys
  auto err = BasicOperator::GetInstance().EncryptSign(
      keys, signerKeys, in_buffer, out_buffer, encr_res, sign_res);

  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  write_buffer_to_file(path + ".gpg", *out_buffer);

  return err;
}

gpg_error_t GpgFrontend::GpgFileOpera::DecryptVerifyFile(
    const std::string& path,
    GpgDecrResult& decr_res,
    GpgVerifyResult& verify_res) {
  auto in_buffer = read_all_data_in_file(path);
  std::unique_ptr<std::string> out_buffer = nullptr;

  auto err = BasicOperator::GetInstance().DecryptVerify(in_buffer, out_buffer,
                                                        decr_res, verify_res);
  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  std::string out_file_name = get_file_name_with_path(path),
              file_extension = get_file_extension(path);

  if (!(file_extension == ".asc" || file_extension == ".gpg"))
    out_file_name = path + ".out";

  write_buffer_to_file(out_file_name, *out_buffer);

  return err;
}
