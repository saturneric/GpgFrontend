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

#include <limits>

#include "core/profile/ProfileStoragePlan.h"

namespace GpgFrontend::Test {

// Everything here is pure. Nothing in this file provisions storage, spawns a
// tool, mounts anything, or reads the environment — which is the point of
// having split the decisions out of the driver in the first place. A machine
// with no tmpfs, no NTFS and no disk image support still exercises every branch
// a machine with all three would.

namespace {

auto Volatile(const QString &path, bool usable = true, qint64 free_bytes = 0)
    -> StorageCandidate {
  StorageCandidate candidate;
  candidate.path = path;
  candidate.driver = "fs-tmpfs";
  candidate.is_volatile = true;
  candidate.usable = usable;
  candidate.free_bytes = free_bytes;
  if (!usable) candidate.reason = "not a RAM-backed filesystem";
  return candidate;
}

auto Encrypted(const QString &path, bool usable = true) -> StorageCandidate {
  StorageCandidate candidate;
  candidate.path = path;
  candidate.driver = "fs-efs";
  candidate.is_encrypted_at_rest = true;
  candidate.usable = usable;
  if (!usable) candidate.reason = "this volume does not support encryption";
  return candidate;
}

auto Plain(const QString &path, const QString &driver = "fs-temp")
    -> StorageCandidate {
  StorageCandidate candidate;
  candidate.path = path;
  candidate.driver = driver;
  candidate.usable = true;
  return candidate;
}

}  // namespace

TEST(ProfileStoragePlanTest, PolicyTokensRoundTrip) {
  for (const auto policy :
       {ProfileStoragePolicy::kAUTO, ProfileStoragePolicy::kPROTECTED_ONLY,
        ProfileStoragePolicy::kDISK}) {
    EXPECT_EQ(
        ProfileStoragePolicyFromString(ProfileStoragePolicyToString(policy)),
        policy);
  }
}

TEST(ProfileStoragePlanTest, UnknownPolicyTokenReadsAsAuto) {
  // A settings file written by a newer build must never be able to stop this
  // one from opening a package: refusing is the one outcome a typo may not
  // cause.
  for (const auto &token :
       {"", " ", "volatile_only", "ramdisk", "PROTECTED", "nonsense"}) {
    EXPECT_EQ(ProfileStoragePolicyFromString(token),
              ProfileStoragePolicy::kAUTO)
        << "token: " << token;
  }
}

TEST(ProfileStoragePlanTest, PolicyTokenIgnoresCaseAndSurroundingSpace) {
  EXPECT_EQ(ProfileStoragePolicyFromString("  Protected_Only "),
            ProfileStoragePolicy::kPROTECTED_ONLY);
  EXPECT_EQ(ProfileStoragePolicyFromString("DISK"),
            ProfileStoragePolicy::kDISK);
}

TEST(ProfileStoragePlanTest, AutoTakesTheFirstProtectedCandidate) {
  const auto plan = PlanProfileStorage(
      ProfileStoragePolicy::kAUTO,
      {Volatile("/run/user/1000"), Plain("/tmp"), Plain("/profiles", "fs")});

  EXPECT_FALSE(plan.refuse);
  EXPECT_EQ(plan.path, "/run/user/1000");
  EXPECT_TRUE(plan.is_volatile);
  EXPECT_FALSE(plan.is_encrypted_at_rest);
  EXPECT_TRUE(plan.rejections.isEmpty());
}

TEST(ProfileStoragePlanTest, AutoPrefersAProtectedCandidateListedLater) {
  // The plain temporary directory is always usable, so taking candidates purely
  // in order would mean a platform whose protected option sits behind it never
  // uses that option at all.
  const auto plan = PlanProfileStorage(
      ProfileStoragePolicy::kAUTO, {Plain("/tmp"), Encrypted("/localtemp")});

  EXPECT_FALSE(plan.refuse);
  EXPECT_EQ(plan.path, "/localtemp");
  EXPECT_TRUE(plan.is_encrypted_at_rest);
}

TEST(ProfileStoragePlanTest, AutoFallsBackAndRecordsEveryRejection) {
  const auto plan =
      PlanProfileStorage(ProfileStoragePolicy::kAUTO,
                         {Volatile("/run/user/1000", false),
                          Encrypted("/localtemp", false), Plain("/tmp")});

  EXPECT_FALSE(plan.refuse);
  EXPECT_EQ(plan.path, "/tmp");
  EXPECT_FALSE(plan.is_volatile);
  EXPECT_FALSE(plan.is_encrypted_at_rest);

  ASSERT_EQ(plan.rejections.size(), 2);
  EXPECT_EQ(plan.rejections.at(0),
            "/run/user/1000: not a RAM-backed filesystem");
  EXPECT_EQ(plan.rejections.at(1),
            "/localtemp: this volume does not support encryption");
}

TEST(ProfileStoragePlanTest, ProtectedOnlyAcceptsAVolatileCandidate) {
  const auto plan =
      PlanProfileStorage(ProfileStoragePolicy::kPROTECTED_ONLY,
                         {Volatile("/run/user/1000"), Plain("/tmp")});

  EXPECT_FALSE(plan.refuse);
  EXPECT_EQ(plan.path, "/run/user/1000");
}

TEST(ProfileStoragePlanTest, ProtectedOnlyAcceptsAnEncryptedCandidate) {
  // The reason the two axes are separate: macOS and Windows never produce a
  // volatile candidate, and a policy that only understood volatility would be a
  // permanent refusal on both.
  const auto plan =
      PlanProfileStorage(ProfileStoragePolicy::kPROTECTED_ONLY,
                         {Encrypted("/localtemp"), Plain("/tmp")});

  EXPECT_FALSE(plan.refuse);
  EXPECT_EQ(plan.path, "/localtemp");
  EXPECT_TRUE(plan.is_encrypted_at_rest);
  EXPECT_FALSE(plan.is_volatile);
}

TEST(ProfileStoragePlanTest, ProtectedOnlyRefusesRatherThanUseAPlainFolder) {
  const auto plan =
      PlanProfileStorage(ProfileStoragePolicy::kPROTECTED_ONLY,
                         {Volatile("/run/user/1000", false), Plain("/tmp"),
                          Plain("/profiles", "fs")});

  EXPECT_TRUE(plan.refuse);
  EXPECT_TRUE(plan.path.isEmpty());
  EXPECT_FALSE(plan.rejections.isEmpty());
}

TEST(ProfileStoragePlanTest, ProtectedOnlyReportsWhatItPassedOver) {
  // The refusal dialog shows these verbatim. "This machine cannot" is not
  // something a user can act on; "8 MB free, 96 MB needed" is.
  auto tight = Volatile("/run/user/1000", false);
  tight.reason = "8 MB free, 96 MB needed";

  const auto plan = PlanProfileStorage(ProfileStoragePolicy::kPROTECTED_ONLY,
                                       {tight, Plain("/tmp")});

  ASSERT_TRUE(plan.refuse);
  ASSERT_EQ(plan.rejections.size(), 2);
  EXPECT_EQ(plan.rejections.at(0), "/run/user/1000: 8 MB free, 96 MB needed");
  EXPECT_TRUE(plan.rejections.at(1).startsWith("/tmp"));
}

TEST(ProfileStoragePlanTest, DiskIgnoresAUsableProtectedCandidate) {
  const auto plan =
      PlanProfileStorage(ProfileStoragePolicy::kDISK,
                         {Volatile("/run/user/1000"), Encrypted("/localtemp"),
                          Plain("/profiles", "fs")});

  EXPECT_FALSE(plan.refuse);
  EXPECT_EQ(plan.path, "/profiles");
  EXPECT_EQ(plan.driver, "fs");
  EXPECT_FALSE(plan.is_volatile);
  EXPECT_FALSE(plan.is_encrypted_at_rest);
}

TEST(ProfileStoragePlanTest, NothingUsableAtAllRefusesUnderEveryPolicy) {
  auto broken = Plain("/profiles", "fs");
  broken.usable = false;
  broken.reason = "the folder could not be made";

  for (const auto policy :
       {ProfileStoragePolicy::kAUTO, ProfileStoragePolicy::kPROTECTED_ONLY,
        ProfileStoragePolicy::kDISK}) {
    const auto plan = PlanProfileStorage(policy, {broken});
    EXPECT_TRUE(plan.refuse);
    EXPECT_TRUE(plan.path.isEmpty());
    ASSERT_EQ(plan.rejections.size(), 1);
    EXPECT_EQ(plan.rejections.at(0), "/profiles: the folder could not be made");
  }
}

TEST(ProfileStoragePlanTest, NoCandidatesRefuses) {
  const auto plan = PlanProfileStorage(ProfileStoragePolicy::kAUTO, {});
  EXPECT_TRUE(plan.refuse);
  EXPECT_TRUE(plan.path.isEmpty());
}

TEST(ProfileStoragePlanTest, RamBackedMagicAcceptsOnlyMemoryFilesystems) {
  // The single check the whole Linux side rests on. Getting it wrong either
  // disables the feature silently, or — far worse — accepts a real disk while
  // telling the user the session is in memory.
  EXPECT_TRUE(IsRamBackedMagic(0x01021994));  // TMPFS_MAGIC
  EXPECT_TRUE(IsRamBackedMagic(0x858458f6));  // RAMFS_MAGIC

  EXPECT_FALSE(IsRamBackedMagic(0xEF53));      // EXT2/3/4
  EXPECT_FALSE(IsRamBackedMagic(0x9123683E));  // BTRFS
  EXPECT_FALSE(IsRamBackedMagic(0x58465342));  // XFS
  EXPECT_FALSE(IsRamBackedMagic(0x794c7630));  // OVERLAYFS
  EXPECT_FALSE(IsRamBackedMagic(0x6969));      // NFS
  EXPECT_FALSE(IsRamBackedMagic(0xFF534D42));  // CIFS
  EXPECT_FALSE(IsRamBackedMagic(0x4d44));      // MSDOS
  EXPECT_FALSE(IsRamBackedMagic(0x2FC12FC1));  // ZFS
  EXPECT_FALSE(IsRamBackedMagic(0));
}

TEST(ProfileStoragePlanTest, OwnershipRefusesAnythingNotOursAndPrivate) {
  constexpr uint kUs = 1000;
  constexpr uint kThem = 1001;

  EXPECT_TRUE(IsAcceptableOwnership(kUs, 0700, false, kUs));

  EXPECT_FALSE(IsAcceptableOwnership(kUs, 0755, false, kUs));
  EXPECT_FALSE(IsAcceptableOwnership(kUs, 0770, false, kUs));
  EXPECT_FALSE(IsAcceptableOwnership(kUs, 0777, false, kUs));
  EXPECT_FALSE(IsAcceptableOwnership(kThem, 0700, false, kUs));
  EXPECT_FALSE(IsAcceptableOwnership(0, 0700, false, kUs));

  // Refused even when it currently points somewhere acceptable: what it points
  // at can change between this check and the write.
  EXPECT_FALSE(IsAcceptableOwnership(kUs, 0700, true, kUs));
}

TEST(ProfileStoragePlanTest, OwnershipIgnoresBitsAboveThePermissions) {
  // Callers pass st_mode straight through, so the file-type bits ride along.
  constexpr uint kDirBit = 0040000;
  EXPECT_TRUE(IsAcceptableOwnership(1000, kDirBit | 0700, false, 1000));
}

TEST(ProfileStoragePlanTest, SearchPathsAreOrderedMostSpecificFirst) {
  // The override first, because it is the only way to try a fix on someone
  // else's machine; /dev/shm last, because filling it breaks unrelated programs
  // in a way the per-user runtime directory cannot.
  const auto paths =
      VolatileStoreSearchPaths("/run/user/1000", "/mnt/ram", 1000);

  ASSERT_EQ(paths.size(), 3);
  EXPECT_EQ(paths.at(0), "/mnt/ram");
  EXPECT_EQ(paths.at(1), "/run/user/1000");
  EXPECT_EQ(paths.at(2), "/dev/shm/gpgfrontend-1000");
}

TEST(ProfileStoragePlanTest, SearchPathsKeepTheLiteralRuntimeDirWhenItDiffers) {
  // A session whose XDG_RUNTIME_DIR points somewhere unusual still gets the
  // conventional location as a second chance.
  const auto paths = VolatileStoreSearchPaths("/tmp/custom-runtime", {}, 1000);

  ASSERT_EQ(paths.size(), 3);
  EXPECT_EQ(paths.at(0), "/tmp/custom-runtime");
  EXPECT_EQ(paths.at(1), "/run/user/1000");
  EXPECT_EQ(paths.at(2), "/dev/shm/gpgfrontend-1000");
}

TEST(ProfileStoragePlanTest, SearchPathsDeduplicate) {
  // The literal /run/user/<uid> candidate exists for the launches where
  // XDG_RUNTIME_DIR is unset; when it is set to the same place it must not be
  // probed twice.
  const auto paths = VolatileStoreSearchPaths("/run/user/1000", {}, 1000);

  ASSERT_EQ(paths.size(), 2);
  EXPECT_EQ(paths.at(0), "/run/user/1000");
  EXPECT_EQ(paths.at(1), "/dev/shm/gpgfrontend-1000");
}

TEST(ProfileStoragePlanTest, SearchPathsSubstituteWhenTheVariableIsUnset) {
  const auto paths = VolatileStoreSearchPaths({}, {}, 501);

  ASSERT_EQ(paths.size(), 2);
  EXPECT_EQ(paths.at(0), "/run/user/501");
  EXPECT_EQ(paths.at(1), "/dev/shm/gpgfrontend-501");
}

TEST(ProfileStoragePlanTest, SearchPathsIgnoreRelativeInput) {
  // A relative value would name whatever directory the process happens to sit
  // in, which for a GUI launched from a file manager is anyone's guess.
  const auto paths = VolatileStoreSearchPaths("run/user/1000", "../ram", 1000);

  ASSERT_EQ(paths.size(), 2);
  EXPECT_EQ(paths.at(0), "/run/user/1000");
  EXPECT_EQ(paths.at(1), "/dev/shm/gpgfrontend-1000");
}

TEST(ProfileStoragePlanTest, SearchPathsCleanTheirInput) {
  const auto paths = VolatileStoreSearchPaths("/run/user/1000/", {}, 1000);
  EXPECT_EQ(paths.at(0), "/run/user/1000");
}

TEST(ProfileStoragePlanTest, BudgetPrefersTheDeclaredSize) {
  constexpr qint64 kMb = 1024 * 1024;

  // Two and a half times the real unpacked size, once that is past the floor.
  EXPECT_EQ(ProfileStorageBudget(20 * kMb, 100 * kMb), 250 * kMb);

  // The declared size wins outright: a package that compresses unusually well
  // would otherwise be handed a budget built from its compressed size.
  EXPECT_EQ(ProfileStorageBudget(1 * kMb, 100 * kMb), 250 * kMb);
}

TEST(ProfileStoragePlanTest, BudgetFallsBackToTheHeuristic) {
  constexpr qint64 kMb = 1024 * 1024;

  // Packages written before the manifest recorded an unpacked size.
  EXPECT_EQ(ProfileStorageBudget(50 * kMb, 0), 300 * kMb);

  // And a small one lands on the floor rather than below it.
  EXPECT_EQ(ProfileStorageBudget(1 * kMb, 0), 16 * kMb);
}

TEST(ProfileStoragePlanTest, BudgetHasAFloor) {
  constexpr qint64 kMb = 1024 * 1024;
  constexpr qint64 kFloor = 16 * kMb;

  // A tiny package still needs room to be a working session: the tree, the
  // copy the write-back stages beside it, and what GnuPG adds once an agent
  // starts. Kept small enough that a container's 64 MB /dev/shm still passes —
  // a larger floor rejected the only memory-backed storage such a machine has.
  EXPECT_EQ(ProfileStorageBudget(1024, 0), kFloor);
  EXPECT_EQ(ProfileStorageBudget(1024, 2048), kFloor);
  EXPECT_EQ(ProfileStorageBudget(0, 0), kFloor);
}

TEST(ProfileStoragePlanTest, BudgetHasACeiling) {
  constexpr qint64 kCeiling = 16LL * 1024 * 1024 * 1024;

  // The ceiling is a guard against nonsense, not a real limit: whether the
  // storage has room is the probe's question, and it answers it per candidate.
  // It is deliberately far above any real profile, so a genuinely large one is
  // budgeted honestly and then rejected by a candidate that cannot hold it,
  // rather than being handed a budget that silently does not fit.
  EXPECT_EQ(ProfileStorageBudget(1LL << 40, 0), kCeiling);
  EXPECT_EQ(ProfileStorageBudget(0, 1LL << 40), kCeiling);
}

TEST(ProfileStoragePlanTest, BudgetIsNotCappedByTheExportPayloadCap) {
  constexpr qint64 kMb = 1024 * 1024;

  // Regression guard. This ceiling was once ProfilePackagePayloadCap() * 6,
  // which is derived from RLIMIT_MEMLOCK and falls back to a 16 MB floor on an
  // ordinary machine -- capping every budget at 96 MB and under-provisioning
  // any profile over about 38 MB. The two limits are unrelated: that cap bounds
  // the compressed payload held in locked memory during an export.
  EXPECT_EQ(ProfileStorageBudget(0, 500 * kMb), 1250 * kMb);
  EXPECT_GT(ProfileStorageBudget(0, 1024 * kMb), 1024 * kMb);
}

TEST(ProfileStoragePlanTest, BudgetSurvivesAbsurdInput) {
  // Both numbers come off disk, and a truncated or hostile package can claim
  // anything. An overflow here would produce a small budget, which reads as
  // "plenty of room" and then fails at close time — the worst moment.
  constexpr qint64 kCeiling = 16LL * 1024 * 1024 * 1024;
  constexpr qint64 kFloor = 16LL * 1024 * 1024;
  constexpr auto kHuge = std::numeric_limits<qint64>::max();

  EXPECT_EQ(ProfileStorageBudget(kHuge, kHuge), kCeiling);
  EXPECT_EQ(ProfileStorageBudget(kHuge, 0), kCeiling);
  EXPECT_EQ(ProfileStorageBudget(0, kHuge), kCeiling);

  EXPECT_EQ(ProfileStorageBudget(-1, -1), kFloor);
  EXPECT_EQ(ProfileStorageBudget(-1, 0), kFloor);
  EXPECT_EQ(ProfileStorageBudget(kHuge, -1), kCeiling);
}

}  // namespace GpgFrontend::Test
