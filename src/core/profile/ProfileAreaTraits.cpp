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

#include "core/profile/ProfileAreaTraits.h"

#include <array>

namespace GpgFrontend {

namespace {

// Spelled out rather than pulled in with `using enum`: this translation unit
// is built as C++17.
constexpr auto kPathRequired = AreaResidency::kPathRequired;
constexpr auto kVirtualisable = AreaResidency::kVirtualisable;
constexpr auto kNever = AreaPackaging::kNever;
constexpr auto kAlways = AreaPackaging::kAlways;
constexpr auto kOptional = AreaPackaging::kOptional;
constexpr auto kTree = AreaPackSource::kTree;
constexpr auto kAccessor = AreaPackSource::kAccessor;

/// One row per top-level name a profile root may contain.
///
/// Read this as the answer to "what is in a package", because it is: a name
/// absent from here is absent from every package, and that is deliberate. The
/// rule it replaced was a deny-list, which shipped anything a user happened to
/// leave in the profile folder and would have shipped whatever the next feature
/// started writing there.
constexpr std::array kAreaTable{
    // profile.json lives here and is the only thing at the root that travels;
    // the tree walk admits the root itself so its children can be judged.
    ProfileAreaTraits{QLatin1StringView(""), ProfileArea::kRoot, kPathRequired,
                      kAlways, kTree, false},

    // QSettings opens config.ini by path, so this cannot be virtualised. The
    // file that travels is regenerated from the live store rather than copied.
    ProfileAreaTraits{QLatin1StringView("config"), ProfileArea::kConfig,
                      kPathRequired, kAlways, kTree, false},

    // Already sealed per object under a key derived from the profile key, with
    // HMAC'd filenames, so it is secret but not plaintext.
    ProfileAreaTraits{QLatin1StringView("data_objs"), ProfileArea::kDataObjects,
                      kPathRequired, kAlways, kTree, true},

    // The only virtualisable area, and the reason the column exists: nothing
    // outside this process opens these files. A packaged session therefore
    // holds the profile's own key in memory and never writes it here.
    //
    // kAccessor follows from that: once a driver may hold the area in memory,
    // a packer that walked the tree would silently ship nothing.
    ProfileAreaTraits{QLatin1StringView("secure"), ProfileArea::kSecure,
                      kVirtualisable, kAlways, kAccessor, true},

    // This machine's history with the profile, not the profile.
    ProfileAreaTraits{QLatin1StringView("logs"), ProfileArea::kLogs,
                      kPathRequired, kNever, kTree, false},

    // dlopen'd, and the recipient's platform may not even be able to load them.
    ProfileAreaTraits{QLatin1StringView("mods"), ProfileArea::kModules,
                      kPathRequired, kNever, kTree, false},

    // The user's own files: theirs to send or not, so the export asks.
    ProfileAreaTraits{QLatin1StringView("workspace"), ProfileArea::kWorkspace,
                      kPathRequired, kOptional, kTree, false},

    // Staging for this session alone. Dot-prefixed so a profiles-folder scan
    // skips it, and never packed whatever it happens to hold at the time.
    ProfileAreaTraits{QLatin1StringView(".scratch"), ProfileArea::kScratch,
                      kPathRequired, kNever, kTree, false},

    // The key databases. Directories GnuPG and rPGP own rather than areas this
    // program addresses, so they carry no ProfileArea — but they are the most
    // important thing in the package and the table is what says so.
    //
    // A database the user pointed at by hand lands outside all three and
    // therefore does not travel; see ManagedKeyDatabaseDirs().
    ProfileAreaTraits{QLatin1StringView("db"), std::nullopt, kPathRequired,
                      kAlways, kTree, true},
    ProfileAreaTraits{QLatin1StringView("dbs"), std::nullopt, kPathRequired,
                      kAlways, kTree, true},
    ProfileAreaTraits{QLatin1StringView("rpgp_db"), std::nullopt, kPathRequired,
                      kAlways, kTree, true},
};

}  // namespace

auto ProfileAreaTable() -> const QList<ProfileAreaTraits> & {
  // Built once from the constexpr table. Returned by reference because every
  // caller iterates it and none of them owns it.
  static const QList<ProfileAreaTraits> kTable(kAreaTable.begin(),
                                               kAreaTable.end());
  return kTable;
}

auto TraitsForTopLevel(QStringView name) -> const ProfileAreaTraits * {
  for (const auto &row : kAreaTable) {
    if (name == row.dir) return &row;
  }
  return nullptr;
}

auto TraitsForArea(ProfileArea area) -> const ProfileAreaTraits * {
  for (const auto &row : kAreaTable) {
    if (row.area == area) return &row;
  }

  // Unreachable: every enumerator has a row, and the test suite asserts it.
  // Returning null rather than asserting keeps a future enumerator from
  // crashing a release build before anyone has run the tests.
  return nullptr;
}

auto ProfileAreaDirName(ProfileArea area) -> QString {
  // An area with no row would return an empty name, and an empty directory name
  // resolves to the profile root itself -- so PathOf(new_area, "x") would
  // quietly become <root>/x. Adding an enumerator without a row is a programming
  // error, and this is where it should be noticed.
  Q_ASSERT(TraitsForArea(area) != nullptr);
  const auto *traits = TraitsForArea(area);
  return traits == nullptr ? QString{} : QString(traits->dir);
}

auto IsRefusedInsidePackagedArea(const QString &relative_path) -> bool {
  const auto name = relative_path.section('/', -1);

  // A lock describes a process that will not exist at the other end, and one
  // that travels would tell the recipient their profile is already open.
  if (name == "profile.lock" || name == "profiles.lock") return true;

  // gpg-agent's sockets, which are bound to a path on this machine.
  if (name.startsWith("S.gpg-agent") || name == "S.dirmngr") return true;

  // Desktop clutter and editor leftovers, which are nobody's data.
  if (name == ".DS_Store" || name == "Thumbs.db" || name == "desktop.ini") {
    return true;
  }
  if (name.endsWith('~')) return true;

  return false;
}

auto IsIncludedInPackage(const QString &relative_path, bool include_workspace)
    -> bool {
  if (relative_path.isEmpty()) return false;

  // Never let a path climb out of the profile, however it is spelled. Checked
  // before anything else because every rule below reads a component of it.
  const auto components = relative_path.split('/', Qt::SkipEmptyParts);
  if (components.contains("..")) return false;
  if (components.isEmpty()) return false;

  const auto &top = components.first();

  // The root's own children are judged one by one: profile.json travels, and
  // anything else sitting loose at the root is somebody's stray file.
  if (components.size() == 1) {
    if (top == "profile.json") return true;
    const auto *traits = TraitsForTopLevel(top);
    if (traits == nullptr) return false;

    // A directory is admitted so the walk can descend into it; whether its
    // contents travel is decided per entry on the way down.
    return traits->packaging == AreaPackaging::kAlways ||
           (traits->packaging == AreaPackaging::kOptional && include_workspace);
  }

  const auto *traits = TraitsForTopLevel(top);
  if (traits == nullptr) return false;

  switch (traits->packaging) {
    case AreaPackaging::kNever:
      return false;
    case AreaPackaging::kOptional:
      if (!include_workspace) return false;
      break;
    case AreaPackaging::kAlways:
      break;
  }

  // An area whose bytes come from the accessor is emitted separately, from
  // wherever the driver actually put it. Walking the tree for it as well would
  // pack it twice on a filesystem driver and not at all on a memory one.
  if (traits->pack_source == AreaPackSource::kAccessor) return false;

  return !IsRefusedInsidePackagedArea(relative_path);
}

auto ManagedKeyDatabaseDirs() -> QStringList {
  // Built once. The table is constexpr, and the one caller is a predicate run
  // per path during a tree walk.
  static const QStringList kDirs = []() {
    QStringList dirs;
    for (const auto &row : kAreaTable) {
      if (!row.area.has_value() && row.packaging == AreaPackaging::kAlways) {
        dirs << QString(row.dir);
      }
    }
    return dirs;
  }();
  return kDirs;
}

auto IsManagedKeyDatabasePath(const QString &relative_path) -> bool {
  if (relative_path.isEmpty()) return false;

  const auto components = relative_path.split('/', Qt::SkipEmptyParts);
  if (components.isEmpty() || components.contains("..")) return false;

  return ManagedKeyDatabaseDirs().contains(components.first());
}

}  // namespace GpgFrontend
