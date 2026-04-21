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
#include "core/function/gpg/FileCryptoOpera.h"
#include "core/function/rpgp/FileCryptoOpera.h"

namespace GpgFrontend {

// File Crypto Operations
struct FileEncryptOpTag {};
struct FileEncryptSymmetricOpTag {};
struct FileDecryptOpTag {};
struct FileVerifyOpTag {};
struct FileSignOpTag {};
struct FileDecryptVerifyOpTag {};
struct FileEncryptSignOpTag {};

// Directory/Archive Crypto Operations
struct DirEncryptOpTag {};
struct DirEncryptSymmetricOpTag {};
struct DirEncryptSignOpTag {};
struct ArchiveDecryptOpTag {};
struct ArchiveDecryptVerifyOpTag {};

template <>
struct OpTraits<FileEncryptOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_file_encrypt";

  using ImplFn = GpgError (*)(GpgContext&, const GpgAbstractKeyPtrList&,
                              const QString&, bool, const QString&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptFileGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptFileRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                   const QString& in_path, bool ascii, const QString& out_path,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), keys, in_path, ascii, out_path,
                            data_object);
  }
};

template <>
struct OpTraits<FileEncryptSymmetricOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_file_encrypt_symmetric";

  using ImplFn = GpgError (*)(GpgContext&, const QString&, bool, const QString&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptSymmetricFileGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptSymmetricFileRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const QString& in_path, bool ascii,
                   const QString& out_path, const DataObjectPtr& data_object)
      -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_path, ascii, out_path,
                            data_object);
  }
};

template <>
struct OpTraits<FileDecryptOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_file_decrypt";

  using ImplFn = GpgError (*)(GpgContext&, const QString&, const QString&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &DecryptFileGnuPGImpl},
        {OpenPGPEngine::kRPGP, &DecryptFileRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const QString& in_path,
                   const QString& out_path, const DataObjectPtr& data_object)
      -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_path, out_path, data_object);
  }
};

template <>
struct OpTraits<FileSignOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_file_sign";

  using ImplFn = GpgError (*)(GpgContext&, const GpgAbstractKeyPtrList&,
                              const QString&, bool, const QString&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const EngineOpImplTable<ImplFn> kTable = {
        {OpenPGPEngine::kGNUPG, &SignFileGnuPGImpl},
        {OpenPGPEngine::kRPGP, &SignFileRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GpgAbstractKeyPtrList& signers,
                   const QString& in_path, bool ascii, const QString& out_path,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), signers, in_path, ascii, out_path,
                            data_object);
  }
};

template <>
struct OpTraits<FileVerifyOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_file_verify";

  using ImplFn = GpgError (*)(GpgContext&, const QString&, const QString&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &VerifyFileGnuPGImpl},
        {OpenPGPEngine::kRPGP, &VerifyFileRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const QString& data_path,
                   const QString& sign_path, const DataObjectPtr& data_object)
      -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), data_path, sign_path,
                            data_object);
  }
};

template <>
struct OpTraits<FileEncryptSignOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_file_encrypt_sign";

  using ImplFn = GpgError (*)(GpgContext&, const GpgAbstractKeyPtrList&,
                              const GpgAbstractKeyPtrList&, const QString&,
                              bool, const QString&, const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptSignFileGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptSignFileRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                   const GpgAbstractKeyPtrList& signers, const QString& in_path,
                   bool ascii, const QString& out_path,
                   const DataObjectPtr& data_object) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), keys, signers, in_path, ascii,
                            out_path, data_object);
  }
};

template <>
struct OpTraits<FileDecryptVerifyOpTag> : EmptyOpTraitsBase {
  static constexpr const char* kOpName = "op_file_decrypt_verify";

  using ImplFn = GpgError (*)(GpgContext&, const QString&, const QString&,
                              const DataObjectPtr&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &DecryptVerifyFileGnuPGImpl},
        {OpenPGPEngine::kRPGP, &DecryptVerifyFileRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const QString& in_path,
                   const QString& out_path, const DataObjectPtr& data_object)
      -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_path, out_path, data_object);
  }
};

template <>
struct OpTraits<DirEncryptOpTag> : EmptyOpTraitsBase {
  using ImplFn = GpgError (*)(GpgContext&, const GpgAbstractKeyPtrList&,
                              const QString&, bool, const QString&,
                              const GpgOperationCallback&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptDirGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptDirRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                   const QString& in_path, bool ascii, const QString& out_path,
                   const GpgOperationCallback& cb) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), keys, in_path, ascii, out_path,
                            cb);
  }
};

template <>
struct OpTraits<ArchiveDecryptOpTag> : EmptyOpTraitsBase {
  using ImplFn = GpgError (*)(GpgContext&, const QString&, const QString&,
                              const GpgOperationCallback&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &DecryptArchiveGnuPGImpl},
        {OpenPGPEngine::kRPGP, &DecryptArchiveRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const QString& in_path,
                   const QString& out_path, const GpgOperationCallback& cb)
      -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_path, out_path, cb);
  }
};

template <>
struct OpTraits<DirEncryptSignOpTag> : EmptyOpTraitsBase {
  using ImplFn = GpgError (*)(GpgContext&, const GpgAbstractKeyPtrList&,
                              const GpgAbstractKeyPtrList&, const QString&,
                              bool, const QString&,
                              const GpgOperationCallback&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptSignDirGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptSignDirRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                   const GpgAbstractKeyPtrList& signers, const QString& in_path,
                   bool ascii, const QString& out_path,
                   const GpgOperationCallback& cb) -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), keys, signers, in_path, ascii,
                            out_path, cb);
  }
};

template <>
struct OpTraits<DirEncryptSymmetricOpTag> : EmptyOpTraitsBase {
  using ImplFn = GpgError (*)(GpgContext&, const QString&, bool, const QString&,
                              const GpgOperationCallback&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &EncryptSymmetricDirGnuPGImpl},
        {OpenPGPEngine::kRPGP, &EncryptSymmetricDirRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const QString& in_path, bool ascii,
                   const QString& out_path, const GpgOperationCallback& cb)
      -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_path, ascii, out_path, cb);
  }
};

template <>
struct OpTraits<ArchiveDecryptVerifyOpTag> : EmptyOpTraitsBase {
  using ImplFn = GpgError (*)(GpgContext&, const QString&, const QString&,
                              const GpgOperationCallback&);

  static auto ImplTable() -> const auto& {
    static const QContainer<EngineOpImpl<ImplFn>> kTable = {
        {OpenPGPEngine::kGNUPG, &DecryptVerifyArchiveGnuPGImpl},
        {OpenPGPEngine::kRPGP, &DecryptVerifyArchiveRpgpImpl},
    };
    return kTable;
  }

  static auto Call(GpgContext& ctx, const QString& in_path,
                   const QString& out_path, const GpgOperationCallback& cb)
      -> GpgError {
    return DispatchByEngine(ctx, ImplTable(), in_path, out_path, cb);
  }
};

}  // namespace GpgFrontend