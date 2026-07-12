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

#include "core/function/DoubleRatchet.h"

#include "core/function/SecureRandomGenerator.h"

namespace GpgFrontend::Test {

namespace {

const GFBuffer kAd(QByteArray("gf-im-test-ad"));

// A captured wire message: the ratchet header plus its ciphertext.
struct Wire {
  RatchetHeader header;
  GFBuffer ciphertext;
};

// Establish a fresh Alice(sender)/Bob(receiver) session sharing a random SK.
auto NewSession(RatchetState& alice, RatchetState& bob) -> bool {
  auto sk = SecureRandomGenerator::Generate(32);
  auto bob_spk = X25519::Generate();
  if (!sk || !bob_spk) return false;

  auto a = DoubleRatchet::InitSender(*sk, bob_spk->pub);
  if (!a) return false;
  alice = *a;
  bob = DoubleRatchet::InitReceiver(*sk, *bob_spk);
  return true;
}

auto Send(RatchetState& from, const QByteArray& msg) -> std::optional<Wire> {
  Wire w;
  auto ct = DoubleRatchet::Encrypt(from, GFBuffer(msg), kAd, w.header);
  if (!ct) return {};
  w.ciphertext = *ct;
  return w;
}

auto Recv(RatchetState& to, const Wire& w) -> std::optional<QByteArray> {
  auto pt = DoubleRatchet::Decrypt(to, w.header, w.ciphertext, kAd);
  if (!pt) return {};
  return pt->ConvertToQByteArray();
}

}  // namespace

TEST(DoubleRatchetTest, SimpleRoundTrip) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto w = Send(alice, "hello bob");
  ASSERT_TRUE(w.has_value());
  auto got = Recv(bob, *w);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(*got, QByteArray("hello bob"));
}

// After Bob receives once he can reply, and the ratchet advances both ways.
TEST(DoubleRatchetTest, BidirectionalConversation) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto a1 = Send(alice, "a1");
  ASSERT_TRUE(a1 && Recv(bob, *a1) == QByteArray("a1"));

  auto b1 = Send(bob, "b1");
  ASSERT_TRUE(b1.has_value());
  EXPECT_EQ(Recv(alice, *b1), QByteArray("b1"));

  auto a2 = Send(alice, "a2");
  ASSERT_TRUE(a2.has_value());
  EXPECT_EQ(Recv(bob, *a2), QByteArray("a2"));

  auto b2 = Send(bob, "b2");
  ASSERT_TRUE(b2.has_value());
  EXPECT_EQ(Recv(alice, *b2), QByteArray("b2"));
}

// Messages delivered out of order (3,1,2) all decrypt via the skipped store.
TEST(DoubleRatchetTest, OutOfOrderDelivery) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto m1 = Send(alice, "m1");
  auto m2 = Send(alice, "m2");
  auto m3 = Send(alice, "m3");
  ASSERT_TRUE(m1 && m2 && m3);

  EXPECT_EQ(Recv(bob, *m3), QByteArray("m3"));
  EXPECT_EQ(Recv(bob, *m1), QByteArray("m1"));
  EXPECT_EQ(Recv(bob, *m2), QByteArray("m2"));
}

// A message dropped in transit is still recoverable when it later arrives.
TEST(DoubleRatchetTest, DroppedThenLateDelivery) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto m1 = Send(alice, "m1");
  auto m2 = Send(alice, "m2");  // "lost" for now
  auto m3 = Send(alice, "m3");
  ASSERT_TRUE(m1 && m2 && m3);

  EXPECT_EQ(Recv(bob, *m1), QByteArray("m1"));
  EXPECT_EQ(Recv(bob, *m3), QByteArray("m3"));
  EXPECT_EQ(Recv(bob, *m2), QByteArray("m2"));  // arrives late
}

// A DH-ratchet step happens when the peer's ratchet key changes on reply.
TEST(DoubleRatchetTest, DhRatchetStepOnReply) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto a1 = Send(alice, "a1");
  ASSERT_TRUE(a1 && Recv(bob, *a1));

  auto b1 = Send(bob, "b1");
  ASSERT_TRUE(b1.has_value());
  // Bob's reply carries a new ratchet public key (the DH step).
  EXPECT_NE(b1->header.dh_pub.ConvertToQByteArray(),
            a1->header.dh_pub.ConvertToQByteArray());
  EXPECT_EQ(Recv(alice, *b1), QByteArray("b1"));
}

// Once a message key is consumed it is deleted: replaying the token fails.
TEST(DoubleRatchetTest, ConsumedKeyIsDeleted) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto m1 = Send(alice, "m1");
  ASSERT_TRUE(m1.has_value());
  EXPECT_EQ(Recv(bob, *m1), QByteArray("m1"));
  // Second delivery of the same token: key already gone.
  EXPECT_FALSE(Recv(bob, *m1).has_value());
}

// A skip larger than kMaxSkip is rejected rather than looping unbounded.
TEST(DoubleRatchetTest, MaxSkipOverflowRejected) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto m = Send(alice, "far ahead");
  ASSERT_TRUE(m.has_value());
  m->header.n = DoubleRatchet::kMaxSkip + 5;  // pretend a huge gap
  EXPECT_FALSE(Recv(bob, *m).has_value());
}

// Any tampering with the ciphertext or header fails authentication.
TEST(DoubleRatchetTest, TamperFails) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto m = Send(alice, "authentic");
  ASSERT_TRUE(m.has_value());

  Wire bad = *m;
  QByteArray ct = bad.ciphertext.ConvertToQByteArray();
  ct[0] = static_cast<char>(ct[0] ^ 0x01);
  bad.ciphertext = GFBuffer(ct);
  EXPECT_FALSE(Recv(bob, bad).has_value());

  // A forged token must not have desynchronised Bob's state.
  EXPECT_EQ(Recv(bob, *m), QByteArray("authentic"));
}

// State survives an encrypted-at-rest serialize/deserialize round-trip.
TEST(DoubleRatchetTest, StateSerializationRoundTrip) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto a1 = Send(alice, "a1");
  ASSERT_TRUE(a1 && Recv(bob, *a1));

  // Persist Bob mid-conversation, restore, and keep decrypting.
  auto blob = DoubleRatchet::SerializeState(bob);
  auto restored = DoubleRatchet::DeserializeState(blob);
  ASSERT_TRUE(restored.has_value());

  auto a2 = Send(alice, "a2");
  ASSERT_TRUE(a2.has_value());
  EXPECT_EQ(Recv(*restored, *a2), QByteArray("a2"));
}

// Serialized state preserves the skipped-key store (out-of-order after reload).
TEST(DoubleRatchetTest, SerializationPreservesSkippedKeys) {
  RatchetState alice;
  RatchetState bob;
  ASSERT_TRUE(NewSession(alice, bob));

  auto m1 = Send(alice, "m1");
  auto m2 = Send(alice, "m2");
  auto m3 = Send(alice, "m3");
  ASSERT_TRUE(m1 && m2 && m3);

  EXPECT_EQ(Recv(bob, *m3), QByteArray("m3"));  // banks keys for 1 and 2

  auto restored = DoubleRatchet::DeserializeState(
      DoubleRatchet::SerializeState(bob));
  ASSERT_TRUE(restored.has_value());
  EXPECT_EQ(Recv(*restored, *m1), QByteArray("m1"));
  EXPECT_EQ(Recv(*restored, *m2), QByteArray("m2"));
}

}  // namespace GpgFrontend::Test
