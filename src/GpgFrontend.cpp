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

#include <qcommandlineparser.h>
#include <qloggingcategory.h>

//
#include "Application.h"
#include "Command.h"
#include "GpgFrontendContext.h"
#include "Initialize.h"
#include "core/GFCoreLog.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/BuildInfoUtils.h"
#include "platform/PlatformSecretStore.h"
#include "res/GpgFrontendResource.h"
#include "ui/function/GuiProfileLoaderDelegate.h"

auto main(int argc, char* argv[]) -> int {
  // initialize qt resources (embedded in the gf_res shared library)
  GpgFrontend::InitResources();

  auto const ctx =
      QSharedPointer<GpgFrontend::GpgFrontendContext>::create(argc, argv);

  // create qt core application, and work out which profile was asked for
  ctx->InitApplication();

  const auto* app = ctx->GetApp();
  Q_ASSERT(app != nullptr);

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOptions({
      {{"v", "version"}, "show version information"},
      {{"t", "test"}, "run all unit test cases"},
      {{"e", "environment"}, "show environment information"},
      {{"l", "log-level"}, "set log level (debug, info, warn, error)", "none"},
      // Declaration only: this was already resolved during InitApplication(),
      // long before this parser existed, because where the settings live is
      // exactly what it decides. Registering it here just stops
      // parser.process() rejecting it as unknown.
      {{{}, "profile"}, "open the named local profile", "id"},
  });
  parser.addPositionalArgument("file", "a .gfp package to open",
                               "[file]");

  // Hold back GoogleTest flags (`--gtest_*`) from the app's parser, which would
  // otherwise reject them as unknown options. They are consumed later by
  // testing::InitGoogleTest, which reads the unmodified argv during RunTest, so
  // `gpgfrontend -t --gtest_filter=...` works without the GTEST_* env vars.
  QStringList parser_args;
  for (const auto& arg : QCoreApplication::arguments()) {
    if (arg.startsWith("--gtest_")) continue;
    parser_args << arg;
  }
  parser.process(parser_args);

  if (parser.isSet("v")) {
    return GpgFrontend::PrintVersion();
  }

  // Applied straight away so that everything logged between here and PreInit()
  // honours the flag, and carried on the context so that the layered resolution
  // in LoadEnvProperties() puts it on top rather than overwriting it.
  if (parser.isSet("l")) {
    if (const auto level = GpgFrontend::ParseLogLevelName(parser.value("l"))) {
      ctx->cli_log_level = *level;
      GpgFrontend::ApplyLogLevel(*level);
    } else {
      qWarning() << "ignoring unrecognised log level:" << parser.value("l");
    }
  }

  // The selection was resolved during InitApplication(), before any setting was
  // read. A failure there is reported here, where there is a usable dialog, and
  // still before anything opens key material.
  if (!ctx->profile_error.isEmpty()) {
    qCritical() << "profile selection failed:" << ctx->profile_error;
    QMessageBox::critical(nullptr, QObject::tr("Cannot Open Profile"),
                          ctx->profile_error, QMessageBox::Ok);
    return 1;
  }

  // Everything below this line is keyed by the profile: its settings, its log
  // directory, its key material. So the profile is mounted first — the lock
  // taken, a package extracted — and only then is anything read.
  GpgFrontend::UI::GuiProfileLoaderDelegate delegate;
  GpgFrontend::ProfileLoader loader(
      GpgFrontend::MakeProfile(ctx->profile_selection), &delegate);

  if (!loader.Mount(GpgFrontend::GetAppProfileSchemaVersion())) return 1;

  ctx->LoadEnvProperties();

  // do some early init, now that the log has somewhere to write and the
  // properties it reads have been resolved against the right profile
  GpgFrontend::PreInit(ctx);

  // Installed before the -e early return so that environment information can
  // report the credential store. Safe this early: on every platform this only
  // loads a library and registers, never probing the store, so it cannot raise
  // a keyring unlock prompt from a command that just prints to stdout.
  GpgFrontend::InstallPlatformSecretStore();

  // The compatibility gate, the migrations and the key set. Everything that
  // needs a human goes through the delegate.
  const auto secure_level = app->property("GFSecureLevel").toInt();
  if (!loader.Open(GpgFrontend::GetAppProfileSchemaVersion(),
                   secure_level >= 3)) {
    return 1;
  }

  // After Open(), because the key-database list it prints is a sealed data
  // object. It used to abort here instead: the key set was a singleton nothing
  // on this path had ever initialised, and reading it was undefined rather than
  // refused.
  if (parser.isSet("e")) {
    return GpgFrontend::PrintEnvInfo();
  }

  auto rtn = 0;

  if (parser.isSet("t")) {
    ctx->gather_external_gnupg_info = false;
    ctx->unit_test_mode = true;

    InitGlobalBasicEnvSync(ctx);
    rtn = RunTest(ctx);
    ShutdownGlobalBasicEnv(ctx);
    return rtn;
  }

  ctx->gather_external_gnupg_info = true;
  ctx->unit_test_mode = false;

  InitGlobalBasicEnv(ctx, true);

  rtn = StartApplication(ctx);
  ShutdownGlobalBasicEnv(ctx);
  return rtn;
}
