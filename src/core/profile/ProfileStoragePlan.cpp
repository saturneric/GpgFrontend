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

#include "core/profile/ProfileStoragePlan.h"

#include <QDir>

namespace GpgFrontend {

namespace {

/// The stored tokens.
constexpr auto kTokenAuto = "auto";
constexpr auto kTokenProtectedOnly = "protected_only";
constexpr auto kTokenDisk = "disk";

/// tmpfs and ramfs, from linux/magic.h. Spelled out rather than included: this
/// translation unit is built on every platform, and the header is Linux's.
constexpr quint64 kTmpfsMagic = 0x01021994;
constexpr quint64 kRamfsMagic = 0x858458f6;

/// ext4 and f2fs, likewise from linux/magic.h and likewise spelled out. These
/// are the two filesystems a desktop is likely to have that can carry an
/// fscrypt policy; UBIFS and CephFS can too, and neither is where a temporary
/// directory lives.
constexpr quint64 kExt4Magic = 0xEF53;
constexpr quint64 kF2fsMagic = 0xF2F52010;

/// Below this a session is not worth opening. A small package needs room for
/// the tree, for the second copy the write-back stages beside it, and for what
/// GnuPG adds once an agent starts — a keybox, private-keys-v1.d, a trustdb —
/// which together come to single-digit megabytes.
///
/// Deliberately not larger. A container's /dev/shm is 64 MB by default, and a
/// floor at that figure rejected the only memory-backed storage such a machine
/// has by a single megabyte, silently sending every session to disk.
constexpr qint64 kBudgetFloor = 16LL * 1024 * 1024;

/// A package compresses; a tree of key packets and SQLite pages does not
/// compress much. Six is the conservative reading of that, and it is only
/// reached for packages written before the manifest recorded a real size.
constexpr qint64 kUnknownSizeFactor = 6;

/// Purely a guard against nonsense. Both inputs come off disk, so a truncated
/// or hostile package can claim any size at all, and the answer to that is to
/// stop believing it — not to pretend a real limit exists here.
///
/// The real limit is whether the chosen storage actually has room, which the
/// probe measures and reports per candidate. Deriving this from
/// ProfilePackagePayloadCap() was tried and was wrong: that cap bounds the
/// *compressed* payload held in locked memory during an export, so on a machine
/// with a small RLIMIT_MEMLOCK it capped every budget at 96 MB and quietly
/// under-provisioned any profile over about 38 MB.
constexpr qint64 kBudgetCeiling = 16LL * 1024 * 1024 * 1024;

/// Three halves of the real unpacked size: one for the tree, and a half for
/// what GnuPG adds while the window is open -- a keybox, private-keys-v1.d, a
/// trustdb, a random seed.
///
/// It used to be two and a half, because a write-back staged a second full copy
/// of the tree beside the first before packing it. Packing now reads the live
/// profile, so that copy does not exist and a session no longer needs twice its
/// own size in free space to be able to save itself.
constexpr qint64 kDeclaredSizeNumerator = 3;
constexpr qint64 kDeclaredSizeDenominator = 2;

/// Multiply without wrapping. The inputs come off disk — a truncated or hostile
/// package can claim any size at all — and an overflow here would produce a
/// tiny budget, which reads as "plenty of room" and fails at close time.
///
/// Clamping before the multiply rather than after is what makes that
/// impossible: anything already past the ceiling is going to be clamped to it
/// regardless, so there is nothing to learn from multiplying it out first.
auto SaturatingScale(qint64 value, qint64 numerator, qint64 denominator)
    -> qint64 {
  if (value <= 0) return 0;
  if (value >= kBudgetCeiling) return kBudgetCeiling;

  return value * numerator / denominator;
}

auto Rejection(const StorageCandidate &candidate) -> QString {
  const auto where =
      candidate.path.isEmpty() ? candidate.driver : candidate.path;
  return candidate.reason.isEmpty()
             ? where
             : QString("%1: %2").arg(where, candidate.reason);
}

auto PlanFrom(const StorageCandidate &candidate) -> ProfileStoragePlan {
  ProfileStoragePlan plan;
  plan.path = candidate.path;
  plan.driver = candidate.driver;
  plan.is_volatile = candidate.is_volatile;
  plan.is_encrypted_at_rest = candidate.is_encrypted_at_rest;
  return plan;
}

}  // namespace

auto ProfileStoragePolicyToString(ProfileStoragePolicy policy) -> QString {
  switch (policy) {
    case ProfileStoragePolicy::kPROTECTED_ONLY:
      return kTokenProtectedOnly;
    case ProfileStoragePolicy::kDISK:
      return kTokenDisk;
    case ProfileStoragePolicy::kAUTO:
      break;
  }
  return kTokenAuto;
}

auto ProfileStoragePolicyFromString(const QString &s) -> ProfileStoragePolicy {
  const auto token = s.trimmed().toLower();
  if (token == kTokenProtectedOnly) {
    return ProfileStoragePolicy::kPROTECTED_ONLY;
  }
  if (token == kTokenDisk) return ProfileStoragePolicy::kDISK;
  return ProfileStoragePolicy::kAUTO;
}

auto ResolveProfileStoragePolicy() -> ProfileStoragePolicy {
  // The environment first, and for now the only source: reading it from the
  // launching profile's settings is wired up separately, because working out
  // which profile launched this one is a different question from what the
  // policy means.
  const auto from_env = qEnvironmentVariable(kProfileStoragePolicyEnv);
  if (!from_env.trimmed().isEmpty()) {
    return ProfileStoragePolicyFromString(from_env);
  }

  return ProfileStoragePolicy::kAUTO;
}

auto VolatileStoreSearchPaths(const QString &xdg_runtime_dir,
                              const QString &override_path, uint uid)
    -> QStringList {
  QStringList candidates;
  const auto add = [&candidates](const QString &path) {
    if (path.isEmpty() || candidates.contains(path)) return;
    if (!QDir::isAbsolutePath(path)) return;
    candidates << QDir::cleanPath(path);
  };

  add(override_path.trimmed());

  // The session-scoped tmpfs every desktop already has: 0700, per user, and
  // emptied when the session ends, which is the cleanup this feature would
  // otherwise have to do itself.
  add(xdg_runtime_dir.trimmed());

  // The same directory named directly, for the launches where the variable is
  // not set to begin with — `su -`, a cron job, a service unit without
  // PAMName. Those are exactly the sessions nobody notices are unprotected.
  add(QString("/run/user/%1").arg(uid));

  // Last, because it is shared with POSIX shared-memory segments: filling it
  // breaks unrelated programs, which the per-user runtime directory cannot do.
  // The uid component is ours to create at 0700; /dev/shm itself is not.
  add(QString("/dev/shm/gpgfrontend-%1").arg(uid));

  return candidates;
}

auto EncryptableStoreSearchPaths(const QString &override_path,
                                 const QString &temp_path, uint uid)
    -> QStringList {
  QStringList candidates;
  const auto add = [&candidates](const QString &path) {
    if (path.isEmpty() || candidates.contains(path)) return;
    if (!QDir::isAbsolutePath(path)) return;
    candidates << QDir::cleanPath(path);
  };

  add(override_path.trimmed());

  // The same temporary directory the unprotected fallback would use. That is
  // the point of this rung: it is not another place to put a session, it is
  // that place with a policy on it, so the rung below is the same directory
  // without one.
  if (!temp_path.trimmed().isEmpty()) {
    add(QString("%1/gpgfrontend-%2").arg(temp_path.trimmed()).arg(uid));
  }

  // Because /tmp is a tmpfs on most systemd distributions, and a tmpfs cannot
  // carry an fscrypt policy at all -- so on exactly the machines where the
  // memory-backed rung is unavailable for want of room, the entry above would
  // be unusable too. /var/tmp is required to survive a reboot, so it is on a
  // real filesystem, which is what this needs.
  add(QString("/var/tmp/gpgfrontend-%1").arg(uid));

  return candidates;
}

auto IsRamBackedMagic(quint64 f_type) -> bool {
  return f_type == kTmpfsMagic || f_type == kRamfsMagic;
}

auto IsFscryptCapableMagic(quint64 f_type) -> bool {
  return f_type == kExt4Magic || f_type == kF2fsMagic;
}

auto IsAcceptableOwnership(uint owner_uid, uint mode, bool is_symlink,
                           uint euid) -> bool {
  // A symlink is refused even when it points somewhere fine: what it points at
  // can change between this check and the write.
  if (is_symlink) return false;
  if (owner_uid != euid) return false;
  return (mode & 0777U) == 0700U;
}

auto PlanProfileStorage(ProfileStoragePolicy policy,
                        const QList<StorageCandidate> &candidates)
    -> ProfileStoragePlan {
  ProfileStoragePlan plan;

  const StorageCandidate *unprotected = nullptr;

  for (const auto &candidate : candidates) {
    if (!candidate.usable) {
      plan.rejections << Rejection(candidate);
      continue;
    }

    if (policy == ProfileStoragePolicy::kDISK) {
      // Asked for the plain folder by name. A protected candidate is not a
      // better answer to that question, it is a different one.
      if (candidate.IsProtected()) continue;
      auto chosen = PlanFrom(candidate);
      chosen.rejections = plan.rejections;
      return chosen;
    }

    if (candidate.IsProtected()) {
      auto chosen = PlanFrom(candidate);
      chosen.rejections = plan.rejections;
      return chosen;
    }

    // Remembered rather than taken: a protected candidate further down the list
    // still wins under kAUTO, and under kPROTECTED_ONLY this one never will.
    if (unprotected == nullptr) unprotected = &candidate;
  }

  if (policy == ProfileStoragePolicy::kPROTECTED_ONLY) {
    if (unprotected != nullptr) {
      plan.rejections << Rejection(*unprotected);
    }
    plan.refuse = true;
    return plan;
  }

  if (unprotected != nullptr) {
    auto chosen = PlanFrom(*unprotected);
    chosen.rejections = plan.rejections;
    return chosen;
  }

  // Nothing at all was usable, including the plain folder. That is a broken
  // machine rather than a policy outcome, but the caller needs the same answer.
  plan.refuse = true;
  return plan;
}

auto ProfileStorageBudget(qint64 package_bytes, qint64 declared_uncompressed)
    -> qint64 {
  const auto wanted =
      declared_uncompressed > 0
          ? SaturatingScale(declared_uncompressed, kDeclaredSizeNumerator,
                            kDeclaredSizeDenominator)
          : SaturatingScale(package_bytes, kUnknownSizeFactor, 1);

  return qBound(kBudgetFloor, wanted, kBudgetCeiling);
}

}  // namespace GpgFrontend
