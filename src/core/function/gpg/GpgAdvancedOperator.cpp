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

GpgFrontend::GpgAdvancedOperator::GpgAdvancedOperator(int channel)
    : SingletonFunctionObject(channel) {}

bool GpgFrontend::GpgAdvancedOperator::ClearGpgPasswordCache() {
  bool success = false;
  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().GpgConfPath, {"--reload", "gpg-agent"},
      [&](int exit_code, const std::string &p_out, const std::string &p_err) {
        if (exit_code == 0) {
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
          LOG(ERROR) << "gpgconf execute error, process stderr:" << p_err
                     << ", process stdout:" << p_out;
          return;
        }
      });

  return success;
}

bool GpgFrontend::GpgAdvancedOperator::RestartGpgComponents() {
  bool success = false;
  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().GpgConfPath, {"--kill", "all"},
      [&](int exit_code, const std::string &p_out, const std::string &p_err) {
        if (exit_code == 0) {
          success = true;
        } else {
          LOG(ERROR) << "gpgconf execute error, process stderr:" << p_err
                     << ", process stdout:" << p_out;
          return;
        }
      });

  GpgFrontend::GpgCommandExecutor::GetInstance().Execute(
      ctx_.GetInfo().GpgConfPath, {"--launch", "all"},
      [&](int exit_code, const std::string &p_out, const std::string &p_err) {
        if (exit_code == 0) {
          success = true;
        } else {
          success = false;
          LOG(ERROR) << "gpgconf execute error, process stderr:" << p_err
                     << ", process stdout:" << p_out;
          return;
        }
      });

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
          LOG(ERROR) << "gpgconf execute error, process stderr:" << p_err
                     << ", process stdout:" << p_out;
          return;
        }
      });

  return success;
}
