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

#include <boost/algorithm/string.hpp>

namespace GpgFrontend {

inline void Ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

inline void Rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

inline auto Trim(std::string& s) -> std::string {
  Ltrim(s);
  Rtrim(s);
  return s;
}

auto GetGpgmeErrorString(size_t buffer_size, gpgme_error_t err) -> std::string {
  std::vector<char> buffer(buffer_size);

  gpgme_error_t const ret = gpgme_strerror_r(err, buffer.data(), buffer.size());
  if (ret == ERANGE && buffer_size < 1024) {
    return GetGpgmeErrorString(buffer_size * 2, err);
  }

  return {buffer.data()};
}

auto GetGpgmeErrorString(gpgme_error_t err) -> std::string {
  return GetGpgmeErrorString(64, err);
}

auto CheckGpgError(GpgError err) -> GpgError {
  auto err_code = gpg_err_code(err);
  if (err_code != GPG_ERR_NO_ERROR) {
    SPDLOG_ERROR(
        "gpg operation failed [error code: {}], source: {} description: {}",
        err_code, gpgme_strsource(err), GetGpgmeErrorString(err));
  }
  return err_code;
}

auto CheckGpgError2ErrCode(GpgError err, GpgError predict) -> GpgErrorCode {
  auto err_code = gpg_err_code(err);
  if (err_code != gpg_err_code(predict)) {
    if (err_code == GPG_ERR_NO_ERROR) {
      SPDLOG_WARN("[Warning {}] Source: {} description: {} predict: {}",
                  gpg_err_code(err), gpgme_strsource(err),
                  GetGpgmeErrorString(err), GetGpgmeErrorString(predict));
    } else {
      SPDLOG_ERROR("[Error {}] Source: {} description: {} predict: {}",
                   gpg_err_code(err), gpgme_strsource(err),
                   GetGpgmeErrorString(err), GetGpgmeErrorString(predict));
    }
  }
  return err_code;
}

auto DescribeGpgErrCode(GpgError err) -> GpgErrorDesc {
  return {gpgme_strsource(err), GetGpgmeErrorString(err)};
}

auto CheckGpgError(GpgError err, const std::string& /*comment*/) -> GpgError {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    SPDLOG_WARN("[Error {}] Source: {} description: {}", gpg_err_code(err),
                gpgme_strsource(err), GetGpgmeErrorString(err));
  }
  return err;
}

auto TextIsSigned(BypeArrayRef text) -> int {
  using boost::algorithm::ends_with;
  using boost::algorithm::starts_with;

  auto trim_text = Trim(text);
  if (starts_with(trim_text, PGP_SIGNED_BEGIN) &&
      ends_with(trim_text, PGP_SIGNED_END)) {
    return 2;
  }
  if (text.find(PGP_SIGNED_BEGIN) != std::string::npos &&
      text.find(PGP_SIGNED_END) != std::string::npos) {
    return 1;
  }
  return 0;
}

auto SetExtensionOfOutputFile(std::filesystem::path path, GpgOperation opera,
                              bool ascii) -> std::filesystem::path {
  std::string extension;

  if (ascii) {
    switch (opera) {
      case kENCRYPT:
      case kSIGN:
      case kENCRYPT_SIGN:
        extension = path.extension().string() + ".asc";
        break;
      default:
        break;
    }
  } else {
    switch (opera) {
      case kENCRYPT:
      case kENCRYPT_SIGN:
        extension = path.extension().string() + ".gpg";
        break;
      case kSIGN:
        extension = path.extension().string() + ".sig";
        break;
      default:
        break;
    }
  }
  return path.replace_extension(extension);
}

auto SetExtensionOfOutputFileForArchive(std::filesystem::path path,
                                        GpgOperation opera, bool ascii)
    -> std::filesystem::path {
  std::string extension;

  if (ascii) {
    switch (opera) {
      case kENCRYPT:
      case kENCRYPT_SIGN:
        extension += ".tar.asc";
        return path.replace_extension(extension);
      default:
        break;
    }
  } else {
    switch (opera) {
      case kENCRYPT:
      case kENCRYPT_SIGN:
        extension += ".tar.gpg";
        return path.replace_extension(extension);
      default:
        break;
    }
  }
  return path.parent_path();
}

}  // namespace GpgFrontend
