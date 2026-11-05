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

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QMimeDatabase>
#include <QPointer>
#include <QRegularExpression>
#include <algorithm>

#include "crypto/ContentKind.h"
#include "crypto/OperationQueue.h"
#include "crypto/ResultPresenter.h"
#include "files/FileOperations.h"
#include "files/ShareRetentionStore.h"
#include "lifecycle/AppLifecycle.h"
#include "plat/common/PlatformFolder.h"
#include "plat/common/PlatformShare.h"
#include "shell/result_analyse/GpgOpResultInfo.h"
#include "shell/result_analyse/GpgOperaOutcome.h"
#include "shell/rules/FileOperationRules.h"
#include "shell/rules/FileOutputRules.h"

namespace GpgFrontend::Mobile {

namespace {

QPointer<FileWorkspace> g_instance;

/// The operations by name, in FileWorkspace::Op's order: what a record
/// stores and what a retry reads back.
auto OpNames() -> const QStringList& {
  static const QStringList kNames = {
      QStringLiteral("encrypt"), QStringLiteral("decrypt"),
      QStringLiteral("sign"), QStringLiteral("verify")};
  return kNames;
}

auto OpName(int op) -> QString { return OpNames().value(op); }

/// How large the document at @p url is, and whether it can be read at all.
auto Probe(const QUrl& url) -> std::pair<bool, qint64> {
  QFile file(LocalOrContent(url));
  if (!file.open(QIODevice::ReadOnly)) return {false, 0};
  return {true, file.size()};
}

}  // namespace

FileWorkspace::FileWorkspace(OperationQueue* queue, AppLifecycle* lifecycle,
                             QObject* parent)
    : QAbstractListModel(parent), queue_(queue) {
  // A document can go away while the application is in the background.
  if (lifecycle != nullptr) {
    connect(lifecycle, &AppLifecycle::SignalResumed, this,
            &FileWorkspace::refreshAvailability);
  }
  if (queue != nullptr) {
    connect(queue, &OperationQueue::SignalFinished, this,
            &FileWorkspace::dropped);
  }
}

// The outputs go with their entries (StagedFile); what still waits or runs
// goes with the workspace, which hears nothing more of it.
FileWorkspace::~FileWorkspace() {
  // A hash under way stops at its next chunk; the pool waits for it.
  cancelHash();
  hash_pool_.waitForDone();
  if (queue_ != nullptr) {
    disconnect(queue_, nullptr, this, nullptr);
    queue_->ForgetOwner(this);
  }
}

void FileWorkspace::SetInstance(FileWorkspace* instance) {
  g_instance = instance;
}

auto FileWorkspace::create(QQmlEngine*, QJSEngine*) -> FileWorkspace* {
  QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
  return g_instance;
}

auto FileWorkspace::Instance() -> FileWorkspace* { return g_instance; }

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
      {kOperationRole, "operation"},
      {kMadeByRole, "madeBy"},
      {kStateRole, "phase"},
      {kResultRole, "result"},
      {kSelectedRole, "chosen"},
      {kAvailableRole, "available"},
      {kSavedNameRole, "savedName"},
      {kOriginRole, "origin"},
      {kContentRole, "content"},
      {kProgressRole, "progress"},
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
      return entry.origin == QStringLiteral("made")
                 ? QStringLiteral("output")
                 : QStringLiteral("external");
    case kPlaintextRole:
      return entry.plaintext;
    case kSourceIdRole:
      return entry.source_id;
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
    case kSavedNameRole:
      return entry.saved_name;
    case kOriginRole:
      return entry.origin;
    case kContentRole:
      return entry.content;
    case kProgressRole:
      return entry.progress;
    default:
      return {};
  }
}

