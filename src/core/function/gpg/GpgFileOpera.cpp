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

#include <utility>

#include "core/function/gpg/GpgBasicOperator.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

void GpgFrontend::GpgFileOpera::EncryptFile(std::vector<GpgKey> keys,
                                            const std::string& in_path,
                                            const std::string& out_path,
                                            bool ascii,
                                            const GpgOperationCallback& cb) {
#ifdef WINDOWS
  auto in_path_std =
      std::filesystem::path(QString::fromStdString(in_path).toStdU16String());
  auto out_path_std =
      std::filesystem::path(QString::fromStdString(out_path).toStdU16String());
#else
  auto in_path_std = std::filesystem::path(in_path);
  auto out_path_std = std::filesystem::path(out_path);
#endif

  auto read_result = ReadFileGFBuffer(in_path_std);
  if (!std::get<0>(read_result)) {
    throw std::runtime_error("read file error");
  }

  GpgBasicOperator::GetInstance().Encrypt(
      std::move(keys), std::get<1>(read_result), ascii,
      [=](GpgError err, const DataObjectPtr& data_object) {
        if (!data_object->Check<GpgEncryptResult, GFBuffer>()) {
          throw std::runtime_error("data object transfers wrong arguments");
        }
        auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
        auto buffer = ExtractParams<GFBuffer>(data_object, 1);
        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          cb(err, TransferParams(result));
          return;
        }

        if (!WriteFileGFBuffer(out_path_std, buffer)) {
          throw std::runtime_error("write buffer to file error");
        }
      });
}

void GpgFrontend::GpgFileOpera::DecryptFile(const std::string& in_path,
                                            const std::string& out_path,
                                            const GpgOperationCallback& cb) {
#ifdef WINDOWS
  auto in_path_std =
      std::filesystem::path(QString::fromStdString(in_path).toStdU16String());
  auto out_path_std =
      std::filesystem::path(QString::fromStdString(out_path).toStdU16String());
#else
  auto in_path_std = std::filesystem::path(in_path);
  auto out_path_std = std::filesystem::path(out_path);
#endif

  auto read_result = ReadFileGFBuffer(in_path_std);
  if (!std::get<0>(read_result)) {
    throw std::runtime_error("read file error");
  }

  GpgBasicOperator::GetInstance().Decrypt(
      std::get<1>(read_result),
      [=](GpgError err, const DataObjectPtr& data_object) {
        if (!data_object->Check<GpgEncryptResult, GFBuffer>()) {
          throw std::runtime_error("data object transfers wrong arguments");
        }
        auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
        auto buffer = ExtractParams<GFBuffer>(data_object, 1);

        if (CheckGpgError(err) == GPG_ERR_NO_ERROR &&
            !WriteFileGFBuffer(out_path_std, buffer)) {
          throw std::runtime_error("write buffer to file error");
        }

        cb(err, TransferParams(result));
      });
}

auto GpgFrontend::GpgFileOpera::SignFile(KeyListPtr keys,
                                         const std::string& in_path,
                                         const std::string& out_path,
                                         GpgSignResult& result, int _channel)
    -> gpgme_error_t {
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
  if (!ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }
  ByteArrayPtr out_buffer;

  auto err = GpgBasicOperator::GetInstance(_channel).Sign(
      std::move(keys), in_buffer, out_buffer, GPGME_SIG_MODE_DETACH, result);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR)
    if (!WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("WriteBufferToFile error");
    };

  return err;
}

auto GpgFrontend::GpgFileOpera::VerifyFile(const std::string& data_path,
                                           const std::string& sign_path,
                                           GpgVerifyResult& result,
                                           int _channel) -> gpgme_error_t {
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
  if (!ReadFileStd(data_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }
  ByteArrayPtr sign_buffer = nullptr;
  if (!sign_path.empty()) {
    std::string sign_buffer_str;
    if (!ReadFileStd(sign_path_std, sign_buffer_str)) {
      throw std::runtime_error("read file error");
    }
    sign_buffer = std::make_unique<std::string>(sign_buffer_str);
  }
  auto err = GpgBasicOperator::GetInstance(_channel).Verify(
      in_buffer, sign_buffer, result);
  return err;
}

auto GpgFrontend::GpgFileOpera::EncryptSignFile(
    KeyListPtr keys, KeyListPtr signer_keys, const std::string& in_path,
    const std::string& out_path, GpgEncrResult& encr_res,
    GpgSignResult& sign_res, int _channel) -> gpg_error_t {
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
  if (!ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }
  ByteArrayPtr out_buffer = nullptr;

  auto err = GpgBasicOperator::GetInstance(_channel).EncryptSign(
      std::move(keys), std::move(signer_keys), in_buffer, out_buffer, encr_res,
      sign_res);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR)
    if (!WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("WriteBufferToFile error");
    };

  return err;
}

auto GpgFrontend::GpgFileOpera::DecryptVerifyFile(const std::string& in_path,
                                                  const std::string& out_path,
                                                  GpgDecrResult& decr_res,
                                                  GpgVerifyResult& verify_res)
    -> gpg_error_t {
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
  if (!ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }

  ByteArrayPtr out_buffer = nullptr;
  auto err = GpgBasicOperator::GetInstance().DecryptVerify(
      in_buffer, out_buffer, decr_res, verify_res);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR)
    if (!WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("write file error");
    };

  return err;
}
auto GpgFrontend::GpgFileOpera::EncryptFileSymmetric(
    const std::string& in_path, const std::string& out_path,
    GpgFrontend::GpgEncrResult& result, int _channel) -> unsigned int {
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
  if (!ReadFileStd(in_path_std, in_buffer)) {
    throw std::runtime_error("read file error");
  }

  ByteArrayPtr out_buffer;
  auto err = GpgBasicOperator::GetInstance(_channel).EncryptSymmetric(
      in_buffer, out_buffer, result);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    if (!WriteFileStd(out_path_std, *out_buffer)) {
      throw std::runtime_error("WriteBufferToFile error");
    }
  };

  return err;
}
