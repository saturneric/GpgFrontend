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
struct GpgOperaOutcome;
enum class ShareOutcome;
}  // namespace GpgFrontend

namespace GpgFrontend::Mobile {

class StagingArea;
class ShareRetentionStore;

/**
 * @file FileOperations.h
 * @brief What the Files page does to one staged file, apart from which file
 * it is: copying in and out off the interface thread, the four engine
 * operations, and the names their outputs get.
 *
 * Every operation runs on paths in the staging area, never on a picked
 * document: a content:// URL is not a path the engine can open. @p done is
 * called on the interface thread, and not at all once @p context is gone.
 */

/// Where a staged copy may be at most this large.
constexpr qint64 kMaxStagedInputSize = 2LL * 1024 * 1024 * 1024;

using OutcomeCallback = std::function<void(const GpgOperaOutcome&)>;

/// The staging area and the store the Files page uses (AppServices).
void SetFileStores(StagingArea* staging, ShareRetentionStore* retention);

/// The staging area SetFileStores() gave; null before and after AppServices.
auto FileStagingArea() -> StagingArea*;

/// The store SetFileStores() gave; null before and after AppServices.
auto SharedFileStore() -> ShareRetentionStore*;

/// Copy @p from to @p to on the I/O runner, refusing more than @p limit
/// bytes; @p done gets whether it worked, on @p context's thread.
void CopyOffThread(const QString& from, const QString& to, qint64 limit,
                   QObject* context, std::function<void(bool)> done);

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

/// Encrypt @p in to @p recipients (fingerprints), signed by @p signer when
/// given; to a passphrase with no recipients.
void EncryptStagedFile(const QString& in, const QString& out,
                       const QStringList& recipients, const QString& signer,
                       QObject* context, OutcomeCallback done);

/// Decrypt @p in, verifying any signature inside.
void DecryptStagedFile(const QString& in, const QString& out, QObject* context,
                       OutcomeCallback done);

/// Sign @p in with @p signer, writing a detached signature to @p out.
void SignStagedFile(const QString& in, const QString& out,
                    const QString& signer, QObject* context,
                    OutcomeCallback done);

/// Verify the detached signature @p signature over @p data.
void VerifyStagedFile(const QString& data, const QString& signature,
                      QObject* context, OutcomeCallback done);

}  // namespace GpgFrontend::Mobile
