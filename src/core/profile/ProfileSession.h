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

#include <functional>

#include "core/profile/Profile.h"
#include "core/profile/ProfileAccessor.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfileSecureKeyManager.h"

namespace GpgFrontend {

/**
 * @brief The one profile this process is running, and everything that follows
 * from it.
 *
 * A session is a profile plus the three things that make it usable: the
 * **accessor** that reaches its storage, the **key manager** that opens what is
 * stored there, and the **marker** that records what it is. Everything else in
 * the application asks the session rather than resolving a path or a key for
 * itself.
 *
 * **One session per process, for the life of the process.** Not a limitation
 * to work around but the invariant the rest of the design rests on: singletons
 * cache paths and key material at construction, the profile lock is held for
 * the lifetime of the process, and GnuPG is handed one home directory. Opening
 * a second profile is starting a second process, which is exactly what the
 * profile manager does.
 *
 * That invariant is enforced the way it has to be to be worth anything: reading
 * a session that does not exist, or publishing a second one, is fatal in every
 * build configuration rather than an assertion a release build drops. The
 * failure it prevents is operating on another profile's key material.
 */
class GF_CORE_EXPORT ProfileSession {
 public:
  /**
   * @brief Whether the loader has published a session yet.
   *
   * @return true once ProfileLoader::Mount() has succeeded
   */
  static auto Loaded() -> bool;

  /**
   * @brief The session. Aborts if there is none.
   *
   * @return the session
   */
  static auto Instance() -> ProfileSession &;

  ~ProfileSession() = default;

  ProfileSession(const ProfileSession &) = delete;
  auto operator=(const ProfileSession &) -> ProfileSession & = delete;
  ProfileSession(ProfileSession &&) = delete;
  auto operator=(ProfileSession &&) -> ProfileSession & = delete;

  /// What was opened, and the rules that come with its shape.
  [[nodiscard]] auto Profile() const -> const class Profile &;

  /// Its storage.
  [[nodiscard]] auto Accessor() const -> ProfileAccessor &;

  /**
   * @brief Its key set. Aborts if read before the keys are loaded.
   *
   * There is a window during startup — after the storage is reachable, before
   * the secret has been resolved — in which the settings and the log are live
   * but no key is. Anything that reads a key there would silently get nothing
   * and file its data under an id that will not exist on the next start, so
   * this refuses loudly instead.
   *
   * @return the key manager
   */
  [[nodiscard]] auto Keys() const -> ProfileSecureKeyManager &;

  /**
   * @brief Whether the key set has been loaded yet.
   *
   * @return true once ProfileLoader::Open() has succeeded
   */
  [[nodiscard]] auto KeysLoaded() const -> bool;

  /// Identity, layout version and history, as last read from storage.
  [[nodiscard]] auto Marker() const -> const ProfileMarker &;

  /**
   * @brief Change the marker and write it back.
   *
   * Read-modify-write against what is on disk rather than against the cached
   * copy: this file carries the migration history and any field a newer build
   * added, and rebuilding it from what this build happens to know would erase
   * all of that.
   *
   * @param mutate applied to the marker before it is written
   * @return true when the new marker reached storage
   */
  auto UpdateMarker(const std::function<void(ProfileMarker &)> &mutate) -> bool;

  /// Absolute root of the profile's storage.
  [[nodiscard]] auto Root() const -> QString;

  /**
   * @brief The directory this profile keeps the user's own files in.
   *
   * Honours the `workspace/path` override when one is set, which points the
   * workspace at a synced folder or an external volume. An overridden workspace
   * is never packaged: it is somewhere else on this machine, exactly like a key
   * database outside the profile.
   *
   * Empty for the installed root, which has no workspace by default: its
   * storage is split and its default path setting is whatever the user already
   * had, so turning one on silently would move where their file panel opens.
   *
   * @return absolute path, or empty when this profile has no workspace
   */
  [[nodiscard]] auto WorkspacePath() const -> QString;

  /**
   * @brief The workspace directory, created if it is missing.
   *
   * @return the path, or empty when there is none or it could not be created
   */
  [[nodiscard]] auto EnsureWorkspace() const -> QString;

  /// The profile's settings store.
  [[nodiscard]] auto Settings() const -> QSettings;

  /**
   * @brief End the session: give the storage back and release the lock.
   *
   * Idempotent, and safe to call from the shutdown watchdog as well as from an
   * orderly exit — whichever gets there first does it, and the other returns.
   *
   * @param mode how much time there is
   */
  void Unload(ProfileUnmountMode mode = ProfileUnmountMode::kNORMAL);

 private:
  friend class ProfileLoader;

  ProfileSession(QSharedPointer<class Profile> profile,
                 QSharedPointer<ProfileAccessor> accessor);

  /// Publish this session as the process's one session. Fatal if there is one.
  static void publish(QSharedPointer<ProfileSession> session);

  /// Attach the key set once the loader has resolved the secret.
  void attach_keys(QSharedPointer<ProfileSecureKeyManager> keys);

  /// Re-read the marker from storage, e.g. after a migration wrote it.
  void reload_marker();

  QSharedPointer<class Profile> profile_;
  QSharedPointer<ProfileAccessor> accessor_;
  QSharedPointer<ProfileSecureKeyManager> keys_;
  ProfileMarker marker_;
  std::atomic<bool> unloaded_ = false;
};

}  // namespace GpgFrontend
