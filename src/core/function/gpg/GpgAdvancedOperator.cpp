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

//
// Created by eric on 07.01.2023.
//

#include "GpgAdvancedOperator.h"

#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/module/ModuleManager.h"

void GpgFrontend::GpgAdvancedOperator::ClearGpgPasswordCache(
    OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--reload", "gpg-agent"},
       [=](int exit_code, const QString & /*p_out*/,
           const QString & /*p_err*/) {
         cb(exit_code == 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::ReloadGpgComponents(
    OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--reload"},
       [=](int exit_code, const QString &, const QString &) {
         FLOG_D("gpgconf reload exit code: %d", exit_code);
         cb(exit_code == 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::KillAllGpgComponents() {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--verbose", "--kill", "all"},
       [=](int exit_code, const QString &p_out, const QString &p_err) {
         bool success = true;
         if (exit_code != 0) {
           success = false;
           LOG_W() << "gpgconf execute error, process stderr: " << p_err
                   << ", process stdout: " << p_out;
           return;
         }

         FLOG_D("gpgconf --kill --all execute result: %d", success);
       }});
}

void GpgFrontend::GpgAdvancedOperator::RestartGpgComponents() {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--verbose", "--kill", "all"},
       [=](int exit_code, const QString &p_out, const QString &p_err) {
         FLOG_D("gpgconf --kill all command got exit code: %d", exit_code);
         bool success = true;
         if (exit_code != 0) {
           success = false;
           LOG_W() << "gpgconf execute error, process stderr: " << p_err
                   << ", process stdout: " << p_out;
           return;
         }

         FLOG_D("gpgconf --kill --all execute result: %d", success);
         if (!success) {
           qCWarning(core,
                     "restart all component after core initilized failed");
           Module::UpsertRTValue(
               "core", "gpg_advanced_operator.restart_gpg_components", false);
           return;
         }

         StartGpgAgent([](int err, DataObjectPtr) {
           if (err >= 0) {
             Module::UpsertRTValue(
                 "core", "gpg_advanced_operator.restart_gpg_components", true);
             return;
           }
         });
       }});
}

void GpgFrontend::GpgAdvancedOperator::ResetConfigures(OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--apply-defaults"},
       [=](int exit_code, const QString &, const QString &) {
         FLOG_D("gpgconf apply-defaults exit code: %d", exit_code);
         cb(exit_code == 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::StartGpgAgent(OperationCallback cb) {
  if (!Module::IsModuleActivate(kGnuPGInfoGatheringModuleID)) {
    cb(-1, TransferParams());
    return;
  }

  const auto gpg_agent_path = Module::RetrieveRTValueTypedOrDefault<>(
      kGnuPGInfoGatheringModuleID, "gnupg.gpg_agent_path", QString{});

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      kGnuPGInfoGatheringModuleID, "gnupg.home_path", QString{});

  if (gpg_agent_path.isEmpty()) {
    FLOG_W("cannot get valid gpg agent path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpg_agent_path, QStringList{"--homedir", home_path, "--daemon"},
       [=](int exit_code, const QString &, const QString &) {
         FLOG_D("gpgconf daemon exit code: %d", exit_code);
         cb(exit_code >= 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::StartDirmngr(OperationCallback cb) {
  if (!Module::IsModuleActivate(kGnuPGInfoGatheringModuleID)) {
    cb(-1, TransferParams());
    return;
  }

  const auto dirmngr_path = Module::RetrieveRTValueTypedOrDefault<>(
      kGnuPGInfoGatheringModuleID, "gnupg.dirmngr_path", QString{});

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      kGnuPGInfoGatheringModuleID, "gnupg.home_path", QString{});

  if (dirmngr_path.isEmpty()) {
    FLOG_W("cannot get valid dirmngr path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {dirmngr_path, QStringList{"--homedir", home_path, "--daemon"},
       [=](int exit_code, const QString &, const QString &) {
         FLOG_D("gpgconf daemon exit code: %d", exit_code);
         cb(exit_code >= 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::StartKeyBoxd(OperationCallback cb) {
  if (!Module::IsModuleActivate(kGnuPGInfoGatheringModuleID)) {
    cb(-1, TransferParams());
    return;
  }

  const auto keyboxd_path = Module::RetrieveRTValueTypedOrDefault<>(
      kGnuPGInfoGatheringModuleID, "gnupg.keyboxd_path", QString{});

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      kGnuPGInfoGatheringModuleID, "gnupg.home_path", QString{});

  if (keyboxd_path.isEmpty()) {
    FLOG_W("cannot get valid keyboxd path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {keyboxd_path, QStringList{"--homedir", home_path, "--daemon"},
       [=](int exit_code, const QString &, const QString &) {
         FLOG_D("gpgconf daemon exit code: %d", exit_code);
         cb(exit_code >= 0 ? 0 : -1, TransferParams());
       }});
}
