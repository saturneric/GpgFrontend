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

#include "core/profile/ProfileMember.h"

#include <QDir>
#include <QSaveFile>

#include "core/profile/ProfileAreaTraits.h"

namespace GpgFrontend {

namespace {

/// Both shared with ProfileSecureKeyManager and ProfilePackage, from the area
/// table -- see kProfileRootKeyName for why these are not local strings.
constexpr auto kRootKeyName = kProfileRootKeyName;
constexpr auto kTreePrefix = kProfileTreePrefix;

/// Storage of its own, not a share of somebody else's.
///
/// A GFBuffer copy shares its storage, and Zeroize() deliberately wipes through
/// every share. These members are resolved on the window's thread and packed on
/// a worker, so a release of the session in between -- an unmount, or the
/// shutdown watchdog -- would erase key material the pack is still reading, and
/// the package would be written with zeroes where the rotated keys should be.
/// Nothing fails, nothing is short: the data objects of every earlier period
/// simply never open again on the other machine.
auto DetachedCopy(const GFBuffer &source) -> GFBuffer {
  if (source.Empty()) return {};
  return GFBuffer(source.Data(), source.Size());
}

}  // namespace

auto TransferProfileMembers(ProfileMemberSource &source,
                            ProfileMemberSink &sink) -> QString {
  ProfileMember member;
  while (source.Next(member)) {
    if (!sink.Accept(member)) {
      const auto error = sink.Error();
      return error.isEmpty() ? QString("a member could not be stored") : error;
    }
    member = {};
  }
  return source.Error();
}

auto ResolveSecureAreaMembers(const ProfileAccessor &storage,
                              const GFBuffer &root_key)
    -> QList<ProfileMember> {
  QList<ProfileMember> members;
  if (root_key.Empty()) return members;

  // The root key, from the key in hand. Never storage.Read(): the stored form
  // may be sealed by this machine's credential store, and a package carrying
  // that would not open on the computer it was made for.
  ProfileMember root;
  root.path = QString("secure/") + kRootKeyName;
  root.area = ProfileArea::kSecure;
  root.bytes = DetachedCopy(root_key);
  members.append(root);

  // Everything else exactly as stored. Rotated keys are already encrypted under
  // the root key, which is travelling in the member above, so they open at the
  // other end without being rewritten -- and rewriting them would mean
  // decrypting key material for no reason.
  for (const auto &name : storage.List(ProfileArea::kSecure, "*.key")) {
    if (name == kRootKeyName) continue;

    auto value = storage.Read(ProfileArea::kSecure, name);
    if (!value) {
      // Skipped rather than fatal: a rotated key that will not read is one
      // period's data objects lost, and refusing the whole export would lose
      // the profile instead.
      LOG_W() << "a rotated key could not be read and will not travel:" << name;
      continue;
    }

    ProfileMember rotated;
    rotated.path = "secure/" + name;
    rotated.area = ProfileArea::kSecure;
    rotated.bytes = DetachedCopy(*value);
    members.append(rotated);
  }

  return members;
}

ListMemberSource::ListMemberSource(QList<ProfileMember> members)
    : members_(std::move(members)) {}

auto ListMemberSource::Next(ProfileMember &out) -> bool {
  if (next_ >= members_.size()) return false;
  out = members_.at(next_++);
  return true;
}

TreeMemberSource::TreeMemberSource(QString profile_root, bool include_workspace,
                                   QString guard, QSet<QString> superseded)
    : profile_root_(std::move(profile_root)),
      include_workspace_(include_workspace),
      guard_(std::move(guard)),
      superseded_(std::move(superseded)) {}

auto TreeMemberSource::Skipped() const -> QStringList {
  QStringList names(skipped_.begin(), skipped_.end());
  names.sort();
  return names;
}

void TreeMemberSource::Collect() {
  collected_ = true;

  QDir root(profile_root_);
  if (!root.exists()) {
    error_ = "the profile folder is not there";
    return;
  }

  // Iterative rather than recursive: a profile root can contain the profiles
  // folder, and a walk that recurses on directories before deciding whether
  // they travel would descend into every other profile on the machine.
  QStringList queue;
  queue << QString();

  while (!queue.isEmpty()) {
    const auto relative_dir = queue.takeFirst();
    const auto absolute_dir = relative_dir.isEmpty()
                                  ? profile_root_
                                  : profile_root_ + "/" + relative_dir;

    const auto entries =
        QDir(absolute_dir)
            .entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot |
                           QDir::Hidden);

    for (const auto &entry : entries) {
      const auto relative = relative_dir.isEmpty()
                                ? entry.fileName()
                                : relative_dir + "/" + entry.fileName();

      if (!IsIncludedInPackage(relative, include_workspace_)) {
        // Only the top level is worth reporting: a user knows what they left in
        // their profile folder, not what is inside a keyring directory.
        if (!relative.contains('/')) skipped_.insert(relative);
        continue;
      }

      // A package carries no links, by design: what one points at on this
      // machine means nothing on another.
      if (entry.isSymLink()) continue;

      // The staging directory may sit inside the tree being walked, and walking
      // it into itself does not terminate. The allow-list should have taken it
      // already -- it is dot-prefixed -- so this is what makes a mistake there
      // a bug rather than a hang.
      if (!guard_.isEmpty() &&
          QDir::cleanPath(entry.absoluteFilePath()) == guard_) {
        continue;
      }

      if (entry.isDir()) {
        queue << relative;

        // Emitted as a member of its own. A directory with files in it would be
        // created by their paths anyway, but an empty one has no files to
        // create it, and dropping it means the profile that arrives is not the
        // profile that was sent.
        //
        // Except for an area packed from the accessor: its contents are emitted
        // from wherever the driver actually put them, and a session holding it
        // in memory has no directory here at all.
        const auto *traits = TraitsForTopLevel(relative.section('/', 0, 0));
        if (traits != nullptr &&
            traits->pack_source == AreaPackSource::kAccessor) {
          continue;
        }

        ProfileMember dir_member;
        dir_member.path = relative;
        dir_member.directory = true;
        if (traits != nullptr) dir_member.area = traits->area;
        pending_.append(dir_member);
        continue;
      }

      // Supplied by the caller from something better than this copy, so it is
      // neither yielded nor measured. Deliberately not recorded as skipped: it
      // travels, just not from here.
      if (superseded_.contains(relative)) continue;

      ProfileMember member;
      member.path = relative;
      const auto *traits = TraitsForTopLevel(relative.section('/', 0, 0));
      if (traits != nullptr) member.area = traits->area;
      member.source_path = entry.absoluteFilePath();
      pending_.append(member);
      bytes_ += entry.size();
    }
  }
}

