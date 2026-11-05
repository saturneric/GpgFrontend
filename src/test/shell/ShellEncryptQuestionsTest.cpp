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

#include "shell/rules/RecipientConfirm.h"

namespace GpgFrontend::Test {

using Shell::EncryptQuestionKind;
using Shell::EncryptQuestions;

TEST(ShellEncryptQuestionsTest, SeveralRecipientsAreConfirmedWhenAsked) {
  const auto asked = EncryptQuestions(2, {}, true);
  ASSERT_EQ(asked.size(), 1);
  EXPECT_EQ(asked.first().kind, EncryptQuestionKind::kManyRecipients);
  EXPECT_EQ(asked.first().text, u"Encrypt to 2 recipients?"_s);
  EXPECT_FALSE(asked.first().detail.isEmpty());
  EXPECT_FALSE(asked.first().accept.isEmpty());
}

TEST(ShellEncryptQuestionsTest, OneRecipientOrTheSettingOffAsksNothing) {
  EXPECT_TRUE(EncryptQuestions(1, {}, true).isEmpty());
  EXPECT_TRUE(EncryptQuestions(5, {}, false).isEmpty());
  EXPECT_TRUE(EncryptQuestions(0, {}, true).isEmpty());
}

}  // namespace GpgFrontend::Test
