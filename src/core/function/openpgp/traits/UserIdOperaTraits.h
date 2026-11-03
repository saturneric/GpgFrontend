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
 * @file UserIdOperaTraits.h
 * @brief OpTraits specializations for user ID management operations.
 *
 * Wires AddUserId, SetPrimaryUserId, DeleteUserId, RevokeUserId op tags to
 * GnuPG and rPGP implementations.
 */

#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/UserIdOperaOpSupport.h"

// Engine Impl
#if GF_ENGINE_GNUPG
#include "core/function/gpg/UserIdOpera.h"
#endif
#include "core/function/rpgp/UserIdOpera.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(AddUserIdOpTag, &AddUIDRpgpImpl,
                      {OpenPGPEngine::kRPGP, &AddUIDRpgpImpl},
                      GF_GNUPG_IMPL(&AddUIDGnuPGImpl));

GF_DEF_OP_IMPL_TRAITS(SetPrimaryUserIdOpTag, &SetPrimaryUIDRpgpImpl,
                      {OpenPGPEngine::kRPGP, &SetPrimaryUIDRpgpImpl},
                      GF_GNUPG_IMPL(&SetPrimaryUIDGnuPGImpl));

GF_DEF_OP_IMPL_TRAITS(DeleteUserIdOpTag, &DeleteUIDRpgpImpl,
                      {OpenPGPEngine::kRPGP, &DeleteUIDRpgpImpl},
                      GF_GNUPG_IMPL(&DeleteUIDGnuPGImpl));

GF_DEF_OP_IMPL_TRAITS(RevokeUserIdOpTag, &RevokeUIDRpgpImpl,
                      {OpenPGPEngine::kRPGP, &RevokeUIDRpgpImpl},
                      GF_GNUPG_IMPL(&RevokeUIDGnuPGImpl));

}  // namespace GpgFrontend