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
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/gpg/GpgCoreTest.h"
#include "core/model/GpgKeyGroup.h"
#include "core/model/GpgKeyTableModel.h"
#include "ui/main_window/RecipientConfirm.h"

namespace GpgFrontend::Test {

using UI::BuildRecipientConfirmHtml;
using UI::NeedsRecipientConfirmation;

// Off by default means never asking; on asks only once a second recipient
// would learn about the first.
TEST(RecipientConfirmTest, AsksOnlyWhenOnAndMoreThanOneRecipient) {
  for (const qsizetype n : {0, 1, 2, 5}) {
    EXPECT_FALSE(NeedsRecipientConfirmation(n, false)) << n;
  }

  EXPECT_FALSE(NeedsRecipientConfirmation(0, true));
  EXPECT_FALSE(NeedsRecipientConfirmation(1, true));
  EXPECT_TRUE(NeedsRecipientConfirmation(2, true));
  EXPECT_TRUE(NeedsRecipientConfirmation(5, true));
}

TEST(RecipientConfirmTest, KeyIdIsGroupedInFours) {
  EXPECT_EQ(UI::FormatKeyIdForDisplay("2B5F42197E62FBCF"),
            "2B5F 4219 7E62 FBCF");
  EXPECT_EQ(UI::FormatKeyIdForDisplay("ABCDEF"), "ABCD EF");
  EXPECT_EQ(UI::FormatKeyIdForDisplay(""), "");
}

TEST(RecipientConfirmTest, ListsAKeyGroupByNameAndSize) {
  auto group = QSharedPointer<GpgKeyGroup>::create(
      "Team", "team@example.com", "", QStringList{"A", "B", "C"});

  const auto html = BuildRecipientConfirmHtml({group}, Qt::gray);

  EXPECT_TRUE(html.contains("<b>Team</b>")) << html.toStdString();
  EXPECT_TRUE(html.contains("3")) << html.toStdString();
  EXPECT_EQ(html.count("<tr>"), 1);
}

// Names and emails come from keys anyone can hand you; they must not be able
// to inject markup into a security prompt.
TEST(RecipientConfirmTest, EscapesUserSuppliedText) {
  auto group = QSharedPointer<GpgKeyGroup>::create("<img src=x>", "", "",
                                                   QStringList{"A"});

  const auto html = BuildRecipientConfirmHtml({group}, Qt::gray);

  EXPECT_FALSE(html.contains("<img")) << html.toStdString();
  EXPECT_TRUE(html.contains("&lt;img src=x&gt;")) << html.toStdString();
}

TEST_F(GpgCoreTest, RecipientConfirmHtmlListsEveryKeyById) {
  auto model = AbstractKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetGpgKeyTableModel();
  ASSERT_TRUE(model != nullptr);

  GpgAbstractKeyPtrList keys;
  for (const auto& key : model->GetAllKeys()) {
    if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) continue;
    keys.append(key);
    if (keys.size() == 2) break;
  }
  ASSERT_EQ(keys.size(), 2);

  const auto html = BuildRecipientConfirmHtml(keys, Qt::gray);

  EXPECT_EQ(html.count("<tr>"), 2) << html.toStdString();
  for (const auto& key : keys) {
    EXPECT_TRUE(html.contains(UI::FormatKeyIdForDisplay(key->ID())))
        << html.toStdString();
    EXPECT_TRUE(html.contains(key->Name().toHtmlEscaped()))
        << html.toStdString();
  }
}

}  // namespace GpgFrontend::Test
