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

/// Native-library primitives the module loader needs from the operating
/// system. Built into gf_core.

namespace GpgFrontend::Platform {

/**
 * @brief The Authenticode SHA-256 image digest of a PE file, as hex.
 *
 * Computed by the operating system, so it exists only on Windows; elsewhere
 * this refuses.
 *
 * @param reason why there is no digest, when there is none
 * @return the digest, or empty on failure
 */
auto GF_CORE_EXPORT PeImageDigestSha256(const QString &path, QString &reason)
    -> QString;

/**
 * @brief Add a directory to the native library search path for the lifetime
 * of the guard.
 *
 * Windows only (SetDllDirectory); a no-op elsewhere, where a library's own
 * dependencies are found through its rpath. The search path is process-global,
 * so a caller holds this only across a load it serializes.
 */
class GF_CORE_EXPORT ScopedLibrarySearchDirectory {
 public:
  explicit ScopedLibrarySearchDirectory(const QString &directory);
  ~ScopedLibrarySearchDirectory();

  ScopedLibrarySearchDirectory(const ScopedLibrarySearchDirectory &) = delete;
  auto operator=(const ScopedLibrarySearchDirectory &)
      -> ScopedLibrarySearchDirectory & = delete;
  ScopedLibrarySearchDirectory(ScopedLibrarySearchDirectory &&) = delete;
  auto operator=(ScopedLibrarySearchDirectory &&)
      -> ScopedLibrarySearchDirectory & = delete;

 private:
  // Only Windows has a search path to restore.
  [[maybe_unused]] bool applied_ = false;
};

}  // namespace GpgFrontend::Platform
