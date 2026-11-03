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

#include "files/FileWorkspace.h"

#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QPointer>
#include <algorithm>

#include "core/function/result_analyse/GpgOperaOutcome.h"
#include "crypto/ResultPresenter.h"
#include "files/FileOperations.h"
#include "files/ShareRetentionStore.h"
#include "lifecycle/AppLifecycle.h"
#include "plat/common/PlatformShare.h"

namespace GpgFrontend::Mobile {

namespace {

auto OpName(int op) -> QString {
  static const QStringList kNames = {
      QStringLiteral("encrypt"), QStringLiteral("decrypt"),
      QStringLiteral("sign"), QStringLiteral("verify")};
  return kNames.value(op);
}

/// How large the document at @p url is, and whether it can be read at all.
auto Probe(const QUrl& url) -> std::pair<bool, qint64> {
  QFile file(LocalOrContent(url));
  if (!file.open(QIODevice::ReadOnly)) return {false, 0};
  return {true, file.size()};
}

auto Failure(const QString& reason) -> QVariantMap {
  return {
      {QStringLiteral("status"), -1},
      {QStringLiteral("cancelled"), false},
      {QStringLiteral("errors"), QStringList{reason}},
  };
}

}  // namespace

FileWorkspace::FileWorkspace(QObject* parent) : QAbstractListModel(parent) {
  // A document can go away while the application is in the background.
  if (auto* lifecycle = AppLifecycle::create(nullptr, nullptr);
      lifecycle != nullptr) {
    connect(lifecycle, &AppLifecycle::SignalResumed, this,
            &FileWorkspace::refreshAvailability);
  }
}

// The outputs go with their entries (StagedFile).
FileWorkspace::~FileWorkspace() = default;

auto FileWorkspace::rowCount(const QModelIndex& parent) const -> int {
  return parent.isValid() ? 0 : static_cast<int>(entries_.size());
}

auto FileWorkspace::roleNames() const -> QHash<int, QByteArray> {
  return {
      {kIdRole, "fileId"},
      {kNameRole, "name"},
      {kSizeRole, "size"},
      {kKindRole, "kind"},
      {kPlaintextRole, "plaintext"},
      {kSourceIdRole, "sourceId"},
      {kSourceNameRole, "sourceName"},
      {kOperationRole, "operation"},
      {kMadeByRole, "madeBy"},
      {kStateRole, "phase"},
      {kResultRole, "result"},
      {kSelectedRole, "chosen"},
      {kAvailableRole, "available"},
  };
}

auto FileWorkspace::data(const QModelIndex& index, int role) const -> QVariant {
  if (!index.isValid() || index.row() >= rowCount()) return {};
  const auto& entry = *entries_.at(index.row());
  switch (role) {
    case kIdRole:
      return entry.id;
    case kNameRole:
    case Qt::DisplayRole:
      return entry.name;
    case kSizeRole:
      return entry.size;
    case kKindRole:
      return entry.url.isEmpty() ? QStringLiteral("output")
                                 : QStringLiteral("external");
    case kPlaintextRole:
      return entry.plaintext;
    case kSourceIdRole:
      return entry.source_id;
    case kSourceNameRole:
      return entry.source_name;
    case kOperationRole:
      return entry.operation;
    case kMadeByRole:
      return entry.made_by;
    case kStateRole:
      return entry.state;
    case kResultRole:
      return entry.result;
    case kSelectedRole:
      return entry.selected;
    case kAvailableRole:
      return entry.available;
    default:
      return {};
  }
}

auto FileWorkspace::Busy() const -> bool {
  return running_.has_value() || !queue_.empty();
}

auto FileWorkspace::Count() const -> int { return rowCount(); }

auto FileWorkspace::SelectedCount() const -> int {
  return static_cast<int>(
      std::count_if(entries_.begin(), entries_.end(),
                    [](const auto& entry) { return entry->selected; }));
}

auto FileWorkspace::SortKey() const -> QString { return sort_key_; }

void FileWorkspace::SetSortKey(const QString& key) {
  if (key == sort_key_ || !QStringList{"added", "name", "size"}.contains(key)) {
    return;
  }
  sort_key_ = key;
  beginResetModel();
  std::stable_sort(
      entries_.begin(), entries_.end(),
      [this](const auto& a, const auto& b) { return before(*a, *b); });
  endResetModel();
  emit SignalSortChanged();
}

auto FileWorkspace::CanShare() const -> bool { return GpgFrontend::CanShare(); }

void FileWorkspace::addFiles(const QList<QUrl>& urls) {
  for (const auto& url : urls) {
    if (url.isEmpty()) continue;
    const auto known =
        std::any_of(entries_.begin(), entries_.end(),
                    [&url](const auto& entry) { return entry->url == url; });
    if (known) continue;

    auto entry = std::make_unique<Entry>();
    entry->id = next_id_++;
    entry->url = url;
    entry->name = DisplayName(url);
    std::tie(entry->available, entry->size) = Probe(url);
    entry->added = QDateTime::currentDateTimeUtc();
    insert(std::move(entry));
  }
}

void FileWorkspace::remove(const QVariantList& ids) {
  const auto was = Busy();
  bool selection = false;
  for (const auto& value : ids) {
    const auto id = value.toInt();
    if (is_running(id)) continue;

    // What waits for it waits no longer.
    for (auto it = queue_.begin(); it != queue_.end();) {
      if (it->id == id || it->other == id) {
        if (it->id != id) {
          if (auto* source = find(it->id); source != nullptr) {
            source->state = QStringLiteral("idle");
            changed(source->id, {kStateRole});
          }
        }
        it = queue_.erase(it);
      } else {
        ++it;
      }
    }

    const auto row = row_of(id);
    if (row < 0) continue;
    selection = selection || entries_.at(row)->selected;
    beginRemoveRows({}, row, row);
    // An output's file is deleted with it; an added document is only
    // forgotten.
    entries_.erase(entries_.begin() + row);
    endRemoveRows();
  }
  emit SignalCountChanged();
  if (selection) emit SignalSelectionChanged();
  busy_changed(was);
}

void FileWorkspace::clear() {
  QVariantList ids;
  for (const auto& entry : entries_) ids << entry->id;
  remove(ids);
}

void FileWorkspace::encrypt(const QVariantList& ids,
                            const QStringList& recipients,
                            const QString& signer) {
  Job job;
  job.op = Op::kEncrypt;
  job.recipients = recipients;
  job.signer = signer;
  enqueue(ids, job);
}

void FileWorkspace::decrypt(const QVariantList& ids) {
  Job job;
  job.op = Op::kDecrypt;
  enqueue(ids, job);
}

void FileWorkspace::sign(const QVariantList& ids, const QString& signer) {
  Job job;
  job.op = Op::kSign;
  job.signer = signer;
  enqueue(ids, job);
}

void FileWorkspace::verify(int signature_id, int data_id) {
  if (signature_id == data_id || find(data_id) == nullptr) return;
  Job job;
  job.op = Op::kVerify;
  job.other = data_id;
  enqueue({signature_id}, job);
}

void FileWorkspace::cancelQueued() {
  const auto was = Busy();
  for (const auto& job : queue_) {
    if (auto* entry = find(job.id); entry != nullptr) {
      entry->state = QStringLiteral("cancelled");
      entry->result = {{QStringLiteral("cancelled"), true}};
      changed(job.id, {kStateRole, kResultRole});
    }
  }
  queue_.clear();
  busy_changed(was);
}

void FileWorkspace::saveAs(int id, const QUrl& url) {
  const auto* entry = find(id);
  if (entry == nullptr || is_running(id)) {
    emit SignalSaved(id, false);
    return;
  }
  const auto from =
      entry->url.isEmpty() ? entry->file.Path() : LocalOrContent(entry->url);
  QPointer<FileWorkspace> self(this);
  CopyOffThread(from, LocalOrContent(url), std::numeric_limits<qint64>::max(),
                this,
                [self, id](bool saved) { emit self->SignalSaved(id, saved); });
}

void FileWorkspace::share(int id) {
  if (!GpgFrontend::CanShare()) {
    emit SignalShared(id, QStringLiteral("unsupported"));
    return;
  }
  const auto* entry = find(id);
  auto* staging = FileStagingArea();
  if (entry == nullptr || !entry->url.isEmpty() || entry->file.Empty() ||
      staging == nullptr || SharedFileStore() == nullptr) {
    emit SignalShared(id, QStringLiteral("failed"));
    return;
  }

  // The store takes a copy, so the output stays here whatever becomes of
  // the share.
  const auto name = entry->name;
  const auto plaintext = entry->plaintext;
  const auto copy = staging->NewPath(name);
  QPointer<FileWorkspace> self(this);
  CopyOffThread(
      entry->file.Path(), copy, std::numeric_limits<qint64>::max(), this,
      [self, id, name, plaintext, copy](bool copied) {
        auto* store = SharedFileStore();
        if (!copied || store == nullptr) {
          QFile::remove(copy);
          emit self->SignalShared(id, QStringLiteral("failed"));
          return;
        }

        // The next share is one of the store's chances to clean up.
        const auto now = QDateTime::currentDateTimeUtc();
        store->Cleanup(now);
        const auto retained = store->Retain(copy, name, plaintext, now);
        if (!retained) {
          QFile::remove(copy);
          emit self->SignalShared(id, QStringLiteral("failed"));
          return;
        }

        const auto mime =
            QMimeDatabase().mimeTypeForFile(retained->path).name();
        ShareFile(
            retained->path, mime, {},
            [self, id, retained_id = retained->id](const ShareResult& result) {
              // Shown to no one: nothing needs keeping for a receiver.
              if (result.outcome == ShareOutcome::kFailed ||
                  result.outcome == ShareOutcome::kUnsupported) {
                auto* store = SharedFileStore();
                auto* staging = FileStagingArea();
                if (store != nullptr && staging != nullptr) {
                  StagedFile back(
                      staging->NewPath(QStringLiteral("withdrawn")));
                  store->Withdraw(retained_id, back.Path());
                }
              }
              if (self != nullptr) {
                emit self->SignalShared(id, ShareOutcomeName(result.outcome));
              }
            });
      });
}

void FileWorkspace::refreshAvailability() {
  for (const auto& entry : entries_) {
    bool available = false;
    qint64 size = entry->size;
    if (entry->url.isEmpty()) {
      available = QFile::exists(entry->file.Path());
    } else {
      std::tie(available, size) = Probe(entry->url);
      if (!available) size = entry->size;
    }
    if (available != entry->available || size != entry->size) {
      entry->available = available;
      entry->size = size;
      changed(entry->id, {kAvailableRole, kSizeRole});
    }
  }
}

void FileWorkspace::setSelected(int id, bool selected) {
  auto* entry = find(id);
  if (entry == nullptr || entry->selected == selected) return;
  entry->selected = selected;
  changed(id, {kSelectedRole});
  emit SignalSelectionChanged();
}

void FileWorkspace::selectAll() {
  for (const auto& entry : entries_) setSelected(entry->id, true);
}

void FileWorkspace::clearSelection() {
  for (const auto& entry : entries_) setSelected(entry->id, false);
}

auto FileWorkspace::selectedIds() const -> QVariantList {
  QVariantList ids;
  for (const auto& entry : entries_) {
    if (entry->selected) ids << entry->id;
  }
  return ids;
}

auto FileWorkspace::entry(int id) const -> QVariantMap {
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

auto FileWorkspace::find(int id) const -> Entry* {
  const auto row = row_of(id);
  return row < 0 ? nullptr : entries_.at(row).get();
}

auto FileWorkspace::row_of(int id) const -> int {
  for (size_t row = 0; row < entries_.size(); ++row) {
    if (entries_[row]->id == id) return static_cast<int>(row);
  }
  return -1;
}

void FileWorkspace::changed(int id, const QList<int>& roles) {
  const auto row = row_of(id);
  if (row < 0) return;
  emit dataChanged(index(row), index(row), roles);
}

auto FileWorkspace::before(const Entry& a, const Entry& b) const -> bool {
  if (sort_key_ == QStringLiteral("name")) {
    const auto order = a.name.localeAwareCompare(b.name);
    if (order != 0) return order < 0;
  } else if (sort_key_ == QStringLiteral("size")) {
    if (a.size != b.size) return a.size > b.size;
  }
  // Newest first, also among equals.
  if (a.added != b.added) return a.added > b.added;
  return a.id > b.id;
}

void FileWorkspace::insert(std::unique_ptr<Entry> entry) {
  int row = 0;
  while (row < rowCount() && !before(*entry, *entries_.at(row))) ++row;
  beginInsertRows({}, row, row);
  entries_.insert(entries_.begin() + row, std::move(entry));
  endInsertRows();
  emit SignalCountChanged();
}

auto FileWorkspace::is_running(int id) const -> bool {
  return running_.has_value() && (running_->id == id || running_->other == id);
}

void FileWorkspace::busy_changed(bool was) {
  if (Busy() != was) emit SignalBusyChanged();
}

void FileWorkspace::enqueue(const QVariantList& ids, Job job) {
  const auto was = Busy();
  for (const auto& value : ids) {
    auto* entry = find(value.toInt());
    if (entry == nullptr || !entry->available ||
        entry->state == QStringLiteral("queued") || is_running(entry->id)) {
      continue;
    }
    job.id = entry->id;
    queue_.push_back(job);
    entry->state = QStringLiteral("queued");
    entry->operation = OpName(static_cast<int>(job.op));
    entry->result.clear();
    changed(entry->id, {kStateRole, kOperationRole, kResultRole});
  }
  busy_changed(was);
  start_next();
}

void FileWorkspace::start_next() {
  if (running_.has_value() || queue_.empty()) return;
  running_ = queue_.front();
  queue_.pop_front();

  auto* entry = find(running_->id);
  if (entry == nullptr || FileStagingArea() == nullptr) {
    fail(tr("The file could not be read."));
    return;
  }
  entry->state = QStringLiteral("running");
  changed(entry->id, {kStateRole});

  const auto job = *running_;
  QPointer<FileWorkspace> self(this);
  stage(*entry, job_input_, [self, job](bool staged, const QString& in) {
    if (self == nullptr) return;
    if (!staged) {
      self->fail(tr("The file could not be read."));
      return;
    }
    if (job.op != Op::kVerify) {
      self->run(job, in, {});
      return;
    }
    const auto* data = self->find(job.other);
    if (data == nullptr) {
      self->fail(tr("The signed file is no longer in the list."));
      return;
    }
    self->stage(*data, self->job_other_,
                [self, job, in](bool staged, const QString& other) {
                  if (self == nullptr) return;
                  if (!staged) {
                    self->fail(tr("The signed file could not be read."));
                    return;
                  }
                  self->run(job, in, other);
                });
  });
}

void FileWorkspace::stage(const Entry& entry, StagedFile& into,
                          std::function<void(bool, const QString&)> done) {
  // An output is already in the staging area, and stays put while it is
  // worked on.
  if (entry.url.isEmpty()) {
    done(!entry.file.Empty(), entry.file.Path());
    return;
  }
  into = StagedFile(FileStagingArea()->NewPath(entry.name));
  const auto path = into.Path();
  CopyOffThread(
      LocalOrContent(entry.url), path, kMaxStagedInputSize, this,
      [path, done = std::move(done)](bool copied) { done(copied, path); });
}

void FileWorkspace::run(const Job& job, const QString& in,
                        const QString& other) {
  const auto* entry = find(job.id);
  const auto name = entry != nullptr ? entry->name : QStringLiteral("file");
  const auto finished = [this](const GpgOperaOutcome& outcome) {
    finish(outcome);
  };

  switch (job.op) {
    case Op::kEncrypt:
      job_output_name_ = EncryptedName(name);
      break;
    case Op::kDecrypt:
      job_output_name_ = DecryptedName(name);
      break;
    case Op::kSign:
      job_output_name_ = SignatureName(name);
      break;
    case Op::kVerify:
      job_output_name_.clear();
      VerifyStagedFile(other, in, this, finished);
      return;
  }

  job_output_ = StagedFile(FileStagingArea()->NewPath(job_output_name_));
  const auto out = job_output_.Path();
  switch (job.op) {
    case Op::kEncrypt:
      EncryptStagedFile(in, out, job.recipients, job.signer, this, finished);
      break;
    case Op::kDecrypt:
      DecryptStagedFile(in, out, this, finished);
      break;
    case Op::kSign:
      SignStagedFile(in, out, job.signer, this, finished);
      break;
    case Op::kVerify:
      break;
  }
}

void FileWorkspace::finish(const GpgOperaOutcome& outcome) {
  end(PresentResult(outcome.info),
      running_.has_value() && running_->op != Op::kVerify);
}

void FileWorkspace::fail(const QString& reason) { end(Failure(reason), false); }

void FileWorkspace::end(const QVariantMap& result, bool produces) {
  // The copies of the inputs are not needed once the operation has ended.
  job_input_.Reset();
  job_other_.Reset();
  if (!running_.has_value()) return;
  const auto job = *running_;
  running_.reset();

  const auto cancelled = result.value(QStringLiteral("cancelled")).toBool();
  const auto status = result.value(QStringLiteral("status"), -1).toInt();
  auto succeeded = !cancelled && status >= 0;
  const auto produced = succeeded && produces && !job_output_.Empty() &&
                        QFileInfo(job_output_.Path()).size() > 0;
  if (produces && !produced) succeeded = false;

  auto* source = find(job.id);
  if (source != nullptr) {
    source->result = result;
    source->state = cancelled   ? QStringLiteral("cancelled")
                    : succeeded ? QStringLiteral("done")
                                : QStringLiteral("failed");
    changed(job.id, {kStateRole, kResultRole});
  }

  if (produced) {
    auto output = std::make_unique<Entry>();
    output->id = next_id_++;
    output->name = job_output_name_;
    output->size = QFileInfo(job_output_.Path()).size();
    output->file = std::move(job_output_);
    output->plaintext = job.op == Op::kDecrypt;
    output->source_id = job.id;
    output->source_name = source != nullptr ? source->name : QString();
    output->operation = OpName(static_cast<int>(job.op));
    output->made_by = output->operation;
    output->state = QStringLiteral("done");
    output->result = result;
    output->added = QDateTime::currentDateTimeUtc();
    insert(std::move(output));
  } else {
    job_output_.Reset();
  }
  job_output_name_.clear();

  emit SignalFinished(job.id, succeeded);
  if (!Busy()) {
    emit SignalBusyChanged();
  } else {
    start_next();
  }
}

}  // namespace GpgFrontend::Mobile
