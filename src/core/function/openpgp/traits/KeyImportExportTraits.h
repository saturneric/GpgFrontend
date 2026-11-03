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

#if GF_ENGINE_GNUPG
#include "core/function/gpg/GpgErrorMap.h"
#endif
#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/KeyImportExportOpSupport.h"

// Engine Impl
#if GF_ENGINE_GNUPG
#include "core/function/gpg/KeyImportExport.h"
#endif
#include "core/function/rpgp/KeyImportExport.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(ImportKeyOpTag, &ImportKeyRpgpImpl,
                      {OpenPGPEngine::kRPGP, &ImportKeyRpgpImpl},
                      GF_GNUPG_IMPL(&ImportKeyGnuPGImpl));

GF_DEF_OP_IMPL_TRAITS(ImportRevCertOpTag, &ImportRevCertRpgpImpl,
                      {OpenPGPEngine::kRPGP, &ImportRevCertRpgpImpl},
                      GF_GNUPG_IMPL(&ImportKeyGnuPGImpl));

GF_DEF_OP_IMPL_TRAITS(
    ExportKeysOpTag, &ExportKeysRpgpImpl,
    {OpenPGPEngine::kRPGP, &ExportKeysRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&ExportKeysGnuPGImpl>::Call));

// Operations only the GnuPG engine implements, named by signature in
// GpgFrontend's own types: an rPGP-only build has no implementation to take it
// from, and a GnuPG build checks each implementation against it.
using ExportKeyAsOpenSSHFormatFn = std::tuple<GFError, GFBuffer> (*)(
    OpenPGPContext&, const GpgAbstractKeyPtrList&, bool, bool, bool, bool);
using ExportSubkeyFn = std::tuple<GFError, GFBuffer> (*)(OpenPGPContext&,
                                                         const QString&, bool);

GF_DEF_OP_IMPL_TRAITS(
    ExportKeyAsOpenSSHFormatOpTag, ExportKeyAsOpenSSHFormatFn{},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&ExportKeysGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    ExportKeysAsyncOpTag, &ExportKeysAsyncRpgpImpl,
    {OpenPGPEngine::kRPGP, &ExportKeysAsyncRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&ExportKeysAsyncGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    ExportAllKeysOpTag, &ExportAllKeysRpgpImpl,
    {OpenPGPEngine::kRPGP, &ExportAllKeysRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&ExportAllKeysGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    ExportSubkeyOpTag, ExportSubkeyFn{},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&ExportSubkeyGnuPGImpl>::Call));

}  // namespace GpgFrontend