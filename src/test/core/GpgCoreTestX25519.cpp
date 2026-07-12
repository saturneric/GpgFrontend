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

namespace GpgFrontend::Test {

TEST(X25519Test, GenerateProducesCorrectSizes) {
  auto kp = X25519::Generate();
  ASSERT_TRUE(kp.has_value());
  EXPECT_EQ(kp->pub.Size(), X25519::kPubKeyLen);
  EXPECT_EQ(kp->priv.Size(), X25519::kPrivKeyLen);
}

// The public key recomputed from the scalar matches the generated one.
TEST(X25519Test, DerivePublicMatchesGenerate) {
  auto kp = X25519::Generate();
  ASSERT_TRUE(kp.has_value());

  auto pub = X25519::DerivePublic(kp->priv);
  ASSERT_TRUE(pub.has_value());
  EXPECT_EQ(pub->ConvertToQByteArray(), kp->pub.ConvertToQByteArray());
}

// The fundamental DH property: DH(a, B) == DH(b, A).
TEST(X25519Test, DiffieHellmanIsSymmetric) {
  auto a = X25519::Generate();
  auto b = X25519::Generate();
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  auto ab = X25519::DH(a->priv, b->pub);
  auto ba = X25519::DH(b->priv, a->pub);
  ASSERT_TRUE(ab.has_value());
  ASSERT_TRUE(ba.has_value());
  EXPECT_EQ(ab->ConvertToQByteArray(), ba->ConvertToQByteArray());
  EXPECT_EQ(ab->Size(), X25519::kSharedLen);
}

// Two independent pairs must not agree on a shared secret.
TEST(X25519Test, DistinctPairsDiffer) {
  auto a = X25519::Generate();
  auto b = X25519::Generate();
  auto c = X25519::Generate();
  ASSERT_TRUE(a && b && c);

  auto ab = X25519::DH(a->priv, b->pub);
  auto ac = X25519::DH(a->priv, c->pub);
  ASSERT_TRUE(ab && ac);
  EXPECT_NE(ab->ConvertToQByteArray(), ac->ConvertToQByteArray());
}

TEST(X25519Test, RejectsMalformedOperands) {
  auto kp = X25519::Generate();
  ASSERT_TRUE(kp.has_value());

  EXPECT_FALSE(X25519::DH(GFBuffer(QByteArray(10, '\0')), kp->pub).has_value());
  EXPECT_FALSE(X25519::DH(kp->priv, GFBuffer(QByteArray(10, '\0'))).has_value());
  EXPECT_FALSE(X25519::DerivePublic(GFBuffer(QByteArray(31, '\0'))).has_value());
}

// An all-zero peer key is a degenerate point; DH must fail closed.
TEST(X25519Test, RejectsAllZeroPeerKey) {
  auto kp = X25519::Generate();
  ASSERT_TRUE(kp.has_value());
  EXPECT_FALSE(
      X25519::DH(kp->priv, GFBuffer(QByteArray(X25519::kPubKeyLen, '\0')))
          .has_value());
}

}  // namespace GpgFrontend::Test
