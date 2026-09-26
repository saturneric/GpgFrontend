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

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/gpg/GpgCoreTest.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGroup.h"
#include "ui/main_window/EncryptBlockReason.h"

namespace GpgFrontend::Test {

namespace {

using UI::CollectEncryptKeyFacts;
using UI::DescribeEncryptBlock;
using UI::EncryptKeyFacts;
using UI::EncryptSubkeyFacts;

auto Date(const char* iso) -> QDateTime {
  return QDateTime::fromString(iso, Qt::ISODate);
}

auto EncryptSubkey(bool expired, const char* expires) -> EncryptSubkeyFacts {
  EncryptSubkeyFacts s;
  s.can_encrypt = true;
  s.expired = expired;
  s.expires = Date(expires);
  return s;
}

auto Key() -> EncryptKeyFacts {
  EncryptKeyFacts f;
  f.label = "Alice <alice@example.org>";
  EncryptSubkeyFacts primary;  // sign/certify only
  f.subkeys.push_back(primary);
  return f;
}

}  // namespace

TEST(EncryptBlockReasonTest, UsableKeySaysNothing) {
  auto f = Key();
  f.usable = true;
  EXPECT_TRUE(DescribeEncryptBlock(f).isEmpty());
}

// The reported case: the primary is fine, only the encryption subkey lapsed.
TEST(EncryptBlockReasonTest, ExpiredSubkeyNamesKeyAndDate) {
  auto f = Key();
  f.subkeys.push_back(EncryptSubkey(true, "2026-08-03T12:00:00Z"));

  EXPECT_EQ(DescribeEncryptBlock(f),
            "The encryption subkey of Alice <alice@example.org> expired on "
            "2026-08-03.");
}

// Several lapsed subkeys: the latest is the one the owner would extend.
TEST(EncryptBlockReasonTest, NamesTheMostRecentlyExpiredSubkey) {
  auto f = Key();
  f.subkeys.push_back(EncryptSubkey(true, "2024-01-01T00:00:00Z"));
  f.subkeys.push_back(EncryptSubkey(true, "2026-08-03T00:00:00Z"));
  f.subkeys.push_back(EncryptSubkey(true, "2025-05-05T00:00:00Z"));

  EXPECT_TRUE(DescribeEncryptBlock(f).contains("2026-08-03"));
}

TEST(EncryptBlockReasonTest, ExpiredPrimaryWinsOverSubkeys) {
  auto f = Key();
  f.expired = true;
  f.expires = Date("2023-09-05T04:00:00Z");
  f.subkeys.push_back(EncryptSubkey(true, "2023-09-05T04:00:00Z"));

  EXPECT_EQ(DescribeEncryptBlock(f),
            "The key of Alice <alice@example.org> expired on 2023-09-05.");
}

TEST(EncryptBlockReasonTest, RevokedKeySaysRevoked) {
  auto f = Key();
  f.revoked = true;
  f.expired = true;
  EXPECT_TRUE(DescribeEncryptBlock(f).contains("revoked"));
}

TEST(EncryptBlockReasonTest, NoEncryptionSubkey) {
  EXPECT_TRUE(DescribeEncryptBlock(Key()).contains("no subkey that can"));
}

TEST(EncryptBlockReasonTest, RevokedSubkeyIsNotReportedAsExpired) {
  auto f = Key();
  auto s = EncryptSubkey(true, "2024-01-01T00:00:00Z");
  s.revoked = true;
  f.subkeys.push_back(s);

  const auto text = DescribeEncryptBlock(f);
  EXPECT_TRUE(text.contains("revoked or disabled")) << text.toStdString();
  EXPECT_FALSE(text.contains("2024")) << text.toStdString();
}

TEST(EncryptBlockReasonTest, KeyGroupIsNeverBlocked) {
  auto group =
      QSharedPointer<GpgKeyGroup>::create("Team", "", "", QStringList{"A"});
  EXPECT_TRUE(DescribeEncryptBlock(group).isEmpty());
}

// The fixture key and its encryption subkey both expired on 2023-09-05.
TEST_F(GpgCoreTest, EncryptBlockReasonForExpiredFixtureKey) {
  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key != nullptr);
  ASSERT_TRUE(key->IsExpired());

  const auto facts = CollectEncryptKeyFacts(key);
  EXPECT_EQ(facts.label, "GpgFrontendTest <gpgfrontend@gpgfrontend.pub>");
  EXPECT_EQ(facts.subkeys.size(), 2);

  EXPECT_EQ(DescribeEncryptBlock(key),
            "The key of GpgFrontendTest <gpgfrontend@gpgfrontend.pub> expired "
            "on 2023-09-05.");
}

}  // namespace GpgFrontend::Test
