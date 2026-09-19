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

#pragma once

#include <GFSDKLog.h>

#include <QString>

#include "GFModuleConvert.h"

/**
 * @file GFModuleLog.h
 * @brief Logging from inside a module.
 *
 * Two families, differing only in whether they take arguments:
 *
 *     LOG_INFO("started");
 *     FLOG_INFO("started with %1 accounts", count);
 *
 * The placeholders are Qt's `%1`, `%2`, ... and **not** printf's `%s`/`%d`.
 * Passing a printf format produces a message with the format string in it
 * rather than the value, which is the most common mistake made here.
 *
 * ## The levels are real levels
 *
 * They were not. `LOG_INFO`, `LOG_WARN` and `LOG_ERROR` all expanded to
 * `MLogDebug`, so 58 call sites across the modules -- 46 of them `LOG_ERROR`
 * -- were emitted at debug level and never appeared at any higher one. Every
 * module author who logged an error and saw nothing was looking at this. The
 * `FLOG_*` forms were always correct, which is exactly why it went unnoticed:
 * the two families sat beside each other and only one of them worked.
 */

inline void MLogDebug(const QString& s) { GFModuleLogDebug(s.toUtf8()); }
inline void MLogInfo(const QString& s) { GFModuleLogInfo(s.toUtf8()); }
inline void MLogWarn(const QString& s) { GFModuleLogWarn(s.toUtf8()); }
inline void MLogError(const QString& s) { GFModuleLogError(s.toUtf8()); }

#define LOG_DEBUG(format) MLogDebug(FormatString(QString(format)))
#define LOG_INFO(format) MLogInfo(FormatString(QString(format)))
#define LOG_WARN(format) MLogWarn(FormatString(QString(format)))
#define LOG_ERROR(format) MLogError(FormatString(QString(format)))

#define FLOG_DEBUG(format, ...) \
  MLogDebug(FormatString(QString(format), __VA_ARGS__))
#define FLOG_INFO(format, ...) \
  MLogInfo(FormatString(QString(format), __VA_ARGS__))
#define FLOG_WARN(format, ...) \
  MLogWarn(FormatString(QString(format), __VA_ARGS__))
#define FLOG_ERROR(format, ...) \
  MLogError(FormatString(QString(format), __VA_ARGS__))
