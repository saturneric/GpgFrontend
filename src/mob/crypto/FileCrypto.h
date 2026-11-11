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
}  // namespace GpgFrontend

namespace GpgFrontend::Mobile {

/**
 * @file FileCrypto.h
 * @brief The four engine operations on a file in the staging area, as the
 * Text page's controller runs them on text (TextCryptoController): never on
 * a picked document, whose content:// URL is not a path the engine opens.
 * @p done is called on the interface thread, and not at all once @p context
 * is gone.
 */

using OutcomeCallback = std::function<void(const GpgOperaOutcome&)>;

/// Encrypt @p in to @p recipients (fingerprints), signed by @p signer when
/// given; to a passphrase with no recipients.
void EncryptStagedFile(const QString& in, const QString& out,
                       const QStringList& recipients, const QString& signer,
                       QObject* context, OutcomeCallback done);

/// Decrypt @p in; with @p verify, checking any signature inside too.
void DecryptStagedFile(const QString& in, const QString& out, bool verify,
                       QObject* context, OutcomeCallback done);

/// Sign @p in with @p signer, writing a detached signature to @p out.
void SignStagedFile(const QString& in, const QString& out,
                    const QString& signer, QObject* context,
                    OutcomeCallback done);

/// Verify the detached signature @p signature over @p data; with no
/// @p signature, @p data holds its signature inline (a signed or cleartext
/// signed file).
void VerifyStagedFile(const QString& data, const QString& signature,
                      QObject* context, OutcomeCallback done);

}  // namespace GpgFrontend::Mobile
