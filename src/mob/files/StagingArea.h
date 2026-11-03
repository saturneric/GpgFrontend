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

#include <QLockFile>
#include <QString>
#include <QUrl>
#include <memory>

namespace GpgFrontend::Mobile {

/// What QFile opens for @p url: a picked document on Android is a
/// content:// URL, which QFile opens as it is; a local path everywhere else.
auto LocalOrContent(const QUrl& url) -> QString;

/// @p name as one path component: its last part, never "." or "..", and
/// "file" when nothing is left.
auto SafeFileName(const QString& name) -> QString;

/**
 * @brief This run's directory for the files an operation works on.
 *
 * A picked document is copied in before an operation reads it (a content://
 * URL is not a path the engine can open), and the operation's output is
 * written here until it is shared, saved or dropped. The directory is
 * <root>/<session>, held by a lock file inside it: removed when this object
 * goes, and, should the process die first, by RemoveOrphans() on the next
 * start, which takes only directories no live process holds.
 */
class StagingArea {
 public:
  explicit StagingArea(const QString& root);
  ~StagingArea();

  StagingArea(const StagingArea&) = delete;
  auto operator=(const StagingArea&) -> StagingArea& = delete;

  /// A new, unused path in the session for a file called @p name.
  auto NewPath(const QString& name) -> QString;

  [[nodiscard]] auto Directory() const -> QString;

  /// Remove the sessions under @p root that no live process holds.
  static void RemoveOrphans(const QString& root);

 private:
  QString dir_;
  std::unique_ptr<QLockFile> lock_;
  int next_ = 0;
};

/**
 * @brief A staged file that is deleted with this object, unless released.
 */
class StagedFile {
 public:
  StagedFile() = default;
  explicit StagedFile(QString path);
  ~StagedFile();

  StagedFile(StagedFile&& other) noexcept;
  auto operator=(StagedFile&& other) noexcept -> StagedFile&;
  StagedFile(const StagedFile&) = delete;
  auto operator=(const StagedFile&) -> StagedFile& = delete;

  [[nodiscard]] auto Path() const -> QString;
  [[nodiscard]] auto Empty() const -> bool;

  /// Keep the file; it is the caller's now.
  auto Release() -> QString;

  /// Delete the file now.
  void Reset();

 private:
  QString path_;
};

}  // namespace GpgFrontend::Mobile
