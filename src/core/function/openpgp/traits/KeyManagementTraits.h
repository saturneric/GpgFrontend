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
#include "core/function/gpg/KeyManagement.h"
#include "core/function/rpgp/KeyManagement.h"

namespace GpgFrontend {

GF_DEF_OP_TRAITS(DeleteKeysOpTag, "op_delete_keys", &DeleteKeysGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &DeleteKeysGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &DeleteKeysRpgpImpl});

GF_DEF_OP_TRAITS(ModifyKeyPassphraseOpTag, "op_modify_key_passphrase",
                 &ModifyKeyPassphraseGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &ModifyKeyPassphraseGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &ModifyKeyPassphraseRpgpImpl});

GF_DEF_OP_TRAITS(SetExpireOpTag, "op_set_expire", &SetExpireGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &SetExpireGnuPGImpl});

GF_DEF_OP_TRAITS(GenerateRevCertOpTag, "op_generate_rev_cert",
                 &GenerateRevCertGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &GenerateRevCertGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &GenerateRevCertRpgpImpl});

GF_DEF_OP_TRAITS(AddADSKOpTag, "op_add_adsk", &AddADSKGnuPGIImpl,
                 {OpenPGPEngine::kGNUPG, &AddADSKGnuPGIImpl});

}  // namespace GpgFrontend