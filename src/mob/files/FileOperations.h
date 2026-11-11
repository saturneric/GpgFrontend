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

#include <QObject>
#include <QStringList>
#include <functional>

namespace GpgFrontend {
enum class ShareOutcome;
}  // namespace GpgFrontend

namespace GpgFrontend::Mobile {

class StagingArea;
class ShareRetentionStore;

/**
 * @file FileOperations.h
 * @brief What the Files page does to one staged file, apart from which file
 * it is: copying in and out off the interface thread, and the names outputs
 * get. The four engine operations on such a file are crypto/FileCrypto.h.
 *
 * Every operation runs on paths in the staging area, never on a picked
 * document: a content:// URL is not a path the engine can open. @p done is
 * called on the interface thread, and not at all once @p context is gone.
 */

/// Where a staged copy may be at most this large.
constexpr qint64 kMaxStagedInputSize = 2LL * 1024 * 1024 * 1024;

/// The staging area and the store the Files page uses (AppServices).
void SetFileStores(StagingArea* staging, ShareRetentionStore* retention);

/// The staging area SetFileStores() gave; null before and after AppServices.
auto FileStagingArea() -> StagingArea*;

/// The store SetFileStores() gave; null before and after AppServices.
auto SharedFileStore() -> ShareRetentionStore*;

/// How far a copy is: bytes copied, of how many (0 when not known).
using CopyProgress = std::function<void(qint64 copied, qint64 total)>;

/**
 * @brief Copy @p from to @p to here, refusing more than @p limit bytes.
 *
 * Nothing that looks like the whole file is left by a copy that fails: a
 * local file that was at @p to stays as it was, a document (content://) is
 * deleted. @p progress hears each piece of a copy of more than one.
 */
auto CopyLimited(const QString& from, const QString& to, qint64 limit,
                 const CopyProgress& progress = {}) -> bool;

/**
 * @brief Write @p size bytes at @p data to @p to, as CopyLimited() writes:
 * whole, or not at all (a local file there stays as it was, a document is
 * deleted).
 */
auto WriteWhole(const char* data, qint64 size, const QString& to) -> bool;

/// Copy @p from to @p to on the I/O runner, refusing more than @p limit
/// bytes; @p done gets whether it worked, on @p context's thread. A copy of
/// more than one piece reports each piece to @p progress first, on the
/// interface thread, unless @p context has gone.
void CopyOffThread(const QString& from, const QString& to, qint64 limit,
                   QObject* context, std::function<void(bool)> done,
                   CopyProgress progress = {});

/**
 * @brief Save the file at @p from into @p folder as @p name, whole or not at
 * all, on the I/O runner.
 *
 * Always a new document, made only now, as bytes (given a type, a provider
 * may add the extension it prefers: "notes.txt.gpg.pgp"). A copy that fails
 * leaves nothing behind. With @p replace, the document already named so goes
 * only once the copy is whole, and the new one takes its name; should either
 * step fail, both stay, the new one numbered.
 *
 * @p done gets the saved document, or an empty URL, on @p context's thread.
 */
void SaveIntoFolder(const QString& from, const QUrl& folder,
                    const QString& name, bool replace, QObject* context,
                    std::function<void(const QUrl&)> done);

/// A file's SHA-256, or why there is none.
struct FileDigest {
  QString sha256;  ///< lowercase hex; empty when not worked out
  QString error;   ///< "unreadable", or empty
};

/**
 * @brief The SHA-256 of the file at @p path (a local path or a content
 * URI), read a piece at a time on the calling thread. @p wanted is asked
 * before each piece: once it says no, the reading stops and returns nothing
 * (neither a digest nor an error).
 */
auto Sha256OfFile(const QString& path, const std::function<bool()>& wanted)
    -> FileDigest;

/// The name of the file a picker returned, as the picker shows it.
auto DisplayName(const QUrl& url) -> QString;

/// The encrypted file's name: "notes.txt.gpg".
auto EncryptedName(const QString& name) -> QString;

/// The decrypted file's name: the encrypted one without its extension.
auto DecryptedName(const QString& name) -> QString;

/// The detached signature's name: "notes.txt.sig".
auto SignatureName(const QString& name) -> QString;

/// "handed_off", "dismissed", "failed" or "unsupported", as OutputActions.
auto ShareOutcomeName(ShareOutcome outcome) -> QString;

}  // namespace GpgFrontend::Mobile
