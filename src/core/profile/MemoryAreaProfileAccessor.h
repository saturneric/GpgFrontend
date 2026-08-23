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

#include <QMutex>

#include "core/profile/ProfileAccessor.h"

namespace GpgFrontend {

/// Most a single memory-resident area may hold. The areas this is used for
/// contain key files of a few hundred bytes each, so anything approaching this
/// is a package trying to make us allocate rather than a profile.
constexpr qint64 kResidentAreaByteCeiling = 1LL * 1024 * 1024;

/**
 * @brief Holds chosen areas in this process's memory and delegates the rest.
 *
 * A decorator rather than a driver: which areas may be held this way is a
 * property of the area (see ProfileAreaTraits), and *where the rest goes* is
 * still the wrapped driver's decision. Composing the two beats a third driver
 * that would have to reimplement one or the other.
 *
 * The case it exists for is a packaged session. A `.gfp` carries its
 * application key unprotected, so unpacking one normally writes another
 * machine's key material onto this disk in plaintext and then rewrites it in
 * place -- which unlinks the plaintext without overwriting it. Routing the
 * secure area here means those bytes never reach a filesystem at all.
 *
 * **What this does and does not promise.** It promises the area's objects are
 * never written to any filesystem, and that they are erased rather than merely
 * dropped. It does **not** promise the pages cannot be swapped: GFBuffer
 * allocates through the process-wide secure allocator, which only locks memory
 * at GFSecureLevel >= 2 and defaults to 0. Locking this map alone would protect
 * one copy of several and read as a stronger guarantee than it is.
 *
 * **Read() hands out a share, not a copy.** GFBuffer copies are shared, and
 * Zeroize() deliberately wipes through every share -- so a Write() over the
 * same name, a Remove(), or a Release() erases the buffer a caller is still
 * holding from an earlier Read(). That is what makes Release() a real erasure
 * rather than a drop, and it is the point; but a caller that must survive the
 * storage being released has to take storage of its own first, the way
 * ResolveSecureAreaMembers() does before handing key material to a worker
 * thread.
 */
class GF_CORE_EXPORT MemoryAreaProfileAccessor final : public ProfileAccessor {
 public:
  /**
   * @brief Wrap a driver, holding @p resident in memory.
   *
   * @param inner the driver every other area is delegated to; must not be null
   * @param resident areas to hold in memory. Every one must be marked
   * kVirtualisable in the area table -- an area something outside this process
   * opens by path cannot be held here, and asking for one is a programming
   * error rather than a runtime condition.
   */
  MemoryAreaProfileAccessor(QSharedPointer<ProfileAccessor> inner,
                            QSet<ProfileArea> resident);

  ~MemoryAreaProfileAccessor() override;

  [[nodiscard]] auto Driver() const -> QString override;

  auto Ensure(ProfileArea area) -> bool override;

  [[nodiscard]] auto Read(ProfileArea area, const QString &name) const
      -> GFBufferOrNone override;

  auto Write(ProfileArea area, const QString &name, const GFBuffer &value)
      -> bool override;

  auto Remove(ProfileArea area, const QString &name) -> bool override;

  [[nodiscard]] auto Exists(ProfileArea area, const QString &name) const
      -> bool override;

  [[nodiscard]] auto List(ProfileArea area, const QString &pattern) const
      -> QStringList override;

  [[nodiscard]] auto TotalSize(ProfileArea area, const QString &pattern) const
      -> qint64 override;

  [[nodiscard]] auto Settings() const -> QSettings override;

  /**
   * @brief The wrapped driver's path, or empty for a resident area.
   *
   * Empty is the documented answer for storage that has no path, and here it
   * is the truthful one: there is no file, anywhere, to name.
   *
   * @param area area to resolve
   * @param name object within it, or empty for the area itself
   * @return an absolute path, or empty
   */
  [[nodiscard]] auto PathOf(ProfileArea area, const QString &name = {}) const
      -> QString override;

  [[nodiscard]] auto Label() const -> QString override;

  [[nodiscard]] auto IsAreaResident(ProfileArea area) const -> bool override;

  [[nodiscard]] auto IsVolatile() const -> bool override;

  [[nodiscard]] auto IsEncryptedAtRest() const -> bool override;

  [[nodiscard]] auto FreeBytes() const -> qint64 override;

  /**
   * @brief Erase what is held here, then hand the wrapped storage back.
   *
   * Whatever the mode, kKEEP included -- which looks like a divergence from
   * both wrapped drivers and is not one. kKEEP means "the storage outlives the
   * process", and a resident area is this process's memory: there is no mode
   * under which it can be kept, so the only question left is whether it is
   * erased or merely abandoned. Memory this process is about to stop using is
   * not somewhere to leave a key.
   *
   * The wrapped driver still gets the mode unchanged, so what it does with its
   * own storage is unaffected.
   *
   * @param mode passed through to the wrapped driver
   */
  void Release(ProfileStorageRelease mode) override;

 private:
  QSharedPointer<ProfileAccessor> inner_;
  QSet<ProfileArea> resident_;

  /// Guards areas_. Release() runs from the shutdown watchdog's thread while
  /// the main thread may still be reading, and unlike the filesystem driver
  /// this one has mutable state for that to race with.
  mutable QMutex lock_;
  QHash<ProfileArea, QHash<QString, GFBuffer>> areas_;

  /// Erase a buffer that nothing in areas_ still refers to. Never call this on
  /// one that is still stored: GFBuffer::Zeroize() deliberately does not
  /// detach, so it wipes through every share of the storage.
  static void ForgetDetached(GFBuffer &buffer);
};

}  // namespace GpgFrontend
