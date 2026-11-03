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
#include "boot/BootContext.h"
#include "boot/BootLog.h"
#include "boot/Initialize.h"
#include "core/GFCoreLog.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileLoader.h"
#include "core/utils/BuildInfoUtils.h"
#include "res/GpgFrontendResource.h"

namespace {

/// Nobody is there to answer: no PIN, no forced lock, no key reset.
class HeadlessProfileLoaderDelegate
    : public GpgFrontend::ProfileLoaderDelegate {
 public:
  auto AskPackagePassphrase(const QString&, bool)
      -> std::optional<GpgFrontend::GFBuffer> override {
    return std::nullopt;
  }

  auto AskAppKeyPin(const GpgFrontend::AppKeyPinRequest&)
      -> GpgFrontend::AppKeyPinAnswer override {
    return {};
  }

  auto ConfirmForceUnlock(const GpgFrontend::ProfileLockResult&)
      -> bool override {
    return false;
  }

  auto ConfirmKeyReset(GpgFrontend::ProfileKeyResetReason) -> bool override {
    return false;
  }

  void Report(const GpgFrontend::ProfileLoadError& error) override {
    qCritical() << "profile did not open:" << static_cast<int>(error.failure)
                << error.subject << error.detail << error.actor;
  }

  void Note(GpgFrontend::ProfileLoadNotice notice,
            const QString& detail) override {
    qInfo() << "profile notice:" << static_cast<int>(notice) << detail;
  }
};

}  // namespace

/**
 * @brief The core's unit tests, without an interface.
 *
 * Starts the way the application does -- the same profile, the same core --
 * on a plain QCoreApplication, so what passes here does not depend on gf_ui.
 */
auto main(int argc, char* argv[]) -> int {
  GpgFrontend::InitResources();

  QCoreApplication app(argc, argv);
  // The on-disk identity the application uses, so the profile is found where
  // the application would find it.
  GpgFrontend::SetApplicationIdentity();

  auto const ctx = QSharedPointer<GpgFrontend::BootContext>::create(argc, argv);

  // `-l LEVEL`, as `gpgfrontend -t` takes it; GoogleTest ignores it.
  const auto args = QCoreApplication::arguments();
  for (int i = 1; i + 1 < args.size(); i++) {
    if (args[i] != "-l" && args[i] != "--log-level") continue;
    if (const auto level = GpgFrontend::ParseLogLevelName(args[i + 1])) {
      ctx->cli_log_level = *level;
      GpgFrontend::ApplyLogLevel(*level);
    }
  }

  ctx->SelectProfile();
  if (!ctx->profile_error.isEmpty()) {
    qCritical() << "profile selection failed:" << ctx->profile_error;
    return 1;
  }

  HeadlessProfileLoaderDelegate delegate;
  if (!GpgFrontend::OpenProfile(
          ctx, GpgFrontend::MakeProfile(ctx->profile_selection), delegate)) {
    if (!ctx->fatal_error.isEmpty()) qCritical() << ctx->fatal_error;
    return 1;
  }

  ctx->gather_external_gnupg_info = false;
  // The core's unit-test mode, as the tests expect, unless
  // GF_TEST_AS_APPLICATION=1: then the core starts exactly as the application
  // does, key databases from the profile's settings included. The profile
  // interop stages (scripts/run_interop.sh) run that way, against a profile
  // another build wrote.
  ctx->unit_test_mode =
      qEnvironmentVariableIntValue("GF_TEST_AS_APPLICATION") != 1;
  GpgFrontend::InitGlobalBasicEnvSync(ctx);

  ctx->rtn = GpgFrontend::kNonRestartCode;
  const auto rtn = GpgFrontend::Test::ExecuteAllTestCaseOffMainThread(
      {.argc = argc, .argv = argv});

  GpgFrontend::ShutdownGlobalBasicEnv(ctx);
  return rtn;
}
