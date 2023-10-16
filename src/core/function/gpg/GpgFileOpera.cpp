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
#include "GpgFileOpera.h"

#include <memory>
#include <string>

#include "GpgBasicOperator.h"
#include "GpgConstants.h"
#include "function/FileOperator.h"

GpgFrontend::GpgFileOpera::GpgFileOpera(int channel)
    : SingletonFunctionObject<GpgFileOpera>(channel) {}

GpgFrontend::GpgError GpgFrontend::GpgFileOpera::EncryptFile(
    KeyListPtr keys, const std::string& in_path, const std::string& out_path,
    GpgEncrResult& result, int _channel) {
#ifdef WINDOWS
  auto in_path_std =
      std::filesystem::path(QString::fromStdString(in_path).toStdU16String());
  auto out_path_std =
      std::filesystem::path(QString::fromStdString(out_path).toStdU16String());
#else
  auto in_path_std = std::filesystem::path(in_path);
  auto out_path_std = std::filesystem::path(out_path);
#endif

  std::string in_buffer;
  if (!FileOperator::ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }

  ByteArrayPtr out_buffer = nullptr;

  auto err = GpgBasicOperator::GetInstance(_channel).Encrypt(
      std::move(keys), in_buffer, out_buffer, result);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!FileOperator::WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}

GpgFrontend::GpgError GpgFrontend::GpgFileOpera::DecryptFile(
    const std::string& in_path, const std::string& out_path,
    GpgDecrResult& result) {
#ifdef WINDOWS
  auto in_path_std =
      std::filesystem::path(QString::fromStdString(in_path).toStdU16String());
  auto out_path_std =
      std::filesystem::path(QString::fromStdString(out_path).toStdU16String());
#else
  auto in_path_std = std::filesystem::path(in_path);
  auto out_path_std = std::filesystem::path(out_path);
#endif

  std::string in_buffer;
  if (!FileOperator::ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }
  ByteArrayPtr out_buffer;

  auto err =
      GpgBasicOperator::GetInstance().Decrypt(in_buffer, out_buffer, result);

  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!FileOperator::WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}

gpgme_error_t GpgFrontend::GpgFileOpera::SignFile(KeyListPtr keys,
                                                  const std::string& in_path,
                                                  const std::string& out_path,
                                                  GpgSignResult& result,
                                                  int _channel) {
#ifdef WINDOWS
  auto in_path_std =
      std::filesystem::path(QString::fromStdString(in_path).toStdU16String());
  auto out_path_std =
      std::filesystem::path(QString::fromStdString(out_path).toStdU16String());
#else
  auto in_path_std = std::filesystem::path(in_path);
  auto out_path_std = std::filesystem::path(out_path);
#endif

  std::string in_buffer;
  if (!FileOperator::ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }
  ByteArrayPtr out_buffer;

  auto err = GpgBasicOperator::GetInstance(_channel).Sign(
      std::move(keys), in_buffer, out_buffer, GPGME_SIG_MODE_DETACH, result);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!FileOperator::WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}

gpgme_error_t GpgFrontend::GpgFileOpera::VerifyFile(
    const std::string& data_path, const std::string& sign_path,
    GpgVerifyResult& result, int _channel) {
#ifdef WINDOWS
  auto data_path_std =
      std::filesystem::path(QString::fromStdString(data_path).toStdU16String());
  auto sign_path_std =
      std::filesystem::path(QString::fromStdString(sign_path).toStdU16String());
#else
  auto data_path_std = std::filesystem::path(data_path);
  auto sign_path_std = std::filesystem::path(sign_path);
#endif

  std::string in_buffer;
  if (!FileOperator::ReadFileStd(data_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }
  ByteArrayPtr sign_buffer = nullptr;
  if (!sign_path.empty()) {
    std::string sign_buffer_str;
    if (!FileOperator::ReadFileStd(sign_path_std, sign_buffer_str)) {
      throw std::runtime_error("read file error");
    }
    sign_buffer = std::make_unique<std::string>(sign_buffer_str);
  }
  auto err = GpgBasicOperator::GetInstance(_channel).Verify(
      in_buffer, sign_buffer, result);
  return err;
}

gpg_error_t GpgFrontend::GpgFileOpera::EncryptSignFile(
    KeyListPtr keys, KeyListPtr signer_keys, const std::string& in_path,
    const std::string& out_path, GpgEncrResult& encr_res,
    GpgSignResult& sign_res, int _channel) {
#ifdef WINDOWS
  auto in_path_std =
      std::filesystem::path(QString::fromStdString(in_path).toStdU16String());
  auto out_path_std =
      std::filesystem::path(QString::fromStdString(out_path).toStdU16String());
#else
  auto in_path_std = std::filesystem::path(in_path);
  auto out_path_std = std::filesystem::path(out_path);
#endif

  std::string in_buffer;
  if (!FileOperator::ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }
  ByteArrayPtr out_buffer = nullptr;

  auto err = GpgBasicOperator::GetInstance(_channel).EncryptSign(
      std::move(keys), std::move(signer_keys), in_buffer, out_buffer, encr_res,
      sign_res);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!FileOperator::WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}

gpg_error_t GpgFrontend::GpgFileOpera::DecryptVerifyFile(
    const std::string& in_path, const std::string& out_path,
    GpgDecrResult& decr_res, GpgVerifyResult& verify_res) {
#ifdef WINDOWS
  auto in_path_std =
      std::filesystem::path(QString::fromStdString(in_path).toStdU16String());
  auto out_path_std =
      std::filesystem::path(QString::fromStdString(out_path).toStdU16String());
#else
  auto in_path_std = std::filesystem::path(in_path);
  auto out_path_std = std::filesystem::path(out_path);
#endif

  std::string in_buffer;
  if (!FileOperator::ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }

  ByteArrayPtr out_buffer = nullptr;
  auto err = GpgBasicOperator::GetInstance().DecryptVerify(
      in_buffer, out_buffer, decr_res, verify_res);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!FileOperator::WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("write file error");
    };

  return err;
}
unsigned int GpgFrontend::GpgFileOpera::EncryptFileSymmetric(
    const std::string& in_path, const std::string& out_path,
    GpgFrontend::GpgEncrResult& result, int _channel) {
#ifdef WINDOWS
  auto in_path_std =
      std::filesystem::path(QString::fromStdString(in_path).toStdU16String());
  auto out_path_std =
      std::filesystem::path(QString::fromStdString(out_path).toStdU16String());
#else
  auto in_path_std = std::filesystem::path(in_path);
  auto out_path_std = std::filesystem::path(out_path);
#endif

  std::string in_buffer;
  if (!FileOperator::ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }

  ByteArrayPtr out_buffer;
  auto err = GpgBasicOperator::GetInstance(_channel).EncryptSymmetric(
      in_buffer, out_buffer, result);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR)
    if (!FileOperator::WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("write_buffer_to_file error");
    };

  return err;
}
