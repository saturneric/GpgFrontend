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

#include <GFSDKContext.h>
#include <GFSDKLog.h>

#include <QDebug>
#include <QString>
#include <optional>

#include "GFModuleConvert.h"

/**
 * @file GFModuleLog.h
 * @brief Logging from inside a module.
 *
 * Three families. The stream form is the one to reach for:
 *
 *     LOG_I() << "started with" << count << "accounts";
 *
 *     LOG_INFO("started");
 *     FLOG_INFO("started with %1 accounts", count);
 *
 * `LOG_I()` and friends take no format string, so there is no placeholder to
 * get wrong and no arity to mismatch, and they accept anything QDebug can
 * print -- enums, containers, QByteArray, bool -- where `FLOG_*` accepts only
 * what `QString::arg` does. They are spelled exactly like the host's own
 * `LOG_W() << ...` in GpgFrontendCore.h, so the two halves of this codebase
 * read the same way.
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
 * `--log-level trace` is what turns it on.
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

/// This module's SDK context.
///
/// Declared here rather than taken from GFModule.h, which includes this file
/// before declaring it. Reading it is a plain load of a pointer that never
/// changes after activation, which is what lets a module's own worker thread
/// log without any thread-local state being involved.
auto GFModuleSdkContext() -> GFSDKContext*;

/// One message, with the level checked before anything is formatted.
///
/// A do/while so the macro is a single statement and stays safe next to a bare
/// `if`. The QByteArray is named rather than temporary so its lifetime plainly
/// covers the call that reads its buffer.
#define GF_MODULE_LOG_AT(severity, text)                            \
  do {                                                              \
    if (GFLogEnabled(GFModuleSdkContext(), (severity)) != 0) {      \
      const auto gf_log_line_ = (text).toUtf8();                    \
      GFLogAt(GFModuleSdkContext(), (severity), __FILE__, __LINE__, \
              Q_FUNC_INFO, gf_log_line_.constData());               \
    }                                                               \
  } while (false)

/// One log line, accumulated with QDebug and emitted when the object dies.
///
/// QDebug rather than a hand-rolled formatter: it already knows how to print
/// every Qt type, and reusing it is what makes `LOG_W() << some_enum` work
/// where `FLOG_WARN("%1", some_enum)` does not compile.
///
/// Nothing is formatted when the level is suppressed -- the check happens in
/// the constructor and every `operator<<` after it is a no-op, so a silenced
/// LOG_T() costs one call across the ABI and nothing else.
class GFModuleLogStream {
 public:
  GFModuleLogStream(int severity, const char* file, int line,
                    const char* function)
      : severity_(severity),
        file_(file),
        line_(line),
        function_(function),
        enabled_(GFLogEnabled(GFModuleSdkContext(), severity) != 0) {
    if (enabled_) debug_.emplace(&buffer_);
  }

  ~GFModuleLogStream() {
    if (!enabled_) return;

    // Reset before reading the buffer: QDebug flushes its stream in its own
    // destructor, and members outlive this body, so the last thing streamed
    // would otherwise be missing from every line.
    debug_.reset();

    // QDebug writes a separator after every item, so the line otherwise ends
    // in a space. Trailing only: leading whitespace could be something the
    // caller actually streamed.
    while (buffer_.endsWith(u' ')) buffer_.chop(1);

    const auto utf8 = buffer_.toUtf8();
    GFLogAt(GFModuleSdkContext(), severity_, file_, line_, function_,
            utf8.constData());
  }

  GFModuleLogStream(const GFModuleLogStream&) = delete;
  auto operator=(const GFModuleLogStream&) -> GFModuleLogStream& = delete;
  GFModuleLogStream(GFModuleLogStream&&) = delete;
  auto operator=(GFModuleLogStream&&) -> GFModuleLogStream& = delete;

  template <typename T>
  auto operator<<(const T& value) -> GFModuleLogStream& {
    if (enabled_) *debug_ << value;
    return *this;
  }

 private:
  int severity_;
  const char* file_;
  int line_;
  const char* function_;
  bool enabled_;
  QString buffer_;
  std::optional<QDebug> debug_;
};

/// Stream-style logging, spelled as the host spells it in GpgFrontendCore.h.
///
///     LOG_W() << "could not open" << path << ":" << file.errorString();
#define LOG_T() GFModuleLogStream(GF_LOG_TRACE, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_D() GFModuleLogStream(GF_LOG_DEBUG, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_I() GFModuleLogStream(GF_LOG_INFO, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_W() GFModuleLogStream(GF_LOG_WARN, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_E() GFModuleLogStream(GF_LOG_ERROR, __FILE__, __LINE__, Q_FUNC_INFO)

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
