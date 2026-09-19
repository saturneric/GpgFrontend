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

namespace GpgFrontend::Module {

auto ModuleDispatchGate::TryEnter() -> bool {
  QMutexLocker locker(&mutex_);
  if (closed_) return false;
  ++in_flight_;
  return true;
}

void ModuleDispatchGate::Leave() {
  QMutexLocker locker(&mutex_);
  if (in_flight_ > 0) --in_flight_;
  // Wake every waiter rather than one: a teardown may have several watchers
  // (the shutdown path and a test), and waking the wrong single one stalls.
  if (in_flight_ == 0) quiet_.wakeAll();
}

void ModuleDispatchGate::Close() {
  QMutexLocker locker(&mutex_);
  closed_ = true;
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
  while (in_flight_ > 0) {
    if (deadline.hasExpired()) return false;
    quiet_.wait(&mutex_, deadline);
  }
  return true;
}

auto GlobalModuleDispatchGate() -> ModuleDispatchGate& {
  static ModuleDispatchGate gate;
  return gate;
}

}  // namespace GpgFrontend::Module
