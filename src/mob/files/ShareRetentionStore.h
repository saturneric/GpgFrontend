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

#include <QDateTime>
#include <QList>
#include <QString>
#include <chrono>
#include <functional>
#include <optional>

namespace GpgFrontend::Mobile {

/// One file handed to another application.
struct RetainedFile {
  QString id;    ///< also the name of its directory
  QString path;  ///< absolute, under the store's root
  bool plaintext = false;
  QDateTime handed_off;  ///< UTC
  QDateTime eligible;    ///< UTC; may be cleaned up from then on
  qint64 size = 0;
};

/**
 * @brief The files given to other applications, kept until they may go.
 *
 * Sharing a file only hands over permission to read it; the receiving
 * application may read it at once, later, or never, and nothing reports
 * when it is done. So a shared file is moved out of the operation that made
 * it into this store, which outlives every page and controller, and stays
 * readable for a while.
 *
 * Retention is a policy, not proof that the receiver has finished: a file
 * becomes eligible for cleanup a fixed time after it was handed off, and is
 * cleaned up at the next opportunity -- start, return to the foreground, or
 * the next share. Nothing runs while the process is stopped, so a file can
 * outlive its eligibility until the application runs again. The size and
 * count caps evict the oldest files first at the same opportunities, and
 * Clear() removes everything at once. Cleaning a file revokes the grants to
 * read it, then deletes it.
 *
 * Deleting a file does not erase it from flash storage; this limits how long
 * a copy stays reachable, not whether its bytes remain on the medium.
 *
 * The record of what is kept is a JSON file written atomically, so it
 * survives a restart; files nobody recorded and records with no file are
 * dropped when cleaning. Interface thread only.
 */
class ShareRetentionStore {
 public:
  struct Policy {
    std::chrono::seconds keep{std::chrono::hours(24)};
    qint64 max_bytes = 256LL * 1024 * 1024;
    int max_files = 32;
  };

  /// Called with each file's path before it is deleted.
  using Revoke = std::function<void(const QString& path)>;

  /**
   * @param root where the files are kept, one directory each
   * @param index_file where the record of them is kept
   */
  ShareRetentionStore(QString root, QString index_file, Policy policy,
                      Revoke revoke);

  /**
   * @brief Move @p staged here as @p name, recorded as handed off @p now.
   *
   * Applies the caps without evicting the new file.
   *
   * @return the record, or nothing when the file could not be moved
   */
  auto Retain(const QString& staged, const QString& name, bool plaintext,
              const QDateTime& now) -> std::optional<RetainedFile>;

  /**
   * @brief Take back the file recorded as @p id, moved to @p to: its share
   * never reached anyone (the sheet could not be shown), so it is the
   * operation's output again, to be saved instead.
   *
   * @return whether it was moved; when not, it stays recorded here
   */
  auto Withdraw(const QString& id, const QString& to) -> bool;

  /// Clean what is eligible at @p now, apply the caps, drop orphans.
  void Cleanup(const QDateTime& now);

  /// Clean every file now, eligible or not.
  void Clear();

  [[nodiscard]] auto Entries() const -> QList<RetainedFile>;
  [[nodiscard]] auto Root() const -> QString;

 private:
  QString root_;
  QString index_file_;
  Policy policy_;
  Revoke revoke_;
  QList<RetainedFile> entries_;

  void load();
  void save() const;
  void drop(const RetainedFile& entry);
  void apply_caps(const QString& keep_id);
  void drop_orphans();
  [[nodiscard]] auto inside_root(const QString& path) const -> bool;
};

}  // namespace GpgFrontend::Mobile
