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

/**
 * @file MessageCryptoTraits.h
 * @brief OpTraits specializations for message (in-memory) crypto operations.
 *
 * Wires Encrypt, EncryptSymmetric, Decrypt, Verify, Sign, EncryptSign,
 * DecryptVerify op tags to GnuPG and rPGP implementations.
 */

#include "core/function/gpg/GpgErrorMap.h"
#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/MessageCryptoOpSupport.h"

// Engine Impl
#include "core/function/gpg/MessageCryptoOperation.h"
#include "core/function/rpgp/MessageCryptoOperation.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(EncryptOpTag, &EncryptRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(EncryptSymmetricOpTag, &EncryptSymmetricRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptSymmetricGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptSymmetricRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(DecryptOpTag, &DecryptRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&DecryptGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &DecryptRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(VerifyOpTag, &VerifyRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&VerifyGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &VerifyRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(SignOpTag, &SignRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&SignGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &SignRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(EncryptSignOpTag, &EncryptSignRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptSignGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptSignRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(DecryptVerifyOpTag, &DecryptVerifyRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&DecryptVerifyGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &DecryptVerifyRpgpImpl});

}  // namespace GpgFrontend