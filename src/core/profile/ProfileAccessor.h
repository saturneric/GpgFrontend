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

#include <qsettings.h>

#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief The areas a profile's AppData is divided into.
 *
 * Deliberately a closed set rather than free-form paths: every caller that used
 * to assemble `GetAppDataPath() + "/secure/app.key"` for itself is a place the
 * layout could drift, and a driver that is not a filesystem cannot honour an
 * arbitrary path at all.
 */
enum class ProfileArea : std::uint8_t {
  kRoot,         ///< the profile root itself; profile.json lives here
  kConfig,       ///< config/, where an INI-backed settings file lives
  kDataObjects,  ///< data_objs/, sealed with the profile's key
  kSecure,       ///< secure/, holding app.key and any rotated key
  kLogs,         ///< logs/
  kModules,      ///< mods/, the user's own modules
  kWorkspace,    ///< workspace/, the user's own files
};

/**
 * @brief Unified access to one profile's AppData.
 *
 * The point of the indirection is that "where the application's data lives" and
 * "how it is stored" are two questions, and only the first is the profile's.
 * Today there is exactly one driver, the filesystem; a memory-backed or
 * FUSE-backed profile becomes another implementation of this rather than a
 * second set of call sites.
 *
 * Errors are reported as false or an empty optional and logged here, never
 * thrown: every caller of this is on a path where the honest answer to "the
 * data object would not read" is to carry on without it.
 */
class GF_CORE_EXPORT ProfileAccessor {
 public:
  ProfileAccessor() = default;
  virtual ~ProfileAccessor() = default;

  ProfileAccessor(const ProfileAccessor &) = delete;
  auto operator=(const ProfileAccessor &) -> ProfileAccessor & = delete;
  ProfileAccessor(ProfileAccessor &&) = delete;
  auto operator=(ProfileAccessor &&) -> ProfileAccessor & = delete;

  /**
   * @brief Which driver this is, for the startup log.
   *
   * @return a short stable token, e.g. "fs"
   */
  [[nodiscard]] virtual auto Driver() const -> QString = 0;

  /**
   * @brief Make an area usable, creating it when it is absent.
   *
   * Idempotent.
   *
   * @param area area to provision
   * @return true when the area is usable afterwards
   */
  virtual auto Ensure(ProfileArea area) -> bool = 0;

  /**
   * @brief Read one object out of an area.
   *
   * @param area area holding it
   * @param name object name within the area
   * @return the bytes, or nothing when it is absent or unreadable
   */
  [[nodiscard]] virtual auto Read(ProfileArea area, const QString &name) const
      -> GFBufferOrNone = 0;

  /**
   * @brief Write one object into an area, replacing what was there.
   *
   * Atomic where the driver can be: a half-written data object is
   * indistinguishable from one written with the wrong key, and both look like
   * data loss to the user.
   *
   * @param area area to write into
   * @param name object name within the area
   * @param value bytes to store
   * @return true when the bytes are durably stored
   */
  virtual auto Write(ProfileArea area, const QString &name,
                     const GFBuffer &value) -> bool = 0;

  /**
   * @brief Remove one object. Removing something absent succeeds.
   *
   * @param area area holding it
   * @param name object name within the area
   * @return false only when it exists and could not be removed
   */
  virtual auto Remove(ProfileArea area, const QString &name) -> bool = 0;

  /**
   * @brief Whether an object is there, without reading or decrypting it.
   *
   * Existence is deliberately separate from readability — see
   * DataObjectOperator::HasDataObj(), which needs to tell "nothing saved yet"
   * from "saved, but this session's key will not open it".
   *
   * @param area area to look in
   * @param name object name within the area
   * @return true when an object of that name is stored
   */
  [[nodiscard]] virtual auto Exists(ProfileArea area, const QString &name) const
      -> bool = 0;

  /**
   * @brief Every object name in an area matching a wildcard pattern.
   *
   * @param area area to list
   * @param pattern a glob, e.g. "*" or "*.log"
   * @return the names, in no guaranteed order
   */
  [[nodiscard]] virtual auto List(ProfileArea area,
                                  const QString &pattern) const
      -> QStringList = 0;

  /**
   * @brief Total stored size of everything in an area matching a pattern.
   *
   * @param area area to measure
   * @param pattern a glob
   * @return size in bytes
   */
  [[nodiscard]] virtual auto TotalSize(ProfileArea area,
                                       const QString &pattern) const
      -> qint64 = 0;

  /**
   * @brief The settings store of this profile.
   *
   * @return a QSettings; the platform's native store when the profile has no
   * file of its own
   */
  [[nodiscard]] virtual auto Settings() const -> QSettings = 0;

  /**
   * @brief A real filesystem path, or an empty string when there is none.
   *
   * The deliberate escape hatch, and the one part of this interface a non-
   * filesystem driver cannot answer for free. It exists because several things
   * genuinely cannot be virtualised: GnuPG is handed a home directory, a module
   * is dlopen'd, the log sink writes through spdlog, and a file dialog opens a
   * directory. A driver that stores elsewhere has to materialise these or
   * return empty and let the caller refuse.
   *
   * @param area area to resolve
   * @param name object within it, or empty for the area itself
   * @return an absolute path, or empty
   */
  [[nodiscard]] virtual auto PathOf(ProfileArea area,
                                    const QString &name = {}) const
      -> QString = 0;
};

/**
 * @brief The filesystem driver: areas are directories under one root.
 *
 * This is the layout every existing installation already has, so it is not a
 * choice so much as the shape of the data on disk.
 */
class GF_CORE_EXPORT FsProfileAccessor final : public ProfileAccessor {
 public:
  /**
   * @brief Construct the driver.
   *
   * @param root absolute profile root
   * @param settings_file the profile's INI, or empty to use the native store
   */
  FsProfileAccessor(QString root, QString settings_file);

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

  [[nodiscard]] auto PathOf(ProfileArea area, const QString &name = {}) const
      -> QString override;

 private:
  QString root_;
  QString settings_file_;
};

}  // namespace GpgFrontend
