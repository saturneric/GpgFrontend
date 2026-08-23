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

namespace GpgFrontend {

/**
 * @brief One piece of a profile on its way into or out of a package.
 *
 * Paths here are **profile-relative** — `secure/app.key`, `db/pubring.kbx` —
 * the same spelling the area table uses. There were three near-identical path
 * namespaces in this code (profile-relative, staging-relative and
 * archive-relative) and members deliberately use only the first; adding the
 * prefix is the sink's job, once.
 */
struct GF_CORE_EXPORT ProfileMember {
  QString path;

  /// The area it belongs to, for policy lookup. Empty for the key-database
  /// directories, which are GnuPG's rather than areas this program addresses.
  std::optional<ProfileArea> area;

  /// The content, when the member was synthesised or read through the accessor.
  GFBuffer bytes;

  /// A file to copy instead, when the member is a file on the storage.
  ///
  /// Exactly one of this and `bytes` is set. A member is not always bytes in
  /// hand: a workspace can hold files far larger than anything worth holding in
  /// memory, and reading them into a buffer to write them straight back out
  /// would make the size of someone's files a limit on packaging them.
  QString source_path;

  /// A directory rather than a file, carrying neither bytes nor a source.
  ///
  /// Emitted so that a directory which happens to be empty still travels. The
  /// paths of file members create every directory that has something in it, so
  /// an empty one is the only case that would otherwise be lost -- silently,
  /// and only on the machine the profile was sent to.
  bool directory = false;

  [[nodiscard]] auto IsFileOnStorage() const -> bool {
    return !directory && !source_path.isEmpty();
  }
};

/**
 * @brief Yields the members of a profile, in order.
 *
 * The only way profile data moves. Everything that used to reach into the tree
 * with its own directory walk, or write a file into staging directly, goes
 * through a source and a sink instead — so that "what travels" is answered in
 * one place and "where the bytes come from" in another, and neither has to know
 * the other's answer.
 */
class GF_CORE_EXPORT ProfileMemberSource {
 public:
  ProfileMemberSource() = default;
  virtual ~ProfileMemberSource() = default;

  ProfileMemberSource(const ProfileMemberSource &) = delete;
  auto operator=(const ProfileMemberSource &) -> ProfileMemberSource & = delete;
  ProfileMemberSource(ProfileMemberSource &&) = delete;
  auto operator=(ProfileMemberSource &&) -> ProfileMemberSource & = delete;

  /**
   * @brief Produce the next member.
   *
   * @param out filled in when this returns true
   * @return false when there are no more
   */
  virtual auto Next(ProfileMember &out) -> bool = 0;

  /**
   * @brief Why the source stopped, when it stopped early.
   *
   * @return an error, or empty when the source simply ran out
   */
  [[nodiscard]] virtual auto Error() const -> QString { return {}; }
};

/**
 * @brief Takes members somewhere.
 */
class GF_CORE_EXPORT ProfileMemberSink {
 public:
  ProfileMemberSink() = default;
  virtual ~ProfileMemberSink() = default;

  ProfileMemberSink(const ProfileMemberSink &) = delete;
  auto operator=(const ProfileMemberSink &) -> ProfileMemberSink & = delete;
  ProfileMemberSink(ProfileMemberSink &&) = delete;
  auto operator=(ProfileMemberSink &&) -> ProfileMemberSink & = delete;

  /**
   * @brief Take one member.
   *
   * @param member the member to store
   * @return false when it could not be stored, which stops the transfer
   */
  virtual auto Accept(const ProfileMember &member) -> bool = 0;

