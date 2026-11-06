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

#include <QString>
#include <QStringList>
#include <QUrl>
#include <atomic>
#include <functional>
#include <optional>

#include "plat/common/PlatformFolder.h"

namespace GpgFrontend::Mobile {

/**
 * @file FolderArchive.h
 * @brief A picked folder as one TAR archive, and a TAR archive back into a
 * picked folder: what encrypting a folder and extracting a decrypted one do
 * besides the engine's work.
 *
 * Both run on the calling thread, which is never the interface's, and both
 * stream: no archive and no file is held in memory whole. A folder is packed
 * into a staged file, which is then encrypted as any file; an archive is
 * extracted into a staged directory first, checked entry by entry, and only
 * then copied into the destination, so a refused archive writes nothing
 * there and a copy that fails half way is taken back.
 */

/// How far packing or copying is: bytes done, of how many (0 when unknown).
using ArchiveProgress = std::function<void(qint64 done, qint64 total)>;

/// The path each of @p nodes has in an archive, in the same order: its names
/// made safe as path components (never empty, ".", ".." or with a '/', no
/// control characters) and told apart where two would be one ("a (2).txt",
/// also where they differ only by case); a folder's new name carries over to
/// what is in it. None when a path still cannot be an archive entry.
auto ArchivePaths(const QList<FolderNode>& nodes) -> std::optional<QStringList>;

/// What packing a folder came to.
struct FolderPackResult {
  bool ok = false;
  bool cancelled = false;
  /// Why it failed: the path in the folder of a file that could not be read,
  /// or empty for anything else (no room, too large).
  QString unreadable;
  /// The archive came to more than the limit.
  bool too_large = false;
};

/// Pack @p tree into the TAR archive @p to, refusing more than @p limit
/// bytes; @p stop ends it at the next piece. Every file is read through what
/// its URL opens (a content URI on Android); a file that cannot be read fails
/// the whole archive, never leaves it out. Empty folders are kept. @p to is
/// removed unless it is ok.
auto PackFolderTree(const FolderTree& tree, const QString& to, qint64 limit,
                    const std::atomic_bool& stop,
                    const ArchiveProgress& progress = {}) -> FolderPackResult;

/// Whether the file at @p path starts as a TAR archive does: a header block
/// whose checksum adds up. A first look, not a promise: extracting still
/// checks every entry.
auto LooksLikeTarArchive(const QString& path) -> bool;

/// What extracting an archive came to.
struct FolderExtractResult {
  bool ok = false;
  bool cancelled = false;
  /// The folder made in the destination, as named there.
  QString folder_name;
  /// Why nothing was extracted, in words for a person; empty when ok.
  QString reason;
  int files = 0;
};

/// Extract the TAR archive @p archive into a new folder named @p name (or
/// numbered, when taken) in the picked folder @p destination. The archive
/// is first extracted into @p staging_dir (a path that does not exist yet),
/// refusing links, devices, paths that leave it, names twice or differing
/// only by case, and more than @p limit bytes; only an archive that passes
/// is copied over, folders included, empty ones too. Anything that fails
/// after the destination folder was made deletes it again. @p staging_dir is
/// always removed.
auto ExtractArchiveToFolder(const QString& archive, const QUrl& destination,
                            const QString& name, const QString& staging_dir,
                            qint64 limit, const std::atomic_bool& stop,
                            const ArchiveProgress& progress = {})
    -> FolderExtractResult;

}  // namespace GpgFrontend::Mobile
