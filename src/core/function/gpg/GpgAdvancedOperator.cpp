/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
  GF_CORE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  if (gpgconf_path.isEmpty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpgconf path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--reload", "gpg-agent"},
       [=](int exit_code, const QString & /*p_out*/,
           const QString & /*p_err*/) {
         GF_CORE_LOG_DEBUG("gpgconf reload exit code: {}", exit_code);
         cb(exit_code == 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::ReloadGpgComponents(
    OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  GF_CORE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  if (gpgconf_path.isEmpty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpgconf path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--reload"},
       [=](int exit_code, const QString &, const QString &) {
         GF_CORE_LOG_DEBUG("gpgconf reload exit code: {}", exit_code);
         cb(exit_code == 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::RestartGpgComponents() {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  GF_CORE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  if (gpgconf_path.isEmpty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpgconf path from rt, abort.");
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--verbose", "--kill", "all"},
       [=](int exit_code, const QString &p_out, const QString &p_err) {
         GF_CORE_LOG_DEBUG("gpgconf --kill all command got exit code: {}",
                           exit_code);
         bool success = true;
         if (exit_code != 0) {
           success = false;
           GF_CORE_LOG_ERROR(
               "gpgconf execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
           return;
         }

         GF_CORE_LOG_DEBUG("gpgconf --kill --all execute result: {}", success);
         if (!success) {
           GF_CORE_LOG_ERROR(
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
  GF_CORE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  if (gpgconf_path.isEmpty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpgconf path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path, QStringList{"--apply-defaults"},
       [=](int exit_code, const QString &, const QString &) {
         GF_CORE_LOG_DEBUG("gpgconf apply-defaults exit code: {}", exit_code);
         cb(exit_code == 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::StartGpgAgent(OperationCallback cb) {
  const auto gpg_agent_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg_info_gathering",
      "gnupg.gpg_agent_path", QString{});
  GF_CORE_LOG_DEBUG("got gnupg agent path from rt: {}", gpg_agent_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg_info_gathering",
      "gnupg.home_path", QString{});
  GF_CORE_LOG_DEBUG("got gnupg home path from rt: {}", home_path);

  if (gpg_agent_path.isEmpty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpg agent path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpg_agent_path, QStringList{"--homedir", home_path, "--daemon"},
       [=](int exit_code, const QString &, const QString &) {
         GF_CORE_LOG_DEBUG("gpgconf daemon exit code: {}", exit_code);
         cb(exit_code >= 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::StartDirmngr(OperationCallback cb) {
  const auto dirmngr_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg_info_gathering",
      "gnupg.dirmngr_path", QString{});
  GF_CORE_LOG_DEBUG("got gnupg dirmngr path from rt: {}", dirmngr_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg_info_gathering",
      "gnupg.home_path", QString{});
  GF_CORE_LOG_DEBUG("got gnupg home path from rt: {}", home_path);

  if (dirmngr_path.isEmpty()) {
    GF_CORE_LOG_ERROR("cannot get valid dirmngr path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {dirmngr_path, QStringList{"--homedir", home_path, "--daemon"},
       [=](int exit_code, const QString &, const QString &) {
         GF_CORE_LOG_DEBUG("gpgconf daemon exit code: {}", exit_code);
         cb(exit_code >= 0 ? 0 : -1, TransferParams());
       }});
}

void GpgFrontend::GpgAdvancedOperator::StartKeyBoxd(OperationCallback cb) {
  const auto keyboxd_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg_info_gathering",
      "gnupg.keyboxd_path", QString{});
  GF_CORE_LOG_DEBUG("got gnupg keyboxd path from rt: {}", keyboxd_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg_info_gathering",
      "gnupg.home_path", QString{});
  GF_CORE_LOG_DEBUG("got gnupg home path from rt: {}", home_path);

  if (keyboxd_path.isEmpty()) {
    GF_CORE_LOG_ERROR("cannot get valid keyboxd path from rt, abort.");
    cb(-1, TransferParams());
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {keyboxd_path, QStringList{"--homedir", home_path, "--daemon"},
       [=](int exit_code, const QString &, const QString &) {
         GF_CORE_LOG_DEBUG("gpgconf daemon exit code: {}", exit_code);
         cb(exit_code >= 0 ? 0 : -1, TransferParams());
       }});
}
