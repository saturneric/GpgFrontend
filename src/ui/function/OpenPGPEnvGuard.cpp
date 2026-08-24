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

#include "ui/function/OpenPGPEnvGuard.h"

#include "core/GFConstants.h"
#include "core/function/CoreSignalStation.h"
#include "core/module/ModuleManager.h"
#include "ui/UISignalStation.h"

namespace GpgFrontend::UI {

namespace {

bool g_handler_installed = false;
bool g_application_need_to_restart_at_once = false;

}  // namespace

auto DescribeBadOpenPGPEnv(BadOpenPGPEnvReason reason, const QString& detail)
    -> BadOpenPGPEnvText {
  // No default label on purpose: -Wswitch then turns a new enumerator into a
  // compile error rather than a message that silently says the wrong thing.
  switch (reason) {
    case BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE:
      return {QCoreApplication::translate("GpgFrontend::UI",
                                          "No Usable Key Database"),
              QCoreApplication::translate(
                  "GpgFrontend::UI",
                  "None of the configured key databases could be opened. This "
                  "usually means the folder was moved or deleted, or is on a "
                  "drive that is not currently available.") +
                  "\n\n" +
                  QCoreApplication::translate(
                      "GpgFrontend::UI",
                      "You can change where your key databases live in "
                      "Settings, under Key Databases. Details: %1")
                      .arg(detail),
              // A restart re-reads the same unusable configuration.
              false};

    case BadOpenPGPEnvReason::kBASIC_PATH_INIT_FAILED:
      return {QCoreApplication::translate("GpgFrontend::UI",
                                          "Cannot Prepare Application Data"),
              QCoreApplication::translate(
                  "GpgFrontend::UI",
                  "GpgFrontend could not set up the folders it needs to store "
                  "its data. Please check that the application data folder is "
                  "writable. Details: %1")
                  .arg(detail)};

    case BadOpenPGPEnvReason::kDEFAULT_CONTEXT_INIT_FAILED:
    case BadOpenPGPEnvReason::kKEY_CACHE_INIT_FAILED:
      return {QCoreApplication::translate("GpgFrontend::UI",
                                          "Key Database Could Not Be Opened"),
              QCoreApplication::translate(
                  "GpgFrontend::UI",
                  "The key database was found but could not be loaded. It may "
                  "be in use by another program, or its permissions may have "
                  "changed. Details: %1")
                  .arg(detail)};

    case BadOpenPGPEnvReason::kNO_SUPPORTED_ENGINE:
    case BadOpenPGPEnvReason::kUNKNOWN:
      break;
  }

  return {QCoreApplication::translate("GpgFrontend::UI",
                                      "No Supported OpenPGP Engine Found"),
          QCoreApplication::translate(
              "GpgFrontend::UI",
              "It seems that no supported OpenPGP engine is available. "
              "Please check your if GpgFrontend is properly installed and try "
              "again. Reason: %1")
              .arg(detail)};
}

void InstallBadOpenPGPEnvHandler() {
  if (g_handler_installed) return;
  g_handler_installed = true;

  // Contexted on the application object: the core reports this from its own
  // thread, and qApp lives in the main one, so the queued hop lands the modal
  // dialog where it can actually be shown.
  QObject::connect(
      CoreSignalStation::GetInstance(), &CoreSignalStation::SignalBadOpenPGPEnv,
      QApplication::instance(),
      [](BadOpenPGPEnvReason reason, const QString& detail) -> void {
        // The unit test runner shares this process, so a modal dialog
        // ending in std::exit(0) would take the whole test run down with it.
        if (Module::RetrieveRTValueTypedOrDefault<>(
                "core", "env.state.unit_test_mode", 0) == 1) {
          LOG_W() << "bad openpgp env in unit test mode, reason:"
                  << static_cast<int>(reason) << "detail:" << detail;
          return;
        }

        const auto text = DescribeBadOpenPGPEnv(reason, detail);

        QMessageBox msg_box;
        msg_box.setText(text.title);
        msg_box.setInformativeText(text.body);

        // Offering "Retry" for a failure that a restart cannot change just
        // invites the user to loop.
        msg_box.setStandardButtons(text.offer_retry ? QMessageBox::Retry |
                                                          QMessageBox::Cancel
                                                    : QMessageBox::Close);
        msg_box.setDefaultButton(text.offer_retry ? QMessageBox::Retry
                                                  : QMessageBox::Close);
        int ret = msg_box.exec();

        if (ret == QMessageBox::Retry) {
          g_application_need_to_restart_at_once = true;
          emit UISignalStation::GetInstance()
              -> SignalRestartApplication(kDeepRestartCode);
        } else {
          emit UISignalStation::GetInstance() -> SignalRestartApplication(0);
        }
      });
}

auto IsApplicationNeedRestart() -> bool {
  return g_application_need_to_restart_at_once;
}

}  // namespace GpgFrontend::UI
