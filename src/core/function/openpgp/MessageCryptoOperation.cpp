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

#include "MessageCryptoOperation.h"

#include "core/function/openpgp/Async.h"
#include "core/function/openpgp/traits/MessageCryptoTraits.h"

namespace GpgFrontend {

MessageCryptoOperation::MessageCryptoOperation(int channel)
    : SingletonFunctionObject<MessageCryptoOperation>(channel) {}

void MessageCryptoOperation::Encrypt(const GpgAbstractKeyPtrList& keys,
                                     const GFBuffer& in_buffer, bool ascii,
                                     const GpgOperationCallback& cb) {
  RunRegisteredAsync<EncryptOpTag>(ctx_, cb, keys, in_buffer, ascii);
}

auto MessageCryptoOperation::EncryptSync(const GpgAbstractKeyPtrList& keys,
                                         const GFBuffer& in_buffer, bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<EncryptOpTag>(ctx_, keys, in_buffer, ascii);
}

void MessageCryptoOperation::EncryptSymmetric(const GFBuffer& in_buffer,
                                              bool ascii,
                                              const GpgOperationCallback& cb) {
  RunRegisteredAsync<EncryptSymmetricOpTag>(ctx_, cb, in_buffer, ascii);
}

auto MessageCryptoOperation::EncryptSymmetricSync(const GFBuffer& in_buffer,
                                                  bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<EncryptSymmetricOpTag>(ctx_, in_buffer, ascii);
}

void MessageCryptoOperation::Decrypt(const GFBuffer& in_buffer,
                                     const GpgOperationCallback& cb) {
  RunRegisteredAsync<DecryptOpTag>(ctx_, cb, in_buffer);
}

auto MessageCryptoOperation::DecryptSync(const GFBuffer& in_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<DecryptOpTag>(ctx_, in_buffer);
}

void MessageCryptoOperation::Verify(const GFBuffer& in_buffer,
                                    const GFBuffer& sig_buffer,
                                    const GpgOperationCallback& cb) {
  RunRegisteredAsync<VerifyOpTag>(ctx_, cb, in_buffer, sig_buffer);
}

auto MessageCryptoOperation::VerifySync(const GFBuffer& in_buffer,
                                        const GFBuffer& sig_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<VerifyOpTag>(ctx_, in_buffer, sig_buffer);
}

void MessageCryptoOperation::Sign(const GpgAbstractKeyPtrList& signers,
                                  const GFBuffer& in_buffer, GpgSignMode mode,
                                  bool ascii, const GpgOperationCallback& cb) {
  RunRegisteredAsync<SignOpTag>(ctx_, cb, signers, in_buffer, mode, ascii);
}

auto MessageCryptoOperation::SignSync(const GpgAbstractKeyPtrList& signers,
                                      const GFBuffer& in_buffer,
                                      GpgSignMode mode, bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<SignOpTag>(ctx_, signers, in_buffer, mode, ascii);
}

void MessageCryptoOperation::DecryptVerify(const GFBuffer& in_buffer,
                                           const GpgOperationCallback& cb) {
  RunRegisteredAsync<DecryptVerifyOpTag>(ctx_, cb, in_buffer);
}

auto MessageCryptoOperation::DecryptVerifySync(const GFBuffer& in_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<DecryptVerifyOpTag>(ctx_, in_buffer);
}

void MessageCryptoOperation::EncryptSign(const GpgAbstractKeyPtrList& keys,
                                         const GpgAbstractKeyPtrList& signers,
                                         const GFBuffer& in_buffer, bool ascii,
                                         const GpgOperationCallback& cb) {
  RunRegisteredAsync<EncryptSignOpTag>(ctx_, cb, keys, signers, in_buffer,
                                       ascii);
}

auto MessageCryptoOperation::EncryptSignSync(
    const GpgAbstractKeyPtrList& keys, const GpgAbstractKeyPtrList& signers,
    const GFBuffer& in_buffer, bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<EncryptSignOpTag>(ctx_, keys, signers, in_buffer,
                                             ascii);
}

}  // namespace GpgFrontend
