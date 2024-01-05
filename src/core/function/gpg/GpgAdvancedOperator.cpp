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

auto GpgFrontend::GpgAdvancedOperator::ClearGpgPasswordCache() -> bool {
  bool success = false;

  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", std::string{});
  GF_CORE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  if (gpgconf_path.empty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpgconf path from rt, abort.");
    return false;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path,
       {"--reload", "gpg-agent"},
       [&](int exit_code, const std::string & /*p_out*/,
           const std::string & /*p_err*/) {
         if (exit_code == 0) {
           GF_CORE_LOG_DEBUG("gpgconf reload exit code: {}", exit_code);
           success = true;
         }
       }});
  return success;
}

auto GpgFrontend::GpgAdvancedOperator::ReloadGpgComponents() -> bool {
  bool success = false;

  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", std::string{});
  GF_CORE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  if (gpgconf_path.empty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpgconf path from rt, abort.");
    return false;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path,
       {"--reload"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
         } else {
           GF_CORE_LOG_ERROR(
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
  GF_CORE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  if (gpgconf_path.empty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpgconf path from rt, abort.");
    return;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path,
       {"--verbose", "--kill", "all"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         GF_CORE_LOG_DEBUG("gpgconf --kill all command got exit code: {}",
                           exit_code);
         bool success = true;
         if (exit_code != 0) {
           success = false;
           GF_CORE_LOG_ERROR(
               "gpgconf execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
         }

         GF_CORE_LOG_DEBUG("gpgconf --kill --all execute result: {}", success);
         if (!success) {
           GF_CORE_LOG_ERROR(
               "restart all component after core initilized failed");
           Module::UpsertRTValue(
               "core", "gpg_advanced_operator.restart_gpg_components", false);
           return;
         }

         success &= StartGpgAgent();

         if (!success) {
           GF_CORE_LOG_ERROR("start gpg agent after core initilized failed");
         }

         success &= StartDirmngr();

         if (!success) {
           GF_CORE_LOG_ERROR("start dirmngr after core initilized failed");
         }

         success &= StartKeyBoxd();

         if (!success) {
           GF_CORE_LOG_ERROR("start keyboxd after core initilized failed");
         }

         Module::UpsertRTValue(
             "core", "gpg_advanced_operator.restart_gpg_components", true);
       }});
}

auto GpgFrontend::GpgAdvancedOperator::ResetConfigures() -> bool {
  bool success = false;

  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", std::string{});
  GF_CORE_LOG_DEBUG("got gpgconf path from rt: {}", gpgconf_path);

  if (gpgconf_path.empty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpgconf path from rt, abort.");
    return false;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpgconf_path,
       {"--apply-defaults"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
         } else {
           GF_CORE_LOG_ERROR(
               "gpgconf execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
           return;
         }
       }});

  return success;
}

auto GpgFrontend::GpgAdvancedOperator::StartGpgAgent() -> bool {
  bool success = false;

  const auto gpg_agent_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      "gnupg.gpg_agent_path", std::string{});
  GF_CORE_LOG_DEBUG("got gnupg agent path from rt: {}", gpg_agent_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      "gnupg.home_path", std::string{});
  GF_CORE_LOG_DEBUG("got gnupg home path from rt: {}", home_path);

  if (gpg_agent_path.empty()) {
    GF_CORE_LOG_ERROR("cannot get valid gpg agent path from rt, abort.");
    return false;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {gpg_agent_path,
       {"--homedir", home_path, "--daemon"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
           GF_CORE_LOG_INFO("start gpg-agent successfully");
         } else if (exit_code == 2) {
           success = true;
           GF_CORE_LOG_INFO("gpg-agent already started");
         } else {
           GF_CORE_LOG_ERROR(
               "gpg-agent execute error, process stderr: {}, process stdout: "
               "{}",
               p_err, p_out);
           return;
         }
       }});

  return success;
}

auto GpgFrontend::GpgAdvancedOperator::StartDirmngr() -> bool {
  bool success = false;

  const auto dirmngr_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      "gnupg.dirmngr_path", std::string{});
  GF_CORE_LOG_DEBUG("got gnupg dirmngr path from rt: {}", dirmngr_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      "gnupg.home_path", std::string{});
  GF_CORE_LOG_DEBUG("got gnupg home path from rt: {}", home_path);

  if (dirmngr_path.empty()) {
    GF_CORE_LOG_ERROR("cannot get valid dirmngr path from rt, abort.");
    return false;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {dirmngr_path,
       {"--homedir", home_path, "--daemon"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
           GF_CORE_LOG_INFO("start dirmngr successfully");
         } else if (exit_code == 2) {
           success = true;
           GF_CORE_LOG_INFO("dirmngr already started");
         } else {
           GF_CORE_LOG_ERROR(
               "dirmngr execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
           return;
         }
       }});

  return success;
}

auto GpgFrontend::GpgAdvancedOperator::StartKeyBoxd() -> bool {
  bool success = false;

  const auto keyboxd_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      "gnupg.keyboxd_path", std::string{});
  GF_CORE_LOG_DEBUG("got gnupg keyboxd path from rt: {}", keyboxd_path);

  const auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "com.bktus.gpgfrontend.module.integrated.gnupg-info-gathering",
      "gnupg.home_path", std::string{});
  GF_CORE_LOG_DEBUG("got gnupg home path from rt: {}", home_path);

  if (keyboxd_path.empty()) {
    GF_CORE_LOG_ERROR("cannot get valid keyboxd path from rt, abort.");
    return false;
  }

  GpgFrontend::GpgCommandExecutor::ExecuteSync(
      {keyboxd_path,
       {"--homedir", home_path, "--daemon"},
       [&](int exit_code, const std::string &p_out, const std::string &p_err) {
         if (exit_code == 0) {
           success = true;
           GF_CORE_LOG_INFO("start keyboxd successfully");
         } else if (exit_code == 2) {
           success = true;
           GF_CORE_LOG_INFO("keyboxd already started");
         } else {
           GF_CORE_LOG_ERROR(
               "keyboxd execute error, process stderr: {}, process stdout: {}",
               p_err, p_out);
           return;
         }
       }});

  return success;
}
