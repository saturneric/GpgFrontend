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
 * @file FileCryptoOpSupport.h
 * @brief Engine/version support tags for file and directory/archive crypto operations.
 *
 * File operations: FileEncrypt, FileEncryptSymmetric, FileDecrypt, FileSign,
 * FileVerify, FileEncryptSign, FileDecryptVerify.
 *
 * Directory/archive operations: DirEncrypt, DirEncryptSymmetric, DirEncryptSign,
 * ArchiveDecrypt, ArchiveDecryptVerify.
 */

#include "core/function/openpgp/helper/OpSupport.h"

namespace GpgFrontend {

// File Crypto Operations

GF_DEF_OP_SUPPORT_TRAITS(FileEncryptOpTag, "op_file_encrypt",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(FileEncryptSymmetricOpTag, "op_file_encrypt_symmetric",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(FileDecryptOpTag, "op_file_decrypt",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(FileSignOpTag, "op_file_sign",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(FileVerifyOpTag, "op_file_verify",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(FileEncryptSignOpTag, "op_file_encrypt_sign",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(FileDecryptVerifyOpTag, "op_file_decrypt_verify",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

// Directory/Archive Crypto Operations

GF_DEF_OP_SUPPORT_TRAITS(DirEncryptOpTag, "op_dir_encrypt",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(DirEncryptSymmetricOpTag, "op_dir_encrypt_symm",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(DirEncryptSignOpTag, "op_dir_encrypt_sign",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(ArchiveDecryptOpTag, "op_archive_decrypt",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(ArchiveDecryptVerifyOpTag, "op_archive_decrypt_verify",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

}  // namespace GpgFrontend