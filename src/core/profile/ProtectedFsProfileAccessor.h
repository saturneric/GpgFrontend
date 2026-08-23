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

#include "core/profile/ProfileAccessor.h"
#include "core/profile/ProfileStoragePlan.h"

namespace GpgFrontend {

/**
 * @brief Areas are directories under a root this driver provisioned itself.
 *
 * The same layout as the filesystem driver, and it inherits every area
 * operation unchanged — the storage is still directories under a root. What
 * differs is only *which* root, and what happens on the way out.
 *
 * The root is somewhere the platform does not leave the tree readable, where it
 * has such a place: memory on Linux and FreeBSD, an NTFS-encrypted directory on
 * Windows, and an ordinary hardened temporary directory where there is neither
 * — which today includes macOS. Which of those happened is reported honestly
 * through IsVolatile() and IsEncryptedAtRest() rather than assumed from the
 * platform.
 */
class GF_CORE_EXPORT ProtectedFsProfileAccessor final
    : public FsProfileAccessor {
 public:
  /**
   * @brief Take the storage a plan chose.
   *
   * @param digest the session digest, which names the directory
   * @param plan what PlanProfileStorage() decided
   * @return the driver, or null when the directory could not be made
   */
  static auto Provision(const QString &digest, const ProfileStoragePlan &plan)
      -> QSharedPointer<ProtectedFsProfileAccessor>;

  ProtectedFsProfileAccessor(QString root, QString settings_file,
                             ProfileStoragePlan plan);

  ~ProtectedFsProfileAccessor() override;

  ProtectedFsProfileAccessor(const ProtectedFsProfileAccessor &) = delete;
  auto operator=(const ProtectedFsProfileAccessor &)
      -> ProtectedFsProfileAccessor & = delete;
  ProtectedFsProfileAccessor(ProtectedFsProfileAccessor &&) = delete;
  auto operator=(ProtectedFsProfileAccessor &&)
      -> ProtectedFsProfileAccessor & = delete;

  [[nodiscard]] auto Driver() const -> QString override;
  [[nodiscard]] auto Label() const -> QString override;
  [[nodiscard]] auto IsVolatile() const -> bool override;
  [[nodiscard]] auto IsEncryptedAtRest() const -> bool override;

  /**
   * @brief Destroy the session tree.
   *
   * Idempotent, and safe from a teardown path that cannot log. Scrubbing is
   * skipped where it would buy nothing — unlinking a tmpfs file already frees
   * the page, and overwriting ciphertext whose key is gone is pure cost — and
   * always under kFAST, which runs against the shutdown watchdog's deadline.
   *
   * @param mode how much effort destruction is worth here
   */
  void Release(ProfileStorageRelease mode) override;

  /**
   * @brief What the anchor records so a later sweep can find an orphan.
   *
   * The anchor is on disk and this storage may not be, so a process that dies
   * leaves a pointer here and nothing else. Without it a crash would strand the
   * tree somewhere nothing thinks to look.
   *
   * @return the driver token and the root, as JSON
   */
  [[nodiscard]] auto AnchorState() const -> QJsonObject;

 private:
  ProfileStoragePlan plan_;

  bool released_ = false;
};

/**
 * @brief Everything MakeProfileAccessorFor() needs to place a session.
 */
struct GF_CORE_EXPORT ProfileAccessorSpec {
  QString digest;  ///< names the directory, so two open packages cannot collide

  /// Where the lock lives, and where the plain-folder candidate puts the tree.
  /// That candidate is deliberately the anchor itself: it is what every earlier
  /// build did, and a `disk` policy has to keep meaning the same thing.
  QString anchor;

  qint64 budget_bytes = 0;
  ProfileStoragePolicy policy = ProfileStoragePolicy::kAUTO;
};

/**
 * @brief What placing a session produced, including what it passed over.
 */
struct GF_CORE_EXPORT ProfileAccessorResult {
  QSharedPointer<ProfileAccessor> accessor;  ///< null when refused or broken

  /// One line per candidate not chosen. Shown verbatim in the refusal dialog.
  QStringList rejections;

  /// The policy forbade the only storage that was available, rather than
  /// nothing being available at all. The difference is what the user is told.
  bool refused_protected_only = false;

  /// What a later sweep needs to find this session's tree if the process dies:
  /// the driver, and the root it provisioned. Empty when the storage is not
  /// this session's to clean up.
  ///
  /// Returned here rather than asked of the accessor afterwards, because the
  /// accessor may be wrapped -- a decorator that holds an area in memory is
  /// still a ProfileAccessor, and a dynamic_cast to the concrete driver would
  /// quietly start returning null and strand the tree.
  QJsonObject anchor_state;
};

/**
 * @brief Choose and provision the storage for one packaged session.
 *
 * The single place placement is decided. Everything above the accessor asks
 * Profile::MakeAccessor() and learns nothing new.
 *
 * @param spec where the lock is, how much room is needed, and what the user
 * asked for
 * @return the driver and the reasons, or a refusal
 */
auto GF_CORE_EXPORT MakeProfileAccessorFor(const ProfileAccessorSpec &spec)
    -> ProfileAccessorResult;

/**
 * @brief Every place this platform could put a session tree, most preferred
 * first.
 *
 * The only platform-conditional code in the feature. Each candidate is probed
 * to the point of proof rather than assumed from the platform: a directory that
 * merely looks like the right place is exactly the failure this exists to
 * prevent.
 *
 * @param budget_bytes how much room the session will need
 * @param anchor where the plain-folder candidate goes
 * @return candidates, each either usable or carrying the reason it is not
 */
auto GF_CORE_EXPORT ProbeStorageCandidates(qint64 budget_bytes,
                                           const QString &anchor)
    -> QList<StorageCandidate>;

/**
 * @brief Make a directory as private as this platform allows.
 *
 * Best effort by nature, and none of it is the protection — that is the
 * driver's job. This only stops the obvious ways a session tree gets copied
 * somewhere else: a backup tool walking it, a desktop search engine indexing
 * it, another local account reading it.
 *
 * @param path an existing directory
 */
void GF_CORE_EXPORT HardenStorageDirectory(const QString &path);

/**
 * @brief Overwrite and remove a tree, as far as that means anything.
 *
 * A best-effort last resort for the unprotected fallback, never the primary
 * protection. On an SSD with wear levelling, on a copy-on-write filesystem
 * (btrfs, ZFS, APFS), on a data-journalling filesystem, or on anything with
 * snapshots, the overwrite lands on a *new* block and the original contents
 * survive untouched. It is worth doing because it defeats ordinary undelete on
 * the filesystems where it does work, and worth being honest about everywhere
 * else.
 *
 * Refuses an empty or relative path: a bug in the caller must not be able to
 * turn this loose on the wrong tree.
 *
 * @param path the tree to destroy
 */
void GF_CORE_EXPORT ScrubDirectory(const QString &path);

}  // namespace GpgFrontend
