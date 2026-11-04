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
#include <QVariantMap>
#include <cstdint>
#include <deque>

namespace GpgFrontend::Mobile {

/// Where an operation was asked for.
enum class OperationSurface : std::uint8_t { kTEXT, kFILES };

enum class OperationPhase : std::uint8_t {
  kQUEUED,
  kRUNNING,
  kWAITING_PASSPHRASE,  ///< running, and its passphrase prompt is on screen
  kDONE,
  kFAILED,
  kCANCELLED,
};

/// How an operation that ran ended, as both pages record it.
inline auto EndedPhase(bool cancelled, bool succeeded) -> OperationPhase {
  if (cancelled) return OperationPhase::kCANCELLED;
  return succeeded ? OperationPhase::kDONE : OperationPhase::kFAILED;
}

/// What an operation was asked to do, fixed when it was submitted: a retry
/// repeats this, never what is chosen by the time it is retried.
struct OperationParams {
  QString op;              ///< "encrypt", "decrypt", "sign" or "verify"
  QStringList recipients;  ///< fingerprints
  QString signer;          ///< a fingerprint; empty when not signing
  bool symmetric = false;  ///< encrypted to a passphrase
  /// A decryption also checks the signatures inside (Decrypt & Verify).
  bool verify = false;
};

/// What an operation works on.
struct OperationTarget {
  int file_id = -1;  ///< the Files page's id; -1 for text
  QString name;      ///< the file's name; empty for text
  /// Which content it was: for text a keyed digest (TextVersion()), for a
  /// file its size. Compared, never shown.
  QString version;
};

/**
 * @brief One operation, from its submission on.
 *
 * Only its course -- phase, reason, report, outputs, times -- changes after
 * it is submitted. It never holds what an operation reads or writes: no text,
 * no passphrase, only the report ResultPresenter makes and the ids of the
 * files it made.
 */
struct OperationRecord {
  int id = 0;
  int batch_id = 0;  ///< the first operation of its batch; its own id alone
  OperationSurface surface = OperationSurface::kTEXT;
  OperationParams params;
  QList<OperationTarget> targets;
  OperationPhase phase = OperationPhase::kQUEUED;
  /// Why it ended as it did when no engine said so: "background" (a prompt
  /// declined by going to the background), "stopped" (the queue was
  /// stopped), "removed" (its file was), "closed" (its page or the
  /// application was).
  QString reason;
  QVariantMap report;
  QList<int> outputs;  ///< the Files page's ids of the files it made
  /// The text it made, by TextVersion(); empty when it made none.
  QString output_version;
  QDateTime submitted;
  QDateTime started;
  QDateTime finished;

  /// Queued or running: not yet ended.
  [[nodiscard]] auto Active() const -> bool;
};

/// @p text's version: a digest keyed afresh in each process, so equal text
/// compares equal and the digest says nothing about the text to anyone else.
auto TextVersion(const QString& text) -> QString;

/// The phase as QML reads it: "queued", "running", "waiting_passphrase",
/// "done", "failed", "cancelled".
auto PhaseName(OperationPhase phase) -> QString;

/**
 * @brief The operations of this session, newest first: what the status
 * sheet lists and what its actions act on.
 *
 * Bounded: past the capacity the oldest ended operations go, a batch only
 * as a whole. A batch still going -- every operation of it, ended or not --
 * neither counts nor goes, so an active operation is never dropped. Nothing is
 * written anywhere; the log ends with the process.
 */
class OperationLog : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("provided by OperationQueue")

 public:
  enum Role : int {
    kIdRole = Qt::UserRole + 1,
    kBatchIdRole,
    kSurfaceRole,  ///< "text" or "files"
    kOperationRole,
    kPhaseRole,
    kReasonRole,
    kReportRole,
    kTargetsRole,  ///< [{fileId, name}]
    kRecipientsRole,
    kSignerRole,
    kSymmetricRole,
    kOutputsRole,
    kSubmittedRole,
    kFinishedRole,
  };
  Q_ENUM(Role)

  /// How many ended operations are kept.
  static constexpr int kDefaultCapacity = 50;

  explicit OperationLog(QObject* parent = nullptr);

  [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const
      -> int override;
  [[nodiscard]] auto data(const QModelIndex& index, int role) const
      -> QVariant override;
  [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;

  /// Add @p record, newest first.
  void Add(OperationRecord record);

  /// The operation @p id; null once it has gone.
  [[nodiscard]] auto Find(int id) const -> const OperationRecord*;

  void SetStarted(int id);
  void SetPhase(int id, OperationPhase phase);
  /// End @p id in @p phase (done, failed or cancelled).
  void SetFinished(int id, OperationPhase phase, const QString& reason,
                   const QVariantMap& report, const QList<int>& outputs);

  /// The text @p id made, by its version (TextVersion()).
  void SetOutputVersion(int id, const QString& version);

  /**
   * @brief Which text of @p id's @p text is: "input" when the operation
   * read it, "output" when it made it, empty when neither (it was edited
   * since, or is another text altogether) or there is no such operation.
   */
  [[nodiscard]] Q_INVOKABLE QString textRelation(int id,
                                                 const QString& text) const;

  void SetCapacity(int capacity);

  /// The ids of the operations of batch @p batch_id, oldest first.
  [[nodiscard]] auto IdsOfBatch(int batch_id) const -> QList<int>;

  /// The operation @p id's roles, by name; empty when there is none.
  [[nodiscard]] Q_INVOKABLE QVariantMap record(int id) const;

  /// The records of batch @p batch_id, oldest first, as record() has them.
  [[nodiscard]] Q_INVOKABLE QVariantList batch(int batch_id) const;

  /// The newest operation; with @p surface ("text" or "files"), the newest
  /// of that page's; 0 when there is none.
  [[nodiscard]] Q_INVOKABLE int latest(const QString& surface = {}) const;

  /// The newest operation on the Files page's file @p file_id, as the file
  /// it works on or the one a signature is checked over; 0 when none.
  [[nodiscard]] Q_INVOKABLE int latestOnFile(int file_id) const;

 private:
  std::deque<OperationRecord> records_;  ///< newest first
  int capacity_ = kDefaultCapacity;

  [[nodiscard]] auto row_of(int id) const -> int;
  void changed(int id, const QList<int>& roles);
  void evict();
};

}  // namespace GpgFrontend::Mobile
