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

#include "core/function/ImContactBundle.h"

#include "core/function/X25519.h"

namespace GpgFrontend::Test {

namespace {

auto SampleBundle() -> ImContactBundle {
  ImContactBundle b;
  b.ik_pub = X25519::Generate()->pub;
  b.spk_id = 0x11223344;
  b.spk_pub = X25519::Generate()->pub;
  b.opks.push_back({1, X25519::Generate()->pub});
  b.opks.push_back({2, X25519::Generate()->pub});
  b.pgp_fpr = "1234567890ABCDEF1234567890ABCDEF12345678";
  b.pgp_sig = QByteArray("a-detached-signature-blob");
  return b;
}

}  // namespace

TEST(ImContactBundleTest, SerializeParseRoundTrip) {
  const auto b = SampleBundle();
  const auto parsed = ImContactBundle::Parse(b.Serialize());
  ASSERT_TRUE(parsed.has_value());

  EXPECT_EQ(parsed->ik_pub.ConvertToQByteArray(), b.ik_pub.ConvertToQByteArray());
  EXPECT_EQ(parsed->spk_id, b.spk_id);
  EXPECT_EQ(parsed->spk_pub.ConvertToQByteArray(),
            b.spk_pub.ConvertToQByteArray());
  ASSERT_EQ(parsed->opks.size(), b.opks.size());
  EXPECT_EQ(parsed->opks[0].id, 1U);
  EXPECT_EQ(parsed->opks[1].pub.ConvertToQByteArray(),
            b.opks[1].pub.ConvertToQByteArray());
  EXPECT_EQ(parsed->pgp_fpr, b.pgp_fpr);
  EXPECT_EQ(parsed->pgp_sig, b.pgp_sig);
}

// The signed portion excludes the signature and is stable across (de)serialize.
TEST(ImContactBundleTest, SignedPortionExcludesSignature) {
  auto b = SampleBundle();
  const auto portion = b.SignedPortion().ConvertToQByteArray();

  // Changing only the signature must not change the signed portion.
  b.pgp_sig = QByteArray("a-totally-different-signature");
  EXPECT_EQ(b.SignedPortion().ConvertToQByteArray(), portion);

  // But it must not contain the signature bytes.
  EXPECT_FALSE(portion.contains(QByteArray("different-signature")));
}

TEST(ImContactBundleTest, NoOneTimePrekeys) {
  auto b = SampleBundle();
  b.opks.clear();
  const auto parsed = ImContactBundle::Parse(b.Serialize());
  ASSERT_TRUE(parsed.has_value());
  EXPECT_TRUE(parsed->opks.isEmpty());
}

TEST(ImContactBundleTest, RejectsTruncated) {
  const auto full = SampleBundle().Serialize().ConvertToQByteArray();
  EXPECT_FALSE(ImContactBundle::Parse(GFBuffer(full.left(10))).has_value());
  EXPECT_FALSE(ImContactBundle::Parse(GFBuffer(QByteArray())).has_value());
}

}  // namespace GpgFrontend::Test