auto TreeMemberSource::Prepare() -> QString {
  if (!collected_) Collect();
  return error_;
}

auto TreeMemberSource::Next(ProfileMember &out) -> bool {
  if (!collected_) Collect();
  if (!error_.isEmpty()) return false;
  if (next_ >= pending_.size()) return false;

  out = pending_.at(next_++);
  return true;
}

StagingMemberSink::StagingMemberSink(QString staging_dir)
    : staging_dir_(std::move(staging_dir)) {}

auto StagingMemberSink::Accept(const ProfileMember &member) -> bool {
  if (member.path.isEmpty()) {
    error_ = "a member with no name cannot be stored";
    return false;
  }

  // The prefix is added here and only here. Members speak profile-relative
  // paths; where a package puts the tree is the sink's business.
  const auto target = staging_dir_ + "/" + kTreePrefix + "/" + member.path;

  if (member.directory) {
    if (QDir().mkpath(target)) return true;
    error_ = QString("cannot create %1").arg(member.path);
    return false;
  }

  const QFileInfo info(target);
  if (!QDir().mkpath(info.absolutePath())) {
    error_ = QString("cannot create %1").arg(info.absolutePath());
    return false;
  }

  if (member.IsFileOnStorage()) {
    if (QFile::copy(member.source_path, target)) {
      bytes_ += QFileInfo(target).size();
      return true;
    }

    // A file the collector removed while the walk went past it is not a reason
    // to lose the whole export; anything else would have failed the mkpath.
    if (!QFileInfo::exists(member.source_path)) {
      LOG_W() << "file vanished while staging, skipped:" << member.path;
      return true;
    }

    error_ = QString("cannot copy %1").arg(member.path);
    return false;
  }

  QSaveFile file(target);
  if (!file.open(QIODevice::WriteOnly)) {
    error_ = QString("cannot write %1").arg(member.path);
    return false;
  }

  const auto payload = member.bytes.ConvertToQByteArray();
  if (file.write(payload) != payload.size() || !file.commit()) {
    error_ = QString("cannot write %1").arg(member.path);
    return false;
  }

  bytes_ += payload.size();
  return true;
}

}  // namespace GpgFrontend
