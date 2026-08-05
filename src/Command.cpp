/**
 * Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
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

#include "Command.h"

#include <qdatetime.h>
#include <qglobal.h>
#include <qloggingcategory.h>
#include <qstring.h>
#include <qtextstream.h>

#include <optional>

#include "core/GFCoreInit.h"
#include "core/GFCoreLog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SystemSecretStore.h"
#include "core/module/ModuleManager.h"
#include "core/profile/ProfileSession.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/GpgUtils.h"
#include "test/GpgFrontendTest.h"

// GpgFrontend
#include "Application.h"
#include "GpgFrontendContext.h"

namespace GpgFrontend {

inline auto Tr(const char* t) -> QString { return QCoreApplication::tr(t); }

auto PrintVersion() -> int {
  QTextStream stream(stdout);
  stream << GetProjectName() << " " << GetProjectVersion() << '\n';
  stream << QString("Copyright (©) 2021-%1 Saturneric <eric@bktus.com>")
                .arg(QDate::currentDate().year())
         << '\n'
         << Tr("This is free software; see the source for copying conditions.")
         << '\n'
         << '\n';

  stream << Tr("Build Date & Time: ")
         << QLocale().toString(GetProjectBuildTimestamp()) << '\n'
         << Tr("Build Version: ") << GetProjectBuildVersion() << '\n'
         << Tr("Source Code Infomation: ") << GetProjectBuildGitVersion()
         << '\n';

  stream << Qt::endl;
  return 0;
}

auto PrintEnvInfo() -> int {
  QTextStream stream(stdout);
  stream << GetProjectName() << " " << GetProjectVersion() << " "
         << "Environemnt Information:" << '\n';

  stream << '\n';

  stream << Tr("Qt Version: ") << GetProjectQtVersion() << '\n';
  stream << Tr("OpenSSL Version: ") << GetProjectOpenSSLVersion() << '\n';
  stream << Tr("Libarchive Version: ") << GetProjectLibarchiveVersion() << '\n';

  stream << '\n';

  auto& setting_station = GlobalSettingStation::GetInstance();

  stream << Tr("App Data Path: ") << setting_station.GetAppDataPath() << '\n';
  stream << Tr("App Log Path: ") << setting_station.GetAppLogPath() << '\n';
  stream << Tr("Modules Path: ") << setting_station.GetModulesDir() << '\n';
  stream << Tr("App Binary Directory: ") << setting_station.GetAppDir() << '\n';

  stream << '\n';

  // Printed here rather than only logged because the log level defaults to
  // critical, which silences every diagnostic this path emits. stdout is the
  // one channel that always reaches a user asking why the system keychain is
  // greyed out in the settings.
  if (auto* store = GetSystemSecretStore(); store != nullptr) {
    stream << Tr("System Credential Store: ") << store->Name() << '\n';
  } else {
    stream << Tr("System Credential Store: ") << Tr("Unavailable") << '\n';

    // Deliberately untranslated: it carries a library name and the dynamic
    // loader's own message, which are meant to be read back verbatim.
    const auto reason = SystemSecretStoreUnavailableReason();
    if (!reason.isEmpty()) {
      stream << Tr("Credential Store Detail: ") << reason << '\n';
    }
  }

  stream << '\n';

  stream << Tr("GnuPG: ") << '\n';
  stream << '\n';

  auto init_result = InitGpgME();

  stream << Tr("GpgError Version: ") << GetProjectGpgErrorVersion() << '\n';
  stream << Tr("Assuan Version: ") << GetProjectAssuanVersion() << '\n';
  stream << Tr("GpgME Version: ") << GetProjectGpgMEVersion() << '\n';

  auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{});
  stream << Tr("GnuPG Version: ") << gnupg_version << '\n';

  stream << "\n";

  stream << Tr("GpgME Init Status: ")
         << (init_result ? Tr("Success") : Tr("Failed")) << '\n';

  auto gpgconf = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.engine.gpgconf", 0);
  auto openpgp = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.engine.openpgp", 0);
  auto cms =
      Module::RetrieveRTValueTypedOrDefault<>("core", "gpgme.engine.cms", 0);

  stream << Tr("Engine 'GPGCONF' Status: ")
         << (gpgconf == 1 ? Tr("Exists") : Tr("NOT Exists")) << '\n';
  stream << Tr("Engine 'OPENPGP' Status: ")
         << (openpgp == 1 ? Tr("Exists") : Tr("NOT Exists")) << '\n';
  stream << Tr("Engine 'CMS' Status: ")
         << (cms == 1 ? Tr("Exists") : Tr("NOT Exists")) << '\n';

  stream << '\n';

  InitBasicPath();

  auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});
  auto default_database_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.default_database_path", QString{});
  auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  auto cms_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.cms_path", QString{});

  if (gpgconf == 1) {
    stream << Tr("GPGCONF Path: ") << gpgconf_path << '\n';
  }

  if (openpgp == 1) {
    stream << Tr("GnuPG Path: ") << app_path << '\n';
    stream << Tr("Default Key Database Path: ") << default_database_path
           << '\n';
  }

  if (cms == 1) {
    stream << Tr("CMS Path: ") << cms_path << '\n';
  }

  stream << '\n';

  stream << "Key Database(s): " << '\n';
  stream << '\n';

  // The list is a data object, so it is sealed with the profile's key. Reading
  // it without one used to abort the whole command; saying so instead is what
  // makes this usable on a profile that will not open, which is exactly when
  // somebody runs it.
  if (!ProfileSession::Instance().KeysLoaded()) {
    stream << Tr("Unavailable: the profile's key set is not loaded.") << '\n';
    stream << Qt::endl;
    return 0;
  }

  int index = 0;
  auto key_dbs = GetKeyDatabaseInfoBySettings();
  for (const auto& key_database : key_dbs) {
    stream << Tr("Key Database [") << index++ << "] " << Tr("Name: ")
           << key_database.name << " " << Tr("-> Path: ") << key_database.path
           << '\n';
  }
  stream << Qt::endl;
  return 0;
}

auto ParseLogLevelName(const QString& log_level) -> std::optional<int> {
  const auto level = log_level.trimmed().toLower();

  // "none" is what the option declares as its default placeholder rather than
  // a level anybody asks for, so it means "say nothing" — not "be quiet".
  if (level.isEmpty() || level == "none") return {};

  if (level == "debug") return static_cast<int>(GFLogLevel::kDEBUG);
  if (level == "info") return static_cast<int>(GFLogLevel::kINFO);
  if (level == "warn") return static_cast<int>(GFLogLevel::kWARNING);
  if (level == "error") return static_cast<int>(GFLogLevel::kCRITICAL);
  return {};
}

void ApplyLogLevel(int level) {
  SetGFLogLevel(level);
  QLoggingCategory::setFilterRules(BuildQtLoggingFilterRules(level));

  // env_logger level name for the Rust (rPGP) crate, propagated via RUST_LOG
  // and read by gfr_init_logger() at core init. A RUST_LOG already set by the
  // user wins.
  const char* rust_level;
  switch (static_cast<GFLogLevel>(level)) {
    case GFLogLevel::kDEBUG:
      rust_level = "debug";
      break;
    case GFLogLevel::kINFO:
      rust_level = "info";
      break;
    case GFLogLevel::kWARNING:
      rust_level = "warn";
      break;
    default:
      rust_level = "error";
      break;
  }

  if (!qEnvironmentVariableIsSet("RUST_LOG")) {
    qputenv("RUST_LOG", rust_level);
  }
}

auto RunTest(const GFCxtWPtr& p_ctx) -> int {
  GpgFrontend::GFCxtSPtr const ctx = p_ctx.lock();
  if (ctx == nullptr) {
    qWarning("cannot get gpgfrontend context for test running");
    return -1;
  }

  GpgFrontend::Test::GpgFrontendContext test_init_args;
  test_init_args.argc = ctx->argc;
  test_init_args.argv = ctx->argv;

  QEventLoop looper;

  auto* task = new GpgFrontend::Thread::Task(
      [=](const DataObjectPtr&) -> int {
        return GpgFrontend::Test::ExecuteAllTestCase(test_init_args);
      },
      "unit-test", TransferParams());

  QObject::connect(task, &GpgFrontend::Thread::Task::SignalTaskEnd, &looper,
                   &QEventLoop::quit);

  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(task);

  ctx->rtn = kNonRestartCode;
  looper.exec();
  return 0;
}

}  // namespace GpgFrontend