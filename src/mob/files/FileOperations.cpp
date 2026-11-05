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
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QUrl>

#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/FileCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "crypto/KeySelection.h"
#include "files/StagingArea.h"
#include "plat/common/PlatformShare.h"
#include "shell/result_analyse/GpgDecryptResultAnalyse.h"
#include "shell/result_analyse/GpgEncryptResultAnalyse.h"
#include "shell/result_analyse/GpgOperaOutcome.h"
#include "shell/result_analyse/GpgSignResultAnalyse.h"
#include "shell/result_analyse/GpgVerifyResultAnalyse.h"
#include "shell/rules/FileNameRules.h"

namespace GpgFrontend::Mobile {

namespace {

StagingArea* g_staging = nullptr;
ShareRetentionStore* g_retention = nullptr;

/// Copy @p from to @p to in pieces, refusing more than @p limit bytes.
auto CopyLimited(const QString& from, const QString& to, qint64 limit,
                 const CopyProgress& progress = {}) -> bool {
  QFile in(from);
  QFile out(to);
  // Truncated: overwriting a longer document must not keep its tail, which a
  // document provider does unless asked ("wt", not "w").
  if (!in.open(QIODevice::ReadOnly) ||
      !out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }

  // A copy that stops half way leaves nothing behind that looks like the
  // whole file: a local one is removed, a document emptied.
  const auto fail = [&out, &to]() {
    out.resize(0);
    out.close();
    if (!to.startsWith(QStringLiteral("content:"))) QFile::remove(to);
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
    if (progress && total > kChunk) {
      progress(copied, total);
    }
  }
  return out.flush() || fail();
}

/// @p done, unless @p context has gone in the meantime.
auto Guarded(QObject* context, OutcomeCallback done) {
  return [guard = QPointer<QObject>(context),
          done = std::move(done)](const GpgOperaOutcome& outcome) {
    if (guard != nullptr) done(outcome);
  };
}

}  // namespace

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

void EncryptStagedFile(const QString& in, const QString& out,
                       const QStringList& recipients, const QString& signer,
                       QObject* context, OutcomeCallback done) {
  auto finish = Guarded(context, std::move(done));
  const auto channel = UsableDefaultChannelHandle();
  auto& opera = FileCryptoOperation::GetInstance(channel);

  if (recipients.isEmpty()) {
    opera.EncryptFileSymmetric(
        in, false, out, [finish, channel](GFError err, DataObjectPtr data) {
          finish(AnalyseEncrypt<false>(channel, err, data, {}, false));
        });
    return;
  }

  const auto selected = SelectEncryptKeys(channel, recipients, signer);
  if (const auto* missing = std::get_if<GpgOperaOutcome>(&selected)) {
    finish(*missing);
    return;
  }
  const auto keys = std::get<EncryptKeys>(selected);
  const auto signs = !keys.signers.isEmpty();
  auto analysed = [finish, channel, keys, signs](GFError err,
                                                 DataObjectPtr data) {
    finish(AnalyseEncrypt<false>(channel, err, data, keys.recipients, signs));
  };
  if (signs) {
    opera.EncryptSignFile(keys.recipients, keys.signers, in, false, out,
                          std::move(analysed));
  } else {
    opera.EncryptFile(keys.recipients, in, false, out, std::move(analysed));
  }
}

void DecryptStagedFile(const QString& in, const QString& out, bool verify,
                       QObject* context, OutcomeCallback done) {
  auto finish = Guarded(context, std::move(done));
  const auto channel = UsableDefaultChannelHandle();
  if (!verify) {
    FileCryptoOperation::GetInstance(channel).DecryptFile(
        in, out, [finish, channel](GFError err, DataObjectPtr data) {
          finish(AnalyseDecrypt<false>(channel, err, data));
        });
    return;
  }
  FileCryptoOperation::GetInstance(channel).DecryptVerifyFile(
      in, out, [finish, channel](GFError err, DataObjectPtr data) {
        finish(AnalyseDecryptVerify<false>(channel, err, data));
      });
}

void SignStagedFile(const QString& in, const QString& out,
                    const QString& signer, QObject* context,
                    OutcomeCallback done) {
  auto finish = Guarded(context, std::move(done));
  const auto channel = UsableDefaultChannelHandle();
  const auto signers = SelectKeys(channel, {signer});
  if (!signers.missing.isEmpty()) {
    finish(MissingKeysOutcome(GpgOperation::kSIGN, channel, signers.missing,
                              true));
    return;
  }
  // Binary, as the desktop signs files: a .sig beside the file.
  FileCryptoOperation::GetInstance(channel).SignFile(
      signers.keys, in, false, out,
      [finish, channel](GFError err, DataObjectPtr data) {
        finish(AnalyseOperaOutcome<GpgSignResult, GpgSignResultAnalyse, false>(
            channel, err, data));
      });
}

void VerifyStagedFile(const QString& data, const QString& signature,
                      QObject* context, OutcomeCallback done) {
  auto finish = Guarded(context, std::move(done));
  const auto channel = UsableDefaultChannelHandle();
  FileCryptoOperation::GetInstance(channel).VerifyFile(
      data, signature, [finish, channel](GFError err, DataObjectPtr result) {
        finish(
            AnalyseOperaOutcome<GpgVerifyResult, GpgVerifyResultAnalyse, false>(
                channel, err, result));
      });
}

}  // namespace GpgFrontend::Mobile
