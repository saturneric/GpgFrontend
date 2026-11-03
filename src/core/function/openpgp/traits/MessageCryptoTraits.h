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

#if GF_ENGINE_GNUPG
#include "core/function/gpg/GpgErrorMap.h"
#endif
#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/MessageCryptoOpSupport.h"

// Engine Impl
#if GF_ENGINE_GNUPG
#include "core/function/gpg/MessageCryptoOperation.h"
#endif
#include "core/function/rpgp/MessageCryptoOperation.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(EncryptOpTag, &EncryptRpgpImpl,
                      {OpenPGPEngine::kRPGP, &EncryptRpgpImpl},
                      GF_GNUPG_IMPL(&FromGnuPGImpl<&EncryptGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    EncryptSymmetricOpTag, &EncryptSymmetricRpgpImpl,
    {OpenPGPEngine::kRPGP, &EncryptSymmetricRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&EncryptSymmetricGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(DecryptOpTag, &DecryptRpgpImpl,
                      {OpenPGPEngine::kRPGP, &DecryptRpgpImpl},
                      GF_GNUPG_IMPL(&FromGnuPGImpl<&DecryptGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(VerifyOpTag, &VerifyRpgpImpl,
                      {OpenPGPEngine::kRPGP, &VerifyRpgpImpl},
                      GF_GNUPG_IMPL(&FromGnuPGImpl<&VerifyGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(SignOpTag, &SignRpgpImpl,
                      {OpenPGPEngine::kRPGP, &SignRpgpImpl},
                      GF_GNUPG_IMPL(&FromGnuPGImpl<&SignGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    EncryptSignOpTag, &EncryptSignRpgpImpl,
    {OpenPGPEngine::kRPGP, &EncryptSignRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&EncryptSignGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    DecryptVerifyOpTag, &DecryptVerifyRpgpImpl,
    {OpenPGPEngine::kRPGP, &DecryptVerifyRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&DecryptVerifyGnuPGImpl>::Call));

}  // namespace GpgFrontend