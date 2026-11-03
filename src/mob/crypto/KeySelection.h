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

#include "core/function/result_analyse/GpgOperaOutcome.h"
#include "core/typedef/GpgTypedef.h"

/**
 * @file KeySelection.h
 * @brief The keys a page chose, and the results every page reports alike.
 */

namespace GpgFrontend::Mobile {

/// The keys a page chose by fingerprint, and those no longer in the keyring.
struct KeySelection {
  GpgAbstractKeyPtrList keys;
  QStringList missing;
};

/**
 * @brief Look up @p fingerprints in @p channel's keyring.
 *
 * A key deleted or refreshed away between choosing it and using it is
 * reported, never dropped: a message encrypted to fewer people than chosen
 * looks exactly like one encrypted to all of them.
 */
auto SelectKeys(int channel, const QStringList& fingerprints) -> KeySelection;

/**
 * @brief The outcome of an operation refused because chosen keys are gone.
 *
 * @param op the operation that was asked for
 * @param secret whether the missing keys were to sign with
 */
auto MissingKeysOutcome(GpgOperation op, int channel,
                        const QStringList& missing, bool secret = false)
    -> GpgOperaOutcome;

/**
 * @brief Name the recipients of a successful encryption to @p keys; @p data
 * starts with its GpgEncryptResult.
 */
void AttachRecipients(GpgOperaOutcome& outcome, const DataObjectPtr& data,
                      const GpgAbstractKeyPtrList& keys);

/**
 * @brief A decryption with its verification, where a message that was never
 * signed reports its decryption only: it is not a verification that found
 * nothing to verify.
 *
 * Data object [decrypt result, verify result, output], or the two results
 * alone when @p kWithOutput is false.
 */
template <bool kWithOutput>
auto AnalyseDecryptVerify(int channel, const GFError& err, const DataObjectPtr& data)
    -> GpgOperaOutcome;

}  // namespace GpgFrontend::Mobile
