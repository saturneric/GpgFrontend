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

/// The names of the documents in @p folder; none when it cannot be listed.
auto FolderDocumentNames(const QUrl& folder) -> std::optional<QStringList>;

/// What is directly in a folder: its files, and how many folders beside
/// them were not gone into.
struct FolderContents {
  QList<QUrl> files;
  int folders = 0;
};

/// The files directly in @p folder, each a URL to open, and the folders
/// beside them counted; none when it cannot be listed.
auto FolderFiles(const QUrl& folder) -> std::optional<FolderContents>;

/// The document named @p name in @p folder; empty when there is none.
auto FindFolderDocument(const QUrl& folder, const QString& name) -> QUrl;

/// A new, empty document named @p name in @p folder; empty when it could
/// not be made. A name already taken is numbered, as the provider numbers
/// it ("notes (1).txt"), so the name may not be @p name.
auto CreateFolderDocument(const QUrl& folder, const QString& name,
                          const QString& mime_type) -> QUrl;

}  // namespace GpgFrontend
