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

#include "Application.h"

#include "core/GFCoreLog.h"
#include "core/GpgConstants.h"
#include "ui/GpgFrontendUIInit.h"

// main
#include "GpgFrontendContext.h"
#include "Initialize.h"

namespace {

std::atomic<int> g_log_level{static_cast<int>(GpgFrontend::GFLogLevel::kINFO)};

auto ShouldLogMessage(QtMsgType type, GpgFrontend::GFLogLevel level) -> bool {
  switch (level) {
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

#ifdef Q_OS_WINDOWS
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

auto StartApplication(const GFCxtWPtr& p_ctx) -> int {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    qWarning("cannot get gpgfrontend context.");
    return -1;
  }

  auto* app = ctx->GetApp();
  if (app == nullptr) {
    qWarning("cannot get QApplication from gpgfrontend context.");
    return -1;
  }

  /**
   * internationalization. loop to restart main window
   * with changed translation when settings change.
   */
  int return_from_event_loop_code;
  int restart_count = 0;

  do {
    // refresh locale settings
    InitLocale();

    // after that load ui totally
    GpgFrontend::UI::InitGpgFrontendUI(app);

    // check and waiting for condition
    GpgFrontend::UI::WaitingAllInitializationFinished();

    // load module's translations
    GpgFrontend::UI::InitModulesTranslations();

    // finally create main window
    return_from_event_loop_code = GpgFrontend::UI::RunGpgFrontendUI(app);

  } while (return_from_event_loop_code == GpgFrontend::kRestartCode &&
           restart_count++ < 99);

  // set return code
  ctx->rtn = return_from_event_loop_code;

  // exit the program
  return return_from_event_loop_code;
}

}  // namespace GpgFrontend
