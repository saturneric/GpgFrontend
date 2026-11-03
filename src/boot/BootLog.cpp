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

#include "boot/BootLog.h"

#include "core/GFCoreLog.h"

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

namespace {

std::atomic<int> g_log_level{static_cast<int>(GpgFrontend::GFLogLevel::kINFO)};

auto ShouldLogMessage(QtMsgType type, GpgFrontend::GFLogLevel level) -> bool {
  switch (level) {
    // Trace rides on QtDebugMsg and is separated from debug by its own Qt
    // category, not by this gate -- so the gate must let it through here, or
    // the most verbose level would be the one that silences it.
    case GpgFrontend::GFLogLevel::kTRACE:
    case GpgFrontend::GFLogLevel::kDEBUG:
      return true;

    case GpgFrontend::GFLogLevel::kINFO:
      return type != QtDebugMsg;

    case GpgFrontend::GFLogLevel::kWARNING:
      return type != QtDebugMsg && type != QtInfoMsg;

    case GpgFrontend::GFLogLevel::kCRITICAL:
      return type == QtCriticalMsg || type == QtFatalMsg;

    case GpgFrontend::GFLogLevel::kFATAL:
      return type == QtFatalMsg;

    default:
      return type != QtDebugMsg;
  }
}

#ifdef Q_OS_ANDROID
auto AndroidLogPriority(QtMsgType type) -> int {
  switch (type) {
    case QtDebugMsg:
      return ANDROID_LOG_DEBUG;
    case QtInfoMsg:
      return ANDROID_LOG_INFO;
    case QtWarningMsg:
      return ANDROID_LOG_WARN;
    case QtCriticalMsg:
      return ANDROID_LOG_ERROR;
    case QtFatalMsg:
      return ANDROID_LOG_FATAL;
  }
  return ANDROID_LOG_INFO;
}
#endif

}  // namespace

namespace GpgFrontend {

void SetGFLogLevel(int level) {
  g_log_level.store(level, std::memory_order_relaxed);
}

void GFMessageHandler(QtMsgType type, const QMessageLogContext& context,
                      const QString& msg) {
  const auto level =
      static_cast<GFLogLevel>(g_log_level.load(std::memory_order_relaxed));

  if (!ShouldLogMessage(type, level)) {
    return;
  }

  const QString formatted = qFormatLogMessage(type, context, msg);

  GFLogEntry entry;
  entry.timestamp = QDateTime::currentDateTime();
  entry.type = type;
  entry.category =
      QString::fromUtf8((context.category != nullptr) ? context.category : "");
  entry.file = QString::fromUtf8((context.file != nullptr) ? context.file : "");
  entry.line = context.line;
  entry.function =
      QString::fromUtf8((context.function != nullptr) ? context.function : "");
  entry.formatted_message = formatted;
  entry.raw_message = msg;

  GFLogManager::Instance().Push(entry);

#if defined(Q_OS_ANDROID)
  // An application's stdout and stderr go nowhere on Android: logcat is where
  // its messages are read, tagged with their category as Qt's own handler
  // does.
  const QByteArray tag =
      (context.category != nullptr && context.category[0] != '\0')
          ? QByteArray(context.category)
          : QByteArrayLiteral("GpgFrontend");
  __android_log_write(AndroidLogPriority(type), tag.constData(),
                      formatted.toUtf8().constData());
#elif defined(Q_OS_WINDOWS)
  QByteArray local = formatted.toLocal8Bit();
  fprintf(stdout, "%s\n", local.constData());
  fflush(stdout);
#else
  QByteArray local = formatted.toLocal8Bit();
  fprintf(stderr, "%s\n", local.constData());
  fflush(stderr);
#endif

  if (type == QtFatalMsg) {
    abort();
  }
}

void ApplyLogLevel(int level) {
  SetGFLogLevel(level);
  QLoggingCategory::setFilterRules(BuildQtLoggingFilterRules(level));

  // env_logger level name for the Rust (rPGP) crate, propagated via RUST_LOG
  // and read by gfr_init_logger() at core init. A RUST_LOG already set by the
  // user wins.
  const char* rust_level;
  switch (static_cast<GFLogLevel>(level)) {
    case GFLogLevel::kTRACE:
      rust_level = "trace";
      break;
    case GFLogLevel::kDEBUG:
      rust_level = "debug";
      break;
    case GFLogLevel::kINFO:
      rust_level = "info";
      break;
    case GFLogLevel::kWARNING:
      rust_level = "warn";
      break;
    default:
      rust_level = "error";
      break;
  }

  if (!qEnvironmentVariableIsSet("RUST_LOG")) {
    qputenv("RUST_LOG", rust_level);
  }
}

}  // namespace GpgFrontend
