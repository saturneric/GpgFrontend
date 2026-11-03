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
 * @file KeyGenerationTraits.h
 * @brief OpTraits specializations for key generation operations.
 *
 * Wires GenerateKey, GenerateSubKey, GenerateKeyWithSubKey, FilterKeyAlgoByKey
 * op tags to GnuPG and rPGP implementations.
 */

#if GF_ENGINE_GNUPG
#include "core/function/gpg/GpgErrorMap.h"
#endif
#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/KeyGenerationOpSupport.h"

// Engine Impl
#if GF_ENGINE_GNUPG
#include "core/function/gpg/KeyGenerate.h"
#endif
#include "core/function/rpgp/KeyGenerate.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(
    GenerateKeyTag, &GenerateKeyRpgpImpl,
    {OpenPGPEngine::kRPGP, &GenerateKeyRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&GenerateKeyGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    GenerateSubKeyTag, &GenerateSubKeyRpgpImpl,
    {OpenPGPEngine::kRPGP, &GenerateSubKeyRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&GenerateSubKeyGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    GenerateKeyWithSubKeyTag, &GenerateKeyWithSubkeyRpgpImpl,
    {OpenPGPEngine::kRPGP, &GenerateKeyWithSubkeyRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&GenerateKeyWithSubkeyGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(FilterKeyAlgoByKeyTag, &FilterKeyAlgoByKeyRpgpImpl,
                      {OpenPGPEngine::kRPGP, &FilterKeyAlgoByKeyRpgpImpl},
                      GF_GNUPG_IMPL(&FilterKeyAlgoByKeyGnuPGImpl));

}  // namespace GpgFrontend