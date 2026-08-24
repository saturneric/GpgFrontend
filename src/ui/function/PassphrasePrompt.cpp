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

#include "ui/function/PassphrasePrompt.h"

#include "core/function/CoreSignalStation.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/module/ModuleManager.h"
#include "ui/dialog/PassphraseDialog.h"

namespace GpgFrontend::UI {

namespace {

bool g_handler_installed = false;

}  // namespace

void InstallPassphrasePromptHandler() {
  if (g_handler_installed) return;
  g_handler_installed = true;

  QObject::connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalNeedUserInputPassphrase,
      QApplication::instance(),
      [](const QSharedPointer<GpgPassphraseContext>& c) {
        if (!c) return;

        // The unit test runner shares this process and goes through
        // PreInitGpgFrontendUI(), so this handler is live there too. A test has
        // nobody to type into a modal prompt; leave the request unanswered
        // instead, and let a test that cares about passphrases answer it by
        // handling the same signal itself.
        if (Module::RetrieveRTValueTypedOrDefault<>(
                "core", "env.state.unit_test_mode", 0) == 1) {
          LOG_W() << "passphrase requested in unit test mode; no prompt shown";
          return;
        }

        // Parent to the modal dialog on top when there is one -- typically the
        // (also modal) waiting dialog of a running operation. A sibling of it
        // can end up below it in Qt's modal stack and then refuses input, which
        // reads as a frozen prompt; a modal child always stays above.
        QWidget* parent_widget = QApplication::activeModalWidget();
        if (parent_widget == nullptr)
          parent_widget = QApplication::activeWindow();

        PassphraseDialog dialog(c, parent_widget);

        if (dialog.exec() == QDialog::Accepted) {
          c->SetPassphrase(dialog.Passphrase());
        } else {
          // Set empty passphrase and flag the explicit cancellation so the
          // engine can surface GPG_ERR_CANCELED instead of a generic failure.
          c->SetPassphrase(GFBuffer());
          c->SetCancelled(true);
        }

        dialog.Clear();  // Clear the passphrase from memory as soon as possible

        emit CoreSignalStation::GetInstance()
            -> SignalUserInputPassphraseReady(c);
      });
}

}  // namespace GpgFrontend::UI
