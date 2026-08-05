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

#include "core/profile/ProfileSession.h"

#include <mutex>

#include "core/profile/ProfileLock.h"

namespace GpgFrontend {

namespace {

struct SessionHolder {
  std::mutex mutex;
  QSharedPointer<ProfileSession> session;
};

auto Holder() -> SessionHolder& {
  static SessionHolder holder;
  return holder;
}

}  // namespace

ProfileSession::ProfileSession(QSharedPointer<class Profile> profile,
                               QSharedPointer<ProfileAccessor> accessor)
    : profile_(std::move(profile)), accessor_(std::move(accessor)) {
  reload_marker();
}

void ProfileSession::publish(QSharedPointer<ProfileSession> session) {
  auto& holder = Holder();
  std::unique_lock<std::mutex> const lock(holder.mutex);

  if (!holder.session.isNull()) {
    qFatal(
        "a second profile session was published; the first was '%s', the "
        "second '%s'",
        qPrintable(holder.session->profile_->Id()),
        qPrintable(session->profile_->Id()));
  }
  holder.session = std::move(session);
}

auto ProfileSession::Loaded() -> bool {
  auto& holder = Holder();
  std::unique_lock<std::mutex> const lock(holder.mutex);
  return !holder.session.isNull();
}

auto ProfileSession::Instance() -> ProfileSession& {
  auto& holder = Holder();
  std::unique_lock<std::mutex> const lock(holder.mutex);

  if (holder.session.isNull()) {
    // Everything below this point derives a path or a key from the profile.
    // Continuing without one would read and write another profile's key
    // material, so this is fatal in release builds too rather than an assertion
    // a release build drops.
    qFatal("the profile session was read before it was loaded");
  }
  return *holder.session;
}

auto ProfileSession::Profile() const -> const class Profile& {
  return *profile_;
}

auto ProfileSession::Accessor() const -> ProfileAccessor& { return *accessor_; }

auto ProfileSession::Keys() const -> ProfileSecureKeyManager& {
  if (keys_.isNull()) {
    qFatal("the profile's keys were read before they were loaded");
  }
  return *keys_;
}

auto ProfileSession::KeysLoaded() const -> bool { return !keys_.isNull(); }

void ProfileSession::attach_keys(QSharedPointer<ProfileSecureKeyManager> keys) {
  if (!keys_.isNull()) {
    qFatal("the profile's keys were loaded twice");
  }
  keys_ = std::move(keys);
}

auto ProfileSession::Marker() const -> const ProfileMarker& { return marker_; }

void ProfileSession::reload_marker() {
  marker_ = ReadProfileMarker(profile_->MarkerPath()).value_or(ProfileMarker{});
}

auto ProfileSession::UpdateMarker(
    const std::function<void(ProfileMarker&)>& mutate) -> bool {
  const auto path = profile_->MarkerPath();

  // Read-modify-write against disk, not against the cached copy: the file
  // carries the migration history and any field a newer build added, and this
  // build rebuilding it from what it happens to know would erase all of that on
  // every single start.
  auto marker = ReadProfileMarker(path).value_or(ProfileMarker{});
  mutate(marker);

  if (!WriteProfileMarker(path, marker)) return false;

  marker_ = marker;
  return true;
}

auto ProfileSession::Root() const -> QString { return profile_->Root(); }

auto ProfileSession::WorkspacePath() const -> QString {
  const auto override_path =
      Settings().value("workspace/path").toString().trimmed();
  if (!override_path.isEmpty()) return QDir::cleanPath(override_path);

  // The installed root has no workspace by default: its storage is split and
  // its default path setting is whatever the user already had. Turning one on
  // for it silently would move where their file panel opens.
  if (profile_->Kind() == ProfileKind::kINSTALLED_ROOT) return {};

  return accessor_->PathOf(ProfileArea::kWorkspace);
}

auto ProfileSession::EnsureWorkspace() const -> QString {
  const auto path = WorkspacePath();
  if (path.isEmpty()) return {};

  if (!QDir(path).exists() && !QDir().mkpath(path)) {
    LOG_W() << "cannot create the profile workspace:" << path;
    return {};
  }
  return path;
}

auto ProfileSession::Settings() const -> QSettings {
  return accessor_->Settings();
}

void ProfileSession::Unload(ProfileUnmountMode mode) {
  bool expected = false;
  if (!unloaded_.compare_exchange_strong(expected, true)) return;

  // Storage first, lock last. Releasing the lock is what tells another process
  // this root is free, and it must not say so while a transient tree is still
  // being taken apart underneath it.
  profile_->Unmount(mode);
  ProfileLock::Release();
}

}  // namespace GpgFrontend
