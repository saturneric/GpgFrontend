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

#include "GpgFrontendContext.h"

#include "ui/GpgFrontendApplication.h"

namespace {

/// How long startup may wait for macOS to hand over a double-clicked document.
/// A ceiling, not a delay: the wait leaves as soon as something arrives, and
/// leaves early when the loop has gone quiet.
constexpr int kLaunchDocumentWaitMs = 250;

}  // namespace

namespace GpgFrontend {

void GpgFrontendContext::InitApplication() {
  // An HTML renderer module may bring QtWebEngine in after the application
  // exists, and WebEngine needs contexts shared from the start. Harmless
  // when no engine ever loads, and the host links nothing of WebEngine.
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  app_ = new UI::GpgFrontendApplication(argc, argv);

  // A double-clicked document does not reach argv on macOS; it arrives as an
  // Apple Event that nothing has spun the loop to collect yet. It has to be in
  // hand before the profile is chosen, because it *is* the choice — so the loop
  // is pumped for a moment here rather than the document being dropped and the
  // default profile opened over the top of it. A no-op everywhere else.
  app_->WaitForLaunchDocument(kLaunchDocumentWaitMs);

  // Only decides *which* profile. Opening it needs a passphrase prompt and a
  // lock, so it belongs to the loader — and nothing here may read a setting
  // until that has happened, because where the settings live is exactly what
  // is being decided.
  SelectProfile(app_->TakePendingProfilePackage());
}

auto GpgFrontendContext::GetApp() -> QApplication* { return app_; }

GpgFrontendContext::GpgFrontendContext(int argc, char** argv)
    : BootContext(argc, argv) {}

GpgFrontendContext::~GpgFrontendContext() { delete app_; }

}  // namespace GpgFrontend
