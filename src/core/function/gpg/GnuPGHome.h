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

#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

/**
 * @brief The length limit GnuPG's agent sockets have to live within.
 *
 * gpg-agent binds its sockets inside the home directory, and a unix domain
 * socket address is capped by sockaddr_un::sun_path -- 104 bytes on Darwin, 108
 * on Linux, including the terminating NUL. Exceed it and gpg-agent refuses the
 * name and exits, so the socket never appears and every later operation fails
 * with GPG_ERR_ENOTSOCK. That is silent unless somebody checks the length up
 * front, which is what this exists for.
 *
 * This is the only place the arithmetic lives. It used to be duplicated between
 * the resolver and the settings dialog, and the two drifted: the dialog refused
 * paths the engine would have rescued with a link.
 */
struct GF_CORE_EXPORT GnuPGSocketBudget {
  /**
   * @brief Longest home directory GnuPG can still put its sockets in.
   *
   * Measured against "/S.gpg-agent.browser", the longest name gpg-agent builds
   * -- and it builds it even though we never pass --enable-browser-socket. It
   * validates the name before deciding whether to bind it, and a name that does
   * not fit makes it exit(2) outright:
   *
   *   gpg-agent: socket name '.../S.gpg-agent.browser' is too long
   *   exit=2
   *
   * So a home directory where only that one socket is over is still a home
   * directory GnuPG refuses to run in. Budgeting against S.gpg-agent.extra
   * instead would pass a directory gpg-agent then rejects, which is exactly the
   * failure this is meant to catch.
   *
   * @return the maximum home directory length in bytes, or -1 where no such
   *         limit applies (Windows, which does not address these by path)
   */
  static auto Bytes() -> int;

  /**
   * @brief Whether GnuPG can host its agent sockets in @p path.
   *
   * Measured in UTF-8 bytes rather than characters: sun_path is a byte buffer,
   * so a non-ASCII user name costs more room than its character count suggests.
   *
   * @param path a candidate GnuPG home directory
   * @return true when the sockets fit, and always true where no limit applies
   */
  static auto Fits(const QString& path) -> bool;
};

/**
 * @brief Where a key database really is, and what GnuPG is told about it.
 *
 * Two strings are the whole state. Everything else -- usable, redirected -- is
 * a question asked of them rather than a field that could disagree with them.
 *
 * The failure reason lives here rather than in a process-global, which is what
 * it used to be. A global is last-writer-wins across channels, so opening a
 * second key database silently overwrote the first one's explanation.
 */
struct GF_CORE_EXPORT GnuPGHome {
  /// Where the files really are. Storage, settings and packaging use this, so a
  /// profile stays self-contained and a package describes its own contents.
  QString key_db_path;

  /// What GnuPG is told. Equal to key_db_path unless that was too long, in
  /// which case it is a short link to it. Empty when Inspect() has determined a
  /// link is needed but Provision() has not made one yet, and empty when the
  /// directory is unusable.
  QString engine_path;

  /// Untranslated and free of the user's paths, so it is safe to paste into a
  /// bug report. Empty when the directory is usable.
  QString unusable_reason;

  /**
   * @brief Whether GnuPG can be run against this key database at all.
   *
   * Keyed on the reason rather than on engine_path, because Inspect() answers
   * this before any link exists. On a home that Provision() returned, a usable
   * home always has an engine_path.
   */
  [[nodiscard]] auto IsUsable() const -> bool {
    return unusable_reason.isEmpty();
  }

  /**
   * @brief Whether GnuPG is being pointed at a link rather than the real path.
   */
  [[nodiscard]] auto IsRedirected() const -> bool {
    return !engine_path.isEmpty() && engine_path != key_db_path;
  }
};

/**
 * @brief The short temporary links handed to GnuPG, and their lifetime.
 *
 * A link is only ever created for a key database whose own path is too long for
 * the agent sockets. It works because bind() resolves the symlink when it
 * creates the socket -- the socket file still lands in the real directory --
 * while the string that has to fit in sun_path is the short one.
 *
 * Shortening the on-disk layout cannot replace this. The budget is fixed but
 * the path is not: the user name alone moves it, so any fixed layout is only
 * ever a few characters from failing again on somebody else's machine.
 */
