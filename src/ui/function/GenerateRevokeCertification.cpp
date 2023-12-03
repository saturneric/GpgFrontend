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

#include "GenerateRevokeCertification.h"

#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/model/GpgKey.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend::UI {

GenerateRevokeCertification::GenerateRevokeCertification(QWidget* parent)
    : QWidget(parent) {}

auto GenerateRevokeCertification::Exec(const GpgKey& key,
                                       const std::string& output_path) -> int {
  const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", std::string{});
  // get all components
  GpgCommandExecutor::ExecuteSync(
      {app_path,
       {"--command-fd", "0", "--status-fd", "1", "--no-tty", "-o",
        std::move(output_path), "--gen-revoke", key.GetFingerprint()},
       [=](int exit_code, const std::string& p_out, const std::string& p_err) {
         if (exit_code != 0) {
           SPDLOG_ERROR(
               "gnupg gen revoke execute error, process stderr: {}, process "
               "stdout: {}",
               p_err, p_out);
         } else {
           SPDLOG_DEBUG(
               "gnupg gen revoke exit_code: {}, process stdout size: {}",
               exit_code, p_out.size());
         }
       },
       nullptr,
       [](QProcess* proc) -> void {
         // Code From Gpg4Win
         while (proc->canReadLine()) {
           const QString line = QString::fromUtf8(proc->readLine()).trimmed();
           SPDLOG_DEBUG("line: {}", line.toStdString());
           if (line == QLatin1String("[GNUPG:] GET_BOOL gen_revoke.okay")) {
             proc->write("y\n");
           } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                            "ask_revocation_reason.code")) {
             proc->write("0\n");
           } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                            "ask_revocation_reason.text")) {
             proc->write("\n");
           } else if (line ==
                      QLatin1String(
                          "[GNUPG:] GET_BOOL openfile.overwrite.okay")) {
             // We asked before
             proc->write("y\n");
           } else if (line == QLatin1String("[GNUPG:] GET_BOOL "
                                            "ask_revocation_reason.okay")) {
             proc->write("y\n");
           }
         }
       }});
  return 0;
}

}  // namespace GpgFrontend::UI