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

#include "ModuleUiTeardown.h"

#include <QCoreApplication>
#include <QThread>

#include "ui/command/CommandRegistry.h"
#include "ui/lua/LuaHost.h"
#include "ui/lua/LuaMounts.h"
#include "ui/lua/NativeWidgetRegistry.h"

namespace GpgFrontend::UI {

void ModuleUiTeardown(const QString& module) {
  if (module.isEmpty()) return;

  // 1. Nothing enters the module's Lua from here on -- set now, from
  //    whatever thread this is, before anything else can deliver.
  auto* app = QCoreApplication::instance();
  const bool on_gui = app == nullptr || QThread::currentThread() == app->thread();
  if (on_gui) {
    if (auto* rt = Lua::LuaHost::Instance().Runtime(module)) rt->StopCallbacks();
  }

  // 2. Its command calls, both ways, and the commands it provides. This is
  //    also what drops every continuation its script is still owed.
  CommandRegistry::Instance().RemoveAllFor(module);

  // 3-6. The runtime itself, on the thread that owns it. Queued rather than
  //      waited for when this is a module thread: that thread must never
  //      block on the GUI thread. Nothing can enter the runtime meanwhile --
  //      its calls are gone and its flag is set -- and it holds no pointer
  //      into module code.
  const auto rest = [module]() {
    if (auto* rt = Lua::LuaHost::Instance().Runtime(module)) {
      rt->StopCallbacks();
    }
    Lua::LuaHost::Instance().Teardown(module);
    // Its dialogs close; its settings pages and document views live in
    // containers that forget the instance when they go, and a document tab
    // keeps its bytes -- the page, not the view, owns the document.
    Lua::CloseDialogsOf(module);
    NativeWidgetRegistry::Instance().RemoveAllFor(module);
  };
  if (on_gui) {
    rest();
  } else {
    QMetaObject::invokeMethod(app, rest, Qt::QueuedConnection);
  }
}

}  // namespace GpgFrontend::UI
