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

#pragma once

namespace GpgFrontend {

/**
 * @brief Why a profile lock could not be taken.
 */
enum class ProfileLockStatus {
  kACQUIRED,
  kHELD_ELSEWHERE,  ///< another process has this profile open
  kIO_FAILED,       ///< the lock file could not be created at all
};

/**
 * @brief Outcome of taking a profile lock, with enough to explain a refusal.
 */
struct GF_CORE_EXPORT ProfileLockResult {
  ProfileLockStatus status = ProfileLockStatus::kACQUIRED;

  qint64 pid = 0;
  QString host;
  QString application;
  QString path;

  [[nodiscard]] auto Ok() const -> bool {
    return status == ProfileLockStatus::kACQUIRED;
  }
};

/**
 * @brief One process at a time per profile root.
 *
 * Two processes sharing a root corrupt `data_objs/` — every write there is a
 * whole-file read-modify-write with no locking of its own — let the garbage
 * collector quarantine objects the other process is still writing, and leave
 * the GnuPG home directory unprotected, since SQLite only protects itself.
 *
 * QLockFile rather than a hand-rolled file: it records the pid and hostname, so
 * a refusal can say which process is holding the profile instead of just
 * refusing, and it detects a stale lock left by a process that crashed.
 */
class GF_CORE_EXPORT ProfileLock {
 public:
  /**
   * @brief Take the lock for the lifetime of the process.
   *
   * @param profile_root root whose lock to take
   * @param timeout_ms how long to wait; a deep restart hands the lock over
   * while the old process is still exiting, so the successor waits
   * @return the outcome, with holder details on refusal
   */
  static auto Acquire(const QString &profile_root, int timeout_ms = 0)
      -> ProfileLockResult;

  /**
   * @brief Whether this process holds a profile lock.
   *
   * @return true when held
   */
  static auto IsHeld() -> bool;

  /**
   * @brief Ask whether a root is free, without keeping it.
   *
   * Opening a profile in a new window would otherwise mean launching a process
   * that immediately dies on a modal, so the offer is checked before it is
   * made. The mere presence of `profile.lock` is not the answer: a crashed
   * process leaves one behind, and a profile nobody is using would look busy
   * forever. This takes the lock briefly and gives it straight back, so stale
   * locks resolve exactly as they do on a real open.
   *
   * Advisory by nature — the answer can be out of date the moment it returns,
   * which is why the launched process still takes the lock itself.
   *
   * @param profile_root root to probe
   * @return Ok() when the root is free; otherwise the holder's details
   */
  static auto Probe(const QString &profile_root) -> ProfileLockResult;

  /**
   * @brief Give up the lock.
   *
   * Called during a deep restart, and only after every writer has stopped and
   * the no-further-writes flag is set. Releasing it while anything can still
   * write would make the lock stop meaning anything at exactly the moment two
   * processes overlap — the one case it exists for.
   */
  static void Release();

  /**
   * @brief Break a lock left behind by a process that is genuinely gone.
   *
   * Destructive by nature: if the holder is in fact alive, this reintroduces
   * the concurrent-write window. Every caller confirms with the user twice.
   *
   * @param profile_root root whose lock to break
   * @return true when the lock file was removed
   */
  static auto ForceUnlock(const QString &profile_root) -> bool;

  /**
   * @brief Where the lock for a root lives.
   *
   * @param profile_root the root
   * @return absolute path
   */
  static auto PathFor(const QString &profile_root) -> QString;
};

}  // namespace GpgFrontend
