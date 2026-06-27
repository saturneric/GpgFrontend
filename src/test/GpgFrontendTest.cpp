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

#include "GpgFrontendTest.h"

#include <qglobal.h>

#include <chrono>
#include <future>
#include <memory>
#include <thread>

#include "core/function/GlobalSettingStation.h"
Q_LOGGING_CATEGORY(test, "test")

auto GF_TEST_EXPORT GFTestValidateSymbol() -> int { return 0; }

namespace GpgFrontend::Test {

void SetupGlobalTestEnv() {
  auto app_path = GlobalSettingStation::GetInstance().GetAppDir();
  auto test_path = app_path + "/test";
  auto test_config_path = test_path + "/conf/test.ini";
  auto test_data_path = test_path + "/data";

  LOG_I() << "test config file path: " << test_config_path;
  LOG_I() << "test data file path: " << test_data_path;
}

auto ExecuteAllTestCase(GpgFrontendContext args) -> int {
  SetupGlobalTestEnv();

  testing::InitGoogleTest(&args.argc, args.argv);
  return RUN_ALL_TESTS();
}

auto WaitFor(std::function<bool()> cond, int timeout_ms) -> bool {
  QEventLoop loop;
  QTimer timer;
  timer.setSingleShot(true);

  bool matched = false;

  QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  QTimer check_timer;
  check_timer.setInterval(20);

  QObject::connect(&check_timer, &QTimer::timeout, [&]() {
    if (cond()) {
      matched = true;
      loop.quit();
    }
  });

  timer.start(timeout_ms);
  check_timer.start();

  if (cond()) {
    matched = true;
  } else {
    loop.exec();
  }

  return matched;
}

auto RunWithTimeout(std::function<bool()> op, int timeout_ms) -> bool {
  auto task = std::make_shared<std::packaged_task<bool()>>(std::move(op));
  auto future = task->get_future();

  // Detach: if the operation hangs (e.g. a stuck gpg subprocess) we abandon the
  // thread rather than block teardown. The test has already failed by then.
  std::thread([task]() { (*task)(); }).detach();

  if (future.wait_for(std::chrono::milliseconds(timeout_ms)) !=
      std::future_status::ready) {
    return false;
  }
  return future.get();
}

auto GenerateRandomString(size_t length) -> QString {
  const QString characters =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, characters.size() - 1);

  QString random_string;
  for (size_t i = 0; i < length; ++i) {
    random_string += characters[distribution(generator)];
  }

  return random_string;
}

}  // namespace GpgFrontend::Test