auto FileWorkspace::Busy() const -> bool {
  return running_.has_value() || !waiting_.empty();
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

auto FileWorkspace::addFiles(const QList<QUrl>& urls) -> QVariantList {
  QVariantList ids;
  for (const auto& url : urls) {
    if (url.isEmpty()) continue;
    const auto known =
        std::find_if(entries_.begin(), entries_.end(),
                     [&url](const auto& entry) { return entry->url == url; });
    if (known != entries_.end()) {
      // Picked again: looked at again, as it may have changed or come back
      // since (relink()); a file being worked on is left as it is.
      auto& entry = **known;
      ids << entry.id;
      if (!is_running(entry.id) && entry.state != QStringLiteral("queued")) {
        std::tie(entry.available, entry.size) = Probe(url);
        entry.content = ClassifyFile(entry.name, LocalOrContent(url));
        changed(entry.id, {kAvailableRole, kSizeRole, kContentRole});
      }
      continue;
    }

    auto entry = std::make_unique<Entry>();
    entry->id = next_id_++;
    ids << entry->id;
    entry->url = url;
    // Two files of one name from two folders are told apart, as shared
    // files and outputs are: a signature is never paired with the wrong one
    // by name, and a save never writes both into one document.
    entry->name = unique_name(DisplayName(url));
    std::tie(entry->available, entry->size) = Probe(url);
    // What it holds, by its name and its first bytes, as a file made here.
    entry->content = ClassifyFile(entry->name, LocalOrContent(url));
    entry->added = QDateTime::currentDateTimeUtc();
    insert(std::move(entry));
  }
  return ids;
}

auto FileWorkspace::receiveFiles(const QList<QUrl>& urls) -> QVariantList {
  QVariantList ids;
  auto* staging = FileStagingArea();
  if (staging == nullptr) return ids;
  QPointer<FileWorkspace> self(this);
  for (const auto& url : urls) {
    if (url.isEmpty()) continue;
    auto entry = std::make_unique<Entry>();
    entry->id = next_id_++;
    entry->origin = QStringLiteral("shared");
    entry->name = unique_name(DisplayName(url));
    entry->content = ClassifyFileName(entry->name);
    entry->available = false;
    entry->progress = 0;
    entry->file = StagedFile(staging->NewPath(entry->name));
    entry->added = QDateTime::currentDateTimeUtc();
    const auto id = entry->id;
    const auto name = entry->name;
    const auto path = entry->file.Path();
    ids << id;
    insert(std::move(entry));

    CopyOffThread(
        LocalOrContent(url), path, kMaxStagedInputSize, this,
        [self, id, name, path](bool copied) {
          auto* entry = self->find(id);
          if (entry == nullptr) return;
          // The copy is over either way.
          entry->progress = -1;
          if (!copied) {
            self->remove({id});
            emit self->SignalReceiveFailed(name);
            return;
          }
          entry->available = true;
          entry->size = QFileInfo(path).size();
          entry->content = ClassifyFile(entry->name, path);
          self->changed(
              id, {kAvailableRole, kProgressRole, kSizeRole, kContentRole});
        },
        [self, id](qint64 copied, qint64 total) {
          self->copying(id, copied, total);
        });
  }
  return ids;
}

auto FileWorkspace::allows(const QString& op, const QStringList& names) const
    -> bool {
  std::optional<Shell::FileOperation> kind;
  if (op == QStringLiteral("encrypt") || op == QStringLiteral("encrypt_sign") ||
      op == QStringLiteral("symmetric")) {
    kind = Shell::FileOperation::kEncrypt;
  } else if (op == QStringLiteral("sign")) {
    kind = Shell::FileOperation::kSign;
  } else if (op == QStringLiteral("decrypt") ||
             op == QStringLiteral("decrypt_verify")) {
    kind = Shell::FileOperation::kDecrypt;
  } else if (op == QStringLiteral("verify")) {
    kind = Shell::FileOperation::kVerify;
  }
  if (!kind) return false;

  QList<Shell::FileOperand> operands;
  for (const auto& name : names) operands.append({.name = name});
  return Shell::FileOperationAllowed(*kind, operands);
}

auto FileWorkspace::verifyMatch(const QString& name) const -> QVariantMap {
  const auto match = Shell::MatchVerifyFile(name);
  QString role;
  switch (match.role) {
    case Shell::VerifyFileRole::kCombined:
      role = QStringLiteral("combined");
      break;
    case Shell::VerifyFileRole::kSignature:
      role = QStringLiteral("signature");
      break;
    case Shell::VerifyFileRole::kData:
      role = QStringLiteral("data");
      break;
  }
  return {{QStringLiteral("role"), role},
          {QStringLiteral("partners"), match.partner_names}};
}

void FileWorkspace::remove(const QVariantList& ids) {
  const auto was = Busy();
  bool selection = false;
  for (const auto& value : ids) {
    const auto id = value.toInt();
    if (is_running(id)) continue;
    // Still being copied in: the copy would recreate its file after it was
    // removed, and nobody would remove that one.
    if (const auto* arriving = find(id);
        arriving != nullptr && arriving->progress >= 0) {
      continue;
    }

    // What waits for it waits no longer.
    for (auto it = waiting_.begin(); it != waiting_.end();) {
      if (it->second.id == id || it->second.other == id) {
        if (it->second.id != id) {
          if (auto* source = find(it->second.id); source != nullptr) {
            source->state = QStringLiteral("idle");
            changed(source->id, {kStateRole});
          }
        }
        const auto op_id = it->first;
        it = waiting_.erase(it);
        if (queue_ != nullptr) queue_->Cancel(op_id, QStringLiteral("removed"));
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
  emit SignalUnsavedChanged();
  if (selection) emit SignalSelectionChanged();
  busy_changed(was);
}

void FileWorkspace::clear() {
  QVariantList ids;
  for (const auto& entry : entries_) ids << entry->id;
  remove(ids);
}

auto FileWorkspace::encrypt(const QVariantList& ids,
                            const QStringList& recipients,
                            const QString& signer, bool symmetric) -> bool {
  if (!symmetric && recipients.isEmpty()) return false;
  Job job;
  job.op = Op::kEncrypt;
  job.symmetric = symmetric;
  if (!symmetric) {
    job.recipients = recipients;
    job.signer = signer;
  }
  return enqueue(ids, job) != 0;
}

auto FileWorkspace::decrypt(const QVariantList& ids, bool verify) -> bool {
  Job job;
  job.op = Op::kDecrypt;
  job.verify = verify;
  return enqueue(ids, job) != 0;
}

auto FileWorkspace::sign(const QVariantList& ids, const QString& signer)
    -> bool {
  Job job;
  job.op = Op::kSign;
  job.signer = signer;
  return enqueue(ids, job) != 0;
}

auto FileWorkspace::verify(int signature_id, int data_id) -> bool {
  if (find(signature_id) == nullptr) return false;
  // Detached: two files, each in the list.
  if (data_id >= 0 && (signature_id == data_id || find(data_id) == nullptr)) {
    return false;
  }
  Job job;
  job.op = Op::kVerify;
  job.other = data_id;
  return enqueue({signature_id}, job) != 0;
}

void FileWorkspace::cancelQueued() {
  const auto was = Busy();
  const auto waiting = std::exchange(waiting_, {});
  for (const auto& [op_id, job] : waiting) {
    if (auto* entry = find(job.id); entry != nullptr) {
      entry->state = QStringLiteral("cancelled");
      entry->result = {{QStringLiteral("cancelled"), true}};
      changed(job.id, {kStateRole, kResultRole});
    }
    if (queue_ != nullptr) queue_->Cancel(op_id, QStringLiteral("stopped"));
  }
  busy_changed(was);
}

auto FileWorkspace::retry(int op_id) -> QString {
  int batch = 0;
  return retry_into(op_id, batch);
}

auto FileWorkspace::retry_into(int op_id, int& batch) -> QString {
  const auto* record = queue_ != nullptr ? queue_->Log()->Find(op_id) : nullptr;
  if (record == nullptr || record->surface != OperationSurface::kFILES ||
      record->targets.isEmpty()) {
    return QStringLiteral("unknown");
  }
  const auto op = OpNames().indexOf(record->params.op);
  if (op < 0) return QStringLiteral("unknown");

  const auto id = record->targets.first().file_id;
  auto* entry = find(id);
  if (entry == nullptr) return QStringLiteral("missing");
  Job job;
  job.op = static_cast<Op>(op);
  job.recipients = record->params.recipients;
  job.signer = record->params.signer;
  job.symmetric = record->params.symmetric;
  job.verify = record->params.verify;
  if (job.op == Op::kVerify && record->targets.size() >= 2) {
    job.other = record->targets.at(1).file_id;
    const auto* data = find(job.other);
    if (data == nullptr) return QStringLiteral("missing");
    if (!data->available) return QStringLiteral("unavailable");
  }
  if (!entry->available) return QStringLiteral("unavailable");
  if (entry->state == QStringLiteral("queued") || is_running(id)) {
    return QStringLiteral("busy");
  }
  const auto into = enqueue({id}, job, batch);
  if (into == 0) return QStringLiteral("busy");
  batch = into;
  return {};
}

auto FileWorkspace::retryFailed(int batch_id) -> int {
  if (queue_ == nullptr) return 0;
  int queued = 0;
  // One batch again, shown as one, as the first time.
  int batch = 0;
  for (const auto op_id : queue_->Log()->IdsOfBatch(batch_id)) {
    const auto* record = queue_->Log()->Find(op_id);
    if (record == nullptr || (record->phase != OperationPhase::kFAILED &&
                              record->phase != OperationPhase::kCANCELLED)) {
      continue;
    }
    if (retry_into(op_id, batch).isEmpty()) ++queued;
  }
  return queued;
}

auto FileWorkspace::relink(int id, const QUrl& url) -> bool {
  auto* entry = find(id);
  if (entry == nullptr || entry->url.isEmpty() || url.isEmpty() ||
      is_running(id)) {
    return false;
  }
  entry->url = url;
  // Its own name is not taken from it again by the file itself.
  entry->name.clear();
  entry->name = unique_name(DisplayName(url));
  std::tie(entry->available, entry->size) = Probe(url);
  entry->content = ClassifyFile(entry->name, LocalOrContent(url));
  changed(id, {kNameRole, kAvailableRole, kSizeRole, kContentRole});
  return true;
}

void FileWorkspace::dropped(int op_id) {
  const auto it = waiting_.find(op_id);
  if (it == waiting_.end()) return;
  const auto was = Busy();
  const auto job = it->second;
  waiting_.erase(it);
  if (auto* entry = find(job.id); entry != nullptr) {
    entry->state = QStringLiteral("cancelled");
    entry->result = {{QStringLiteral("cancelled"), true}};
    changed(job.id, {kStateRole, kResultRole});
  }
  busy_changed(was);
}

void FileWorkspace::saveAs(int id, const QUrl& url) {
  const auto* entry = find(id);
  // Not while it is still arriving from a share: half of it would be saved.
  if (entry == nullptr || is_running(id) || !entry->available ||
      entry->progress >= 0) {
    emit SignalSaved(id, false);
    return;
  }
  const auto from =
      entry->url.isEmpty() ? entry->file.Path() : LocalOrContent(entry->url);
  QPointer<FileWorkspace> self(this);
  const auto saved_name = DisplayName(url);
  CopyOffThread(from, LocalOrContent(url), std::numeric_limits<qint64>::max(),
                this, [self, id, saved_name](bool saved) {
                  // The name the picker gave it, which may not be the one
                  // offered: a folder that has one already numbers it.
                  if (auto* entry = saved ? self->find(id) : nullptr;
                      entry != nullptr) {
                    entry->saved_name = saved_name;
                    self->changed(id, {kSavedNameRole});
                    emit self->SignalUnsavedChanged();
                  }
                  emit self->SignalSaved(id, saved);
                });
}

void FileWorkspace::share(int id) { hand_off(id, false); }

void FileWorkspace::openWith(int id) { hand_off(id, true); }

auto FileWorkspace::takenIn(const QVariantList& ids, const QUrl& folder) const
    -> QStringList {
  const auto names = FolderDocumentNames(folder);
  QStringList taken;
  if (!names) return taken;
  for (const auto& value : ids) {
    const auto* entry = find(value.toInt());
    if (entry != nullptr && names->contains(entry->name)) taken << entry->name;
  }
  return taken;
}

void FileWorkspace::saveAll(const QVariantList& ids, const QUrl& folder,
                            const QString& taken) {
  const auto names = FolderDocumentNames(folder).value_or(QStringList());
  std::vector<Save> saves;
  int skipped = 0;
  int failed = 0;
  for (const auto& value : ids) {
    const auto* entry = find(value.toInt());
    if (entry == nullptr || !entry->available || is_running(entry->id) ||
        entry->progress >= 0) {
      ++failed;
      continue;
    }
    QUrl to;
    if (names.contains(entry->name) && taken == QStringLiteral("skip")) {
      ++skipped;
      continue;
    }
    if (names.contains(entry->name) && taken == QStringLiteral("replace")) {
      to = FindFolderDocument(folder, entry->name);
    }
    // As bytes: given a type, a provider may add the extension it prefers
    // for it ("notes.txt.gpg.pgp").
    if (to.isEmpty()) {
      to = CreateFolderDocument(folder, entry->name,
                                QStringLiteral("application/octet-stream"));
    }
    if (to.isEmpty()) {
      ++failed;
      continue;
    }
    saves.push_back({entry->id, to});
  }
  save_next(std::move(saves), 0, 0, skipped, failed);
}

void FileWorkspace::save_next(std::vector<Save> saves, size_t next, int saved,
                              int skipped, int failed) {
  if (next >= saves.size()) {
    emit SignalSavedAll(saved, skipped, failed);
    return;
  }
  const auto save = saves.at(next);
  const auto* entry = find(save.id);
  if (entry == nullptr) {
    save_next(std::move(saves), next + 1, saved, skipped, failed + 1);
    return;
  }
  const auto from =
      entry->url.isEmpty() ? entry->file.Path() : LocalOrContent(entry->url);
  QPointer<FileWorkspace> self(this);
  CopyOffThread(
      from, LocalOrContent(save.to), std::numeric_limits<qint64>::max(), this,
      [self, saves = std::move(saves), next, saved, skipped,
       failed](bool copied) mutable {
        const auto& save = saves.at(next);
        if (auto* entry = copied ? self->find(save.id) : nullptr;
            entry != nullptr) {
          entry->saved_name = DisplayName(save.to);
          self->changed(save.id, {kSavedNameRole});
          emit self->SignalUnsavedChanged();
        }
        self->save_next(std::move(saves), next + 1, saved + (copied ? 1 : 0),
                        skipped, failed + (copied ? 0 : 1));
      });
}

void FileWorkspace::hand_off(int id, bool open) {
  QPointer<FileWorkspace> self(this);
  const auto report = [self, open](int id, const QString& outcome) {
    if (self == nullptr) return;
    if (open) {
      emit self->SignalOpened(id, outcome);
    } else {
      emit self->SignalShared(id, outcome);
    }
  };
  if (!GpgFrontend::CanShare()) {
    report(id, QStringLiteral("unsupported"));
    return;
  }
  const auto* entry = find(id);
  auto* staging = FileStagingArea();
  if (entry == nullptr || !entry->url.isEmpty() || entry->file.Empty() ||
      !entry->available || entry->progress >= 0 || staging == nullptr ||
      SharedFileStore() == nullptr) {
    report(id, QStringLiteral("failed"));
    return;
  }

  // The store takes a copy, so the output stays here whatever becomes of
  // the share.
  const auto name = entry->name;
  const auto plaintext = entry->plaintext;
  const auto copy = staging->NewPath(name);
  CopyOffThread(
      entry->file.Path(), copy, std::numeric_limits<qint64>::max(), this,
      [report, id, name, plaintext, copy, open](bool copied) {
        auto* store = SharedFileStore();
        if (!copied || store == nullptr) {
          QFile::remove(copy);
          report(id, QStringLiteral("failed"));
          return;
        }

        // The next share is one of the store's chances to clean up.
        const auto now = QDateTime::currentDateTimeUtc();
        store->Cleanup(now);
        const auto retained = store->Retain(copy, name, plaintext, now);
        if (!retained) {
          QFile::remove(copy);
          report(id, QStringLiteral("failed"));
          return;
        }

        const auto mime =
            QMimeDatabase().mimeTypeForFile(retained->path).name();
        const auto hand = open ? OpenFile : ShareFile;
        hand(
            retained->path, mime, {},
            [report, id,
             retained_id = retained->id](const ShareResult& result) {
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
              report(id, ShareOutcomeName(result.outcome));
            });
      });
}

void FileWorkspace::refreshAvailability() {
  for (const auto& entry : entries_) {
    // Still being copied in: not there yet, and not gone either.
    if (entry->progress >= 0 && entry->origin == QStringLiteral("shared")) {
      continue;
    }
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

void FileWorkspace::selectAll() { select_every(true); }

void FileWorkspace::clearSelection() { select_every(false); }

void FileWorkspace::select(const QVariantList& ids) {
  QSet<int> chosen;
  for (const auto& id : ids) chosen.insert(id.toInt());
  select_every(true, &chosen);
}

void FileWorkspace::select_every(bool selected, const QSet<int>* only) {
  // One change for the lot: a page that reads the selection again on every
  // change would otherwise read it once per file, a thousand times over.
  int first = -1;
  int last = -1;
  for (int row = 0; row < rowCount(); ++row) {
    auto& entry = *entries_.at(row);
    if (entry.selected == selected) continue;
    if (only != nullptr && !only->contains(entry.id)) continue;
    entry.selected = selected;
    if (first < 0) first = row;
    last = row;
  }
  if (first < 0) return;
  emit dataChanged(index(first), index(last), {kSelectedRole});
  emit SignalSelectionChanged();
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

auto FileWorkspace::idAt(int row) const -> int {
  return row >= 0 && row < rowCount() ? entries_.at(row)->id : -1;
}

auto FileWorkspace::rowOf(int id) const -> int { return row_of(id); }

auto FileWorkspace::idNamed(const QString& name) const -> int {
  for (const auto& entry : entries_) {
    if (entry->name == name) return entry->id;
  }
  return -1;
}

auto FileWorkspace::idsNamed(const QString& name) const -> QVariantList {
  QVariantList ids;
  if (name.isEmpty()) return ids;
  if (const auto exact = idNamed(name); exact >= 0) ids << exact;
  // As unique_name() names a second file of the same name.
  const auto dot = name.lastIndexOf(QLatin1Char('.'));
  const auto base = dot > 0 ? name.left(dot) : name;
  const auto extension = dot > 0 ? name.mid(dot) : QString();
  const QRegularExpression apart(
      QStringLiteral("^%1 \\(\\d+\\)%2$")
          .arg(QRegularExpression::escape(base),
               QRegularExpression::escape(extension)));
  for (const auto& entry : entries_) {
    if (entry->name != name && apart.match(entry->name).hasMatch()) {
      ids << entry->id;
    }
  }
  return ids;
}

auto FileWorkspace::signatureOf(int id) const -> int {
  for (const auto& entry : entries_) {
    if (entry->made_by == QStringLiteral("sign") && entry->source_id == id) {
      return entry->id;
    }
  }
  return -1;
}

auto FileWorkspace::UnsavedCount() const -> int {
  return static_cast<int>(
      std::count_if(entries_.begin(), entries_.end(), [](const auto& entry) {
        return entry->origin == QStringLiteral("made") &&
               entry->saved_name.isEmpty();
      }));
}

void FileWorkspace::hash(int id) {
  const auto ticket = ++*hash_ticket_;
  const auto* entry = find(id);
  if (entry == nullptr) {
    emit hashReady(id, {}, QStringLiteral("missing"));
    return;
  }
  const auto path =
      entry->url.isEmpty() ? entry->file.Path() : LocalOrContent(entry->url);
  hash_pool_.setMaxThreadCount(1);
  QPointer<FileWorkspace> self(this);
  hash_pool_.start([self, id, path, ticket, current = hash_ticket_]() {
    constexpr qint64 kChunk = 1 << 20;
    QFile file(path);
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    QString sha256;
    QString error;
    if (!file.open(QIODevice::ReadOnly)) {
      error = QStringLiteral("unreadable");
    } else {
      while (!file.atEnd()) {
        // Asked again, or no longer wanted: nothing to report.
        if (current->load() != ticket) return;
        const auto chunk = file.read(kChunk);
        if (chunk.isEmpty() && file.error() != QFileDevice::NoError) {
          error = QStringLiteral("unreadable");
          break;
        }
        hasher.addData(chunk);
      }
      if (error.isEmpty())
        sha256 = QString::fromLatin1(hasher.result().toHex());
    }
    if (current->load() != ticket) return;
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [self, id, sha256, error, ticket, current]() {
          if (self == nullptr || current->load() != ticket) return;
          emit self->hashReady(id, sha256, error);
        },
        Qt::QueuedConnection);
  });
}

void FileWorkspace::cancelHash() { ++*hash_ticket_; }

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
  emit SignalUnsavedChanged();
}

auto FileWorkspace::is_running(int id) const -> bool {
  return running_.has_value() && (running_->id == id || running_->other == id);
}

auto FileWorkspace::unique_name(const QString& name) const -> QString {
  const auto taken = [this](const QString& candidate) {
    return std::any_of(
        entries_.begin(), entries_.end(),
        [&candidate](const auto& entry) { return entry->name == candidate; });
  };
  if (!taken(name)) return name;
  // Before the last extension, so the name still says what kind it is.
  const auto dot = name.lastIndexOf(QLatin1Char('.'));
  const auto base = dot > 0 ? name.left(dot) : name;
  const auto extension = dot > 0 ? name.mid(dot) : QString();
  for (int n = 2;; ++n) {
    const auto candidate =
        QStringLiteral("%1 (%2)%3").arg(base).arg(n).arg(extension);
    if (!taken(candidate)) return candidate;
  }
}

void FileWorkspace::busy_changed(bool was) {
  if (Busy() != was) emit SignalBusyChanged();
}

auto FileWorkspace::enqueue(const QVariantList& ids, Job job, int batch)
    -> int {
  auto* queue = queue_.data();
  if (queue == nullptr) return 0;
  const auto was = Busy();
  int into = 0;
  for (const auto& value : ids) {
    auto* entry = find(value.toInt());
    if (entry == nullptr || !entry->available ||
        entry->state == QStringLiteral("queued") || is_running(entry->id)) {
      continue;
    }
    job.id = entry->id;
    entry->state = QStringLiteral("queued");
    entry->operation = OpName(static_cast<int>(job.op));
    entry->result.clear();
    changed(entry->id, {kStateRole, kOperationRole, kResultRole});

    OperationQueue::Request request;
    request.owner = this;
    request.surface = OperationSurface::kFILES;
    request.params.op = entry->operation;
    request.params.recipients = job.recipients;
    request.params.signer = job.signer;
    request.params.symmetric = job.symmetric;
    request.params.verify = job.verify;
    request.targets = {
        OperationTarget{entry->id, entry->name, QString::number(entry->size)}};
    if (const auto* data = find(job.other); data != nullptr) {
      request.targets << OperationTarget{data->id, data->name,
                                         QString::number(data->size)};
    }
    request.batch_id = batch;
    request.run = [this, job](int op_id) mutable {
      job.op_id = op_id;
      start(job);
    };
    // It may start, and even end, before Submit() returns: only a job still
    // waiting is kept here.
    const auto op_id = queue->Submit(std::move(request));
    if (op_id == 0) {
      entry->state = QStringLiteral("cancelled");
      entry->result = {{QStringLiteral("cancelled"), true}};
      changed(entry->id, {kStateRole, kResultRole});
      continue;
    }
    if (batch == 0) batch = op_id;
    into = batch;
    if (const auto* record = queue->Log()->Find(op_id);
        record != nullptr && record->phase == OperationPhase::kQUEUED) {
      job.op_id = op_id;
      waiting_[op_id] = job;
    }
  }
  busy_changed(was);
  return into;
}

void FileWorkspace::start(Job next) {
  waiting_.erase(next.op_id);
  running_ = next;

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
    // An inline signed file is verified alone, as any other operation runs.
    if (job.op != Op::kVerify || job.other < 0) {
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
  QPointer<FileWorkspace> self(this);
  CopyOffThread(
      LocalOrContent(entry.url), path, kMaxStagedInputSize, this,
      [self, id = entry.id, path, done = std::move(done)](bool copied) {
        self->copying(id, -1, 0);
        done(copied, path);
      },
      [self, id = entry.id](qint64 copied, qint64 total) {
        self->copying(id, copied, total);
      });
}

void FileWorkspace::copying(int id, qint64 copied, qint64 total) {
  auto* entry = find(id);
  if (entry == nullptr) return;
  const auto progress =
      copied < 0 ? -1.0
      : total > 0
          ? std::clamp(static_cast<double>(copied) / static_cast<double>(total),
                       0.0, 1.0)
          : 0.0;
  if (progress == entry->progress) return;
  entry->progress = progress;
  changed(id, {kProgressRole});
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
      // Detached: the data, then its signature; inline: the file alone.
      if (job.other < 0) {
        VerifyStagedFile(in, {}, this, finished);
      } else {
        VerifyStagedFile(other, in, this, finished);
      }
      return;
  }

  job_output_ = StagedFile(FileStagingArea()->NewPath(job_output_name_));
  const auto out = job_output_.Path();
  switch (job.op) {
    case Op::kEncrypt:
      EncryptStagedFile(in, out, job.recipients, job.signer, this, finished);
      break;
    case Op::kDecrypt:
      DecryptStagedFile(in, out, job.verify, this, finished);
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
      running_.has_value() && running_->op != Op::kVerify,
      outcome.info.OutputReleasable());
}

void FileWorkspace::fail(const QString& reason) {
  // Shaped as every other result, as the core makes one for an operation that
  // never reached the engine; what went wrong is its summary.
  auto scope = GpgOperation::kNONE;
  if (running_.has_value()) {
    switch (running_->op) {
      case Op::kEncrypt:
        scope = running_->signer.isEmpty() ? GpgOperation::kENCRYPT
                                           : GpgOperation::kENCRYPT_SIGN;
        break;
      case Op::kDecrypt:
        scope = running_->verify ? GpgOperation::kDECRYPT_VERIFY
                                 : GpgOperation::kDECRYPT;
        break;
      case Op::kSign:
        scope = GpgOperation::kSIGN;
        break;
      case Op::kVerify:
        scope = GpgOperation::kVERIFY;
        break;
    }
  }
  auto info = MakeAbortedOpInfo(
      scope, GFError::Application(GFErrorCode::kGENERAL), QString());
  info.description = reason;
  end(PresentResult(info), false, false);
}

void FileWorkspace::end(const QVariantMap& result, bool produces,
                        bool releasable) {
  // The copies of the inputs are not needed once the operation has ended.
  job_input_.Reset();
  job_other_.Reset();
  if (!running_.has_value()) return;
  const auto job = *running_;
  running_.reset();

  const auto cancelled = result.value(QStringLiteral("cancelled")).toBool();
  const auto status = result.value(QStringLiteral("status"), -1).toInt();
  auto succeeded = !cancelled && status >= 0;
  // Kept by how making it went, as on the desktop: a file whose signature
  // fails still decrypted, and is offered with the failure said.
  const QFileInfo output(job_output_.Empty() ? QString() : job_output_.Path());
  const auto produced =
      produces && Shell::KeepsFileOutput({
                      .cancelled = cancelled,
                      .releasable = releasable,
                      .exists = output.exists(),
                      .size = output.exists() ? output.size() : 0,
                      .may_be_empty = job.op == Op::kDecrypt,
                  });
  if (produces && !produced) succeeded = false;

  auto* source = find(job.id);
  if (source != nullptr) {
    source->result = result;
    source->state = cancelled   ? QStringLiteral("cancelled")
                    : succeeded ? QStringLiteral("done")
                                : QStringLiteral("failed");
    changed(job.id, {kStateRole, kResultRole});
  }

  QList<int> output_ids;
  if (produced) {
    auto output = std::make_unique<Entry>();
    output->id = next_id_++;
    output_ids << output->id;
    output->name = unique_name(job_output_name_);
    output->origin = QStringLiteral("made");
    output->size = QFileInfo(job_output_.Path()).size();
    output->file = std::move(job_output_);
    output->content = ClassifyFile(output->name, output->file.Path());
    output->plaintext = job.op == Op::kDecrypt;
    output->source_id = job.id;
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

  const auto phase = EndedPhase(cancelled, succeeded);
  // Recorded before it is announced, so a page that reads the record on the
  // signal finds it ended.
  if (queue_ != nullptr) {
    queue_->Log()->SetFinished(job.op_id, phase, {}, result, output_ids);
  }
  emit SignalFinished(job.id, succeeded);

  // Last: the next operation in the queue starts here.
  if (queue_ != nullptr) {
    queue_->Complete(job.op_id, phase, result, output_ids);
  }
  if (!Busy()) emit SignalBusyChanged();
}

}  // namespace GpgFrontend::Mobile
