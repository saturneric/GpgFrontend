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

#include "GpgCoreTest.h"
#include "GpgFrontendTest.h"
#include "core/function/openpgp/KeyGroupRepository.h"

namespace GpgFrontend::Test {

namespace {

// KeyGroupRepository binds a reference to its channel's OpenPGPContext at
// construction, so it must not be instantiated before the unit-test context
// exists. That is why these live in the GpgCoreTest fixture, which builds the
// context in SetUpTestSuite(), rather than in the plain core fixture.
auto Repo() -> KeyGroupRepository& {
  return KeyGroupRepository::GetInstance(kGpgChannelForUnitTest);
}

// Create an empty key group and return its generated id. Every test here nests
// key groups inside key groups, so no keys need to exist in the test keyring.
auto MakeKeyGroup(KeyGroupRepository& repo, const QString& name) -> QString {
  GpgKeyGroup group(name, {}, {}, {});
  auto id = group.ID();
  repo.AddKeyGroup(group);
  return id;
}

auto NestKeyGroup(KeyGroupRepository& repo, const QString& parent_id,
                  const QString& child_id) -> bool {
  auto child = repo.KeyGroup(child_id);
  if (child == nullptr) return false;
  return repo.AddKey2KeyGroup(parent_id, child);
}

}  // namespace

TEST_F(GpgCoreTest, CoreKeyGroupMetadataUpdate) {
  auto& repo = Repo();

  auto id = MakeKeyGroup(repo, "Original Name");
  ASSERT_NE(repo.KeyGroup(id), nullptr);

  ASSERT_TRUE(repo.UpdateMetadata(id, "New Name", "new@example.com", "note"));
  ASSERT_EQ(repo.KeyGroup(id)->Name(), "New Name");
  ASSERT_EQ(repo.KeyGroup(id)->Email(), "new@example.com");
  ASSERT_EQ(repo.KeyGroup(id)->Comment(), "note");

  ASSERT_TRUE(repo.Rename(id, "Renamed"));
  ASSERT_EQ(repo.KeyGroup(id)->Name(), "Renamed");

  // The metadata must survive a round trip through the durable cache.
  repo.FlushCache();
  ASSERT_EQ(repo.KeyGroup(id)->Name(), "Renamed");
  ASSERT_EQ(repo.KeyGroup(id)->Email(), "new@example.com");

  ASSERT_FALSE(repo.UpdateMetadata("#&NOPE", "X Name", {}, {}));
  ASSERT_FALSE(repo.Rename("#&NOPE", "X Name"));

  // A blank name is rejected and leaves the old one intact.
  ASSERT_FALSE(repo.Rename(id, "   "));
  ASSERT_FALSE(repo.UpdateMetadata(id, "", "other@example.com", {}));
  ASSERT_EQ(repo.KeyGroup(id)->Name(), "Renamed");
  ASSERT_EQ(repo.KeyGroup(id)->Email(), "new@example.com");
}

TEST_F(GpgCoreTest, CoreKeyGroupNestingRejectsCycles) {
  auto& repo = Repo();

  auto a = MakeKeyGroup(repo, "Cycle Group A");
  auto b = MakeKeyGroup(repo, "Cycle Group B");
  auto c = MakeKeyGroup(repo, "Cycle Group C");

  ASSERT_TRUE(NestKeyGroup(repo, a, b));
  ASSERT_TRUE(NestKeyGroup(repo, b, c));

  ASSERT_TRUE(repo.IsAncestorOf(a, c));
  ASSERT_TRUE(repo.IsAncestorOf(a, b));
  ASSERT_FALSE(repo.IsAncestorOf(c, a));

  // Closing the loop would create a cycle, and adding a group to itself is
  // never legal.
  ASSERT_FALSE(repo.CanAddKeyToKeyGroup(c, a));
  ASSERT_FALSE(repo.CanAddKeyToKeyGroup(a, a));
  ASSERT_FALSE(NestKeyGroup(repo, c, a));

  // Already a direct member.
  ASSERT_FALSE(repo.CanAddKeyToKeyGroup(a, b));
  // But a is not yet a direct member of c's parent chain in this direction.
  ASSERT_TRUE(repo.CanAddKeyToKeyGroup(a, c));
}

TEST_F(GpgCoreTest, CoreKeyGroupDagParentsAreAllowed) {
  auto& repo = Repo();

  auto a = MakeKeyGroup(repo, "Dag Group A");
  auto b = MakeKeyGroup(repo, "Dag Group B");
  auto c = MakeKeyGroup(repo, "Dag Group C");

  ASSERT_TRUE(NestKeyGroup(repo, a, c));
  ASSERT_TRUE(NestKeyGroup(repo, b, c));

  auto parents = repo.ParentsOf(c);
  ASSERT_EQ(parents.size(), 2);
  ASSERT_TRUE(parents.contains(a));
  ASSERT_TRUE(parents.contains(b));

  ASSERT_TRUE(repo.KeyGroup(a)->KeyIds().contains(c));
  ASSERT_TRUE(repo.KeyGroup(b)->KeyIds().contains(c));
}

TEST_F(GpgCoreTest, CoreKeyGroupContainsIsDirectMembershipOnly) {
  auto& repo = Repo();

  auto a = MakeKeyGroup(repo, "Direct Group A");
  auto b = MakeKeyGroup(repo, "Direct Group B");
  auto c = MakeKeyGroup(repo, "Direct Group C");

  ASSERT_TRUE(NestKeyGroup(repo, a, b));
  ASSERT_TRUE(NestKeyGroup(repo, b, c));

  ASSERT_TRUE(repo.Contains(a, b));
  ASSERT_TRUE(repo.Contains(b, c));
  // c is reachable from a, but only through b, so it is not contained.
  ASSERT_FALSE(repo.Contains(a, c));

  ASSERT_FALSE(repo.Contains("#&NOPE", c));
}

TEST_F(GpgCoreTest, CoreKeyGroupRemoveDetachesFromParentsAndChildren) {
  auto& repo = Repo();

  auto a = MakeKeyGroup(repo, "Detach Group A");
  auto b = MakeKeyGroup(repo, "Detach Group B");
  auto c = MakeKeyGroup(repo, "Detach Group C");

  ASSERT_TRUE(NestKeyGroup(repo, a, b));
  ASSERT_TRUE(NestKeyGroup(repo, b, c));

  ASSERT_TRUE(repo.Remove(b));
  ASSERT_EQ(repo.KeyGroup(b), nullptr);
  ASSERT_FALSE(repo.KeyGroup(a)->KeyIds().contains(b));

  // c must no longer point back at the freed node: a stale back-pointer here
  // is walked by the cycle check on the next nesting attempt.
  ASSERT_TRUE(repo.ParentsOf(c).isEmpty());
  ASSERT_TRUE(NestKeyGroup(repo, a, c));
  ASSERT_TRUE(repo.Contains(a, c));
}

TEST_F(GpgCoreTest, CoreKeyGroupRemoveUnknownIsHarmless) {
  auto& repo = Repo();

  auto before = repo.Fetch().size();
  ASSERT_FALSE(repo.Remove(""));
  ASSERT_FALSE(repo.Remove("#&NOPE"));
  ASSERT_EQ(repo.Fetch().size(), before);

  ASSERT_TRUE(repo.ParentsOf("#&NOPE").isEmpty());
  ASSERT_FALSE(repo.IsAncestorOf("#&NOPE", "#&ALSONOPE"));
  ASSERT_FALSE(repo.CanAddKeyToKeyGroup("#&NOPE", "SOMEKEYID"));
}

}  // namespace GpgFrontend::Test
