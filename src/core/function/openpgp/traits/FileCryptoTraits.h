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
 * @file FileCryptoTraits.h
 * @brief OpTraits specialisations for file and directory/archive crypto
 * operations.
 *
 * Wires file encrypt, decrypt, sign, verify, encrypt+sign, decrypt+verify op
 * tags and their directory/archive counterparts to GnuPG and rPGP
 * implementations.
 */

#include "core/function/gpg/GpgErrorMap.h"
#include "core/function/openpgp/helper/Op.h"

// Support Meta
#include "core/function/openpgp/support/FileCryptoOpSupport.h"

// Engine Impl
#include "core/function/gpg/FileCryptoOpera.h"
#include "core/function/rpgp/FileCryptoOpera.h"

namespace GpgFrontend {

// File Crypto Operations
GF_DEF_OP_IMPL_TRAITS(FileEncryptOpTag, &EncryptFileRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptFileGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptFileRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(FileEncryptSymmetricOpTag, &EncryptSymmetricFileRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptSymmetricFileGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptSymmetricFileRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(FileDecryptOpTag, &DecryptFileRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&DecryptFileGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &DecryptFileRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(FileSignOpTag, &SignFileRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&SignFileGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &SignFileRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(FileVerifyOpTag, &VerifyFileRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&VerifyFileGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &VerifyFileRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(FileEncryptSignOpTag, &EncryptSignFileRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptSignFileGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptSignFileRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(FileDecryptVerifyOpTag, &DecryptVerifyFileRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&DecryptVerifyFileGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &DecryptVerifyFileRpgpImpl});

// Directory/Archive Crypto Operations
GF_DEF_OP_IMPL_TRAITS(DirEncryptOpTag, &EncryptDirRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptDirGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptDirRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(DirEncryptSymmetricOpTag, &EncryptSymmetricDirRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptSymmetricDirGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptSymmetricDirRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(DirEncryptSignOpTag, &EncryptSignDirRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&EncryptSignDirGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &EncryptSignDirRpgpImpl});
GF_DEF_OP_IMPL_TRAITS(ArchiveDecryptOpTag, &DecryptArchiveRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&DecryptArchiveGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &DecryptArchiveRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(ArchiveDecryptVerifyOpTag, &DecryptVerifyArchiveRpgpImpl,
                      {OpenPGPEngine::kGNUPG,
                       &FromGnuPGImpl<&DecryptVerifyArchiveGnuPGImpl>::Call},
                      {OpenPGPEngine::kRPGP, &DecryptVerifyArchiveRpgpImpl});

}  // namespace GpgFrontend