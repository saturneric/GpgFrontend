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
#include "ui/UIModuleManager.h"

namespace GpgFrontend::Test {

// The registry a module reaches through GFUIRegisterSettingsPage. The factory
// is never called here: these tests run on a worker thread, where constructing
// a QWidget is not allowed, and the registry only ever stores the pointer.

namespace {

auto NullFactory(void* /*data*/) -> void* { return nullptr; }

auto Registration(const QString& id) -> UI::SettingsPageRegistration {
  UI::SettingsPageRegistration registration;
  registration.id = id;
  registration.section_id = "keys_engines";
  registration.title = "Key Servers";
  // Spelled out: on Qt 5 QStringList derives from QList<QString>, so a bare
  // braced list makes the two inherited operator= overloads ambiguous.
  registration.keywords = QStringList{"keyserver", "hkp"};
  registration.factory = NullFactory;
  return registration;
}

/**
 * @brief Drop everything a test registered.
 *
 * The manager is a singleton shared by the whole binary, so a test that leaves
 * entries behind changes what the next one sees.
 */
void Clear(const QStringList& ids) {
  for (const auto& id : ids) {
    UI::UIModuleManager::GetInstance().UnregisterSettingsPage(id);
  }
}

auto IdsOf(const QList<UI::SettingsPageRegistration>& pages) -> QStringList {
  QStringList ids;
  for (const auto& page : pages) ids << page.id;
  return ids;
}

}  // namespace

TEST(SettingsPageRegistryTest, RegistersAndKeepsRegistrationOrder) {
  auto& manager = UI::UIModuleManager::GetInstance();

  EXPECT_TRUE(manager.RegisterSettingsPage(Registration("test.page.first")));
  EXPECT_TRUE(manager.RegisterSettingsPage(Registration("test.page.second")));

  // Order is the tiebreak between pages sharing a section, so it has to
  // survive.
  const auto ids = IdsOf(manager.ListSettingsPages());
  EXPECT_LT(ids.indexOf("test.page.first"), ids.indexOf("test.page.second"));

  Clear({"test.page.first", "test.page.second"});
}

TEST(SettingsPageRegistryTest, StoresWhatItWasGiven) {
  auto& manager = UI::UIModuleManager::GetInstance();
  ASSERT_TRUE(manager.RegisterSettingsPage(Registration("test.page.fields")));

  const auto pages = manager.ListSettingsPages();
  const auto page = std::find_if(pages.cbegin(), pages.cend(),
                                 [](const UI::SettingsPageRegistration& p) {
                                   return p.id == "test.page.fields";
                                 });
  ASSERT_NE(page, pages.cend());

  EXPECT_EQ(page->section_id, "keys_engines");
  EXPECT_EQ(page->title, "Key Servers");
  EXPECT_EQ(page->keywords, QStringList({"keyserver", "hkp"}));
  EXPECT_EQ(page->factory, &NullFactory);

  Clear({"test.page.fields"});
}

TEST(SettingsPageRegistryTest, DuplicateIdIsRejectedAndFirstOneSurvives) {
  auto& manager = UI::UIModuleManager::GetInstance();
  ASSERT_TRUE(manager.RegisterSettingsPage(Registration("test.page.dup")));

  auto second = Registration("test.page.dup");
  second.title = "Something Else";
  EXPECT_FALSE(manager.RegisterSettingsPage(second));

  // Replacing would strand any dialog already holding a widget from the first
  // factory, so the original has to be what remains.
  const auto pages = manager.ListSettingsPages();
  EXPECT_EQ(IdsOf(pages).count("test.page.dup"), 1);
  const auto page = std::find_if(pages.cbegin(), pages.cend(),
                                 [](const UI::SettingsPageRegistration& p) {
                                   return p.id == "test.page.dup";
                                 });
  ASSERT_NE(page, pages.cend());
  EXPECT_EQ(page->title, "Key Servers");

  Clear({"test.page.dup"});
}

TEST(SettingsPageRegistryTest, IncompleteRegistrationIsRejected) {
  auto& manager = UI::UIModuleManager::GetInstance();

  auto no_id = Registration(QString());
  EXPECT_FALSE(manager.RegisterSettingsPage(no_id));

  auto no_title = Registration("test.page.no_title");
  no_title.title.clear();
  EXPECT_FALSE(manager.RegisterSettingsPage(no_title));

  // A null factory would be a crash the first time the dialog is opened.
  auto no_factory = Registration("test.page.no_factory");
  no_factory.factory = nullptr;
  EXPECT_FALSE(manager.RegisterSettingsPage(no_factory));

  const auto ids = IdsOf(manager.ListSettingsPages());
  EXPECT_FALSE(ids.contains("test.page.no_title"));
  EXPECT_FALSE(ids.contains("test.page.no_factory"));
}

TEST(SettingsPageRegistryTest, UnregisterRemovesOnlyTheNamedPage) {
  auto& manager = UI::UIModuleManager::GetInstance();
  ASSERT_TRUE(manager.RegisterSettingsPage(Registration("test.page.keep")));
  ASSERT_TRUE(manager.RegisterSettingsPage(Registration("test.page.drop")));

  EXPECT_TRUE(manager.UnregisterSettingsPage("test.page.drop"));

  const auto ids = IdsOf(manager.ListSettingsPages());
  EXPECT_FALSE(ids.contains("test.page.drop"));
  EXPECT_TRUE(ids.contains("test.page.keep"));

  Clear({"test.page.keep"});
}

TEST(SettingsPageRegistryTest, UnregisterUnknownIdReportsFailure) {
  auto& manager = UI::UIModuleManager::GetInstance();
  EXPECT_FALSE(manager.UnregisterSettingsPage("test.page.never_registered"));
  EXPECT_FALSE(manager.UnregisterSettingsPage(QString()));
}

TEST(SettingsPageRegistryTest, AnIdCanBeReusedAfterUnregistering) {
  // A module that is deactivated and activated again must be able to come back.
  auto& manager = UI::UIModuleManager::GetInstance();
  ASSERT_TRUE(manager.RegisterSettingsPage(Registration("test.page.cycle")));
  ASSERT_TRUE(manager.UnregisterSettingsPage("test.page.cycle"));
  EXPECT_TRUE(manager.RegisterSettingsPage(Registration("test.page.cycle")));

  Clear({"test.page.cycle"});
}

}  // namespace GpgFrontend::Test
