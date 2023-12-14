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

#include <qcoreapplication.h>

#include <memory>

#include "core/utils/MemoryUtils.h"
#include "ui/GpgFrontendApplication.h"

namespace GpgFrontend {

std::shared_ptr<GpgFrontendContext> GpgFrontendContext::global_context =
    nullptr;

auto GpgFrontendContext::CreateInstance(int argc, char** argv)
    -> std::weak_ptr<GpgFrontendContext> {
  global_context = std::make_shared<GpgFrontendContext>();
  global_context->argc = argc;
  global_context->argv = argv;
  return global_context;
}

auto GpgFrontendContext::GetInstance() -> std::weak_ptr<GpgFrontendContext> {
  return global_context;
}

void GpgFrontendContext::InitCoreApplication() {
  app = SecureCreateUniqueObject<QCoreApplication>(argc, argv);
}

void GpgFrontendContext::InitGUIApplication() {
  app = SecureCreateUniqueObject<UI::GpgFrontendApplication>(argc, argv);
}

}  // namespace GpgFrontend