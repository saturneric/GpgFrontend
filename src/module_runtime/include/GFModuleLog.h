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
 * rather than the value, which is the most common mistake made here. Note that
 * the identically-spelled `FLOG_*` in the host's own `GpgFrontendCore.h` is
 * printf, so code moved between the two needs its format string rewritten.
 *
 * ## Five levels, and they are all real
 *
 * `LOG_TRACE` through `LOG_ERROR`. Trace is for per-item chatter -- one line
 * per key, per message, per request -- and is off even at `--log-level debug`;
 * `--log-level trace` is what turns it on. It used to be an alias of debug at
 * the ABI (`GFModuleLogTrace` had a body byte-identical to `GFModuleLogDebug`)
 * and had no macro at all, so nothing in any module ever used it.
 *
 * Before that, `LOG_INFO`, `LOG_WARN` and `LOG_ERROR` all expanded to
 * `MLogDebug`, so 58 call sites -- 46 of them `LOG_ERROR` -- were emitted at
 * debug level and never appeared at any higher one. Every module author who
 * logged an error and saw nothing was looking at this. The `FLOG_*` forms were
 * always correct, which is exactly why it went unnoticed: the two families sat
 * beside each other and only one of them worked.
 *
 * ## What a line says now
 *
 * Each module logs under its own Qt category, named for its id, and carries
 * its own `__FILE__`/`__LINE__`:
 *
 *     [module.email] [W] EMailImapController.cpp:412 - imap connect failed
 *
 * rather than the `[module]` / `GFSDKLog.cpp:40` every module shared before.
 * The category is a diagnostic and a filter handle, not an identity -- see
 * GFModuleLogAt's own documentation for why that distinction matters.
 *
 * ## Cost when suppressed
 *
 * The level is checked **before** the message is formatted, so a suppressed
 * `FLOG_TRACE` does not run its `arg()` chain. It used to build the QString
 * first and discard it afterwards.
 */

/// This module's verified identity as a stable C string.
///
/// Declared here rather than taken from GFModule.h, which includes this file
/// before declaring it. Reading it is a plain load, not a thread-local one,
/// which is what lets a module's own worker thread attribute its own output.
auto GFGetModuleID() -> const char*;

/// One message, with the level checked before anything is formatted.
///
/// A do/while so the macro is a single statement and stays safe next to a bare
/// `if`. The QByteArray is named rather than temporary so its lifetime plainly
/// covers the call that reads its buffer.
#define GF_MODULE_LOG_AT(severity, text)                             \
  do {                                                               \
    if (GFModuleLogEnabled(GFGetModuleID(), (severity)) != 0) {      \
      const auto gf_log_line_ = (text).toUtf8();                     \
      GFModuleLogAt(GFGetModuleID(), (severity), __FILE__, __LINE__, \
                    Q_FUNC_INFO, gf_log_line_.constData());          \
    }                                                                \
  } while (false)

/// The QString-taking forms, kept because modules call them directly.
///
/// Macros rather than the inline functions they used to be, so that they pick
/// up the caller's `__FILE__`/`__LINE__` like everything else here. They do NOT
/// run FormatString: a caller of these has already done its own `arg()` chain,
/// and re-interpreting the result would corrupt any message that legitimately
/// contains a `%1`.
#define MLogTrace(text) GF_MODULE_LOG_AT(GF_LOG_TRACE, QString(text))
#define MLogDebug(text) GF_MODULE_LOG_AT(GF_LOG_DEBUG, QString(text))
#define MLogInfo(text) GF_MODULE_LOG_AT(GF_LOG_INFO, QString(text))
#define MLogWarn(text) GF_MODULE_LOG_AT(GF_LOG_WARN, QString(text))
#define MLogError(text) GF_MODULE_LOG_AT(GF_LOG_ERROR, QString(text))

#define LOG_TRACE(format) \
  GF_MODULE_LOG_AT(GF_LOG_TRACE, FormatString(QString(format)))
#define LOG_DEBUG(format) \
  GF_MODULE_LOG_AT(GF_LOG_DEBUG, FormatString(QString(format)))
#define LOG_INFO(format) \
  GF_MODULE_LOG_AT(GF_LOG_INFO, FormatString(QString(format)))
#define LOG_WARN(format) \
  GF_MODULE_LOG_AT(GF_LOG_WARN, FormatString(QString(format)))
#define LOG_ERROR(format) \
  GF_MODULE_LOG_AT(GF_LOG_ERROR, FormatString(QString(format)))

#define FLOG_TRACE(format, ...) \
  GF_MODULE_LOG_AT(GF_LOG_TRACE, FormatString(QString(format), __VA_ARGS__))
#define FLOG_DEBUG(format, ...) \
  GF_MODULE_LOG_AT(GF_LOG_DEBUG, FormatString(QString(format), __VA_ARGS__))
#define FLOG_INFO(format, ...) \
  GF_MODULE_LOG_AT(GF_LOG_INFO, FormatString(QString(format), __VA_ARGS__))
#define FLOG_WARN(format, ...) \
  GF_MODULE_LOG_AT(GF_LOG_WARN, FormatString(QString(format), __VA_ARGS__))
#define FLOG_ERROR(format, ...) \
  GF_MODULE_LOG_AT(GF_LOG_ERROR, FormatString(QString(format), __VA_ARGS__))
