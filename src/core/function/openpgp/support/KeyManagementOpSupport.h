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
 * @file KeyManagementOpSupport.h
 * @brief Engine/version support tags for key management operations.
 *
 * Defines op tags for: DeleteKeys, ModifyKeyPassphrase, SetExpire (GnuPG only),
 * GenerateRevCert, RevokeSubKey, DeleteSubKey, AddADSK (GnuPG >= 2.4.1),
 * SignKey (GnuPG only), RevKeySignature (GnuPG only), SetOwnerTrustLevel (GnuPG
 * only).
 */

#include "core/function/openpgp/helper/OpSupport.h"

namespace GpgFrontend {

GF_DEF_OP_SUPPORT_TRAITS(DeleteKeysOpTag, "op_delete_keys",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(ModifyKeyPassphraseOpTag, "op_modify_key_passphrase",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(SetExpireOpTag, "op_set_expire",
                         {OpenPGPEngine::kGNUPG, "2.2.0"});

GF_DEF_OP_SUPPORT_TRAITS(GenerateRevCertOpTag, "op_generate_rev_cert",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(RevokeSubKeyOpTag, "op_revoke_subkey",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(DeleteSubKeyOpTag, "op_delete_subkey",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(AddADSKOpTag, "op_add_adsk",
                         {OpenPGPEngine::kGNUPG, "2.4.1"});

GF_DEF_OP_SUPPORT_TRAITS(SignKeyOpTag, "op_sign_key",
                         {OpenPGPEngine::kGNUPG, "2.2.0"});

GF_DEF_OP_SUPPORT_TRAITS(RevKeySignatureOpTag, "op_rev_key_signature",
                         {OpenPGPEngine::kGNUPG, "2.2.0"});

GF_DEF_OP_SUPPORT_TRAITS(SetOwnerTrustLevelOpTag, "op_set_owner_trust_level",
                         {OpenPGPEngine::kGNUPG, "2.2.0"});

}  // namespace GpgFrontend