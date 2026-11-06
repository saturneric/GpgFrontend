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
#include <QPointer>
#include <QThreadPool>
#include <QUrl>
#include <QVariantMap>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "files/StagingArea.h"

namespace GpgFrontend {
struct GpgOperaOutcome;
}  // namespace GpgFrontend

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend::Mobile {

class AppLifecycle;
class OperationQueue;

/**
 * @brief The files the Files page works on: those the person added, and
 * what operations made of them.
 *
 * Names in the list are unique: an output named like a file already there
 * is called "notes.txt (2).gpg", the number before its last extension.
 *
 * An added file is a reference to the document the picker returned; it is
 * never copied for keeping, changed or deleted, and removing it from the
 * workspace only forgets it. What an operation makes is the application's
 * own: a file in this session's staging area, deleted when it is removed,
 * when the workspace goes, or -- after a crash -- by the next start's
 * orphan cleanup. Nothing about the workspace is written anywhere else: it
 * lasts as long as the session.
 *
 * Operations run one at a time, in their turn in the application's
 * OperationQueue (one record each; a batch shares its first one's id). Each
 * copies its input into the staging area first (a content:// URL is not a
 * path the engine can open) and deletes that copy as soon as it ends. Saving or
 * sharing an output copies it, and the output stays until it is removed: a
 * share sheet that was opened says nothing of whether the file reached anyone.
 */
class FileWorkspace : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)
  Q_PROPERTY(int count READ Count NOTIFY SignalCountChanged)
  Q_PROPERTY(int selectedCount READ SelectedCount NOTIFY SignalSelectionChanged)
  Q_PROPERTY(
      QString sortKey READ SortKey WRITE SetSortKey NOTIFY SignalSortChanged)
  Q_PROPERTY(bool canShare READ CanShare CONSTANT)
  Q_PROPERTY(int unsavedCount READ UnsavedCount NOTIFY SignalUnsavedChanged)

 public:
  enum Role : int {
    kIdRole = Qt::UserRole + 1,
    kNameRole,
    kSizeRole,
    /// "external" (a document the person added) or "output" (made here).
    kKindRole,
    kPlaintextRole,
    /// The file an output was made from: pairs a signature made here with
    /// the file it signs.
    kSourceIdRole,
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
    /// The name it was last saved under ("Save as"); empty while it was
    /// not. An output that was never saved goes with the session.
    kSavedNameRole,
    /// Where it came from: "added" (the picker), "made" (an operation here),
    /// "shared" (another application shared it with GpgFrontend: a copy).
    kOriginRole,
    /// What its name says it is, as the desktop tells (ClassifyFileName()):
    /// "message", "signature", or empty. Nothing is read.
    kContentRole,
    /// How far it is copied in, 0 to 1, while it is; -1 otherwise.
    kProgressRole,
  };
  Q_ENUM(Role)

  /// @p queue runs its operations; @p lifecycle may be null. No default
  /// constructor: QML would make a workspace of its own (see
  /// KeyListController), and the application has one, in AppServices.
  FileWorkspace(OperationQueue* queue, AppLifecycle* lifecycle,
                QObject* parent = nullptr);
  ~FileWorkspace() override;

  static void SetInstance(FileWorkspace* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> FileWorkspace*;
  /// The application's workspace; null before and after AppServices.
  static auto Instance() -> FileWorkspace*;

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
  /// twice. The ids of all of them, in order, the ones already here too.
  Q_INVOKABLE QVariantList addFiles(const QList<QUrl>& urls);

  /// The most files one folder adds.
  static constexpr int kMaxFolderFiles = 500;

  /**
   * @brief Add the files directly in @p folder (a picked folder: on Android
   * a document tree), as added files: read where they are, not copied.
   *
   * Folders in it are not gone into, only counted. Listed and looked at
   * away from the interface thread; folderAdded() says how it went. At most
   * kMaxFolderFiles; a file already here is not added twice.
   */
  Q_INVOKABLE void addFolder(const QUrl& folder);

  /**
   * @brief Add the documents another application shared, as copies.
   *
   * The permission to read a shared document ends with the share, so each
   * is copied into the staging area at once, and is the application's own
   * from then on: removed, it is deleted. Listed while it is copied, not
   * available until it is; one that cannot be copied is taken off again
   * and reported by SignalReceiveFailed. The new ids, in order.
   */
  Q_INVOKABLE QVariantList receiveFiles(const QList<QUrl>& urls);

  /// Forget the added files among @p ids and delete the outputs; one being
  /// worked on stays.
  Q_INVOKABLE void remove(const QVariantList& ids);

  /// Whether @p op ("encrypt", "encrypt_sign", "symmetric", "sign",
  /// "decrypt", "decrypt_verify", "verify") can be done to files called
  /// @p names, by the desktop's rule (Shell::FileOperationAllowed). Whether
  /// they are free to take one is the page's.
  [[nodiscard]] Q_INVOKABLE bool allows(const QString& op,
                                        const QStringList& names) const;

  /// What a file called @p name is for verifying, by the desktop's rule
  /// (Shell::MatchVerifyFile): {role: "combined", "signature" or "data",
  /// partners: the names its other half may have, likeliest first}.
  [[nodiscard]] Q_INVOKABLE QVariantMap verifyMatch(const QString& name) const;

  /// remove() everything.
  Q_INVOKABLE void clear();

  /// Encrypt each of @p ids to @p recipients (fingerprints), signed by
  /// @p signer when given; or, when @p symmetric, to a passphrase. With no
  /// recipients and not @p symmetric nothing is done, and false returned.
  Q_INVOKABLE bool encrypt(const QVariantList& ids,
                           const QStringList& recipients,
                           const QString& signer = {}, bool symmetric = false);

  /// Decrypt each of @p ids; with @p verify, checking any signature inside
  /// too (Decrypt & Verify), else decrypting alone, as the desktop's Decrypt.
  /// Whether any was queued: none is that can no longer be opened, or is
  /// waiting or running already.
  Q_INVOKABLE bool decrypt(const QVariantList& ids, bool verify = true);

  /// Make a detached signature of each of @p ids with @p signer; whether
  /// any was queued, as decrypt().
  Q_INVOKABLE bool sign(const QVariantList& ids, const QString& signer);

  /// Verify the detached signature @p signature_id over @p data_id; the
  /// report is the signature's. With @p data_id -1, @p signature_id holds
  /// the data and its signature in one (an inline or cleartext signed
  /// file), verified alone. Whether it was queued, as decrypt().
  Q_INVOKABLE bool verify(int signature_id, int data_id = -1);

  /// Drop what waits in the queue; the operation running goes on.
  Q_INVOKABLE void cancelQueued();

  /**
   * @brief Run the OperationQueue record @p op_id again, on the same file
   * and as it was asked then: never with the keys chosen now.
   *
   * Empty when it was queued; otherwise why not: "unknown" (no such
   * record, or not one of the Files page's), "missing" (its file, or the
   * one a signature is checked over, is no longer in the list),
   * "unavailable" (it can no longer be opened: relink() it), "busy" (it is
   * waiting or running already).
   */
  Q_INVOKABLE QString retry(int op_id);

  /// retry() every operation of batch @p batch_id that failed or was
  /// cancelled, all in one new batch; how many were queued.
  Q_INVOKABLE int retryFailed(int batch_id);

  /// Point the added file @p id at @p url instead (picked again, after it
  /// moved): the same entry, its history kept. False for an output.
  Q_INVOKABLE bool relink(int id, const QUrl& url);

  /// Write the file @p id to @p url (a document the save picker returned);
  /// reported by SignalSaved. The file stays here.
  Q_INVOKABLE void saveAs(int id, const QUrl& url);

  /// Hand the output @p id to another application; reported by
  /// SignalShared. The output stays here.
  Q_INVOKABLE void share(int id);

  /// Open the output @p id in another application the person picks, as
  /// share() hands it over; reported by SignalOpened.
  Q_INVOKABLE void openWith(int id);

  /**
   * @brief Work out the SHA-256 of the file @p id, off the interface thread;
   * hashReady() says it. Read from where the file is (an added document
   * from the place it was picked from), never copied for it.
   *
   * On a thread of its own, not the one that copies and saves: a large file
   * read for its hash held saves up behind it. One at a time; asking again,
   * or cancelHash(), stops the one under way, which reports nothing.
   */
  Q_INVOKABLE void hash(int id);

  /// Stop working out a hash: the sheet that asked for it is gone.
  Q_INVOKABLE void cancelHash();

  /// The names among @p ids' that the folder @p folder has already.
  [[nodiscard]] Q_INVOKABLE QStringList takenIn(const QVariantList& ids,
                                                const QUrl& folder) const;

  /**
   * @brief Save each of @p ids into the folder @p folder (a folder picker's),
   * one after another; reported by SignalSavedAll. The files stay here.
   *
   * A name the folder has already is, by @p taken: "keep_both" (the new one
   * numbered, as the folder numbers it), "replace" (written over) or
   * "skip". Each saved file is "Saved as" the name it got.
   */
  Q_INVOKABLE void saveAll(const QVariantList& ids, const QUrl& folder,
                           const QString& taken);

  /// Check again which added files can still be read.
  Q_INVOKABLE void refreshAvailability();

  Q_INVOKABLE void setSelected(int id, bool selected);
  Q_INVOKABLE void selectAll();
  /// Select the files @p ids, as one change: those a filtered list shows.
  Q_INVOKABLE void select(const QVariantList& ids);
  Q_INVOKABLE void clearSelection();
  [[nodiscard]] Q_INVOKABLE QVariantList selectedIds() const;

  /// The file @p id's roles, by name; empty when there is none.
  [[nodiscard]] Q_INVOKABLE QVariantMap entry(int id) const;

  /// The id of the file in row @p row; -1 past the end.
  [[nodiscard]] Q_INVOKABLE int idAt(int row) const;

  /// The row of the file @p id; -1 when it is not listed.
  [[nodiscard]] Q_INVOKABLE int rowOf(int id) const;

  /// The id of the file named @p name; -1 when there is none.
  [[nodiscard]] Q_INVOKABLE int idNamed(const QString& name) const;

  /// The ids of the files that may be the one called @p name: so named,
  /// first, then those named apart from it ("name (2).ext"), as two files
  /// of one name are listed. More than one: which is meant is not known.
  [[nodiscard]] Q_INVOKABLE QVariantList idsNamed(const QString& name) const;

  /// The output made here by signing the file @p id; -1 when none is listed.
  [[nodiscard]] Q_INVOKABLE int signatureOf(int id) const;

  /// How many files made here were never saved: they go with the session.
  [[nodiscard]] auto UnsavedCount() const -> int;

 signals:
  void SignalBusyChanged();
  void SignalCountChanged();
  void SignalSelectionChanged();
  void SignalSortChanged();
  void SignalUnsavedChanged();
  /// An operation on @p id ended; @p succeeded unless it failed or was
  /// cancelled.
  void SignalFinished(int id, bool succeeded);
  /// "handed_off", "dismissed", "failed" or "unsupported", as OutputActions.
  void SignalShared(int id, const QString& outcome);
  void SignalSaved(int id, bool succeeded);
  /// openWith() @p id: as SignalShared says.
  void SignalOpened(int id, const QString& outcome);
  /// saveAll() is done: how many were saved, skipped, could not be saved.
  void SignalSavedAll(int saved, int skipped, int failed);
  /// A shared document named @p name could not be copied in.
  void SignalReceiveFailed(const QString& name);
  /// hash() of @p id is known: @p sha256 in lowercase hex, or @p error
  /// ("missing", "unreadable") and an empty hash. A plain name, so a QML
  /// Connections can take it.
  void hashReady(int id, const QString& sha256, const QString& error);
  /// addFolder() is done: @p added files new to the list, @p folders not
  /// gone into, @p left files over the limit not added; @p failed when the
  /// folder could not be listed at all.
  void folderAdded(int added, int folders, int left, bool failed);

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
    QString operation;
    QString made_by;
    QString state = QStringLiteral("idle");
    QVariantMap result;
    QDateTime added;
    bool selected = false;
    bool available = true;
    QString saved_name;
    QString origin = QStringLiteral("added");
    QString content;
    double progress = -1;
  };

  struct Job {
    int id = 0;
    Op op = Op::kEncrypt;
    QStringList recipients;
    QString signer;
    bool symmetric = false;
    bool verify = false;  ///< a decryption checks signatures too
    int other = -1;       ///< the data a signature is verified over; -1: inline
    int op_id = 0;   ///< its OperationQueue record
  };

  QPointer<OperationQueue> queue_;
  std::vector<std::unique_ptr<Entry>> entries_;
  /// Waiting in the OperationQueue, by record id.
  std::map<int, Job> waiting_;
  std::optional<Job> running_;
  StagedFile job_input_;
  StagedFile job_other_;
  StagedFile job_output_;
  QString job_output_name_;
  QString sort_key_ = QStringLiteral("added");
  int next_id_ = 1;
  /// hash(): the one thread hashes run on, and the request that may report
  /// (a hash of an older one stops at its next chunk).
  QThreadPool hash_pool_;
  std::shared_ptr<std::atomic<int>> hash_ticket_ =
      std::make_shared<std::atomic<int>>(0);

  [[nodiscard]] auto find(int id) const -> Entry*;
  [[nodiscard]] auto row_of(int id) const -> int;
  /// Select or deselect every file (or those in @p only), as one change.
  void select_every(bool selected, const QSet<int>* only = nullptr);
  void changed(int id, const QList<int>& roles = {});
  void insert(std::unique_ptr<Entry> entry);
  [[nodiscard]] auto before(const Entry& a, const Entry& b) const -> bool;
  /// Queue @p job on each of @p ids, in the batch @p batch (0: a new one);
  /// the batch they went into, 0 when none was queued.
  auto enqueue(const QVariantList& ids, Job job, int batch = 0) -> int;
  /// retry() into the batch @p batch, which it sets when it was 0.
  auto retry_into(int op_id, int& batch) -> QString;
  void start(Job next);
  /// The queue ended @p op_id without running it (it was closed).
  void dropped(int op_id);
  void stage(const Entry& entry, StagedFile& into,
             std::function<void(bool, const QString&)> done);
  struct Save {
    int id = 0;
    QUrl to;
  };
  /// Hand the output @p id over: shared, or opened when @p open.
  void hand_off(int id, bool open);
  /// The next of @p saves, the ones before done; then SignalSavedAll.
  void save_next(std::vector<Save> saves, size_t next, int saved, int skipped,
                 int failed);
  /// Report @p copied of @p total bytes of the file @p id copied in.
  void copying(int id, qint64 copied, qint64 total);
  void run(const Job& job, const QString& in, const QString& other);
  void finish(const GpgOperaOutcome& outcome);
  void fail(const QString& reason);
  /// End the running job with @p result. @p produces: the operation writes
  /// an output; @p releasable: making it worked
  /// (GpgOpResultInfo::OutputReleasable()).
  void end(const QVariantMap& result, bool produces, bool releasable);
  [[nodiscard]] auto is_running(int id) const -> bool;
  /// @p name, or with " (2)", " (3)" ... before its last extension when the
  /// list already has a file of that name.
  [[nodiscard]] auto unique_name(const QString& name) const -> QString;
  void busy_changed(bool was);
};

}  // namespace GpgFrontend::Mobile