  /**
   * @brief Why the sink refused, when it refused.
   *
   * @return an error, or empty
   */
  [[nodiscard]] virtual auto Error() const -> QString { return {}; }
};

/**
 * @brief Move every member from a source into a sink.
 *
 * @param source where members come from
 * @param sink where they go
 * @return empty on success, or the first error either side reported
 */
auto GF_CORE_EXPORT TransferProfileMembers(ProfileMemberSource &source,
                                           ProfileMemberSink &sink) -> QString;

/**
 * @brief The secure area, with each object's bytes resolved to their source.
 *
 * **This is the one place the difference between the two kinds of key is
 * expressed, and it is not a detail.**
 *
 * `app.key` is emitted from the plaintext root key held in memory, never from
 * the stored form: what is stored may be sealed by *this* machine's credential
 * store, and a package carrying that would not open anywhere else.
 *
 * Rotated `<keyId>.key` files are emitted exactly as stored, because they are
 * already encrypted under the root key — which travels with them.
 *
 * It was three `if (name == "app.key")` tests in three files. Getting the first
 * wrong ships a package nobody can open; getting the second wrong ships one
 * whose older data objects can never be read again, with no sign that anything
 * is missing.
 *
 * Reading through the accessor rather than off the storage is what makes this
 * work for a packaged session, whose secure area is held in memory and has no
 * files to walk.
 *
 * @param storage the session's storage driver
 * @param root_key the resident plaintext application key
 * @return the members, or empty when there is no key to pack
 */
auto GF_CORE_EXPORT ResolveSecureAreaMembers(const ProfileAccessor &storage,
                                             const GFBuffer &root_key)
    -> QList<ProfileMember>;

/**
 * @brief A source over members already in hand.
 *
 * For everything that is not a walk of the storage: the secure area, and the
 * settings file an export regenerates rather than copies.
 */
class GF_CORE_EXPORT ListMemberSource final : public ProfileMemberSource {
 public:
  explicit ListMemberSource(QList<ProfileMember> members);

  auto Next(ProfileMember &out) -> bool override;

 private:
  QList<ProfileMember> members_;
  int next_ = 0;
};

/**
 * @brief Walks a profile on the storage, admitting only what travels.
 *
 * The allow-list from the area table, applied once. A top-level name nothing
 * declared is left behind and recorded, because failing closed silently is its
 * own trap: the sender should learn at export time, not from the copy on
 * somebody else's machine.
 */
class GF_CORE_EXPORT TreeMemberSource final : public ProfileMemberSource {
 public:
  /**
   * @brief Walk @p profile_root.
   *
   * @param profile_root the profile to read
   * @param include_workspace whether the user asked for their own files
   * @param guard an absolute path never to descend into, for the staging
   * directory that may sit inside the tree being walked
   * @param superseded profile-relative paths a caller is supplying itself, and
   * which this must therefore neither yield nor measure. Not the same as a
   * skipped name: these do travel, they simply travel as bytes in hand rather
   * than as the copy on disk, and counting the copy as well would both write
   * the member twice and overstate how much room the recipient needs.
   */
  TreeMemberSource(QString profile_root, bool include_workspace,
                   QString guard = {}, QSet<QString> superseded = {});

  /**
   * @brief Walk now, so failures are known before anything is written.
   *
   * Next() would do this on its first call, but by then a caller packing an
   * archive has already committed to producing one -- and "the profile is not
   * there" would arrive as an empty walk rather than as an error. Anything that
   * writes to a destination should ask here first.
   *
   * @return empty when the profile can be walked, or the reason it cannot
   */
  auto Prepare() -> QString;

  auto Next(ProfileMember &out) -> bool override;

  [[nodiscard]] auto Error() const -> QString override { return error_; }

  /// Top-level names the package does not carry, sorted.
  [[nodiscard]] auto Skipped() const -> QStringList;

  /// Total size of every member yielded so far.
  [[nodiscard]] auto Bytes() const -> qint64 { return bytes_; }

 private:
  auto Collect() -> void;

  QString profile_root_;
  bool include_workspace_;
  QString guard_;
  QSet<QString> superseded_;

  QString error_;
  QSet<QString> skipped_;
  qint64 bytes_ = 0;

  bool collected_ = false;
  QList<ProfileMember> pending_;
  int next_ = 0;
};

/**
 * @brief Writes members into a staging tree, ready to be archived.
 *
 * @param staging_dir the staging directory; members land under its
 * `profile/` prefix, which is where the archive expects the tree
 */
class GF_CORE_EXPORT StagingMemberSink final : public ProfileMemberSink {
 public:
  explicit StagingMemberSink(QString staging_dir);

  auto Accept(const ProfileMember &member) -> bool override;

  [[nodiscard]] auto Error() const -> QString override { return error_; }

  /// Total size written.
  [[nodiscard]] auto Bytes() const -> qint64 { return bytes_; }

 private:
  QString staging_dir_;
  QString error_;
  qint64 bytes_ = 0;
};

}  // namespace GpgFrontend
