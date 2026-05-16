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
 * @file KeyImportExportTraits.h
 * @brief OpTraits specializations for key import and export operations.
 *
 * Wires ImportKey, ImportRevCert, ExportKeys, ExportKeyAsOpenSSHFormat,
 * ExportKeysAsync, ExportAllKeys, ExportSubkey op tags to their engine
 * implementations (GnuPG and where applicable rPGP).
 */

#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/KeyImportExportOpSupport.h"

// Engine Impl
#include "core/function/gpg/KeyImportExport.h"
#include "core/function/rpgp/KeyImportExport.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(ImportKeyOpTag, &ImportKeyGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &ImportKeyGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &ImportKeyRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(ImportRevCertOpTag, &ImportKeyGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &ImportKeyGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &ImportRevCertRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(ExportKeysOpTag, &ExportKeysGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &ExportKeysGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &ExportKeysRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(ExportKeyAsOpenSSHFormatOpTag, &ExportKeysGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &ExportKeysGnuPGImpl});

GF_DEF_OP_IMPL_TRAITS(ExportKeysAsyncOpTag, &ExportKeysAsyncGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &ExportKeysAsyncGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &ExportKeysAsyncRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(ExportAllKeysOpTag, &ExportAllKeysGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &ExportAllKeysGnuPGImpl},
                      {OpenPGPEngine::kRPGP, &ExportAllKeysRpgpImpl});

GF_DEF_OP_IMPL_TRAITS(ExportSubkeyOpTag, &ExportSubkeyGnuPGImpl,
                      {OpenPGPEngine::kGNUPG, &ExportSubkeyGnuPGImpl});

}  // namespace GpgFrontend