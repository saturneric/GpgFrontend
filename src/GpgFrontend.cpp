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

#include <openssl/provider.h>
#include <qcommandlineparser.h>
#include <qloggingcategory.h>

#include <cstddef>

//
#include "Application.h"
#include "BinaryValidate.h"
#include "Command.h"
#include "GpgFrontendContext.h"
#include "Initialize.h"
#include "Security.h"

/**
 *
 * @param argc
 * @param argv
 * @return
 */
auto main(int argc, char* argv[]) -> int {
  // OpenSSL
  auto* defp = OSSL_PROVIDER_load(nullptr, "default");
  if (defp == nullptr) {
    qFatal(
        "The OpenSSL default provider cannot be loaded, and features "
        "such as CSPRNG are not available!");
  }

  // initialize qt resources
  Q_INIT_RESOURCE(gpgfrontend);

  auto const ctx =
      QSharedPointer<GpgFrontend::GpgFrontendContext>::create(argc, argv);

  // create qt core application
  ctx->InitApplication();

  const auto* app = ctx->GetApp();
  Q_ASSERT(app != nullptr);

  // do some early init
  GpgFrontend::PreInit(ctx);

  auto rtn = 0;

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOptions({
      {{"v", "version"}, "show version information"},
      {{"t", "test"}, "run all unit test cases"},
      {{"e", "environment"}, "show environment information"},
      {{"l", "log-level"}, "set log level (debug, info, warn, error)", "none"},
      {{{}, "self-check"}, "check libraries and executables validity"},
  });

  parser.process(*ctx->GetApp());

  const auto self_check = app->property("GFSelfCheck").toBool();
  if ((self_check || parser.isSet("self-check")) && !ValidateLibraries()) {
    QMessageBox::critical(
        nullptr, QObject::tr("Program Self-Test Failed"),
        QObject::tr(
            "The application has detected an issue while verifying essential "
            "libraries and binaries that were digitally signed during the "
            "build. "
            "This means one or more files may have been altered or are being "
            "loaded from the wrong location. For security reasons, the program "
            "must now exit."),
        QMessageBox::Ok);
    return -1;
  }

  if (parser.isSet("v")) {
    return GpgFrontend::PrintVersion();
  }

  if (parser.isSet("l")) {
    GpgFrontend::ParseLogLevel(parser.value("l"));
  }

  if (parser.isSet("e")) {
    return GpgFrontend::PrintEnvInfo();
  }

  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  GpgFrontend::GFBuffer buf;

  if (secure_level > 2) {
    bool ok = false;
    auto pin = QInputDialog::getText(
        nullptr, QObject::tr("PIN Required"),
        QObject::tr("High security mode is enabled.\n\n"
                    "To unlock the application please enter your PIN."),
        QLineEdit::Password, {}, &ok);

    if (!ok || pin.isEmpty()) return 1;

    buf = GpgFrontend::GFBuffer(pin);
    pin.fill('X');
    pin.clear();
  }

  if (!GpgFrontend::InitAppSecureKey(buf)) return 1;

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
