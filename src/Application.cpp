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

#include "GpgFrontendContext.h"
#include "core/GpgConstants.h"
#include "ui/GpgFrontendUIInit.h"

// main
#include "Initialize.h"

namespace GpgFrontend {

void GFMessageHandler(QtMsgType type, const QMessageLogContext& context,
                      const QString& msg) {
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

auto BuildQtLoggingFilterRules(int level) -> QString {
  switch (static_cast<GFLogLevel>(level)) {
    case GFLogLevel::kDEBUG:
      return {};
    case GFLogLevel::kINFO:
      return "*.debug=false\n";
    case GFLogLevel::kWARNING:
      return "*.debug=false\n"
             "*.info=false\n";
    case GFLogLevel::kCRITICAL:
      return "*.debug=false\n"
             "*.info=false\n"
             "*.warning=false\n";
    case GFLogLevel::kFATAL:
      return "*.debug=false\n"
             "*.info=false\n"
             "*.warning=false\n"
             "*.critical=false\n";
    default:
      return "*.debug=false\n";
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

void GFLogRingBuffer::Push(GFLogEntry entry) {
  QMutexLocker locker(&mutex_);

  buffer_[write_index_] = std::move(entry);
  write_index_ = (write_index_ + 1) % capacity_;

  if (size_ < capacity_) {
    ++size_;
  } else {
    full_ = true;
  }
}

auto GFLogRingBuffer::Snapshot() const -> QVector<GFLogEntry> {
  QMutexLocker locker(&mutex_);

  QVector<GFLogEntry> result;
  result.reserve(size_);

  if (size_ == 0) return result;

  const int start = full_ ? write_index_ : 0;
  for (int i = 0; i < size_; ++i) {
    const int index = (start + i) % capacity_;
    result.push_back(buffer_[index]);
  }

  return result;
}

auto GFLogRingBuffer::Size() const -> int {
  QMutexLocker locker(&mutex_);
  return size_;
}

auto GFLogRingBuffer::Capacity() const -> int { return capacity_; }

auto GFLogManager::Instance() -> GFLogManager& {
  static GFLogManager instance;
  return instance;
}

void GFLogManager::InitRingBuffer(int capacity) {
  QMutexLocker locker(&mutex_);
  ring_buffer_ = std::make_unique<GFLogRingBuffer>(capacity);
}

void GFLogManager::Push(const GFLogEntry& entry) {
  QMutexLocker locker(&mutex_);
  if (ring_buffer_ != nullptr) {
    ring_buffer_->Push(entry);
  }
}

auto GFLogManager::Snapshot() const -> QVector<GFLogEntry> {
  QMutexLocker locker(&mutex_);
  if (ring_buffer_ == nullptr) return {};
  return ring_buffer_->Snapshot();
}

}  // namespace GpgFrontend
