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

#include "core/function/openpgp/Op.h"

// Engine Impl
#include "core/function/gpg/KeyImportExport.h"
#include "core/function/rpgp/KeyImportExport.h"

namespace GpgFrontend {

GF_DEF_OP_TRAITS(ImportKeyOpTag, "op_import_key", &ImportKeyGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &ImportKeyGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &ImportKeyRpgpImpl});

GF_DEF_OP_TRAITS(ImportRevCertOpTag, "op_import_rev_cert", &ImportKeyGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &ImportKeyGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &ImportRevCertRpgpImpl});

GF_DEF_OP_TRAITS(ExportKeysOpTag, "op_export_key", &ExportKeysGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &ExportKeysGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &ExportKeysRpgpImpl});

GF_DEF_OP_TRAITS(ExportKeysAsyncOpTag, "op_export_keys_async",
                 &ExportKeysAsyncGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &ExportKeysAsyncGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &ExportKeysAsyncRpgpImpl});

GF_DEF_OP_TRAITS(ExportAllKeysOpTag, "op_export_all_keys",
                 &ExportAllKeysGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &ExportAllKeysGnuPGImpl},
                 {OpenPGPEngine::kRPGP, &ExportAllKeysRpgpImpl});

GF_DEF_OP_TRAITS(ExportSubkeyOpTag, "op_export_subkey", &ExportSubkeyGnuPGImpl,
                 {OpenPGPEngine::kGNUPG, &ExportSubkeyGnuPGImpl});

}  // namespace GpgFrontend