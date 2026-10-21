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

#include "ui/main_window/EncryptBlockReason.h"

namespace GpgFrontend::Test {

namespace {

auto MakeFacts() -> UI::EncryptKeyFacts {
  UI::EncryptKeyFacts facts;
  facts.label = "Alice <alice@example.com>";
  return facts;
}

auto EncryptSubkey() -> UI::EncryptSubkeyFacts {
  UI::EncryptSubkeyFacts s_key;
  s_key.can_encrypt = true;
  return s_key;
}

}  // namespace

TEST(UiEncryptBlockReasonTest, UsableKeyHasNoReason) {
  auto facts = MakeFacts();
  facts.usable = true;
  EXPECT_TRUE(UI::DescribeEncryptBlock(facts).isEmpty());
}

TEST(UiEncryptBlockReasonTest, RevokedKey) {
  auto facts = MakeFacts();
  facts.revoked = true;
  const auto reason = UI::DescribeEncryptBlock(facts);
  EXPECT_TRUE(reason.contains("alice@example.com"));
  EXPECT_TRUE(reason.contains("revoked"));
}

TEST(UiEncryptBlockReasonTest, ExpiredKeyNamesTheDate) {
  auto facts = MakeFacts();
  facts.expired = true;
  facts.expires = QDateTime(QDate(2025, 3, 8), QTime(12, 0));
  const auto reason = UI::DescribeEncryptBlock(facts);
  EXPECT_TRUE(reason.contains("expired"));
  EXPECT_TRUE(reason.contains("2025-03-08"));
}

TEST(UiEncryptBlockReasonTest, NoEncryptionSubkey) {
  auto facts = MakeFacts();
  UI::EncryptSubkeyFacts sign_only;
  facts.subkeys.push_back(sign_only);
  EXPECT_TRUE(UI::DescribeEncryptBlock(facts).contains("no subkey"));
}

TEST(UiEncryptBlockReasonTest, ExpiredSubkeyUnderValidPrimary) {
  auto facts = MakeFacts();
  auto older = EncryptSubkey();
  older.expired = true;
  older.expires = QDateTime(QDate(2024, 1, 1), QTime(0, 0));
  auto newer = EncryptSubkey();
  newer.expired = true;
  newer.expires = QDateTime(QDate(2025, 6, 1), QTime(0, 0));
  facts.subkeys.push_back(older);
  facts.subkeys.push_back(newer);

  const auto reason = UI::DescribeEncryptBlock(facts);
  EXPECT_TRUE(reason.contains("encryption subkey"));
  EXPECT_TRUE(reason.contains("2025-06-01"));
}

TEST(UiEncryptBlockReasonTest, RevokedSubkey) {
  auto facts = MakeFacts();
  auto s_key = EncryptSubkey();
  s_key.revoked = true;
  facts.subkeys.push_back(s_key);
  EXPECT_TRUE(UI::DescribeEncryptBlock(facts).contains("revoked or disabled"));
}

}  // namespace GpgFrontend::Test
