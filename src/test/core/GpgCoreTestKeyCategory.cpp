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

TEST_F(GFCoreTest, CoreKeyCategoryBuiltinFavourite) {
  auto& repo = KeyCategoryRepository::GetInstance();

  auto cats = repo.Fetch();
  ASSERT_TRUE(
      ContainsCategoryId(cats, KeyCategoryRepository::kFavoriteCategoryId));

  // The built-in favourite category is pinned first and marked built-in.
  ASSERT_FALSE(cats.isEmpty());
  ASSERT_EQ(cats.front().id, KeyCategoryRepository::kFavoriteCategoryId);
  ASSERT_TRUE(cats.front().builtin);

  // Built-ins cannot be removed or renamed.
  ASSERT_FALSE(repo.Remove(KeyCategoryRepository::kFavoriteCategoryId));
  ASSERT_FALSE(repo.Rename(KeyCategoryRepository::kFavoriteCategoryId, "X"));
}

TEST_F(GFCoreTest, CoreKeyCategoryFavouriteMembership) {
  auto& repo = KeyCategoryRepository::GetInstance();

  const QString key_id = "TESTKEYID0001";

  ASSERT_TRUE(repo.SetFavorite(key_id, true));
  ASSERT_TRUE(repo.IsFavorite(key_id));

  // Idempotent add returns false and keeps membership.
  ASSERT_FALSE(repo.SetFavorite(key_id, true));
  ASSERT_TRUE(repo.IsFavorite(key_id));

  ASSERT_TRUE(repo.SetFavorite(key_id, false));
  ASSERT_FALSE(repo.IsFavorite(key_id));
}

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

  // Membership is independent of the favourite category.
  ASSERT_FALSE(repo.IsFavorite(key_a));

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

}  // namespace GpgFrontend::Test
