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
 * Wires DeleteKeys, ModifyKeyPassphrase, ModifySubkeyPassphrase, SetExpire,
 * GenerateRevCert,
 * RevokeSubKey, DeleteSubKey, AddADSK, SignKey, RevKeySignature,
 * SetOwnerTrustLevel op tags to their engine implementations.
 */

#if GF_ENGINE_GNUPG
#include "core/function/gpg/GpgErrorMap.h"
#endif
#include "core/function/openpgp/helper/Op.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"

// Engine Impl
#if GF_ENGINE_GNUPG
#include "core/function/gpg/KeyManagement.h"
#endif
#include "core/function/rpgp/KeyManagement.h"

namespace GpgFrontend {

GF_DEF_OP_IMPL_TRAITS(DeleteKeysOpTag, &DeleteKeysRpgpImpl,
                      {OpenPGPEngine::kRPGP, &DeleteKeysRpgpImpl},
                      GF_GNUPG_IMPL(&DeleteKeysGnuPGImpl));

GF_DEF_OP_IMPL_TRAITS(
    ModifyKeyPassphraseOpTag, &ModifyKeyPassphraseRpgpImpl,
    {OpenPGPEngine::kRPGP, &ModifyKeyPassphraseRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&ModifyKeyPassphraseGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    ModifySubkeyPassphraseOpTag, &ModifySubkeyPassphraseRpgpImpl,
    {OpenPGPEngine::kRPGP, &ModifySubkeyPassphraseRpgpImpl},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&ModifySubkeyPassphraseGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(SetExpireOpTag, &SetExpireRpgpImpl,
                      {OpenPGPEngine::kRPGP, &SetExpireRpgpImpl},
                      GF_GNUPG_IMPL(&FromGnuPGImpl<&SetExpireGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(GenerateRevCertOpTag, &GenerateRevCertRpgpImpl,
                      {OpenPGPEngine::kRPGP, &GenerateRevCertRpgpImpl},
                      GF_GNUPG_IMPL(&GenerateRevCertGnuPGImpl));

GF_DEF_OP_IMPL_TRAITS(RevokeSubKeyOpTag, &RevokeSubKeyRpgpImpl,
                      {OpenPGPEngine::kRPGP, &RevokeSubKeyRpgpImpl},
                      GF_GNUPG_IMPL(&RevokeSubKeyGnuPGImpl));

GF_DEF_OP_IMPL_TRAITS(DeleteSubKeyOpTag, &DeleteSubKeyRpgpImpl,
                      {OpenPGPEngine::kRPGP, &DeleteSubKeyRpgpImpl},
                      GF_GNUPG_IMPL(&DeleteSubKeyGnuPGImpl));

// Operations only the GnuPG engine implements, named by signature in
// GpgFrontend's own types: an rPGP-only build has no implementation to take it
// from, and a GnuPG build checks each implementation against it.
using AddADSKFn = GFError (*)(OpenPGPContext&, const GpgKeyPtr&,
                              const GpgSubKey&, const DataObjectPtr&);
using SignKeyFn = GFError (*)(OpenPGPContext&, const GpgKeyPtr&,
                              const GpgAbstractKeyPtrList&, const QString&,
                              const std::optional<QDateTime>&,
                              const DataObjectPtr&);
using RevKeySignatureFn = GFError (*)(OpenPGPContext&, const GpgKeyPtr&,
                                      const SignIdArgsList&,
                                      const DataObjectPtr&);
using SetOwnerTrustLevelFn = bool (*)(OpenPGPContext&, const GpgAbstractKeyPtr&,
                                      int);

GF_DEF_OP_IMPL_TRAITS(AddADSKOpTag, AddADSKFn{},
                      GF_GNUPG_IMPL(&FromGnuPGImpl<&AddADSKGnuPGIImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(SignKeyOpTag, SignKeyFn{},
                      GF_GNUPG_IMPL(&FromGnuPGImpl<&SignKeyGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(
    RevKeySignatureOpTag, RevKeySignatureFn{},
    GF_GNUPG_IMPL(&FromGnuPGImpl<&RevKeySignatureGnuPGImpl>::Call));

GF_DEF_OP_IMPL_TRAITS(SetOwnerTrustLevelOpTag, SetOwnerTrustLevelFn{},
                      GF_GNUPG_IMPL(&SetOwnerTrustLevelGnuPGImpl));

}  // namespace GpgFrontend