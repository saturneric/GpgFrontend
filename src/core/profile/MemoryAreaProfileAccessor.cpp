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

#include "core/profile/MemoryAreaProfileAccessor.h"

#include <QMutexLocker>
#include <QRegularExpression>

#include "core/profile/ProfileAreaTraits.h"

namespace GpgFrontend {

MemoryAreaProfileAccessor::MemoryAreaProfileAccessor(
    QSharedPointer<ProfileAccessor> inner, QSet<ProfileArea> resident)
    : inner_(std::move(inner)), resident_(std::move(resident)) {
  // Not a Q_ASSERT: that compiles out in release, and every method here
  // dereferences this. A driver with nothing underneath it is a programming
  // error worth failing loudly for in any build.
  if (inner_.isNull()) qFatal("MemoryAreaProfileAccessor has no inner driver");

  // An area something outside this process opens by path cannot live here:
  // GnuPG is handed a home directory, QSettings opens a file, modules are
  // dlopen'd. Holding one in memory would not fail loudly, it would hand out an
  // empty path and let the caller operate on the working directory.
  for (auto it = resident_.begin(); it != resident_.end();) {
    const auto *traits = TraitsForArea(*it);
    if (traits != nullptr &&
        traits->residency == AreaResidency::kVirtualisable) {
      ++it;
      continue;
    }

    LOG_E() << "refusing to hold an area in memory that needs a real path:"
            << ProfileAreaDirName(*it);
    it = resident_.erase(it);
  }
}

MemoryAreaProfileAccessor::~MemoryAreaProfileAccessor() {
  // Not merely dropped. At the default GFSecureLevel of 0 the allocator frees
  // without wiping, so a key that is only released stays legible in freed heap.
  Release(ProfileStorageRelease::kFAST);
}

void MemoryAreaProfileAccessor::ForgetDetached(GFBuffer &buffer) {
  buffer.Zeroize();
}

auto MemoryAreaProfileAccessor::IsAreaResident(ProfileArea area) const -> bool {
  return resident_.contains(area);
}

auto MemoryAreaProfileAccessor::Driver() const -> QString {
  // The wrapped driver's token plus what this adds, so one line of the startup
  // log says both where the profile went and what was kept out of it.
  return inner_->Driver() + "+mem";
}

auto MemoryAreaProfileAccessor::Ensure(ProfileArea area) -> bool {
  if (IsAreaResident(area)) return true;
  return inner_->Ensure(area);
}

auto MemoryAreaProfileAccessor::Read(ProfileArea area,
                                     const QString &name) const
    -> GFBufferOrNone {
  if (!IsAreaResident(area)) return inner_->Read(area, name);

  const QMutexLocker locker(&lock_);
  const auto objects = areas_.value(area);
  const auto it = objects.constFind(name);
  if (it == objects.constEnd()) return {};
  return *it;
}

auto MemoryAreaProfileAccessor::Write(ProfileArea area, const QString &name,
                                      const GFBuffer &value) -> bool {
  if (!IsAreaResident(area)) return inner_->Write(area, name, value);
  if (name.isEmpty()) return false;

  const QMutexLocker locker(&lock_);
  auto &objects = areas_[area];

  // Untrusted input reaches this: a package decides what its members are called
  // and how large they are, and this map is process heap rather than the
  // storage the probe measured and budgeted for.
  qint64 total = static_cast<qint64>(value.Size());
  for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
    if (it.key() == name) continue;
    total += static_cast<qint64>(it.value().Size());
  }
  if (total > kResidentAreaByteCeiling) {
    LOG_E() << "refusing to hold" << total << "bytes in the"
            << ProfileAreaDirName(area) << "area for" << name
            << "; the ceiling is" << kResidentAreaByteCeiling;
    return false;
  }

  // Take the old value out of the map first, then erase it. Zeroize() wipes
  // through every share of the storage rather than detaching a private copy, so
  // erasing one the map still held would destroy what is stored rather than
  // what is being replaced.
  auto replaced = objects.take(name);
  objects.insert(name, value);
  ForgetDetached(replaced);
  return true;
}

