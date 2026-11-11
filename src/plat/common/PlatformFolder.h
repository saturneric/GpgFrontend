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

#include <QList>
#include <QStringList>
#include <QUrl>
#include <functional>
#include <optional>

/**
 * @file PlatformFolder.h
 * @brief Writing into a folder the person picked: on Android a document tree
 * (ACTION_OPEN_DOCUMENT_TREE), whose documents are made and found through
 * its provider; elsewhere a local directory.
 *
 * Each call takes the folder as the picker returned it. A document made here
 * is written by opening its URL (LocalOrContent()).
 */

namespace GpgFrontend {

/// What the person picked to read; none when the picker was left.
using DocumentsPicked = std::function<void(const QList<QUrl>&)>;

/**
 * @brief Show the system's picker for documents to read, one or
 * (@p multiple) several, and hand what was picked to @p done on the
 * application's thread.
 *
 * Android only, where it is ACTION_OPEN_DOCUMENT asked for directly: Qt
 * Quick's FileDialog drops, without a word, a pick from any provider but
 * the device's own storage (it checks that each picked file exists, and
 * knows only those providers' URLs), so a file from a cloud folder never
 * arrived. The documents stay readable while the application runs; nothing
 * is kept beyond that.
 *
 * @return false where there is no such picker (the desktop, whose file
 * dialog has no such limit) or it could not be shown; @p done is then
 * never called.
 */
auto PickDocuments(bool multiple, DocumentsPicked done) -> bool;

/// The names of the files in @p folder (not its folders); none when it
/// cannot be listed.
auto FolderDocumentNames(const QUrl& folder) -> std::optional<QStringList>;

/// One document in a folder's tree.
struct FolderNode {
  /// Where it is under the folder: its names joined by '/', as the provider
  /// gives them (a name is not checked here, and may be anything but '/').
  QString path;
  /// What opens it: a file to read, or a folder to go on in.
  QUrl url;
  bool folder = false;
  /// A file's size as the provider tells it; 0 for a folder or unknown.
  qint64 size = 0;
};

/// A folder and everything under it.
struct FolderTree {
  /// The folder's own name, as the picker shows it.
  QString name;
  /// Every folder and file under it, a folder always before what is in it.
  QList<FolderNode> nodes;
  /// The walk stopped at kMaxFolderTreeNodes or kMaxFolderTreeDepth: what
  /// is listed is not all.
  bool truncated = false;
};

/// How many documents a tree is walked to, at most.
constexpr int kMaxFolderTreeNodes = 100000;

/// How deep a tree is walked, at most.
constexpr int kMaxFolderTreeDepth = 64;

/// @p folder and everything under it, walked to kMaxFolderTreeNodes and
/// kMaxFolderTreeDepth; none when it cannot be listed (access lost, gone).
auto FolderTreeOf(const QUrl& folder) -> std::optional<FolderTree>;

/// A new, empty folder named @p name in @p folder (a picked folder, or one
/// made in it); empty when it could not be made. A name already taken is
/// numbered, as for CreateFolderDocument().
auto CreateFolderChild(const QUrl& folder, const QString& name) -> QUrl;

/// Delete the document or folder @p document (and all under it) that was
/// made in a picked folder; false when it could not be.
auto DeleteFolderDocument(const QUrl& document) -> bool;

/// Give the document @p document (made in a picked folder) the name
/// @p name; its new URL, or empty when it could not be renamed (a name
/// taken, or a provider that renames nothing).
auto RenameFolderDocument(const QUrl& document, const QString& name) -> QUrl;

/// The file named @p name in @p folder; empty when there is none (a folder
/// of that name is not one).
auto FindFolderDocument(const QUrl& folder, const QString& name) -> QUrl;

/// A new, empty document named @p name in @p folder; empty when it could
/// not be made. A name already taken is numbered, as the provider numbers
/// it ("notes (1).txt"), so the name may not be @p name.
auto CreateFolderDocument(const QUrl& folder, const QString& name,
                          const QString& mime_type) -> QUrl;

}  // namespace GpgFrontend
