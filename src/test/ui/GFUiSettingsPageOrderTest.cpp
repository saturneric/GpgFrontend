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
#include "ui/dialog/settings/SettingsPageOrder.h"

namespace GpgFrontend::Test {

// The Settings dialog emits a section header the first time it sees a section,
// so its pages have to reach it already grouped. These tests run on a worker
// thread, so every descriptor carries a null page pointer — the ordering rules
// never look at it, which is exactly why they live apart from the dialog.

namespace {

auto Page(const QString& title, const QString& section)
    -> UI::SettingsPageDescriptor {
  return UI::SettingsPageDescriptor{nullptr, title, section, {}};
}

auto TitlesOf(const QVector<UI::SettingsPageDescriptor>& pages) -> QStringList {
  QStringList titles;
  for (const auto& page : pages) titles << page.title;
  return titles;
}

}  // namespace

TEST(SettingsPageOrderTest, LatePageJoinsItsOwnSection) {
  // The regression this exists for: a module page is registered while its
  // module activates, so it always arrives after every built-in page. Left
  // where it landed it would show up under "system", the last header emitted.
  const QVector<UI::SettingsPageDescriptor> pages{
      Page("General", "application"), Page("Key Databases", "keys_engines"),
      Page("GnuPG", "keys_engines"),  Page("Instant Messaging", "features"),
      Page("Advanced", "system"),     Page("Key Servers", "keys_engines"),
  };

  EXPECT_EQ(
      TitlesOf(OrderSettingsPageDescriptors(pages, UI::SettingsSectionOrder())),
      QStringList({"General", "Key Databases", "GnuPG", "Key Servers",
                   "Instant Messaging", "Advanced"}));
}

TEST(SettingsPageOrderTest, SectionsFollowTheCanonicalOrder) {
  const QVector<UI::SettingsPageDescriptor> pages{
      Page("Advanced", "system"),
      Page("Instant Messaging", "features"),
      Page("GnuPG", "keys_engines"),
      Page("General", "application"),
  };

  EXPECT_EQ(
      TitlesOf(OrderSettingsPageDescriptors(pages, UI::SettingsSectionOrder())),
      QStringList({"General", "GnuPG", "Instant Messaging", "Advanced"}));
}

TEST(SettingsPageOrderTest, OrderWithinASectionIsPreserved) {
  // Registration order is the only thing expressing intent inside a section, so
  // the sort has to be stable.
  const QVector<UI::SettingsPageDescriptor> pages{
      Page("General", "application"),
      Page("Appearance", "application"),
      Page("Network", "application"),
  };

  EXPECT_EQ(
      TitlesOf(OrderSettingsPageDescriptors(pages, UI::SettingsSectionOrder())),
      QStringList({"General", "Appearance", "Network"}));
}

TEST(SettingsPageOrderTest, UnknownSectionsComeAfterKnownOnes) {
  // A module naming a section we do not know still has to end up somewhere
  // predictable rather than being folded into another group.
  const QVector<UI::SettingsPageDescriptor> pages{
      Page("Experimental", "labs"),
      Page("General", "application"),
      Page("Advanced", "system"),
  };

  EXPECT_EQ(
      TitlesOf(OrderSettingsPageDescriptors(pages, UI::SettingsSectionOrder())),
      QStringList({"General", "Advanced", "Experimental"}));
}

TEST(SettingsPageOrderTest, UnknownSectionsStayGroupedInFirstSeenOrder) {
  const QVector<UI::SettingsPageDescriptor> pages{
      Page("Zebra One", "zebra"),     Page("Labs One", "labs"),
      Page("Zebra Two", "zebra"),     Page("Labs Two", "labs"),
      Page("General", "application"),
  };

  EXPECT_EQ(
      TitlesOf(OrderSettingsPageDescriptors(pages, UI::SettingsSectionOrder())),
      QStringList(
          {"General", "Zebra One", "Zebra Two", "Labs One", "Labs Two"}));
}

TEST(SettingsPageOrderTest, EmptyInputStaysEmpty) {
  // Qualified: an empty braced list gives argument-dependent lookup nothing to
  // find the namespace from.
  EXPECT_TRUE(UI::OrderSettingsPageDescriptors({}, UI::SettingsSectionOrder())
                  .isEmpty());
}

TEST(SettingsPageOrderTest, CanonicalSectionsAreTheOnesTheDialogUses) {
  EXPECT_EQ(UI::SettingsSectionOrder(),
            QStringList({"application", "keys_engines", "features", "system"}));
}

}  // namespace GpgFrontend::Test
