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

#include <QVariantMap>

#include "core/function/result_analyse/GpgOpResultInfo.h"

namespace GpgFrontend::Mobile {

/**
 * @brief An operation's result as the result sheet reads it.
 *
 * {status, cancelled, operation, description, engine, report,
 *  signatures: [{uid, fingerprint, keyId, validity, good, verdict,
 *                signature, keyStatus, identityTrust, signTime, warnings}],
 *  recipients: [{uid, keyId, keyFound}],
 *  errors: [text]}
 *
 * status is the analysers' own: 1 success, 0 success with warnings, -1
 * failure; cancelled says the failure was a refused passphrase prompt (the
 * person, or going to the background). validity is GpgSigValidity as a word
 * ("fully_valid", "key_missing", ...); good says whether the signature can be
 * relied on.
 *
 * Three facts of a signature, kept apart as the result sheet shows them:
 * signature -- "valid", "invalid", "expired", "unchecked" (no key to check
 * it with), "rejected"; keyStatus -- "ok", "missing", "expired", "revoked",
 * "not_authorized", "issues"; identityTrust -- "not_assessed" where the
 * engine weighs no one's identity (rPGP), else "trusted" or "not_trusted".
 */
auto PresentResult(const GpgOpResultInfo& info) -> QVariantMap;

/// GpgSigValidity as the word PresentResult() uses.
auto ValidityName(GpgSigValidity validity) -> QString;

}  // namespace GpgFrontend::Mobile
