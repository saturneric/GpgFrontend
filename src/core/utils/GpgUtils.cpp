/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "GpgUtils.h"

#include "core/model/KeyDatabaseInfo.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend {

inline auto Trim(QString& s) -> QString { return s.trimmed(); }

auto GetGpgmeErrorString(size_t buffer_size, gpgme_error_t err) -> QString {
  std::vector<char> buffer(buffer_size);

  gpgme_error_t const ret = gpgme_strerror_r(err, buffer.data(), buffer.size());
  if (ret == ERANGE && buffer_size < 1024) {
    return GetGpgmeErrorString(buffer_size * 2, err);
  }

  return {buffer.data()};
}

auto GetGpgmeErrorString(gpgme_error_t err) -> QString {
  return GetGpgmeErrorString(64, err);
}

auto CheckGpgError(GpgError err) -> GpgError {
  auto err_code = gpg_err_code(err);
  if (err_code != GPG_ERR_NO_ERROR) {
    LOG_W() << "gpg operation failed [error code: " << err_code
            << "], source: " << gpgme_strsource(err)
            << " description: " << GetGpgmeErrorString(err);
  }
  return err_code;
}

auto CheckGpgError2ErrCode(GpgError err, GpgError predict) -> GpgErrorCode {
  auto err_code = gpg_err_code(err);
  if (err_code != gpg_err_code(predict)) {
    if (err_code == GPG_ERR_NO_ERROR) {
      LOG_I() << "[Warning " << gpg_err_code(err)
              << "] Source: " << gpgme_strsource(err)
              << " description: " << GetGpgmeErrorString(err)
              << " predict: " << GetGpgmeErrorString(predict);
    } else {
      LOG_W() << "[Error " << gpg_err_code(err)
              << "] Source: " << gpgme_strsource(err)
              << " description: " << GetGpgmeErrorString(err)
              << " predict: " << GetGpgmeErrorString(predict);
    }
  }
  return err_code;
}

auto DescribeGpgErrCode(GpgError err) -> GpgErrorDesc {
  return {gpgme_strsource(err), GetGpgmeErrorString(err)};
}

auto CheckGpgError(GpgError err, const QString& /*comment*/) -> GpgError {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    LOG_W() << "[Error " << gpg_err_code(err)
            << "] Source: " << gpgme_strsource(err)
            << " description: " << GetGpgmeErrorString(err);
  }
  return err;
}

auto TextIsSigned(QString text) -> int {
  auto trim_text = Trim(text);
  if (trim_text.startsWith(PGP_SIGNED_BEGIN) &&
      trim_text.endsWith(PGP_SIGNED_END)) {
    return 2;
  }
  if (text.contains(PGP_SIGNED_BEGIN) && text.contains(PGP_SIGNED_END)) {
    return 1;
  }
  return 0;
}

auto SetExtensionOfOutputFile(const QString& path, GpgOperation opera,
                              bool ascii) -> QString {
  auto file_info = QFileInfo(path);
  QString new_extension;
  QString current_extension = file_info.suffix();

  if (ascii) {
    switch (opera) {
      case kENCRYPT:
      case kSIGN:
      case kENCRYPT_SIGN:
        new_extension = current_extension + ".asc";
        break;
      default:
        break;
    }
  } else {
    switch (opera) {
      case kENCRYPT:
      case kENCRYPT_SIGN:
        new_extension = current_extension + ".gpg";
        break;
      case kSIGN:
        new_extension = current_extension + ".sig";
        break;
      default:
        break;
    }
  }

  if (!new_extension.isEmpty()) {
    return file_info.absolutePath() + "/" + file_info.completeBaseName() + "." +
           new_extension;
  }
  return file_info.absolutePath() + "/" + file_info.completeBaseName();
}

auto SetExtensionOfOutputFileForArchive(const QString& path, GpgOperation opera,
                                        bool ascii) -> QString {
  QString extension;
  auto file_info = QFileInfo(path);

  if (ascii) {
    switch (opera) {
      case kENCRYPT:
      case kENCRYPT_SIGN:
        if (file_info.completeSuffix() != "tar") extension += ".tar";
        extension += ".asc";
        return QFileInfo(path).absoluteFilePath() + extension;
        break;
      default:
        break;
    }
  } else {
    switch (opera) {
      case kENCRYPT:
      case kENCRYPT_SIGN:
        if (file_info.completeSuffix() != "tar") extension += ".tar";
        extension += ".gpg";
        return QFileInfo(path).absoluteFilePath() + extension;
        break;
      default:
        break;
    }
  }

  return file_info.absolutePath() + "/" + file_info.baseName();
}

static QList<KeyDatabaseInfo> gpg_key_database_info_cache;

auto GPGFRONTEND_CORE_EXPORT GetGpgKeyDatabaseInfos()
    -> QList<KeyDatabaseInfo> {
  if (!gpg_key_database_info_cache.empty()) return gpg_key_database_info_cache;

  auto context_index_list = Module::ListRTChildKeys("core", "gpgme.ctx.list");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  gpg_key_database_info_cache.resize(
      static_cast<qsizetype>(context_index_list.size()));
#else
  gpg_key_database_info_cache.reserve(
      static_cast<qsizetype>(context_index_list.size()));
  std::fill_n(std::back_inserter(gpg_key_database_info_cache),
              context_index_list.size(), KeyDatabaseInfo{});
#endif

  for (auto& context_index : context_index_list) {
    LOG_D() << "context grt key: " << context_index;

    const auto grt_key_prefix = QString("gpgme.ctx.list.%1").arg(context_index);
    auto channel = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".channel", -1);
    auto database_name = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".database_name", QString{});
    auto database_path = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".database_path", QString{});

    LOG_D() << "context grt channel: " << channel
            << "GRT key prefix: " << grt_key_prefix
            << "database name: " << database_name;

    auto i = KeyDatabaseInfo();
    i.channel = channel;
    i.name = database_name;
    i.path = database_path;
    gpg_key_database_info_cache[channel] = i;
  }

  return gpg_key_database_info_cache;
}
auto GPGFRONTEND_CORE_EXPORT GetGpgKeyDatabaseName(int channel) -> QString {
  auto info = GetGpgKeyDatabaseInfos();
  if (channel >= info.size()) return {};
  return info[channel].name;
}
}  // namespace GpgFrontend
