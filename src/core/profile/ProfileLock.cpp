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

#include "ProfileLock.h"

#include <QLockFile>
#include <memory>
#include <mutex>

namespace GpgFrontend {

namespace {

/// A crashed process leaves its lock behind. QLockFile treats one older than
/// this as stale and reclaims it when the recorded pid is genuinely gone.
constexpr int kStaleLockTimeoutMs = 30000;

struct LockHolder {
  std::mutex mutex;
  std::unique_ptr<QLockFile> lock;
};

auto Holder() -> LockHolder& {
  static LockHolder holder;
  return holder;
}

}  // namespace

auto ProfileLock::PathFor(const QString& profile_root) -> QString {
  return profile_root + "/profile.lock";
}

auto ProfileLock::Acquire(const QString& profile_root, int timeout_ms)
    -> ProfileLockResult {
  ProfileLockResult result;
  result.path = PathFor(profile_root);

  auto& holder = Holder();
  std::unique_lock<std::mutex> const guard(holder.mutex);

  if (holder.lock) {
    // already ours; acquiring twice is a caller bug, not a conflict
    return result;
  }

  if (!QDir(profile_root).exists() && !QDir().mkpath(profile_root)) {
    LOG_W() << "cannot create profile root for locking:" << profile_root;
    result.status = ProfileLockStatus::kIO_FAILED;
    return result;
  }

  auto lock = std::make_unique<QLockFile>(result.path);
  lock->setStaleLockTime(kStaleLockTimeoutMs);

  if (lock->tryLock(timeout_ms)) {
    holder.lock = std::move(lock);
    return result;
  }

  switch (lock->error()) {
    case QLockFile::LockFailedError: {
      result.status = ProfileLockStatus::kHELD_ELSEWHERE;
      // pid and hostname turn "already open" into something the user can act
      // on, which is the whole reason for not hand-rolling this
      lock->getLockInfo(&result.pid, &result.host, &result.application);
      LOG_W() << "profile is already open elsewhere:" << result.path << "pid"
              << result.pid << "host" << result.host;
      break;
    }
    default:
      result.status = ProfileLockStatus::kIO_FAILED;
      LOG_W() << "cannot take the profile lock:" << result.path;
      break;
  }
  return result;
}

auto ProfileLock::Probe(const QString& profile_root) -> ProfileLockResult {
  ProfileLockResult result;
  result.path = PathFor(profile_root);

  if (!QDir(profile_root).exists()) return result;  // nothing there to hold

  QLockFile lock(result.path);
  lock.setStaleLockTime(kStaleLockTimeoutMs);

  // tryLock() rather than QFileInfo::exists(): it is what distinguishes a live
  // holder from the lock file a crashed one left behind, and it is the same
  // judgement the process being launched will make a moment later.
  if (lock.tryLock(0)) {
    lock.unlock();
    return result;
  }

  result.status = lock.error() == QLockFile::LockFailedError
                      ? ProfileLockStatus::kHELD_ELSEWHERE
                      : ProfileLockStatus::kIO_FAILED;
  lock.getLockInfo(&result.pid, &result.host, &result.application);
  return result;
}

void ProfileLock::Release() {
  auto& holder = Holder();
  std::unique_lock<std::mutex> const guard(holder.mutex);

  if (!holder.lock) return;
  holder.lock->unlock();
  holder.lock.reset();
  LOG_I() << "profile lock released";
}

auto ProfileLock::ForceUnlock(const QString& profile_root) -> bool {
  QLockFile lock(PathFor(profile_root));
  lock.setStaleLockTime(kStaleLockTimeoutMs);
  LOG_W() << "force-unlocking profile:" << profile_root;
  return lock.removeStaleLockFile();
}

}  // namespace GpgFrontend
