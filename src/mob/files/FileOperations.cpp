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

#include "files/FileOperations.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSaveFile>
#include <QUrl>
#include <limits>

#include "core/function/openpgp/ChannelHandle.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "files/StagingArea.h"
#include "plat/common/PlatformFolder.h"
#include "plat/common/PlatformShare.h"
#include "shell/rules/FileNameRules.h"

namespace GpgFrontend::Mobile {

namespace {

StagingArea* g_staging = nullptr;
ShareRetentionStore* g_retention = nullptr;

}  // namespace

namespace {

auto IsDocument(const QString& to) -> bool {
  return to.startsWith(QStringLiteral("content:"));
}

}  // namespace

auto CopyLimited(const QString& from, const QString& to, qint64 limit,
                 const CopyProgress& progress) -> bool {
  // A local file is written beside itself and put in its place only once
  // whole: a copy that fails leaves what was there before as it was. A
  // document (picked to be written, so made for this) is written where it
  // is; one that fails is deleted, never left looking like the whole file.
  const auto document = IsDocument(to);
  QFile in(from);
  if (!in.open(QIODevice::ReadOnly)) {
    if (document) DeleteFolderDocument(QUrl(to));
    return false;
  }
  QSaveFile local(to);
  QFile direct(to);
  QFileDevice& out = document ? static_cast<QFileDevice&>(direct) : local;
  // Truncated: a longer document must not keep its tail, which a provider
  // does unless asked ("wt", not "w").
  if (!out.open(document ? QIODevice::WriteOnly | QIODevice::Truncate
                         : QIODevice::WriteOnly)) {
    return false;
  }
  const auto fail = [&]() {
    if (document) {
      direct.close();
      DeleteFolderDocument(QUrl(to));
    } else {
      local.cancelWriting();
    }
    return false;
  };

  constexpr qint64 kChunk = 1024 * 1024;
  const auto total = in.size();
  qint64 copied = 0;
  while (!in.atEnd()) {
    const auto chunk = in.read(kChunk);
    if (chunk.isEmpty() && in.error() != QFileDevice::NoError) return fail();
    copied += chunk.size();
    if (copied > limit) return fail();
    if (out.write(chunk) != chunk.size()) return fail();
    if (progress && total > kChunk) progress(copied, total);
  }
  if (!document) return local.commit();
  // A provider may only say a write failed once it is closed.
  if (!direct.flush()) return fail();
  direct.close();
  return direct.error() == QFileDevice::NoError || fail();
}

auto WriteWhole(const char* data, qint64 size, const QString& to) -> bool {
  const auto document = IsDocument(to);
  if (!document) {
    QSaveFile local(to);
    return local.open(QIODevice::WriteOnly) &&
           local.write(data, size) == size && local.commit();
  }
  QFile direct(to);
  auto written = direct.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
                 direct.write(data, size) == size && direct.flush();
  direct.close();
  written = written && direct.error() == QFileDevice::NoError;
  if (!written) DeleteFolderDocument(QUrl(to));
  return written;
}

void SetFileStores(StagingArea* staging, ShareRetentionStore* retention) {
  g_staging = staging;
  g_retention = retention;
}

auto FileStagingArea() -> StagingArea* { return g_staging; }

auto SharedFileStore() -> ShareRetentionStore* { return g_retention; }

void CopyOffThread(const QString& from, const QString& to, qint64 limit,
                   QObject* context, std::function<void(bool)> done,
                   CopyProgress progress) {
  auto copied = QSharedPointer<bool>::create(false);
  QPointer<QObject> guard(context);
  // Reported through the application object, which outlives the copy:
  // @p context may be gone by the time a report arrives.
  CopyProgress report;
  if (progress) {
    report = [guard, progress = std::move(progress)](qint64 n, qint64 total) {
      QMetaObject::invokeMethod(
          QCoreApplication::instance(),
          [guard, progress, n, total]() {
            if (guard != nullptr) progress(n, total);
          },
          Qt::QueuedConnection);
    };
  }
  auto* task = new Thread::Task(
      [from, to, limit, copied, report](const DataObjectPtr&) -> int {
        *copied = CopyLimited(from, to, limit, report);
        return 0;
      },
      "mobile_file_copy", TransferParams(),
      [guard, copied, done = std::move(done)](int, const DataObjectPtr&) {
        if (guard != nullptr) done(*copied);
      });
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_IO)
      ->PostTask(task);
}

void SaveIntoFolder(const QString& from, const QUrl& folder,
                    const QString& name, bool replace, QObject* context,
                    std::function<void(const QUrl&)> done) {
  const auto to = CreateFolderDocument(
      folder, name, QStringLiteral("application/octet-stream"));
  if (to.isEmpty()) {
    done({});
    return;
  }
  CopyOffThread(
      from, LocalOrContent(to), std::numeric_limits<qint64>::max(), context,
      [folder, name, replace, to, done = std::move(done)](bool copied) {
        // Made for this copy, and nothing came of it.
        if (!copied) {
          DeleteFolderDocument(to);
          done({});
          return;
        }
        auto where = to;
        if (replace) {
          const auto old = FindFolderDocument(folder, name);
          if (!old.isEmpty() && old != to && DeleteFolderDocument(old)) {
            if (const auto renamed = RenameFolderDocument(to, name);
                !renamed.isEmpty()) {
              where = renamed;
            }
          }
        }
        done(where);
      });
}

auto Sha256OfFile(const QString& path, const std::function<bool()>& wanted)
    -> FileDigest {
  constexpr qint64 kChunk = 1 << 20;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {{}, QStringLiteral("unreadable")};
  QCryptographicHash hasher(QCryptographicHash::Sha256);
  while (!file.atEnd()) {
    if (wanted && !wanted()) return {};
    const auto chunk = file.read(kChunk);
    if (chunk.isEmpty() && file.error() != QFileDevice::NoError) {
      return {{}, QStringLiteral("unreadable")};
    }
    hasher.addData(chunk);
  }
  return {QString::fromLatin1(hasher.result().toHex()), {}};
}

auto DisplayName(const QUrl& url) -> QString {
  // On Android, QFileInfo asks the document provider for its display name.
  auto name = QFileInfo(LocalOrContent(url)).fileName();
  if (name.isEmpty()) name = url.fileName();
  return name.isEmpty() ? QStringLiteral("file") : name;
}

// The names the desktop gives its outputs (SetExtensionOfOutputFile), for a
// bare file name rather than a path.
namespace {
auto OutputName(const QString& name, GpgOperation opera) -> QString {
  return QFileInfo(SetExtensionOfOutputFile(name, opera, false)).fileName();
}
}  // namespace

auto EncryptedName(const QString& name) -> QString {
  return OutputName(name, GpgOperation::kENCRYPT);
}

auto DecryptedName(const QString& name) -> QString {
  return OutputName(name, GpgOperation::kDECRYPT);
}

auto SignatureName(const QString& name) -> QString {
  return OutputName(name, GpgOperation::kSIGN);
}

auto ShareOutcomeName(ShareOutcome outcome) -> QString {
  switch (outcome) {
    case ShareOutcome::kHandedOff:
      return QStringLiteral("handed_off");
    case ShareOutcome::kDismissed:
      return QStringLiteral("dismissed");
    case ShareOutcome::kFailed:
      return QStringLiteral("failed");
    case ShareOutcome::kUnsupported:
      return QStringLiteral("unsupported");
  }
  return QStringLiteral("failed");
}

}  // namespace GpgFrontend::Mobile
