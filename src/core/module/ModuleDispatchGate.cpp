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

#include "ModuleDispatchGate.h"

#include <QDeadlineTimer>
#include <QHash>
#include <memory>

namespace GpgFrontend::Module {

namespace {

/// How many entries THIS thread holds, per gate. A thread that waits for a
/// gate to go quiet must not wait for itself: the module runner can be inside
/// a module's code, spinning a nested event loop, when the task that
/// deactivates that same module runs.
auto HeldByThisThread() -> QHash<const ModuleDispatchGate*, int>& {
  thread_local QHash<const ModuleDispatchGate*, int> held;
  return held;
}

}  // namespace

auto ModuleDispatchGate::TryEnter() -> bool {
  QMutexLocker locker(&mutex_);
  if (closed_) return false;
  ++in_flight_;
  ++HeldByThisThread()[this];
  return true;
}

void ModuleDispatchGate::Leave() {
  QMutexLocker locker(&mutex_);
  if (in_flight_ > 0) --in_flight_;
  auto& held = HeldByThisThread();
  if (--held[this] <= 0) held.remove(this);
  // Wake every waiter, on every leave: a waiter discounts the entries its own
  // thread holds, so "quiet" for it is not necessarily zero.
  quiet_.wakeAll();
}

void ModuleDispatchGate::Close() {
  QMutexLocker locker(&mutex_);
  closed_ = true;
}

void ModuleDispatchGate::Open() {
  QMutexLocker locker(&mutex_);
  closed_ = false;
}

auto ModuleDispatchGate::IsClosed() -> bool {
  QMutexLocker locker(&mutex_);
  return closed_;
}

auto ModuleDispatchGate::InFlight() -> int {
  QMutexLocker locker(&mutex_);
  return in_flight_;
}

auto ModuleDispatchGate::WaitQuiescent(int timeout_ms) -> bool {
  QMutexLocker locker(&mutex_);

  // A deadline rather than a fresh timeout per wakeup: QWaitCondition can
  // return spuriously, and restarting the full timeout each time would let
  // this wait arbitrarily long while still looking bounded.
  QDeadlineTimer deadline(timeout_ms);
  const auto own = HeldByThisThread().value(this, 0);
  while (in_flight_ - own > 0) {
    if (deadline.hasExpired()) return false;
    quiet_.wait(&mutex_, deadline);
  }
  return true;
}

auto GlobalModuleDispatchGate() -> ModuleDispatchGate& {
  static ModuleDispatchGate gate;
  return gate;
}

auto ModuleEntryGate(const QString& module_id) -> ModuleDispatchGate& {
  // Never freed, like the context records: a thread a module failed to stop
  // may still be about to ask for its gate, and a reference must stay valid.
  // One small object per module id a process ever sees.
  static QMutex mutex;
  static QHash<QString, std::shared_ptr<ModuleDispatchGate>> gates;
  QMutexLocker locker(&mutex);
  auto& gate = gates[module_id];
  if (gate == nullptr) gate = std::make_shared<ModuleDispatchGate>();
  return *gate;
}

}  // namespace GpgFrontend::Module
