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
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

void GpgFileOpera::EncryptFile(std::vector<GpgKey> keys,
                               const std::string& in_path,
                               const std::string& out_path, bool ascii,
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
        if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
          if (!WriteFileGFBuffer(out_path_std, buffer)) {
            throw std::runtime_error("write buffer to file error");
          }
        }
        cb(err, TransferParams(result));
      });
}

void GpgFileOpera::DecryptFile(const std::string& in_path,
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
        if (!data_object->Check<GpgDecryptResult, GFBuffer>()) {
          throw std::runtime_error("data object transfers wrong arguments");
        }
        auto result = ExtractParams<GpgDecryptResult>(data_object, 0);
        auto buffer = ExtractParams<GFBuffer>(data_object, 1);

        if (CheckGpgError(err) == GPG_ERR_NO_ERROR &&
            !WriteFileGFBuffer(out_path_std, buffer)) {
          throw std::runtime_error("write buffer to file error");
        }

        cb(err, TransferParams(result));
      });
}

void GpgFileOpera::SignFile(KeyArgsList keys, const std::string& in_path,
                            const std::string& out_path, bool ascii,
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

  GpgBasicOperator::GetInstance().Sign(
      std::move(keys), std::get<1>(read_result), GPGME_SIG_MODE_DETACH, ascii,
      [=](GpgError err, const DataObjectPtr& data_object) {
        if (!data_object->Check<GpgSignResult, GFBuffer>()) {
          throw std::runtime_error("data object transfers wrong arguments");
        }
        auto result = ExtractParams<GpgSignResult>(data_object, 0);
        auto buffer = ExtractParams<GFBuffer>(data_object, 1);

        if (CheckGpgError(err) == GPG_ERR_NO_ERROR &&
            !WriteFileGFBuffer(out_path_std, buffer)) {
          throw std::runtime_error("write buffer to file error");
        }
        cb(err, TransferParams(result));
      });
}

void GpgFileOpera::VerifyFile(const std::string& data_path,
                              const std::string& sign_path,
                              const GpgOperationCallback& cb) {
#ifdef WINDOWS
  auto data_path_std =
      std::filesystem::path(QString::fromStdString(data_path).toStdU16String());
  auto sign_path_std =
      std::filesystem::path(QString::fromStdString(sign_path).toStdU16String());
#else
  auto data_path_std = std::filesystem::path(data_path);
  auto sign_path_std = std::filesystem::path(sign_path);
#endif

  auto read_result = ReadFileGFBuffer(data_path_std);
  if (!std::get<0>(read_result)) {
    throw std::runtime_error("read file error");
  }

  GFBuffer sign_buffer;
  if (!sign_path.empty()) {
    auto read_result = ReadFileGFBuffer(sign_path_std);
    if (!std::get<0>(read_result)) {
      throw std::runtime_error("read file error");
    }
    sign_buffer = std::get<1>(read_result);
  }

  GpgBasicOperator::GetInstance().Verify(
      std::get<1>(read_result), sign_buffer,
      [=](GpgError err, const DataObjectPtr& data_object) {
        if (!data_object->Check<GpgVerifyResult>()) {
          throw std::runtime_error("data object transfers wrong arguments");
        }
        auto result = ExtractParams<GpgVerifyResult>(data_object, 0);
        cb(err, TransferParams(result));
      });
}

void GpgFileOpera::EncryptSignFile(KeyArgsList keys, KeyArgsList signer_keys,
                                   const std::string& in_path,
                                   const std::string& out_path, bool ascii,
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

  GpgBasicOperator::GetInstance().EncryptSign(
      std::move(keys), std::move(signer_keys), std::get<1>(read_result), ascii,
      [=](GpgError err, const DataObjectPtr& data_object) {
        if (!data_object->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()) {
          throw std::runtime_error("data object transfers wrong arguments");
        }
        auto encrypt_result = ExtractParams<GpgEncryptResult>(data_object, 0);
        auto sign_result = ExtractParams<GpgSignResult>(data_object, 1);
        auto buffer = ExtractParams<GFBuffer>(data_object, 2);
        if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
          if (!WriteFileGFBuffer(out_path_std, buffer)) {
            throw std::runtime_error("write buffer to file error");
          }
        }
        cb(err, TransferParams(encrypt_result, sign_result));
      });
}

void GpgFileOpera::DecryptVerifyFile(const std::string& in_path,
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

  GpgBasicOperator::GetInstance().DecryptVerify(
      std::get<1>(read_result),
      [=](GpgError err, const DataObjectPtr& data_object) {
        if (!data_object
                 ->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()) {
          throw std::runtime_error("data object transfers wrong arguments");
        }
        auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object, 0);
        auto verify_result = ExtractParams<GpgVerifyResult>(data_object, 1);
        auto buffer = ExtractParams<GFBuffer>(data_object, 2);
        if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
          if (!WriteFileGFBuffer(out_path_std, buffer)) {
            throw std::runtime_error("write buffer to file error");
          }
        }
        cb(err, TransferParams(decrypt_result, verify_result));
      });
}
void GpgFileOpera::EncryptFileSymmetric(const std::string& in_path,
                                        const std::string& out_path, bool ascii,
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

  GpgBasicOperator::GetInstance().EncryptSymmetric(
      std::get<1>(read_result), ascii,
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

}  // namespace GpgFrontend