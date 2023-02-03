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

#include "core/function/CharsetOperator.h"

#include <spdlog/spdlog.h>
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>

#include <cstddef>
#include <memory>
#include <string>

GpgFrontend::CharsetOperator::CharsetInfo GpgFrontend::CharsetOperator::Detect(
    const std::string &buffer) {
  const UCharsetMatch *ucm;
  UErrorCode status = U_ZERO_ERROR;
  UCharsetDetector *csd = ucsdet_open(&status);

  status = U_ZERO_ERROR;
  if (U_FAILURE(status)) {
    SPDLOG_ERROR("failed to open charset detector: {}", u_errorName(status));
    return {"unknown", "unknown", 0};
  }

  SPDLOG_INFO("detecting charset buffer: {} bytes", buffer.size());

  status = U_ZERO_ERROR;
  ucsdet_setText(csd, buffer.data(), buffer.size(), &status);
  if (U_FAILURE(status)) {
    SPDLOG_ERROR("failed to set text to charset detector: {}",
                 u_errorName(status));
    return {"unknown", "unknown", 0};
  }

  status = U_ZERO_ERROR;
  ucm = ucsdet_detect(csd, &status);

  if (U_FAILURE(status)) return {"unknown", "unknown", 0};

  status = U_ZERO_ERROR;
  const char *name = ucsdet_getName(ucm, &status);
  if (U_FAILURE(status)) return {"unknown", "unknown", 0};

  status = U_ZERO_ERROR;
  int confidence = ucsdet_getConfidence(ucm, &status);
  if (U_FAILURE(status)) return {name, "unknown", 0};

  status = U_ZERO_ERROR;
  const char *language = ucsdet_getLanguage(ucm, &status);
  if (U_FAILURE(status)) return {name, "unknown", confidence};

  SPDLOG_INFO("Detected charset: {} {} {}", name, language, confidence);
  return {name, language, confidence};
}

bool GpgFrontend::CharsetOperator::Convert2Utf8(const std::string &buffer,
                                                std::string &out_buffer,
                                                std::string from_charset_name) {
  UErrorCode status = U_ZERO_ERROR;
  const auto from_encode = std::string("utf-8");
  const auto to_encode = from_charset_name;

  SPDLOG_INFO("Converting buffer: {}", buffer.size());

  // test if the charset is supported
  UConverter *conv = ucnv_open(from_encode.c_str(), &status);
  ucnv_close(conv);
  if (U_FAILURE(status)) {
    SPDLOG_ERROR("failed to open converter: {}, from encode: {}",
                 u_errorName(status), from_encode);
    return false;
  }

  // test if the charset is supported
  conv = ucnv_open(to_encode.c_str(), &status);
  ucnv_close(conv);
  if (U_FAILURE(status)) {
    SPDLOG_ERROR("failed to open converter: {}, to encode: {}",
                 u_errorName(status), to_encode);
    return false;
  }

  status = U_ZERO_ERROR;
  int32_t target_limit = 0, target_capacity = 0;

  target_capacity =
      ucnv_convert(from_encode.c_str(), to_encode.c_str(), nullptr,
                   target_limit, buffer.data(), buffer.size(), &status);

  if (status == U_BUFFER_OVERFLOW_ERROR) {
    status = U_ZERO_ERROR;
    target_limit = target_capacity + 1;
    out_buffer.clear();
    out_buffer.resize(target_capacity);
    target_capacity =
        ucnv_convert(from_encode.c_str(), to_encode.c_str(), out_buffer.data(),
                     out_buffer.size(), buffer.data(), buffer.size(), &status);
  }

  if (U_FAILURE(status)) {
    SPDLOG_ERROR("failed to convert to utf-8: {}", u_errorName(status));
    return false;
  }

  SPDLOG_INFO("converted buffer: {} bytes", out_buffer.size());
  return true;
}