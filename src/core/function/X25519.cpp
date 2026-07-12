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

#include "core/function/X25519.h"

#include <sodium.h>

#include "core/function/SecureRandomGenerator.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

auto X25519::Generate() -> std::optional<X25519KeyPair> {
  if (!EnsureSodiumInit()) return {};

  auto priv = SecureRandomGenerator::Generate(kPrivKeyLen);
  if (!priv) return {};

  auto pub = DerivePublic(*priv);
  if (!pub) {
    priv->Zeroize();
    return {};
  }

  return X25519KeyPair{*pub, *priv};
}

auto X25519::DerivePublic(const GFBuffer& priv) -> GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  if (priv.Size() != kPrivKeyLen) {
    LOG_E() << "invalid x25519 scalar size:" << priv.Size();
    return {};
  }

  GFBuffer pub(kPubKeyLen);
  if (crypto_scalarmult_curve25519_base(
          reinterpret_cast<unsigned char*>(pub.Data()),
          reinterpret_cast<const unsigned char*>(priv.Data())) != 0) {
    LOG_E() << "crypto_scalarmult_curve25519_base failed";
    return {};
  }
  return pub;
}

auto X25519::DH(const GFBuffer& priv, const GFBuffer& peer_pub)
    -> GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  if (priv.Size() != kPrivKeyLen || peer_pub.Size() != kPubKeyLen) {
    LOG_E() << "invalid x25519 DH operand size";
    return {};
  }

  GFBuffer shared(kSharedLen);
  // Returns -1 when the result is all-zero (peer_pub is a small-subgroup /
  // degenerate point); fail closed rather than proceed with a weak secret.
  if (crypto_scalarmult_curve25519(
          reinterpret_cast<unsigned char*>(shared.Data()),
          reinterpret_cast<const unsigned char*>(priv.Data()),
          reinterpret_cast<const unsigned char*>(peer_pub.Data())) != 0) {
    LOG_E() << "crypto_scalarmult_curve25519 failed (degenerate peer key)";
    shared.Zeroize();
    return {};
  }
  return shared;
}

}  // namespace GpgFrontend
