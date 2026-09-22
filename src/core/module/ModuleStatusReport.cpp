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

#include "ModuleStatusReport.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleExternalInspection.h"
#include "core/module/ModuleExternalTrust.h"
#include "core/module/ModuleLoadStats.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleTrustRoot.h"
#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend::Module {

auto WaitForModuleLoading(int timeout_ms) -> bool {
  QElapsedTimer clock;
  clock.start();

  const auto settle = [&clock, timeout_ms](auto predicate) -> bool {
    while (!predicate()) {
      if (clock.elapsed() > timeout_ms) return false;
      // Events as well as sleep: the loader posts back to other runners, and
      // a caller sitting on the main thread has to let those through or the
      // work it is waiting for never completes.
      QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
      QThread::msleep(10);
    }
    return true;
  };

  // Two waits, in this order, because they answer different questions.
  //
  // IsFinished() says the loader has stopped deciding: the counts are final.
  // IsAllModulesRegistered() says the modules it accepted have finished
  // registering, which is what makes the identifier list agree with those
  // counts -- without it a report can say "3 loaded" and name one, because
  // registration lags the loading loop.
  //
  // The order matters: IsAllModulesRegistered() is true before the scan has
  // run at all, when nothing is needed and nothing is registered.
  if (!settle([] { return ModuleLoadStats::GetInstance().IsFinished(); })) {
    return false;
  }

  // Bounded, and a timeout here is not fatal to the report: the counts come
  // from the loader and are already final. A module that verified and then
  // failed to register would otherwise hold this open until the timeout, and
  // the report of that failure is exactly what the caller wants.
  return settle(
      [] { return ModuleManager::GetInstance().IsAllModulesRegistered(); });
}

auto CollectModuleStatusReport() -> ModuleStatusReport {
  ModuleStatusReport report;

  report.integrated_module_path =
      GlobalSettingStation::GetInstance().GetIntegratedModulePath();

  const auto& stats = ModuleLoadStats::GetInstance();
  report.loaded = stats.LoadedModules();
  report.refused = stats.RefusedModules();

  // Not a third counter: discovered IS loaded plus refused, and a separate
  // tally of the same thing is a second truth that can drift.
  report.discovered = report.loaded + report.refused;

  report.loaded_modules =
      ModuleManager::GetInstance().ListAllRegisteredModuleID();
  report.loaded_modules.sort();

  report.refused_modules = ModuleManager::GetInstance().ListModuleRefusals();

  return report;
}

auto WriteModuleStatusReport(const QString& path, QString& reason) -> bool {
  const auto report = CollectModuleStatusReport();

  QJsonArray modules;
  for (const auto& id : report.loaded_modules) modules.append(id);

  // The publisher key is rendered as a fingerprint rather than raw bytes: this
  // file is a testing aid people read, and the fingerprint is the thing a
  // person would compare. It is derived here, exactly as the Controller
  // derives it, because the key itself is the only stored form.
  QJsonArray refused;
  for (const auto& r : report.refused_modules) {
    QJsonObject entry{
        {"descriptor", r.descriptor_path},
        {"origin", QString::fromLatin1(ModuleOriginToString(r.origin))},
        {"reason", r.reason},
        {"pending_user_action", r.pending_user_action},
    };
    if (!r.module_id.isEmpty()) entry.insert("id", r.module_id);
    if (!r.publisher_key.isEmpty()) {
      entry.insert("publisher_key_fingerprint",
                   ModulePublisherKeyFingerprint(r.publisher_key));
    }
    // An external refusal is answered question by question -- signature,
    // publisher trust, enablement, compatibility, binding -- because "refused"
    // alone does not say which of them a person has to act on.
    if (r.origin == ModuleOrigin::kEXTERNAL) {
      entry.insert("inspection", ExternalModuleReportToJson(
                                     InspectExternalModule(r.descriptor_path)));
    }
    refused.append(entry);
  }

  const QJsonObject root{
      {"schema", 1},
      {"build_id", ModuleBuildId()},
      {"app_version", GetProjectVersion()},
      {"integrated_module_path", report.integrated_module_path},
      {"discovered", report.discovered},
      {"loaded", report.loaded},
      {"refused", report.refused},
      {"loaded_modules", modules},
      {"refused_modules", refused},
  };

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    reason =
        QString("%1 could not be written: %2").arg(path, file.errorString());
    return false;
  }

  const auto bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
  if (file.write(bytes) != bytes.size()) {
    reason =
        QString("%1 was not fully written: %2").arg(path, file.errorString());
    file.close();
    return false;
  }
  file.close();
  return true;
}

}  // namespace GpgFrontend::Module
