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
#include <optional>
#include <thread>

#include "core/function/GlobalSettingStation.h"
#include "core/function/InstantMessageOperator.h"
#include "core/utils/RustUtils.h"
Q_LOGGING_CATEGORY(test, "test")

namespace GpgFrontend::Test {

void SetupGlobalTestEnv() {
  auto app_path = GlobalSettingStation::GetInstance().GetAppDir();
  auto test_path = app_path + "/test";
  auto test_config_path = test_path + "/conf/test.ini";
  auto test_data_path = test_path + "/data";

  LOG_I() << "test config file path: " << test_config_path;
  LOG_I() << "test data file path: " << test_data_path;

  // Passphrase encryption derives its key with Argon2id, and the shipped
  // default is RFC 9106's first recommendation -- 2 GiB, about a second per
  // derivation on ordinary hardware. That cost is the point in production and
  // pure dead time here: every symmetric test pays it twice, once to encrypt
  // and once to decrypt, and EncryptSymmetricDecryptStress pays it for every
  // iteration. Switching the whole test process to the RFC's second
  // recommendation (64 MiB) keeps a real Argon2id derivation in the path --
  // the S2K packet is still exercised, still parsed, still RFC 9580 shaped --
  // for roughly a tenth of the work.
  //
  // This changes only what the tests do. The production default lives in
  // InitGpgFrontendCore() and is covered by RpgpArgon2ProfileTest, which tests
  // the token-to-octet mapping rather than this process-global setting.
  SetRpgpArgon2S2kParams(
      RpgpArgon2ParamsOfProfile(kRpgpArgon2ProfileLowMemory));

  // The same trade for instant messages. Their per-message Argon2id is 128 MiB
  // and runs on every encode and every decode with no cache, so the two heavy
  // IM tests alone spent 11s of the suite inside it. Unlike the rPGP S2K this
  // cost is not carried in the token -- it is a protocol constant both sides
  // must agree on -- so it can only ever move for a whole process, never per
  // user. Here both halves move together and round-trips still work.
  //
  // ImPreFilterCostGuard puts the shipped cost back for the one test whose
  // assertion depends on it.
  InstantMessageOperator::SetKdfCostForTesting({1, 8ULL * 1024 * 1024});
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

auto RunWithin(std::function<bool()> op, int timeout_ms)
    -> std::optional<bool> {
  auto task = std::make_shared<std::packaged_task<bool()>>(std::move(op));
  auto future = task->get_future();

  // Detach: if the operation hangs (e.g. a stuck gpg subprocess) we abandon the
  // thread rather than block teardown. The test has already failed by then.
  std::thread([task]() { (*task)(); }).detach();

  if (future.wait_for(std::chrono::milliseconds(timeout_ms)) !=
      std::future_status::ready) {
    return std::nullopt;
  }
  return future.get();
}

auto RunWithTimeout(std::function<bool()> op, int timeout_ms) -> bool {
  return RunWithin(std::move(op), timeout_ms).value_or(false);
}

void RunOnMainThread(const std::function<void()>& op) {
  auto* app = QCoreApplication::instance();

  // A blocking queued call onto our own thread would deadlock, and a test may
  // well already be there (e.g. when driven from a signal handler).
  if (app == nullptr || QThread::currentThread() == app->thread()) {
    op();
    return;
  }

  QMetaObject::invokeMethod(app, op, Qt::BlockingQueuedConnection);
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