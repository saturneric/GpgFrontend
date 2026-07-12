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

#include "GFCoreTest.h"
#include "core/function/openpgp/KeyCategoryRepository.h"

namespace GpgFrontend::Test {

namespace {

auto ContainsCategoryId(const QContainer<KeyCategoryCO>& cats,
                        const QString& id) -> bool {
  for (const auto& c : cats) {
    if (c.id == id) return true;
  }
  return false;
}

}  // namespace

TEST_F(GFCoreTest, CoreKeyCategoryCustomLifecycle) {
  auto& repo = KeyCategoryRepository::GetInstance();

  auto id = repo.AddCategory("Team A", "#3366cc");
  ASSERT_FALSE(id.isEmpty());
  ASSERT_TRUE(ContainsCategoryId(repo.Fetch(), id));

  const QString key_a = "TESTKEYID000A";
  const QString key_b = "TESTKEYID000B";

  ASSERT_TRUE(repo.AddKey2Category(id, key_a));
  ASSERT_TRUE(repo.AddKey2Category(id, key_b));
  ASSERT_FALSE(repo.AddKey2Category(id, key_a));  // duplicate

  ASSERT_TRUE(repo.Contains(id, key_a));
  ASSERT_EQ(repo.KeyIdsOf(id).size(), 2);

  ASSERT_TRUE(repo.RemoveKeyFromCategory(id, key_a));
  ASSERT_FALSE(repo.Contains(id, key_a));

  ASSERT_TRUE(repo.Rename(id, "Team B"));
  ASSERT_TRUE(repo.Remove(id));
  ASSERT_FALSE(ContainsCategoryId(repo.Fetch(), id));
}

TEST_F(GFCoreTest, CoreKeyCategoryPersistAcrossFlush) {
  auto& repo = KeyCategoryRepository::GetInstance();

  auto id = repo.AddCategory("Persisted");
  const QString key_id = "TESTKEYIDPERSIST";
  ASSERT_TRUE(repo.AddKey2Category(id, key_id));

  // Reload from durable cache and confirm the state survived.
  repo.FlushCache();
  ASSERT_TRUE(ContainsCategoryId(repo.Fetch(), id));
  ASSERT_TRUE(repo.Contains(id, key_id));

  ASSERT_TRUE(repo.Remove(id));
}

TEST_F(GFCoreTest, CoreKeyCategoryTabColor) {
  auto& repo = KeyCategoryRepository::GetInstance();

  // No override initially for a built-in tab id.
  ASSERT_TRUE(repo.GetTabColor("default").isEmpty());

  repo.SetTabColor("default", "#112233");
  ASSERT_EQ(repo.GetTabColor("default"), "#112233");

  // A custom category's own colour is returned when there is no override.
  auto id = repo.AddCategory("Coloured", "#abcdef");
  ASSERT_EQ(repo.GetTabColor(id), "#abcdef");
  repo.SetTabColor(id, "#020202");
  ASSERT_EQ(repo.GetTabColor(id), "#020202");

  // Overrides survive a cache reload; clearing resets to empty/own colour.
  repo.FlushCache();
  ASSERT_EQ(repo.GetTabColor("default"), "#112233");
  ASSERT_EQ(repo.GetTabColor(id), "#020202");

  repo.SetTabColor("default", QString{});
  ASSERT_TRUE(repo.GetTabColor("default").isEmpty());

  ASSERT_TRUE(repo.Remove(id));
}

TEST_F(GFCoreTest, CoreKeyCategoryTabOrder) {
  auto& repo = KeyCategoryRepository::GetInstance();

  ASSERT_TRUE(repo.GetTabOrder("scope_x").isEmpty());

  const QStringList order = {"a", "b", "c"};
  repo.SetTabOrder("scope_x", order);
  ASSERT_EQ(repo.GetTabOrder("scope_x"), order);

  // Distinct scopes are independent and survive a reload.
  repo.SetTabOrder("scope_y", QStringList{"z"});
  repo.FlushCache();
  ASSERT_EQ(repo.GetTabOrder("scope_x"), order);
  ASSERT_EQ(repo.GetTabOrder("scope_y"), QStringList{"z"});
}

}  // namespace GpgFrontend::Test
