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

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QThread>
#include <chrono>
#include <functional>
#include <thread>

/**
 * @file MobileTest.h
 * @brief What gf_mob_tests's tests share.
 *
 * The tests run on a worker while the main thread keeps the event loop
 * going, as under the other runners; these two helpers are how a test reaches
 * across.
 */

namespace GpgFrontend::Test::Mobile {

/// Run @p op on the main thread and wait for it.
inline void RunOnMainThread(const std::function<void()>& op) {
  auto* app = QCoreApplication::instance();
  if (QThread::currentThread() == app->thread()) {
    op();
    return;
  }
  QMetaObject::invokeMethod(app, op, Qt::BlockingQueuedConnection);
}

/// Poll @p cond until it holds or @p timeout_ms passes.
inline auto WaitFor(const std::function<bool()>& cond, int timeout_ms = 5000)
    -> bool {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (cond()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return cond();
}

}  // namespace GpgFrontend::Test::Mobile
