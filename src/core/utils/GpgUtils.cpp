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

#include "GpgUtils.h"

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
    GF_CORE_LOG_ERROR(
        "gpg operation failed [error code: {}], source: {} description: {}",
        err_code, gpgme_strsource(err), GetGpgmeErrorString(err));
  }
  return err_code;
}

auto CheckGpgError2ErrCode(GpgError err, GpgError predict) -> GpgErrorCode {
  auto err_code = gpg_err_code(err);
  if (err_code != gpg_err_code(predict)) {
    if (err_code == GPG_ERR_NO_ERROR) {
      GF_CORE_LOG_WARN("[Warning {}] Source: {} description: {} predict: {}",
                       gpg_err_code(err), gpgme_strsource(err),
                       GetGpgmeErrorString(err), GetGpgmeErrorString(predict));
    } else {
      GF_CORE_LOG_ERROR("[Error {}] Source: {} description: {} predict: {}",
                        gpg_err_code(err), gpgme_strsource(err),
                        GetGpgmeErrorString(err), GetGpgmeErrorString(predict));
    }
  }
  return err_code;
}

auto DescribeGpgErrCode(GpgError err) -> GpgErrorDesc {
  return {gpgme_strsource(err), GetGpgmeErrorString(err)};
}

auto CheckGpgError(GpgError err, const QString& /*comment*/) -> GpgError {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    GF_CORE_LOG_WARN("[Error {}] Source: {} description: {}", gpg_err_code(err),
                     gpgme_strsource(err), GetGpgmeErrorString(err));
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
  QString new_extension;
  QString current_extension = QFileInfo(path).suffix();

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
    return QFileInfo(path).path() + "/" + QFileInfo(path).completeBaseName() +
           "." + new_extension;
  }

  return path;
}

auto SetExtensionOfOutputFileForArchive(const QString& path, GpgOperation opera,
                                        bool ascii) -> QString {
  QString extension;

  if (ascii) {
    switch (opera) {
      case kENCRYPT:
      case kENCRYPT_SIGN:
        extension = ".tar.asc";
        return path + extension;
        break;
      default:
        break;
    }
  } else {
    switch (opera) {
      case kENCRYPT:
      case kENCRYPT_SIGN:
        extension = ".tar.gpg";
        return path + extension;
        break;
      default:
        break;
    }
  }

  auto file_info = QFileInfo(path);
  return file_info.absolutePath() + "/" + file_info.baseName();
}

}  // namespace GpgFrontend
