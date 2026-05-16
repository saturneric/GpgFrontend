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
 * @file KeyManagementTraits.h
 * @brief OpTraits specializations for key management operations.
 *
 * Wires DeleteKeys, ModifyKeyPassphrase, SetExpire, GenerateRevCert,
 * RevokeSubKey, DeleteSubKey, AddADSK, SignKey, RevKeySignature,
 * SetOwnerTrustLevel op tags to their engine implementations.
 */

#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"

// Engine Impl
#include "core/function/gpg/KeyManagement.h"
#include "core/function/rpgp/KeyManagement.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(DeleteKeysOpTag, &DeleteKeysGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &DeleteKeysGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &DeleteKeysRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(ModifyKeyPassphraseOpTag, &ModifyKeyPassphraseGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &ModifyKeyPassphraseGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &ModifyKeyPassphraseRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(SetExpireOpTag, &SetExpireGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &SetExpireGnuPGImpl});

GF_DEF_OP_IMPL_TRAITS(GenerateRevCertOpTag, &GenerateRevCertGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &GenerateRevCertGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &GenerateRevCertRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(RevokeSubKeyOpTag, &RevokeSubKeyGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &RevokeSubKeyGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &RevokeSubKeyRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(DeleteSubKeyOpTag, &DeleteSubKeyGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &DeleteSubKeyGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &DeleteSubKeyRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(AddADSKOpTag, &AddADSKGnuPGIImpl,
                      {OpenPGPEngine::kGNUPG, &AddADSKGnuPGIImpl});

GF_DEF_OP_IMPL_TRAITS(SignKeyOpTag, &SignKeyGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &SignKeyGnuPGImpl});

GF_DEF_OP_IMPL_TRAITS(RevKeySignatureOpTag, &RevKeySignatureGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &RevKeySignatureGnuPGImpl});

GF_DEF_OP_IMPL_TRAITS(SetOwnerTrustLevelOpTag, &SetOwnerTrustLevelGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &SetOwnerTrustLevelGnuPGImpl});

}  // namespace GpgFrontend