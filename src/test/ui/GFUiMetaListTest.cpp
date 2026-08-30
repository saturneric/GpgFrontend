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
#include "ui/widgets/MetaListPanel.h"

namespace GpgFrontend::UI::Test {

// Only the row model is asserted here, never the widget: gtest bodies run on a
// worker thread, where constructing one would take the process down. That is
// exactly why the wording and the shape of every list in the application are
// produced by pure functions and handed to a panel that decides nothing.

TEST(MetaListTest, AClaimAlwaysBringsItsCaveat) {
  // The header a package carries sits outside the sealed payload, so anyone
  // holding the file can write anything into it. Rendering what it says without
  // the sentence saying so turns a claim into a fact, which is the one mistake
  // this list must not be able to make.
  EXPECT_TRUE(MetaListNeedsCaveat({{.caption = "Created", .value = "today"},
                                   {.caption = "Written by",
                                    .value = "GpgFrontend 2.2.2",
                                    .unverified = true}}));

  EXPECT_FALSE(MetaListNeedsCaveat({{.caption = "File", .value = "work.gfp"},
                                    {.caption = "Size", .value = "50 kB"}}));

  EXPECT_FALSE(UnverifiedRowsCaveat().isEmpty());
}

TEST(MetaListTest, TheCopiedTextSaysWhatTheListSaid) {
  const QVector<MetaListRow> rows = {
      {.kind = MetaRowKind::kSection, .caption = "Profile"},
      {.caption = "Profile:",
       .value = "work",
       .detail = "kept on this machine"},
      {.kind = MetaRowKind::kRule},
      {.caption = "Total", .bytes = 2048},
  };

  const auto text = MetaListSummaryText(rows);

  EXPECT_TRUE(text.contains("[Profile]"));
  EXPECT_TRUE(text.contains("Profile: work"));
  // A sentence that is only a tooltip on screen is still in the clipboard:
  // nothing the list knows is lost by copying it.
  EXPECT_TRUE(text.contains("kept on this machine"));
  // A rule is furniture, not a reading.
  EXPECT_FALSE(text.contains("---"));
  // A size renders as one, so the copied text reads like the row did.
  EXPECT_TRUE(text.contains("Total"));
  EXPECT_FALSE(text.contains("2048"));
}

TEST(MetaListTest, TheCopiedTextCarriesTheCaveatToo) {
  const auto text = MetaListSummaryText(
      {{.caption = "Created", .value = "today", .unverified = true}});

  EXPECT_TRUE(text.contains(UnverifiedRowsCaveat()));
}

TEST(MetaListTest, AnInfoBoardCardBecomesRows) {
  // The report document paints its own cards, because it paints into a text
  // document rather than into widgets. It need not disagree about what a field
  // is, and an empty value is a row nobody wants either way.
  const QContainer<QPair<QString, QString>> fields = {
      {"Signed by", "Eric"}, {"Key", ""}, {"Status", "Good"}};

  const auto rows = ToMetaListRows(fields);

  ASSERT_EQ(rows.size(), 2);
  EXPECT_EQ(rows.at(0).caption, "Signed by");
  EXPECT_EQ(rows.at(0).value, "Eric");
  EXPECT_EQ(rows.at(1).caption, "Status");
}

}  // namespace GpgFrontend::UI::Test
