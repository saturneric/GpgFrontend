/*
 * Copyright (c) 2023. Saturneric
 *
 *  This file is part of GpgFrontend.
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//
// Created by eric on 07.01.2023.
//

#include "GpgAdvancedOperator.h"

#include "core/function/gpg/GpgCommandExecutor.h"
#include "spdlog/spdlog.h"

GpgFrontend::GpgAdvancedOperator::GpgAdvancedOperator(int channel)
    : SingletonFunctionObject(channel) {}

bool GpgFrontend::GpgAdvancedOperator::ClearGpgPasswordCache() {
  bool success = false;
  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().GpgConfPath, {"--reload", "gpg-agent"},
      [&](int exit_code, const std::string &p_out, const std::string &p_err) {
        if (exit_code == 0) {
          SPDLOG_DEBUG("gpgconf reload exit code: {}", exit_code);
          success = true;
        }
      });
  return success;
}

bool GpgFrontend::GpgAdvancedOperator::ReloadGpgComponents() {
  bool success = false;
  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().GpgConfPath, {"--reload"},
      [&](int exit_code, const std::string &p_out, const std::string &p_err) {
        if (exit_code == 0) {
          success = true;
        } else {
          SPDLOG_ERROR(
              "gpgconf execute error, process stderr: {}, process stdout: {}",
              p_err, p_out);
          return;
        }
      });
  return success;
}

bool GpgFrontend::GpgAdvancedOperator::RestartGpgComponents() {
  bool success = false;

  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().GpgConfPath, {"--verbose", "--kill", "all"},
      [&](int exit_code, const std::string &p_out, const std::string &p_err) {
        if (exit_code == 0) {
          success = true;
          return;
        } else {
          SPDLOG_ERROR(
              "gpgconf execute error, process stderr: {}, process stdout: {}",
              p_err, p_out);
          return;
        }
      });

  if (!success) return false;

  success &= StartGpgAgent();

  success &= StartDirmngr();

  success &= StartKeyBoxd();

  return success;
}

bool GpgFrontend::GpgAdvancedOperator::ResetConfigures() {
  bool success = false;
  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().GpgConfPath, {"--apply-defaults"},
      [&](int exit_code, const std::string &p_out, const std::string &p_err) {
        if (exit_code == 0) {
          success = true;
        } else {
          SPDLOG_ERROR(
              "gpgconf execute error, process stderr: {}, process stdout: {}",
              p_err, p_out);
          return;
        }
      });

  return success;
}

bool GpgFrontend::GpgAdvancedOperator::StartGpgAgent() {
  bool success = false;
  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().GpgAgentPath,
      {"--homedir", ctx_.GetInfo().GnuPGHomePath, "--daemon"},
      [&](int exit_code, const std::string &p_out, const std::string &p_err) {
        if (exit_code == 0) {
          success = true;
          SPDLOG_INFO("start gpg-agent successfully");
        } else if (exit_code == 2) {
          success = true;
          SPDLOG_INFO("gpg-agent already started");
        } else {
          SPDLOG_ERROR(
              "gpg-agent execute error, process stderr: {}, process stdout: {}",
              p_err, p_out);
          return;
        }
      });

  return success;
}

bool GpgFrontend::GpgAdvancedOperator::StartDirmngr() {
  bool success = false;
  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().DirmngrPath,
      {"--homedir", ctx_.GetInfo().GnuPGHomePath, "--daemon"},
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
      });

  return success;
}

bool GpgFrontend::GpgAdvancedOperator::StartKeyBoxd() {
  bool success = false;
  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().KeyboxdPath,
      {"--homedir", ctx_.GetInfo().GnuPGHomePath, "--daemon"},
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
      });

  return success;
}
