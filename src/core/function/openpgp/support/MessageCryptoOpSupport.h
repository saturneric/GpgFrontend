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
 * @file MessageCryptoOpSupport.h
 * @brief Engine/version support tags for message (in-memory) crypto operations.
 *
 * Defines op tags for: Encrypt, EncryptSymmetric, Decrypt, Verify, Sign,
 * EncryptSign, DecryptVerify.
 */

#include "core/function/openpgp/helper/OpSupport.h"

namespace GpgFrontend {

GF_DEF_OP_SUPPORT_TRAITS(EncryptOpTag, "op_encrypt",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(EncryptSymmetricOpTag, "op_encrypt_symmetric",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(DecryptOpTag, "op_decrypt",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(VerifyOpTag, "op_verify",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(SignOpTag, "op_sign", {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(EncryptSignOpTag, "op_encrypt_sign",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(DecryptVerifyOpTag, "op_decrypt_verify",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

}  // namespace GpgFrontend