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

#pragma once

// spdlog library configuration
#undef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

// logger fmt
#include "core/GpgFrontendCoreExport.h"

template <>
struct fmt::formatter<QString> {
  // Parses format specifications.
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  // Formats the QString qstr and writes it to the output.
  template <typename FormatContext>
  auto format(const QString& qstr, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    // Convert QString to UTF-8 QString (to handle Unicode characters
    // correctly)
    QByteArray utf8_array = qstr.toUtf8();
    return fmt::format_to(ctx.out(), "{}", utf8_array.constData());
  }
};

template <>
struct fmt::formatter<QByteArray> {
  // Parses format specifications.
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  // Formats the QString qstr and writes it to the output.
  template <typename FormatContext>
  auto format(const QByteArray& qarray, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    // Convert QString to UTF-8 QString (to handle Unicode characters
    // correctly)
    return fmt::format_to(ctx.out(), "{}", qarray.constData());
  }
};

namespace GpgFrontend {

/**
 * @brief
 *
 * @return std::shared_ptr<spdlog::logger>
 */
auto GPGFRONTEND_CORE_EXPORT GetDefaultLogger()
    -> std::shared_ptr<spdlog::logger>;

/**
 * @brief
 *
 * @return std::shared_ptr<spdlog::logger>
 */
auto GPGFRONTEND_CORE_EXPORT GetCoreLogger() -> std::shared_ptr<spdlog::logger>;

/**
 * @brief
 *
 * @return std::shared_ptr<spdlog::logger>
 */
auto GPGFRONTEND_CORE_EXPORT GetLogger(const QString&)
    -> std::shared_ptr<spdlog::logger>;

/**
 * @brief Set the Default Log Level object
 *
 * @return auto
 */
void GPGFRONTEND_CORE_EXPORT SetDefaultLogLevel(spdlog::level::level_enum);

/**
 * @brief
 *
 * @return auto
 */
void GPGFRONTEND_CORE_EXPORT RegisterAsyncLogger(const QString&,
                                                 spdlog::level::level_enum);

/**
 * @brief
 *
 * @return auto
 */
void GPGFRONTEND_CORE_EXPORT RegisterSyncLogger(const QString&,
                                                spdlog::level::level_enum);

}  // namespace GpgFrontend

#define GF_DEFAULT_LOG_TRACE(...) \
  SPDLOG_LOGGER_TRACE(GpgFrontend::GetDefaultLogger(), __VA_ARGS__)
#define GF_DEFAULT_LOG_DEBUG(...) \
  SPDLOG_LOGGER_DEBUG(GpgFrontend::GetDefaultLogger(), __VA_ARGS__)
#define GF_DEFAULT_LOG_INFO(...) \
  SPDLOG_LOGGER_INFO(GpgFrontend::GetDefaultLogger(), __VA_ARGS__)
#define GF_DEFAULT_LOG_WARN(...) \
  SPDLOG_LOGGER_WARN(GpgFrontend::GetDefaultLogger(), __VA_ARGS__)
#define GF_DEFAULT_LOG_ERROR(...) \
  SPDLOG_LOGGER_ERROR(GpgFrontend::GetDefaultLogger(), __VA_ARGS__)

#define GF_CORE_LOG_TRACE(...) \
  SPDLOG_LOGGER_TRACE(GpgFrontend::GetCoreLogger(), __VA_ARGS__)
#define GF_CORE_LOG_DEBUG(...) \
  SPDLOG_LOGGER_DEBUG(GpgFrontend::GetCoreLogger(), __VA_ARGS__)
#define GF_CORE_LOG_INFO(...) \
  SPDLOG_LOGGER_INFO(GpgFrontend::GetCoreLogger(), __VA_ARGS__)
#define GF_CORE_LOG_WARN(...) \
  SPDLOG_LOGGER_WARN(GpgFrontend::GetCoreLogger(), __VA_ARGS__)
#define GF_CORE_LOG_ERROR(...) \
  SPDLOG_LOGGER_ERROR(GpgFrontend::GetCoreLogger(), __VA_ARGS__)

#define GF_LOG_TRACE(ID, ...) \
  SPDLOG_LOGGER_TRACE(GpgFrontend::GetLogger(ID), __VA_ARGS__)
#define GF_LOG_DEBUG(ID, ...) \
  SPDLOG_LOGGER_DEBUG(GpgFrontend::GetLogger(ID), __VA_ARGS__)
#define GF_LOG_INFO(ID, ...) \
  SPDLOG_LOGGER_INFO(GpgFrontend::GetLogger(ID), __VA_ARGS__)
#define GF_LOG_WARN(ID, ...) \
  SPDLOG_LOGGER_WARN(GpgFrontend::GetLogger(ID), __VA_ARGS__)
#define GF_LOG_ERROR(ID, ...) \
  SPDLOG_LOGGER_ERROR(GpgFrontend::GetLogger(ID), __VA_ARGS__)
