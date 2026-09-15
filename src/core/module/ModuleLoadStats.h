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

#include <QString>
#include <atomic>

namespace GpgFrontend::Module {

/**
 * @brief What one start spent getting modules loaded.
 *
 * Kept because the cost of loading is not something to reason about from the
 * gap between two log lines. Twice while building this, a confident diagnosis
 * from exactly that -- "it must be the hashing", "it must be the pipe" -- was
 * wrong, and the answer only appeared once the work was counted where it
 * happened. So it is counted where it happens.
 *
 * Process-wide and monotonic within a start: modules are loaded once, and a
 * second figure for a second load would mean something has gone wrong
 * elsewhere.
 */
class GF_CORE_EXPORT ModuleLoadStats {
 public:
  static auto GetInstance() -> ModuleLoadStats&;

  /// Bytes pushed through SHA-256 on behalf of module loading, wherever.
  void AddHashedBytes(qint64 bytes);

  /// One more module the loader took all the way to registration.
  void AddLoadedModule();

  /// One more candidate the scan offered and the loader refused.
  void AddRefusedModule();

  /// Begin timing. Called once, when the scan starts.
  void Begin();

  /// A one-line summary, for the log. Empty before Begin().
  [[nodiscard]] auto Summary() const -> QString;

 private:
  ModuleLoadStats() = default;

  std::atomic<qint64> hashed_bytes_{0};
  std::atomic<int> loaded_{0};
  std::atomic<int> refused_{0};
  std::atomic<qint64> began_ms_{0};
};

}  // namespace GpgFrontend::Module
