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

#include "Command.h"

#include <qdatetime.h>
#include <qglobal.h>
#include <qloggingcategory.h>
#include <qstring.h>
#include <qtextstream.h>

#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/GpgUtils.h"

// GpgFrontend

#include "GpgFrontendContext.h"
#include "test/GpgFrontendTest.h"

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

  stream << Tr("GnuPG: ") << '\n';
  stream << '\n';

  auto init_result = InitGpgME();

  auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{});
  stream << Tr("GnuPG Version: ") << gnupg_version << '\n';

  stream << Tr("GpgME Version: ") << GetProjectGpgMEVersion() << '\n';

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

auto ParseLogLevel(const QString& log_level) -> int {
  // default value
  if (log_level == "none") return 0;

  if (log_level == "debug") {
    QLoggingCategory::setFilterRules(
        "core.debug=true\n"
        "ui.debug=true\n"
        "module.debug=true\n"
        "test.debug=true");
  } else if (log_level == "info") {
    QLoggingCategory::setFilterRules(
        "*.debug=false\n"
        "core.info=true\n"
        "ui.info=true\n"
        "module.info=true\n"
        "test.debug=true");
  } else if (log_level == "warn") {
    QLoggingCategory::setFilterRules(
        "*.debug=false\n"
        "*.info=false\n"
        "core.warning=true\n"
        "ui.warning=true\n"
        "module.warning=true\n"
        "test.warning=true\n");
  } else if (log_level == "error") {
    QLoggingCategory::setFilterRules(
        "*.debug=false\n"
        "*.info=false\n"
        "*.warning=false");
  } else {
    qWarning() << "unknown log level: " << log_level;
  }
  return 0;
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