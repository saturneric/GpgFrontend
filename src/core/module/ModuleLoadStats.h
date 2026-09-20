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

  /**
   * @brief Scope held for the whole of one native module load.
   *
   * Exists so that "native loading is serial" is a MEASURED property rather
   * than an argument about the shape of the loop. Phase two must stay serial
   * because QLibrary::load() runs third-party static initialisers, and the
   * host cannot establish that one module's are safe against another's -- but
   * nothing stopped a later refactor from parallelising the loop, and nothing
   * would have failed if it did. Now something does.
   */
  class GF_CORE_EXPORT NativeLoadScope {
   public:
    NativeLoadScope();
    ~NativeLoadScope();

    NativeLoadScope(const NativeLoadScope&) = delete;
    auto operator=(const NativeLoadScope&) -> NativeLoadScope& = delete;
  };

  /**
   * @brief Freeze the figures for this start. Called once, when loading ends.
   *
   * The counters are process-wide and keep accepting work afterwards -- a test
   * that verifies a package of its own adds to them, and so does anything else
   * that hashes on the module system's behalf later in the run. Snapshotting
   * here is what lets "this is what STARTUP cost" be asserted at all, rather
   * than "this is what the process has done so far".
   */
  void Finish();

  /// Whether Finish() has run, i.e. whether the figures below are final.
  [[nodiscard]] auto IsFinished() const -> bool;

  /// Bytes hashed so far, live. A caller can measure its own delta with it.
  [[nodiscard]] auto HashedBytes() const -> qint64;

  /// Bytes hashed, frozen at Finish(). One honest pass over every package.
  [[nodiscard]] auto HashedBytesAtFinish() const -> qint64;

  /// The most native loads that were ever in progress at once. Must be 1.
  [[nodiscard]] auto PeakConcurrentNativeLoads() const -> int;

  /// How many distinct threads have performed a native load. Must be 1.
  [[nodiscard]] auto NativeLoadThreadCount() const -> int;

  /// Modules the loader took all the way to registration.
  ///
  /// Exposed because Summary() is prose: a smoke test that has to recover a
  /// number by parsing "loaded 4 module(s), refused 0" out of a log line is a
  /// test coupled to a sentence, and it breaks when the sentence is improved
  /// or a fifth module ships.
  [[nodiscard]] auto LoadedModules() const -> int;

  /// Candidates the scan offered and the loader refused.
  [[nodiscard]] auto RefusedModules() const -> int;

 private:
  ModuleLoadStats() = default;

  void EnterNativeLoad();
  void LeaveNativeLoad();

  std::atomic<qint64> hashed_bytes_{0};
  std::atomic<int> loaded_{0};
  std::atomic<int> refused_{0};
  std::atomic<qint64> began_ms_{0};

  std::atomic<bool> finished_{false};
  std::atomic<qint64> hashed_bytes_at_finish_{0};

  std::atomic<int> native_loads_in_flight_{0};
  std::atomic<int> peak_native_loads_{0};
  std::atomic<int> native_load_threads_{0};
  std::atomic<Qt::HANDLE> first_native_load_thread_{nullptr};
};

}  // namespace GpgFrontend::Module
