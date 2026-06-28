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
#include "GFCoreLog.h"

namespace GpgFrontend {

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

GFLogManager::~GFLogManager() = default;

void GFLogManager::InitRingBuffer(int capacity) {
  QMutexLocker locker(&mutex_);
  ring_buffer_ = std::make_unique<GFLogRingBuffer>(capacity);
}

void GFLogManager::InitFileLogger(const QString& log_dir, qint64 max_file_bytes,
                                  int max_files) {
  QMutexLocker locker(&mutex_);

  // already logging to a file, or no destination given
  if (log_stream_ != nullptr) return;
  if (log_dir.trimmed().isEmpty()) return;

  QDir dir(log_dir);
  if (!dir.exists() && !dir.mkpath(".")) return;

  max_file_bytes_ = max_file_bytes > 0 ? max_file_bytes : 5LL * 1024 * 1024;
  max_files_ = max_files > 0 ? max_files : 1;
  log_file_path_ = dir.absoluteFilePath("gpgfrontend.log");

  auto file = std::make_unique<QFile>(log_file_path_);
  if (!file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    // cannot open file: leave file logging disabled, ring/stderr still work
    return;
  }

  log_file_ = std::move(file);
  log_stream_ = std::make_unique<QTextStream>(log_file_.get());

  // flush whatever was buffered before file logging started, so the file
  // captures the full history including early startup entries.
  if (ring_buffer_ != nullptr) {
    for (const auto& entry : ring_buffer_->Snapshot()) {
      *log_stream_ << entry.formatted_message << '\n';
    }
    log_stream_->flush();
  }
}

void GFLogManager::StopFileLogger() {
  QMutexLocker locker(&mutex_);
  log_stream_.reset();
  if (log_file_ != nullptr) {
    log_file_->close();
    log_file_.reset();
  }
}

void GFLogManager::WriteEntryToFileUnlocked(const GFLogEntry& entry) {
  if (log_stream_ == nullptr) return;

  // flush every line: a hung or crashed startup must still leave the latest
  // entry on disk, otherwise the file would be useless for the very failures
  // it exists to diagnose.
  *log_stream_ << entry.formatted_message << '\n';
  log_stream_->flush();

  if (log_file_ != nullptr && log_file_->size() >= max_file_bytes_) {
    RotateUnlocked();
  }
}

void GFLogManager::RotateUnlocked() {
  log_stream_.reset();
  if (log_file_ != nullptr) log_file_->close();
  log_file_.reset();

  const QFileInfo info(log_file_path_);
  const auto backup_path = [&](int i) -> QString {
    return info.absolutePath() + "/" + info.completeBaseName() + "." +
           QString::number(i) + "." + info.suffix();
  };

  // drop the oldest backup, then shift the rest up by one
  QFile::remove(backup_path(max_files_));
  for (int i = max_files_ - 1; i >= 1; --i) {
    if (QFile::exists(backup_path(i))) {
      QFile::rename(backup_path(i), backup_path(i + 1));
    }
  }
  if (QFile::exists(log_file_path_)) {
    QFile::rename(log_file_path_, backup_path(1));
  }

  // reopen a fresh active file
  auto file = std::make_unique<QFile>(log_file_path_);
  if (file->open(QIODevice::WriteOnly | QIODevice::Truncate |
                 QIODevice::Text)) {
    log_file_ = std::move(file);
    log_stream_ = std::make_unique<QTextStream>(log_file_.get());
  }
}

void GFLogManager::Push(const GFLogEntry& entry) {
  QMutexLocker locker(&mutex_);
  if (ring_buffer_ != nullptr) {
    ring_buffer_->Push(entry);
  }
  WriteEntryToFileUnlocked(entry);
}

auto GFLogManager::Snapshot() const -> QVector<GFLogEntry> {
  QMutexLocker locker(&mutex_);
  if (ring_buffer_ == nullptr) return {};
  return ring_buffer_->Snapshot();
}

}  // namespace GpgFrontend
