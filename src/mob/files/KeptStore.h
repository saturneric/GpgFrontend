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
#include <QVariantMap>

namespace GpgFrontend::Mobile {

/// A file the workspace keeps between runs, as its index records it.
struct KeptRecord {
  /// Where it is, under the store's root.
  QString path;
  QString name;
  /// "made" (an operation's output) or "shared" (a copy of what another
  /// application shared).
  QString origin;
  QString operation;
  QString made_by;
  bool plaintext = false;
  QString saved_name;
  QDateTime added;
  QVariantMap result;
};

/**
 * @brief The files of the Files page that stay after the application ends:
 * what operations made and what other applications shared, in the open
 * profile's own folder.
 *
 * Each file is in a folder of its own (<root>/<id>/<name>, so a share sheet
 * shows its name), listed in <root>/index.json. What the index does not name
 * is not the store's and goes at the next Load(); a record whose file is gone
 * is dropped.
 */
class KeptStore {
 public:
  explicit KeptStore(QString root);

  [[nodiscard]] auto Root() const -> QString;

  /// A new, unused path in the store for a file called @p name.
  auto NewPath(const QString& name) -> QString;

  /// Move the file at @p from into the store as @p name: renamed where it
  /// can be, else copied and the original removed. Its new path, or empty
  /// (the file is then where it was).
  auto Adopt(const QString& from, const QString& name) -> QString;

  /// What the index lists, every file there; files and folders it does not
  /// list are removed.
  auto Load() -> QList<KeptRecord>;

  /// Write the index: exactly @p records.
  auto Save(const QList<KeptRecord>& records) -> bool;

  /// Remove everything the store holds.
  void Clear();

 private:
  QString root_;
};

}  // namespace GpgFrontend::Mobile
