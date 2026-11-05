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

#include "shell/rules/EncryptBlockReason.h"

namespace GpgFrontend::Test {

using Shell::DescribeEncryptBlock;
using Shell::DescribeSignBlock;
using Shell::EncryptBlockNote;
using Shell::EncryptKeyFacts;
using Shell::SignBlockNote;
using Shell::SignKeyFacts;

TEST(ShellKeyBlockReasonTest, AnEmptyKeyGroupEncryptsToNobody) {
  EncryptKeyFacts group;
  group.label = u"Team"_s;
  group.usable = true;
  group.empty_group = true;
  EXPECT_EQ(DescribeEncryptBlock(group),
            u"The key group Team has no keys in it."_s);
  EXPECT_EQ(EncryptBlockNote(group), u"Has no keys"_s);

  group.empty_group = false;
  EXPECT_TRUE(DescribeEncryptBlock(group).isEmpty());
  EXPECT_TRUE(EncryptBlockNote(group).isEmpty());
}

TEST(ShellKeyBlockReasonTest, ADisabledKeySaysSo) {
  EncryptKeyFacts key;
  key.label = u"Ann <ann@example.org>"_s;
  key.disabled = true;
  EXPECT_EQ(DescribeEncryptBlock(key),
            u"The key of Ann <ann@example.org> is disabled."_s);
  EXPECT_EQ(EncryptBlockNote(key), u"Disabled"_s);
}

TEST(ShellKeyBlockReasonTest, TheNoteGivesTheKeysOwnConditionFirst) {
  EncryptKeyFacts key;
  key.revoked = true;
  key.expired = true;
  EXPECT_EQ(EncryptBlockNote(key), u"Revoked"_s);
  key.revoked = false;
  EXPECT_EQ(EncryptBlockNote(key), u"Expired"_s);
  key.expired = false;
  EXPECT_EQ(EncryptBlockNote(key), u"Cannot be encrypted to"_s);
}

TEST(ShellKeyBlockReasonTest, OnlyAKeyOfYourOwnSigns) {
  SignKeyFacts key;
  key.label = u"Bob"_s;
  EXPECT_EQ(SignBlockNote(key), u"Not a key of your own"_s);
  EXPECT_TRUE(DescribeSignBlock(key).contains(u"Bob"_s));

  key.secret = true;
  key.expired = true;
  key.expires = QDateTime(QDate(2026, 8, 3), QTime(12, 0), QTimeZone::UTC);
  EXPECT_EQ(SignBlockNote(key), u"Expired"_s);
  EXPECT_EQ(DescribeSignBlock(key), u"The key of Bob expired on 2026-08-03."_s);

  key.expired = false;
  key.usable = true;
  EXPECT_TRUE(SignBlockNote(key).isEmpty());
  EXPECT_TRUE(DescribeSignBlock(key).isEmpty());
}

TEST(ShellKeyBlockReasonTest, AKeyGroupNeverSigns) {
  SignKeyFacts group;
  group.label = u"Team"_s;
  group.group = true;
  group.usable = true;
  EXPECT_EQ(SignBlockNote(group), u"Cannot sign"_s);
  EXPECT_FALSE(DescribeSignBlock(group).isEmpty());
}

}  // namespace GpgFrontend::Test
