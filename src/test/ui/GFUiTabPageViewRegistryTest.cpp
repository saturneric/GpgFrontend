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

// The registry a module reaches through GFUIRegisterTabPageView. As with the
// settings page registry, the factory is never called here: these tests run on
// a worker thread, where constructing a QWidget is not allowed, and the
// registry only ever stores the pointer.

namespace {

auto NullFactory(void* /*data*/) -> void* { return nullptr; }

auto Registration(const QString& type) -> UI::TabPageViewRegistration {
  UI::TabPageViewRegistration registration;
  registration.tab_type = type;
  registration.factory = NullFactory;
  return registration;
}

/// The manager is a singleton shared by the whole binary, so a test that
/// leaves entries behind changes what the next one sees.
void Clear(const QStringList& types) {
  for (const auto& type : types) {
    UI::UIModuleManager::GetInstance().UnregisterTabPageView(type);
  }
}

}  // namespace

TEST(GFUiTabPageViewRegistryTest, RegistersAndFindsAView) {
  auto& manager = UI::UIModuleManager::GetInstance();
  Clear({"EMAIL"});

  EXPECT_TRUE(manager.RegisterTabPageView(Registration("EMAIL")));

  const auto found = manager.TabPageViewFor("EMAIL");
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->tab_type, QString("EMAIL"));
  EXPECT_EQ(found->factory, &NullFactory);

  Clear({"EMAIL"});
}

// Tab types reach this from page->property("type"), which the module spells in
// whatever case it likes -- "email" at the call site, "EMAIL" in the event id.
TEST(GFUiTabPageViewRegistryTest, TypeMatchingIgnoresCase) {
  auto& manager = UI::UIModuleManager::GetInstance();
  Clear({"EMAIL"});

  ASSERT_TRUE(manager.RegisterTabPageView(Registration("email")));

  EXPECT_TRUE(manager.TabPageViewFor("EMAIL").has_value());
  EXPECT_TRUE(manager.TabPageViewFor("email").has_value());
  EXPECT_TRUE(manager.TabPageViewFor("EmAiL").has_value());

  Clear({"EMAIL"});
}

// Rejected rather than replaced: a tab that is already open holds a widget
// built by the current factory, and swapping the registration under it would
// leave that tab pointing at a view nobody owns any more.
TEST(GFUiTabPageViewRegistryTest, DuplicateTypeIsRejected) {
  auto& manager = UI::UIModuleManager::GetInstance();
  Clear({"EMAIL"});

  ASSERT_TRUE(manager.RegisterTabPageView(Registration("EMAIL")));
  EXPECT_FALSE(manager.RegisterTabPageView(Registration("EMAIL")));
  EXPECT_FALSE(manager.RegisterTabPageView(Registration("email")));

  Clear({"EMAIL"});
}

TEST(GFUiTabPageViewRegistryTest, IncompleteRegistrationIsRejected) {
  auto& manager = UI::UIModuleManager::GetInstance();

  EXPECT_FALSE(manager.RegisterTabPageView(Registration("")));

  auto no_factory = Registration("NOFACTORY");
  no_factory.factory = nullptr;
  EXPECT_FALSE(manager.RegisterTabPageView(no_factory));

  EXPECT_FALSE(manager.TabPageViewFor("NOFACTORY").has_value());
}

// Modules must unregister before unloading: a factory pointing into an
// unloaded shared object would crash the next time such a tab is opened.
TEST(GFUiTabPageViewRegistryTest, UnregisterRemovesTheView) {
  auto& manager = UI::UIModuleManager::GetInstance();
  Clear({"EMAIL"});

  ASSERT_TRUE(manager.RegisterTabPageView(Registration("EMAIL")));
  EXPECT_TRUE(manager.UnregisterTabPageView("email"));
  EXPECT_FALSE(manager.TabPageViewFor("EMAIL").has_value());

  // Removing something that was never there is not an error worth throwing.
  EXPECT_FALSE(manager.UnregisterTabPageView("EMAIL"));
  EXPECT_FALSE(manager.UnregisterTabPageView(""));
}

// A tab type no module has claimed must keep behaving exactly as it did before
// module views existed: the host builds an ordinary plain text tab.
TEST(GFUiTabPageViewRegistryTest, UnclaimedTypeHasNoView) {
  auto& manager = UI::UIModuleManager::GetInstance();

  EXPECT_FALSE(manager.TabPageViewFor("text").has_value());
  EXPECT_FALSE(manager.TabPageViewFor("file").has_value());
  EXPECT_FALSE(manager.TabPageViewFor("").has_value());
}

}  // namespace GpgFrontend::Test
