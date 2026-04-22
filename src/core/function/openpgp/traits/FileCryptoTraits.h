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

// Engine Impl
#include "core/function/gpg/FileCryptoOpera.h"
#include "core/function/rpgp/FileCryptoOpera.h"

namespace GpgFrontend {

// File Crypto Operations
GF_DEF_OP_TRAITS(FileEncryptOpTag, "op_file_encrypt", &EncryptFileGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &EncryptFileGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &EncryptFileRpgpImpl});

GF_DEF_OP_TRAITS(FileEncryptSymmetricOpTag, "op_file_encrypt_symmetric",
                 &EncryptSymmetricFileGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &EncryptSymmetricFileGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &EncryptSymmetricFileRpgpImpl});

GF_DEF_OP_TRAITS(FileDecryptOpTag, "op_file_decrypt", &DecryptFileGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &DecryptFileGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &DecryptFileRpgpImpl});

GF_DEF_OP_TRAITS(FileSignOpTag, "op_file_sign", &SignFileGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &SignFileGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &SignFileRpgpImpl});

GF_DEF_OP_TRAITS(FileVerifyOpTag, "op_file_verify", &VerifyFileGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &VerifyFileGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &VerifyFileRpgpImpl});

GF_DEF_OP_TRAITS(FileEncryptSignOpTag, "op_file_encrypt_sign",
                 &EncryptSignFileGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &EncryptSignFileGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &EncryptSignFileRpgpImpl});

GF_DEF_OP_TRAITS(FileDecryptVerifyOpTag, "op_file_decrypt_verify",
                 &DecryptVerifyFileGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &DecryptVerifyFileGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &DecryptVerifyFileRpgpImpl});

// Directory/Archive Crypto Operations
GF_DEF_OP_TRAITS(DirEncryptOpTag, "op_dir_encrypt", &EncryptDirGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &EncryptDirGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &EncryptDirRpgpImpl});

GF_DEF_OP_TRAITS(DirEncryptSymmetricOpTag, "op_dir_encrypt_symm",
                 &EncryptSymmetricDirGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &EncryptSymmetricDirGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &EncryptSymmetricDirRpgpImpl});

GF_DEF_OP_TRAITS(DirEncryptSignOpTag, "op_dir_encrypt_sign",
                 &EncryptSignDirGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &EncryptSignDirGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &EncryptSignDirRpgpImpl});
GF_DEF_OP_TRAITS(ArchiveDecryptOpTag, "op_archive_decrypt",
                 &DecryptArchiveGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &DecryptArchiveGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &DecryptArchiveRpgpImpl});

GF_DEF_OP_TRAITS(ArchiveDecryptVerifyOpTag, "op_archive_decrypt_verify",
                 &DecryptVerifyArchiveGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &DecryptVerifyArchiveGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &DecryptVerifyArchiveRpgpImpl});

}  // namespace GpgFrontend