class GF_CORE_EXPORT GnuPGHomeLinkStore
    : public SingletonFunctionObject<GnuPGHomeLinkStore> {
 public:
  /**
   * @brief Construct a new GnuPG Home Link Store object
   *
   * @param channel
   */
  explicit GnuPGHomeLinkStore(int channel);

  /**
   * @brief The directory links are kept in.
   *
   * Chosen once, from the first candidate short enough to host a link at all:
   * the XDG runtime directory (per-user, mode 0700) where there is one, then
   * the temporary directory. Both are already per-user and private, so a
   * predictable name in a world-writable /tmp is not a concern.
   *
   * Every link name is the same length, so this is target-independent and can
   * be asked before a particular key database is in hand.
   *
   * @return the root, or empty when no candidate can host a link within budget
   */
  [[nodiscard]] auto Root() const -> QString;

  /**
   * @brief Point the store at @p root instead of the chosen candidate.
   *
   * The candidate search runs at construction against directories that really
   * exist on this machine, which leaves no way to exercise the cases where the
   * root is missing or itself over budget. This is that seam.
   *
   * Filtered exactly as the candidate search is, so Root() means one thing
   * however it was set: a directory that can host a link inside the budget, or
   * nothing. A root too long to be any use therefore reads back as empty.
   *
   * @param root the directory to keep links in; empty to model having none
   */
  void UseRoot(const QString& root);

  /**
   * @brief The link to give GnuPG for @p real_home, creating one if needed.
   *
   * Leases are keyed by the real path, so sibling channels opening one key
   * database share a link rather than littering one per call.
   *
   * The name is random rather than derived from the target. A fresh name cannot
   * be stale, cannot collide with a hash of a different profile, and cannot
   * land on somebody else's real directory -- which is four failure branches
   * that simply do not arise.
   *
   * @param real_home the real key database directory
   * @return the link path, or empty when one could not be made
   */
  auto Acquire(const QString& real_home) -> QString;

  /**
   * @brief Remove every link this store created.
   *
   * Called from DestroyGpgFrontendCore() before the singletons go, rather than
   * from a destructor: teardown ordering there is already delicate.
   *
   * A link orphaned by a killed process is left behind on purpose. It is a
   * dangling symlink of a few bytes in a directory the system already purges,
   * and sweeping would mean deciding which entries are ours -- which cannot be
   * established from the filesystem alone.
   */
  void ReleaseAll();

 private:
  mutable QMutex mutex_;           ///< guards root_ and leases_
  QString root_;                   ///< where links are kept; empty if none
  QMap<QString, QString> leases_;  ///< real home path -> link path
};

/**
 * @brief Decides what home directory GnuPG is given for a key database.
 *
 * Split by side effect rather than by caller. Inspect() answers the question
 * and touches nothing; Provision() answers it and then makes it true. Both run
 * the same predicate, so the interface cannot refuse a path the engine would
 * have accepted -- which it used to, because the two were unrelated functions.
 */
class GF_CORE_EXPORT GnuPGHomeResolver {
 public:
  /**
   * @brief Construct a new GnuPG Home Resolver object
   *
   * @param store the link store to draw links from
   */
  explicit GnuPGHomeResolver(
      GnuPGHomeLinkStore& store = GnuPGHomeLinkStore::GetInstance());

  /**
   * @brief Whether GnuPG could use @p key_db_path, and how.
   *
   * Pure: creates nothing and records nothing. The returned home has an
   * engine_path only when the real path already fits; when a link is needed the
   * home is usable but has no engine_path yet, because the link does not exist
   * until Provision() makes one.
   *
   * This is what the settings dialog asks while a path is still being chosen.
   *
   * @param key_db_path the real key database directory
   * @return the verdict
   */
  [[nodiscard]] auto Inspect(const QString& key_db_path) const -> GnuPGHome;

  /**
   * @brief The home directory to hand GnuPG for @p key_db_path.
   *
   * Inspect(), and then a link from the store when one is called for. Every
   * path Inspect() rejects is rejected here unchanged.
   *
   * @param key_db_path the real key database directory
   * @return the home to use, usable or with the reason it is not
   */
  [[nodiscard]] auto Provision(const QString& key_db_path) -> GnuPGHome;

 private:
  GnuPGHomeLinkStore& store_;
};

}  // namespace GpgFrontend
