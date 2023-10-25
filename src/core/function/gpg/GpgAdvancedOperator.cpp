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
#include "spdlog/spdlog.h"

GpgFrontend::GpgAdvancedOperator::GpgAdvancedOperator(int channel)
    : SingletonFunctionObject(channel) {}

bool GpgFrontend::GpgAdvancedOperator::ClearGpgPasswordCache() {
  bool success = false;

  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", std::string{});
  SPDLOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  GpgFrontend::GpgCommandExecutor::GetInstance().ExecuteSync(
      {gpgconf_path,
       {"--reload", "gpg-agent"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           SPDLOG_DEBUG("gpgconf reload exit code: {}", exit_code);
           success = true;
         }
       }});
  return success;
}

bool GpgFrontend::GpgAdvancedOperator::ReloadGpgComponents() {
  bool success = false;

  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", std::string{});
  SPDLOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  GpgFrontend::GpgCommandExecutor::GetInstance().ExecuteSync(
      {gpgconf_path,
       {"--reload"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
         } else {
           SPDLOG_ERROR(
               "gpgconf execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
           return;
         }
       }});
  return success;
}

void GpgFrontend::GpgAdvancedOperator::RestartGpgComponents() {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", std::string{});
  SPDLOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  GpgFrontend::GpgCommandExecutor::GetInstance().ExecuteSync(
      {gpgconf_path,
       {"--verbose", "--kill", "all"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         SPDLOG_DEBUG("gpgconf --kill all command got exit code: {}",
                      exit_code);
         bool success = true;
         if (exit_code != 0) {
           success = false;
           SPDLOG_ERROR(
               "gpgconf execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
         }

         SPDLOG_DEBUG("gpgconf --kill --all execute result: {}", success);
         if (!success) {
           SPDLOG_ERROR("restart all component after core initilized failed");
           Module::UpsertRTValue(
               "core", "gpg_advanced_operator.restart_gpg_components", false);
           return;
         }

         success &= StartGpgAgent();

         if (!success) {
           SPDLOG_ERROR("start gpg agent after core initilized failed");
         }

         success &= StartDirmngr();

         if (!success) {
           SPDLOG_ERROR("start dirmngr after core initilized failed");
         }

         success &= StartKeyBoxd();

         if (!success) {
           SPDLOG_ERROR("start keyboxd after core initilized failed");
         }

         Module::UpsertRTValue(
             "core", "gpg_advanced_operator.restart_gpg_components", true);
       }});
}

bool GpgFrontend::GpgAdvancedOperator::ResetConfigures() {
  bool success = false;

  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", std::string{});
  SPDLOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  GpgFrontend::GpgCommandExecutor::GetInstance().ExecuteSync(
      {gpgconf_path,
       {"--apply-defaults"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
         } else {
           SPDLOG_ERROR(
               "gpgconf execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
           return;
         }
       }});

  return success;
}

bool GpgFrontend::GpgAdvancedOperator::StartGpgAgent() {
  bool success = false;

  const auto gpg_agent_path = Module::RetrieveRTValueTypedOrDefault<>(
      Module::GetRealModuleIdentifier(
          "com.bktus.gpgfrontend.module.integrated.gnupginfogathering"),
      "gnupg.gpg_agent_path", std::string{});
  SPDLOG_DEBUG("got gnupg agent path from rt: {}", gpg_agent_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      Module::GetRealModuleIdentifier(
          "com.bktus.gpgfrontend.module.integrated.gnupginfogathering"),
      "gnupg.home_path", std::string{});
  SPDLOG_DEBUG("got gnupg home path from rt: {}", home_path);

  GpgFrontend::GpgCommandExecutor::GetInstance().ExecuteSync(
      {gpg_agent_path,
       {"--homedir", home_path, "--daemon"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
           SPDLOG_INFO("start gpg-agent successfully");
         } else if (exit_code == 2) {
           success = true;
           SPDLOG_INFO("gpg-agent already started");
         } else {
           SPDLOG_ERROR(
               "gpg-agent execute error, process stderr: {}, process stdout: "
               "{}",
               p_err, p_out);
           return;
         }
       }});

  return success;
}

bool GpgFrontend::GpgAdvancedOperator::StartDirmngr() {
  bool success = false;

  const auto dirmngr_path = Module::RetrieveRTValueTypedOrDefault<>(
      Module::GetRealModuleIdentifier(
          "com.bktus.gpgfrontend.module.integrated.gnupginfogathering"),
      "gnupg.dirmngr_path", std::string{});
  SPDLOG_DEBUG("got gnupg dirmngr path from rt: {}", dirmngr_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      Module::GetRealModuleIdentifier(
          "com.bktus.gpgfrontend.module.integrated.gnupginfogathering"),
      "gnupg.home_path", std::string{});
  SPDLOG_DEBUG("got gnupg home path from rt: {}", home_path);

  GpgFrontend::GpgCommandExecutor::GetInstance().ExecuteSync(
      {dirmngr_path,
       {"--homedir", home_path, "--daemon"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
           SPDLOG_INFO("start dirmngr successfully");
         } else if (exit_code == 2) {
           success = true;
           SPDLOG_INFO("dirmngr already started");
         } else {
           SPDLOG_ERROR(
               "dirmngr execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
           return;
         }
       }});

  return success;
}

bool GpgFrontend::GpgAdvancedOperator::StartKeyBoxd() {
  bool success = false;

  const auto keyboxd_path = Module::RetrieveRTValueTypedOrDefault<>(
      Module::GetRealModuleIdentifier(
          "com.bktus.gpgfrontend.module.integrated.gnupginfogathering"),
      "gnupg.keyboxd_path", std::string{});
  SPDLOG_DEBUG("got gnupg keyboxd path from rt: {}", keyboxd_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      Module::GetRealModuleIdentifier(
          "com.bktus.gpgfrontend.module.integrated.gnupginfogathering"),
      "gnupg.home_path", std::string{});
  SPDLOG_DEBUG("got gnupg home path from rt: {}", home_path);

  GpgFrontend::GpgCommandExecutor::GetInstance().ExecuteSync(
      {keyboxd_path,
       {"--homedir", home_path, "--daemon"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
           SPDLOG_INFO("start keyboxd successfully");
         } else if (exit_code == 2) {
           success = true;
           SPDLOG_INFO("keyboxd already started");
         } else {
           SPDLOG_ERROR(
               "keyboxd execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
           return;
         }
       }});

  return success;
}
