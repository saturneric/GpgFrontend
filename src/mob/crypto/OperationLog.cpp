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

#include "crypto/OperationLog.h"

#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <algorithm>
#include <array>

namespace GpgFrontend::Mobile {

namespace {

auto SurfaceName(OperationSurface surface) -> QString {
  return surface == OperationSurface::kTEXT ? QStringLiteral("text")
                                            : QStringLiteral("files");
}

/// This process's key for TextVersion(), made once.
auto VersionKey() -> const QByteArray& {
  static const QByteArray kKey = []() {
    std::array<quint32, 8> words{};
    QRandomGenerator::system()->fillRange(words.data(),
                                          static_cast<qsizetype>(words.size()));
    return QByteArray(reinterpret_cast<const char*>(words.data()),
                      static_cast<qsizetype>(words.size() * sizeof(quint32)));
  }();
  return kKey;
}

}  // namespace

auto OperationRecord::Active() const -> bool {
  return phase == OperationPhase::kQUEUED ||
         phase == OperationPhase::kRUNNING ||
         phase == OperationPhase::kWAITING_PASSPHRASE;
}

auto TextVersion(const QString& text) -> QString {
  return QString::fromLatin1(
      QMessageAuthenticationCode::hash(text.toUtf8(), VersionKey(),
                                       QCryptographicHash::Sha256)
          .toHex());
}

auto PhaseName(OperationPhase phase) -> QString {
  switch (phase) {
    case OperationPhase::kQUEUED:
      return QStringLiteral("queued");
    case OperationPhase::kRUNNING:
      return QStringLiteral("running");
    case OperationPhase::kWAITING_PASSPHRASE:
      return QStringLiteral("waiting_passphrase");
    case OperationPhase::kDONE:
      return QStringLiteral("done");
    case OperationPhase::kFAILED:
      return QStringLiteral("failed");
    case OperationPhase::kCANCELLED:
      return QStringLiteral("cancelled");
  }
  return {};
}

OperationLog::OperationLog(QObject* parent) : QAbstractListModel(parent) {}

auto OperationLog::rowCount(const QModelIndex& parent) const -> int {
  return parent.isValid() ? 0 : static_cast<int>(records_.size());
}

auto OperationLog::roleNames() const -> QHash<int, QByteArray> {
  return {
      {kIdRole, "operationId"},        {kBatchIdRole, "batchId"},
      {kSurfaceRole, "surface"},       {kOperationRole, "operation"},
      {kPhaseRole, "phase"},           {kReasonRole, "reason"},
      {kReportRole, "report"},         {kTargetsRole, "targets"},
      {kRecipientsRole, "recipients"}, {kSignerRole, "signer"},
      {kSymmetricRole, "symmetric"},   {kOutputsRole, "outputs"},
      {kSubmittedRole, "submitted"},   {kFinishedRole, "finished"},
  };
}

auto OperationLog::data(const QModelIndex& index, int role) const -> QVariant {
  if (!index.isValid() || index.row() >= rowCount()) return {};
  const auto& record = records_.at(index.row());
  switch (role) {
    case kIdRole:
      return record.id;
    case kBatchIdRole:
      return record.batch_id;
    case kSurfaceRole:
      return SurfaceName(record.surface);
    case kOperationRole:
    case Qt::DisplayRole:
      return record.params.op;
    case kPhaseRole:
      return PhaseName(record.phase);
    case kReasonRole:
      return record.reason;
    case kReportRole:
      return record.report;
    case kTargetsRole: {
      QVariantList targets;
      for (const auto& target : record.targets) {
        targets << QVariantMap{{QStringLiteral("fileId"), target.file_id},
                               {QStringLiteral("name"), target.name}};
      }
      return targets;
    }
    case kRecipientsRole:
      return record.params.recipients;
    case kSignerRole:
      return record.params.signer;
    case kSymmetricRole:
      return record.params.symmetric;
    case kOutputsRole: {
      QVariantList outputs;
      for (const auto id : record.outputs) outputs << id;
      return outputs;
    }
    case kSubmittedRole:
      return record.submitted;
    case kFinishedRole:
      return record.finished;
    default:
      return {};
  }
}

void OperationLog::Add(OperationRecord record) {
  beginInsertRows({}, 0, 0);
  records_.push_front(std::move(record));
  endInsertRows();
  evict();
}

auto OperationLog::Find(int id) const -> const OperationRecord* {
  const auto row = row_of(id);
  return row < 0 ? nullptr : &records_.at(row);
}

void OperationLog::SetStarted(int id) {
  const auto row = row_of(id);
  if (row < 0) return;
  auto& record = records_.at(row);
  record.phase = OperationPhase::kRUNNING;
  record.started = QDateTime::currentDateTimeUtc();
  changed(id, {kPhaseRole});
}

void OperationLog::SetPhase(int id, OperationPhase phase) {
  const auto row = row_of(id);
  if (row < 0 || records_.at(row).phase == phase) return;
  records_.at(row).phase = phase;
  changed(id, {kPhaseRole});
}

void OperationLog::SetFinished(int id, OperationPhase phase,
                               const QString& reason, const QVariantMap& report,
                               const QList<int>& outputs) {
  const auto row = row_of(id);
  if (row < 0) return;
  auto& record = records_.at(row);
  record.phase = phase;
  record.reason = reason;
  record.report = report;
  record.outputs = outputs;
  record.finished = QDateTime::currentDateTimeUtc();
  changed(id,
          {kPhaseRole, kReasonRole, kReportRole, kOutputsRole, kFinishedRole});
  evict();
}

void OperationLog::SetOutputVersion(int id, const QString& version) {
  const auto row = row_of(id);
  if (row >= 0) records_.at(row).output_version = version;
}

auto OperationLog::textRelation(int id, const QString& text) const -> QString {
  const auto* record = Find(id);
  if (record == nullptr || record->surface != OperationSurface::kTEXT) {
    return {};
  }
  const auto version = TextVersion(text);
  if (!record->targets.isEmpty() &&
      record->targets.first().version == version) {
    return QStringLiteral("input");
  }
  if (!record->output_version.isEmpty() && record->output_version == version) {
    return QStringLiteral("output");
  }
  return {};
}

void OperationLog::SetCapacity(int capacity) {
  capacity_ = std::max(0, capacity);
  evict();
}

auto OperationLog::record(int id) const -> QVariantMap {
  const auto row = row_of(id);
  if (row < 0) return {};
  QVariantMap map;
  const auto at = index(row);
  const auto names = roleNames();
  for (auto it = names.begin(); it != names.end(); ++it) {
    map.insert(QString::fromLatin1(it.value()), data(at, it.key()));
  }
  return map;
}

auto OperationLog::IdsOfBatch(int batch_id) const -> QList<int> {
  QList<int> ids;
  for (auto it = records_.rbegin(); it != records_.rend(); ++it) {
    if (it->batch_id == batch_id) ids << it->id;
  }
  return ids;
}

auto OperationLog::batch(int batch_id) const -> QVariantList {
  QVariantList records;
  for (const auto id : IdsOfBatch(batch_id)) records << record(id);
  return records;
}

auto OperationLog::latest(const QString& surface) const -> int {
  for (const auto& record : records_) {
    if (surface.isEmpty() || SurfaceName(record.surface) == surface) {
      return record.id;
    }
  }
  return 0;
}

auto OperationLog::latestOnFile(int file_id) const -> int {
  for (const auto& record : records_) {
    const auto on = std::any_of(
        record.targets.begin(), record.targets.end(),
        [file_id](const OperationTarget& t) { return t.file_id == file_id; });
    if (on) return record.id;
  }
  return 0;
}

auto OperationLog::row_of(int id) const -> int {
  for (size_t row = 0; row < records_.size(); ++row) {
    if (records_[row].id == id) return static_cast<int>(row);
  }
  return -1;
}

void OperationLog::changed(int id, const QList<int>& roles) {
  const auto row = row_of(id);
  if (row < 0) return;
  emit dataChanged(index(row), index(row), roles);
}

void OperationLog::evict() {
  // A batch counts, and goes, only once all of it has ended.
  const auto over = [this](int batch) {
    return std::none_of(
        records_.begin(), records_.end(),
        [batch](const auto& r) { return r.batch_id == batch && r.Active(); });
  };
  while (true) {
    int counted = 0;
    int oldest = 0;  // the oldest batch that is over
    for (auto it = records_.rbegin(); it != records_.rend(); ++it) {
      if (!over(it->batch_id)) continue;
      ++counted;
      if (oldest == 0) oldest = it->batch_id;
    }
    if (counted <= capacity_ || oldest == 0) return;
    // The newest batch that is over stays, however large: it is the one
    // just finished, which its outputs and Retry still name.
    const auto newest =
        std::find_if(records_.begin(), records_.end(),
                     [&over](const auto& r) { return over(r.batch_id); });
    if (newest != records_.end() && newest->batch_id == oldest) return;
    for (int row = rowCount() - 1; row >= 0; --row) {
      if (records_.at(row).batch_id != oldest) continue;
      beginRemoveRows({}, row, row);
      records_.erase(records_.begin() + row);
      endRemoveRows();
    }
  }
}

}  // namespace GpgFrontend::Mobile