auto MemoryAreaProfileAccessor::Remove(ProfileArea area, const QString &name)
    -> bool {
  if (!IsAreaResident(area)) return inner_->Remove(area, name);
  if (name.isEmpty()) return false;

  const QMutexLocker locker(&lock_);
  auto areas_it = areas_.find(area);
  if (areas_it == areas_.end()) return true;

  auto removed = areas_it->take(name);
  ForgetDetached(removed);
  return true;
}

auto MemoryAreaProfileAccessor::Exists(ProfileArea area,
                                       const QString &name) const -> bool {
  if (!IsAreaResident(area)) return inner_->Exists(area, name);

  const QMutexLocker locker(&lock_);
  return areas_.value(area).contains(name);
}

auto MemoryAreaProfileAccessor::List(ProfileArea area,
                                     const QString &pattern) const
    -> QStringList {
  if (!IsAreaResident(area)) return inner_->List(area, pattern);

  // Anchored, which is what QDir::entryList() does for the driver this
  // decorates and what every caller means. Unanchored made "*.key" a substring
  // match, so a name like "rotated.key.bak" -- and an imported package chooses
  // the names in this area -- was listed here and not by the filesystem driver,
  // then fed to the trial-decrypt loops that walk this listing.
  const auto matcher = QRegularExpression::fromWildcard(
      pattern.isEmpty() ? QString("*") : pattern, Qt::CaseSensitive,
      QRegularExpression::DefaultWildcardConversion);

  const QMutexLocker locker(&lock_);
  QStringList names;
  const auto objects = areas_.value(area);
  for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
    if (matcher.match(it.key()).hasMatch()) names << it.key();
  }
  return names;
}

auto MemoryAreaProfileAccessor::TotalSize(ProfileArea area,
                                          const QString &pattern) const
    -> qint64 {
  if (!IsAreaResident(area)) return inner_->TotalSize(area, pattern);

  const auto names = List(area, pattern);

  const QMutexLocker locker(&lock_);
  const auto objects = areas_.value(area);
  qint64 total = 0;
  for (const auto &name : names) {
    total += static_cast<qint64>(objects.value(name).Size());
  }
  return total;
}

auto MemoryAreaProfileAccessor::Settings() const -> QSettings {
  return inner_->Settings();
}

auto MemoryAreaProfileAccessor::PathOf(ProfileArea area,
                                       const QString &name) const -> QString {
  if (IsAreaResident(area)) return {};
  return inner_->PathOf(area, name);
}

// Everything below describes the storage the wrapped driver provisioned, and
// says so unchanged. Reporting this storage as volatile because one area is
// held in memory would be a claim about the whole tree -- including the GnuPG
// home directory, where the user's own private keys are.
auto MemoryAreaProfileAccessor::Label() const -> QString {
  return inner_->Label();
}

auto MemoryAreaProfileAccessor::IsVolatile() const -> bool {
  return inner_->IsVolatile();
}

auto MemoryAreaProfileAccessor::IsEncryptedAtRest() const -> bool {
  return inner_->IsEncryptedAtRest();
}

auto MemoryAreaProfileAccessor::FreeBytes() const -> qint64 {
  return inner_->FreeBytes();
}

void MemoryAreaProfileAccessor::Release(ProfileStorageRelease mode) {
  {
    const QMutexLocker locker(&lock_);

    // Detached from the map before being erased, for the same reason Write()
    // takes the old value out first.
    auto areas = std::move(areas_);
    areas_.clear();

    for (auto area_it = areas.begin(); area_it != areas.end(); ++area_it) {
      for (auto it = area_it->begin(); it != area_it->end(); ++it) {
        ForgetDetached(it.value());
      }
    }
  }

  inner_->Release(mode);
}

}  // namespace GpgFrontend
