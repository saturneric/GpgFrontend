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
#include "core/function/gpg/BasicCryptoOpera.h"
#include "core/function/rpgp/BasicCryptoOpera.h"

namespace GpgFrontend {

// Basic Crypto Operations
struct EncryptOpTag {};
struct EncryptSymmetricOpTag {};
struct DecryptOpTag {};
struct VerifyOpTag {};
struct SignOpTag {};
struct DecryptVerifyOpTag {};
struct EncryptSignOpTag {};

template <>
struct OpTraits<EncryptOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_encrypt";

  using ImplFn = GpgError (*)(GpgContext&, const GpgAbstractKeyPtrList&,
                              const GFBuffer&, bool, const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                   const GFBuffer& in_buffer, bool ascii,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), keys, in_buffer, ascii,
                            data_object);
  }
};

template <>
struct OpTraits<EncryptSymmetricOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_encrypt_symmetric";

  using ImplFn = GpgError (*)(GpgContext&, const GFBuffer&, bool,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptSymmetricGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptSymmetricRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GFBuffer& in_buffer, bool ascii,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_buffer, ascii, data_object);
  }
};

template <>
struct OpTraits<DecryptOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_decrypt";

  using ImplFn = GpgError (*)(GpgContext&, const GFBuffer&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &DecryptGnuPGImpl},
        {OpenPGPEngine::kRPGP, &DecryptRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GFBuffer& in_buffer,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_buffer, data_object);
  }
};

template <>
struct OpTraits<SignOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_sign";

  using ImplFn = GpgError (*)(GpgContext&, const GpgAbstractKeyPtrList&,
                              const GFBuffer&, GpgSignMode, bool,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &SignGnuPGImpl},
        {OpenPGPEngine::kRPGP, &SignRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GpgAbstractKeyPtrList& signers,
                   const GFBuffer& in_buffer, GpgSignMode mode, bool ascii,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), signers, in_buffer, mode, ascii,
                            data_object);
  }
};

template <>
struct OpTraits<VerifyOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_verify";

  using ImplFn = GpgError (*)(GpgContext&, const GFBuffer&, const GFBuffer&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &VerifyGnuPGImpl},
        {OpenPGPEngine::kRPGP, &VerifyRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GFBuffer& in_buffer,
                   const GFBuffer& sig_buffer, const DataObjectPtr& data_object)
      -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_buffer, sig_buffer,
                            data_object);
  }
};

template <>
struct OpTraits<EncryptSignOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_encrypt_sign";

  using ImplFn = GpgError (*)(GpgContext&, const GpgAbstractKeyPtrList&,
                              const GpgAbstractKeyPtrList&, const GFBuffer&,
                              bool, const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptSignGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptSignRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                   const GpgAbstractKeyPtrList& signers,
                   const GFBuffer& in_buffer, bool ascii,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), keys, signers, in_buffer, ascii,
                            data_object);
  }
};

template <>
struct OpTraits<DecryptVerifyOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_decrypt_verify";

  using ImplFn = GpgError (*)(GpgContext&, const GFBuffer&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &DecryptVerifyGnuPGImpl},
        {OpenPGPEngine::kRPGP, &DecryptVerifyRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GFBuffer& in_buffer,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_buffer, data_object);
  }
};
}  // namespace GpgFrontend