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
#include <QWaitCondition>

#include "core/module/Module.h"

namespace GpgFrontend::Module {

/**
 * @brief Admission control for calls that enter module code.
 *
 * Two kinds exist. The process-wide gate closes once, when shutdown begins.
 * Each module also has its own (ModuleEntryGate), open while it is active and
 * closed from the moment its deactivation begins. A host-to-module call must
 * pass both; see gf_sdk_internal::EnterModule().
 *
 * Teardown needs two things from a gate:
 *
 *   1. a way to STOP admitting new calls, so the set of in-flight calls can
 *      only shrink; and
 *   2. a way to WAIT until the calls already inside have returned.
 *
 * Deliberately a counter and a condition variable rather than a blocking
 * queued call to the module runner: the GUI thread can already be inside a
 * nested event loop waiting on that runner (GpgOperaHelper::WaitForOpera), so
 * a blocking call back the other way would close a cycle.
 */
class GF_CORE_EXPORT ModuleDispatchGate {
 public:
  /**
   * @brief Try to enter module code.
   * @return false when the gate is closed; the caller must NOT proceed.
   */
  auto TryEnter() -> bool;

  /// Balance a successful TryEnter(). Wakes a waiting Close/WaitQuiescent.
  void Leave();

  /// Stop admitting. Calls already inside are unaffected and still Leave().
  void Close();

  /// Admit again. Only a per-module gate reopens -- on reactivation. The
  /// process-wide gate is closed once, at shutdown, and stays closed.
  void Open();

  [[nodiscard]] auto IsClosed() -> bool;

  /// In-flight calls right now. Diagnostic; do not branch on it.
  [[nodiscard]] auto InFlight() -> int;

  /**
   * @brief Wait until no call is inside module code.
   *
   * Calls the WAITING thread itself is inside are not waited for: a thread
   * can only be here while such a call is on its own stack, and waiting for
   * it would wait forever.
   *
   * @param timeout_ms how long to wait before giving up
   * @return true when it went quiet, false on timeout -- and a timeout is
   *         reported rather than swallowed, because proceeding to free a
   *         module's resources after one would be exactly the use-after-free
   *         this class exists to prevent.
   */
  auto WaitQuiescent(int timeout_ms) -> bool;

 private:
  QMutex mutex_;
  QWaitCondition quiet_;
  int in_flight_ = 0;
  bool closed_ = false;
};

/**
 * @brief RAII admission ticket. Check Entered() before touching module code.
 *
 * Deliberately not convertible to bool implicitly: a caller has to say what
 * it is checking, because silently skipping the check is the failure mode.
 */
class GF_CORE_EXPORT ModuleDispatchScope {
 public:
  explicit ModuleDispatchScope(ModuleDispatchGate& gate)
      : gate_(gate), entered_(gate.TryEnter()) {}

  ~ModuleDispatchScope() {
    if (entered_) gate_.Leave();
  }

  ModuleDispatchScope(const ModuleDispatchScope&) = delete;
  auto operator=(const ModuleDispatchScope&) -> ModuleDispatchScope& = delete;

  [[nodiscard]] auto Entered() const -> bool { return entered_; }

 private:
  ModuleDispatchGate& gate_;
  bool entered_;
};

/// Returned to a caller whose call arrived after the gate closed. Negative,
/// so it lands in the existing "execution failed" branch rather than being
/// mistaken for a successful run.
constexpr int kModuleUnloadingCode = -1000;

/// The process-wide gate guarding entry into module code.
auto GF_CORE_EXPORT GlobalModuleDispatchGate() -> ModuleDispatchGate&;

/**
 * @brief The gate guarding entry into ONE module's code.
 *
 * Open while the module is active; closed from the moment its deactivation
 * begins. Every host-to-module call passes this and the process-wide gate,
 * so a call queued before a deactivation cannot enter the module after it.
 * The reference is valid for the life of the process.
 */
auto GF_CORE_EXPORT ModuleEntryGate(const QString& module_id)
    -> ModuleDispatchGate&;

}  // namespace GpgFrontend::Module
