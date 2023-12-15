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

#include "GpgFrontendContext.h"

#include <qapplication.h>
#include <qcoreapplication.h>
#include <qobject.h>
#include <qthread.h>

#include "core/utils/MemoryUtils.h"
#include "ui/GpgFrontendApplication.h"

namespace GpgFrontend {

void GpgFrontendContext::InitApplication(bool gui_mode) {
  if (!gui_mode) {
    app_ = SecureCreateObject<QCoreApplication>(argc, argv);
  } else {
    app_ = SecureCreateObject<QApplication>(argc, argv);
  }
  this->load_ui_env = gui_mode;
}

auto GpgFrontendContext::GetApp() -> QCoreApplication* { return app_; }

auto GpgFrontendContext::GetGuiApp() -> QApplication* {
  return qobject_cast<QApplication*>(app_);
}

GpgFrontendContext::GpgFrontendContext(int argc, char** argv)
    : argc(argc), argv(argv) {}

GpgFrontendContext::~GpgFrontendContext() {
  if (app_ != nullptr) {
    SecureDestroyObject(app_);
  }
}
}  // namespace GpgFrontend