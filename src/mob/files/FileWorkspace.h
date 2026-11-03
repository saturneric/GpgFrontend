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

#pragma once

#include <QtQml/qqmlregistration.h>

#include <QAbstractListModel>
#include <QDateTime>
#include <QUrl>
#include <QVariantMap>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "files/StagingArea.h"

namespace GpgFrontend {
struct GpgOperaOutcome;
}  // namespace GpgFrontend

namespace GpgFrontend::Mobile {

/**
 * @brief The files the Files page works on: those the person added, and
 * what operations made of them.
 *
 * An added file is a reference to the document the picker returned; it is
 * never copied for keeping, changed or deleted, and removing it from the
 * workspace only forgets it. What an operation makes is the application's
 * own: a file in this session's staging area, deleted when it is removed,
 * when the workspace goes, or -- after a crash -- by the next start's
 * orphan cleanup. Nothing about the workspace is written anywhere else: it
 * lasts as long as the session.
 *
 * Operations queue and run one at a time. Each copies its input into the
 * staging area first (a content:// URL is not a path the engine can open)
 * and deletes that copy as soon as it ends. Saving or sharing an output
 * copies it, and the output stays until it is removed: a share sheet that
 * was opened says nothing of whether the file reached anyone.
 */
class FileWorkspace : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)
  Q_PROPERTY(int count READ Count NOTIFY SignalCountChanged)
  Q_PROPERTY(int selectedCount READ SelectedCount NOTIFY SignalSelectionChanged)
  Q_PROPERTY(
      QString sortKey READ SortKey WRITE SetSortKey NOTIFY SignalSortChanged)
  Q_PROPERTY(bool canShare READ CanShare CONSTANT)

 public:
  enum Role : int {
    kIdRole = Qt::UserRole + 1,
    kNameRole,
    kSizeRole,
    /// "external" (a document the person added) or "output" (made here).
    kKindRole,
    kPlaintextRole,
    kSourceIdRole,
    kSourceNameRole,
    /// What was last done to the file: "encrypt", "decrypt", "sign",
    /// "verify"; empty when nothing was.
    kOperationRole,
    /// What made an output, as kOperationRole names it; empty for an added
    /// file. Kept apart: an output can be worked on in turn.
    kMadeByRole,
    /// "idle", "queued", "running", "done", "failed" or "cancelled"; named
    /// "phase", as "state" is every QML item's own.
    kStateRole,
    /// The last operation's report, as ResultPresenter gives it.
    kResultRole,
    kSelectedRole,
    /// Whether the file can still be read.
    kAvailableRole,
  };

  explicit FileWorkspace(QObject* parent = nullptr);
  ~FileWorkspace() override;

  [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const
      -> int override;
  [[nodiscard]] auto data(const QModelIndex& index, int role) const
      -> QVariant override;
  [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;

  [[nodiscard]] auto Busy() const -> bool;
  [[nodiscard]] auto Count() const -> int;
  [[nodiscard]] auto SelectedCount() const -> int;
  [[nodiscard]] auto SortKey() const -> QString;
  void SetSortKey(const QString& key);
  [[nodiscard]] auto CanShare() const -> bool;

  /// Add the documents a picker returned; one already here is not added
  /// twice.
  Q_INVOKABLE void addFiles(const QList<QUrl>& urls);

  /// Forget the added files among @p ids and delete the outputs; one being
  /// worked on stays.
  Q_INVOKABLE void remove(const QVariantList& ids);

  /// remove() everything.
  Q_INVOKABLE void clear();

  /// Encrypt each of @p ids to @p recipients (fingerprints), signed by
  /// @p signer when given; to a passphrase with no recipients.
  Q_INVOKABLE void encrypt(const QVariantList& ids,
                           const QStringList& recipients,
                           const QString& signer = {});

  /// Decrypt each of @p ids, verifying any signature inside.
  Q_INVOKABLE void decrypt(const QVariantList& ids);

  /// Make a detached signature of each of @p ids with @p signer.
  Q_INVOKABLE void sign(const QVariantList& ids, const QString& signer);

  /// Verify the detached signature @p signature_id over @p data_id; the
  /// report is the signature's.
  Q_INVOKABLE void verify(int signature_id, int data_id);

  /// Drop what waits in the queue; the operation running goes on.
  Q_INVOKABLE void cancelQueued();

  /// Write the file @p id to @p url (a document the save picker returned);
  /// reported by SignalSaved. The file stays here.
  Q_INVOKABLE void saveAs(int id, const QUrl& url);

  /// Hand the output @p id to another application; reported by
  /// SignalShared. The output stays here.
  Q_INVOKABLE void share(int id);

  /// Check again which added files can still be read.
  Q_INVOKABLE void refreshAvailability();

  Q_INVOKABLE void setSelected(int id, bool selected);
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void clearSelection();
  [[nodiscard]] Q_INVOKABLE QVariantList selectedIds() const;

  /// The file @p id's roles, by name; empty when there is none.
  [[nodiscard]] Q_INVOKABLE QVariantMap entry(int id) const;

 signals:
  void SignalBusyChanged();
  void SignalCountChanged();
  void SignalSelectionChanged();
  void SignalSortChanged();
  /// An operation on @p id ended; @p succeeded unless it failed or was
  /// cancelled.
  void SignalFinished(int id, bool succeeded);
  /// "handed_off", "dismissed", "failed" or "unsupported", as OutputActions.
  void SignalShared(int id, const QString& outcome);
  void SignalSaved(int id, bool succeeded);

 private:
  enum class Op { kEncrypt, kDecrypt, kSign, kVerify };

  struct Entry {
    int id = 0;
    QString name;
    qint64 size = 0;
    QUrl url;         ///< an added document; empty for an output
    StagedFile file;  ///< an output; empty for an added document
    bool plaintext = false;
    int source_id = -1;
    QString source_name;
    QString operation;
    QString made_by;
    QString state = QStringLiteral("idle");
    QVariantMap result;
    QDateTime added;
    bool selected = false;
    bool available = true;
  };

  struct Job {
    int id = 0;
    Op op = Op::kEncrypt;
    QStringList recipients;
    QString signer;
    int other = -1;  ///< the data a signature is verified over
  };

  std::vector<std::unique_ptr<Entry>> entries_;
  std::deque<Job> queue_;
  std::optional<Job> running_;
  StagedFile job_input_;
  StagedFile job_other_;
  StagedFile job_output_;
  QString job_output_name_;
  QString sort_key_ = QStringLiteral("added");
  int next_id_ = 1;

  [[nodiscard]] auto find(int id) const -> Entry*;
  [[nodiscard]] auto row_of(int id) const -> int;
  void changed(int id, const QList<int>& roles = {});
  void insert(std::unique_ptr<Entry> entry);
  [[nodiscard]] auto before(const Entry& a, const Entry& b) const -> bool;
  void enqueue(const QVariantList& ids, Job job);
  void start_next();
  void stage(const Entry& entry, StagedFile& into,
             std::function<void(bool, const QString&)> done);
  void run(const Job& job, const QString& in, const QString& other);
  void finish(const GpgOperaOutcome& outcome);
  void fail(const QString& reason);
  void end(const QVariantMap& result, bool produces);
  [[nodiscard]] auto is_running(int id) const -> bool;
  void busy_changed(bool was);
};

}  // namespace GpgFrontend::Mobile
