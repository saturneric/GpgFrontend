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
#include "core/function/gpg/UserIdOpera.h"
#include "core/function/rpgp/UserIdOpera.h"

namespace GpgFrontend {

GF_DEF_OP_TRAITS(AddUserIdOpTag, "op_add_user_id", &AddUIDGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &AddUIDGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &AddUIDRpgpImpl});

GF_DEF_OP_TRAITS(SetPrimaryUserIdOpTag, "op_set_primary_user_id",
                 &SetPrimaryUIDGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &SetPrimaryUIDGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &SetPrimaryUIDRpgpImpl});

GF_DEF_OP_TRAITS(DeleteUserIdOpTag, "op_delete_user_id", &DeleteUIDGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &DeleteUIDGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &DeleteUIDRpgpImpl});

GF_DEF_OP_TRAITS(RevokeUserIdOpTag, "op_revoke_user_id", &RevokeUIDGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &RevokeUIDGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &RevokeUIDRpgpImpl});

}  // namespace GpgFrontend