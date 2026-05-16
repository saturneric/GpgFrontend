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
 * @file KeyGenerationOpSupport.h
 * @brief Engine/version support tags for key generation operations.
 *
 * Defines op tags for: GenerateKey, GenerateSubKey, GenerateKeyWithSubKey,
 * FilterKeyAlgoByKey.
 */

#include "core/function/openpgp/helper/OpSupport.h"

namespace GpgFrontend {

GF_DEF_OP_SUPPORT_TRAITS(GenerateKeyTag, "op_generate_key",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(GenerateSubKeyTag, "op_generate_subkey",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(GenerateKeyWithSubKeyTag,
                         "op_generate_key_with_subkey",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

GF_DEF_OP_SUPPORT_TRAITS(FilterKeyAlgoByKeyTag, "op_filter_key_algo_by_key",
                         {OpenPGPEngine::kGNUPG, "2.2.0"},
                         {OpenPGPEngine::kRPGP, "0.1.0"});

}  // namespace GpgFrontend