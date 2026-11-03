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

#include "model/KeyImportController.h"

#include <QFile>
#include <QPointer>

#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/DataObject.h"
#include "core/model/GpgImportInformation.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/IOUtils.h"
#include "model/KeyListController.h"

namespace GpgFrontend::Mobile {

KeyImportController::KeyImportController(QObject* parent) : QObject(parent) {}

auto KeyImportController::Busy() const -> bool { return busy_; }

auto KeyImportController::Result() const -> QVariantMap { return result_; }

void KeyImportController::importText(const QString& text) {
  const auto block = GFBuffer(text);
  run([block]() { return std::optional<GFBuffer>(block); });
}

void KeyImportController::importFile(const QUrl& url) {
  // A picked file is a content:// URI on Android, which QFile opens as is.
  const auto path = url.isLocalFile() ? url.toLocalFile() : url.toString();
  run([path]() -> std::optional<GFBuffer> {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;
    // Keys, not archives: anything this large is not a key file.
    if (file.size() > kMaxKeyFileBytes) return std::nullopt;
    auto bytes = file.read(kMaxKeyFileBytes + 1);
    if (bytes.size() > kMaxKeyFileBytes) return std::nullopt;
    GFBuffer block(bytes);
    bytes.fill('\0');
    return block;
  });
}

void KeyImportController::run(
    std::function<std::optional<GFBuffer>()> read) {
  if (busy_) return;
  busy_ = true;
  emit SignalBusyChanged();

  const auto channel = UsableDefaultChannelHandle();
  QPointer<KeyImportController> self(this);

  auto* task = new Thread::Task(
      [channel, read = std::move(read)](const DataObjectPtr& data) -> int {
        const auto block = read();
        if (!block) return -1;
        auto info =
            KeyImportExportOperation::GetInstance(channel).ImportKey(*block);
        data->Swap({info});
        return 0;
      },
      "mobile_key_import", TransferParams(),
      [self](int rtn, const DataObjectPtr& data) {
        if (self == nullptr) return;

        QVariantMap result;
        bool imported = false;
        if (rtn == 0 && data != nullptr &&
            data->Check<QSharedPointer<GpgImportInformation>>()) {
          const auto info =
              ExtractParams<QSharedPointer<GpgImportInformation>>(data, 0);
          if (info != nullptr) {
            QStringList fingerprints;
            for (const auto& key : info->imported_keys) {
              fingerprints << key.fpr;
            }
            result = {
                {QStringLiteral("considered"), info->considered},
                {QStringLiteral("imported"), info->imported},
                {QStringLiteral("unchanged"), info->unchanged},
                {QStringLiteral("notImported"), info->not_imported},
                {QStringLiteral("secretImported"), info->secret_imported},
                {QStringLiteral("fingerprints"), fingerprints},
            };
            imported = info->imported > 0 || info->secret_imported > 0;
          }
        }

        self->result_ = result;
        self->busy_ = false;
        emit self->SignalResultChanged();
        emit self->SignalBusyChanged();
        emit self->SignalFinished(imported);
      });

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(task);
}

}  // namespace GpgFrontend::Mobile
