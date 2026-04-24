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

#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/MessageCryptoOpSupport.h"

// Engine Impl
#include "core/function/gpg/MessageCryptoOperation.h"
#include "core/function/rpgp/MessageCryptoOperation.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(EncryptOpTag, &EncryptGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &EncryptGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &EncryptRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(EncryptSymmetricOpTag, &EncryptSymmetricGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &EncryptSymmetricGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &EncryptSymmetricRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(DecryptOpTag, &DecryptGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &DecryptGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &DecryptRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(VerifyOpTag, &VerifyGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &VerifyGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &VerifyRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(SignOpTag, &SignGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &SignGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &SignRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(EncryptSignOpTag, &EncryptSignGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &EncryptSignGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &EncryptSignRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(DecryptVerifyOpTag, &DecryptVerifyGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &DecryptVerifyGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &DecryptVerifyRpgpImpl});

}  // namespace GpgFrontend