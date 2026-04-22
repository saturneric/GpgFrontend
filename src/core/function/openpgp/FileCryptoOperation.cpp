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
#include "FileCryptoOperation.h"

#include "core/function/openpgp/traits/FileCryptoTraits.h"
#include "function/openpgp/Async.h"

namespace GpgFrontend {

FileCryptoOperation::FileCryptoOperation(int channel)
    : SingletonFunctionObject<FileCryptoOperation>(channel) {}

void FileCryptoOperation::EncryptFile(const GpgAbstractKeyPtrList& keys,
                                      const QString& in_path, bool ascii,
                                      const QString& out_path,
                                      const GpgOperationCallback& cb) {
  RunRegisteredAsync<FileEncryptOpTag>(ctx_, cb, keys, in_path, ascii,
                                       out_path);
}

auto FileCryptoOperation::EncryptFileSync(const GpgAbstractKeyPtrList& keys,
                                          const QString& in_path, bool ascii,
                                          const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<FileEncryptOpTag>(ctx_, keys, in_path, ascii,
                                             out_path);
}

void FileCryptoOperation::EncryptDirectory(const GpgAbstractKeyPtrList& keys,
                                           const QString& in_path, bool ascii,
                                           const QString& out_path,
                                           const GpgOperationCallback& cb) {
  RunRegisteredForward<DirEncryptOpTag>(ctx_, keys, in_path, ascii, out_path,
                                        cb);
}

void FileCryptoOperation::DecryptFile(const QString& in_path,
                                      const QString& out_path,
                                      const GpgOperationCallback& cb) {
  RunRegisteredAsync<FileDecryptOpTag>(ctx_, cb, in_path, out_path);
}

auto FileCryptoOperation::DecryptFileSync(const QString& in_path,
                                          const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<FileDecryptOpTag>(ctx_, in_path, out_path);
}

void FileCryptoOperation::DecryptArchive(const QString& in_path,
                                         const QString& out_path,
                                         const GpgOperationCallback& cb) {
  RunRegisteredForward<ArchiveDecryptOpTag>(ctx_, in_path, out_path, cb);
}

void FileCryptoOperation::SignFile(const GpgAbstractKeyPtrList& keys,
                                   const QString& in_path, bool ascii,
                                   const QString& out_path,
                                   const GpgOperationCallback& cb) {
  RunRegisteredAsync<FileSignOpTag>(ctx_, cb, keys, in_path, ascii, out_path);
}

auto FileCryptoOperation::SignFileSync(const GpgAbstractKeyPtrList& keys,
                                       const QString& in_path, bool ascii,
                                       const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<FileSignOpTag>(ctx_, keys, in_path, ascii, out_path);
}

void FileCryptoOperation::VerifyFile(const QString& data_path,
                                     const QString& sign_path,
                                     const GpgOperationCallback& cb) {
  RunRegisteredAsync<FileVerifyOpTag>(ctx_, cb, data_path, sign_path);
}

auto FileCryptoOperation::VerifyFileSync(const QString& data_path,
                                         const QString& sign_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<FileVerifyOpTag>(ctx_, data_path, sign_path);
}

void FileCryptoOperation::EncryptSignFile(
    const GpgAbstractKeyPtrList& keys, const GpgAbstractKeyPtrList& signer_keys,
    const QString& in_path, bool ascii, const QString& out_path,
    const GpgOperationCallback& cb) {
  RunRegisteredAsync<FileEncryptSignOpTag>(ctx_, cb, keys, signer_keys, in_path,
                                           ascii, out_path);
}

auto FileCryptoOperation::EncryptSignFileSync(
    const GpgAbstractKeyPtrList& keys, const GpgAbstractKeyPtrList& signer_keys,
    const QString& in_path, bool ascii, const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<FileEncryptSignOpTag>(ctx_, keys, signer_keys,
                                                 in_path, ascii, out_path);
}

void FileCryptoOperation::EncryptSignDirectory(
    const GpgAbstractKeyPtrList& keys, const GpgAbstractKeyPtrList& signer_keys,
    const QString& in_path, bool ascii, const QString& out_path,
    const GpgOperationCallback& cb) {
  RunRegisteredForward<DirEncryptSignOpTag>(ctx_, keys, signer_keys, in_path,
                                            ascii, out_path, cb);
}

void FileCryptoOperation::DecryptVerifyFile(const QString& in_path,
                                            const QString& out_path,
                                            const GpgOperationCallback& cb) {
  RunRegisteredAsync<FileDecryptVerifyOpTag>(ctx_, cb, in_path, out_path);
}

auto FileCryptoOperation::DecryptVerifyFileSync(const QString& in_path,
                                                const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<FileDecryptVerifyOpTag>(ctx_, in_path, out_path);
}

void FileCryptoOperation::DecryptVerifyArchive(const QString& in_path,
                                               const QString& out_path,
                                               const GpgOperationCallback& cb) {
  RunRegisteredForward<ArchiveDecryptVerifyOpTag>(ctx_, in_path, out_path, cb);
}

void FileCryptoOperation::EncryptFileSymmetric(const QString& in_path,
                                               bool ascii,
                                               const QString& out_path,
                                               const GpgOperationCallback& cb) {
  RunRegisteredAsync<FileEncryptSymmetricOpTag>(ctx_, cb, in_path, ascii,
                                                out_path);
}

auto FileCryptoOperation::EncryptFileSymmetricSync(const QString& in_path,
                                                   bool ascii,
                                                   const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<FileEncryptSymmetricOpTag>(ctx_, in_path, ascii,
                                                      out_path);
}

void FileCryptoOperation::EncryptDirectorySymmetric(
    const QString& in_path, bool ascii, const QString& out_path,
    const GpgOperationCallback& cb) {
  RunRegisteredForward<DirEncryptSymmetricOpTag>(ctx_, in_path, ascii, out_path,
                                                 cb);
}

}  // namespace GpgFrontend