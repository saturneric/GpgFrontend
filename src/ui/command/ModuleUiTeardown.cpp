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

#include "ui/UIModuleManager.h"
#include "ui/command/CommandRegistry.h"
#include "ui/lua/LuaHost.h"
#include "ui/lua/NativeInstances.h"
#include "ui/lua/NativeWidgetRegistry.h"

namespace GpgFrontend::UI {

void ModuleUiTeardown(const QString& module) {
  if (module.isEmpty()) return;

  // Thread-safe, so done here and now: from this point the module neither
  // provides nor invokes a command -- its script included, even while the
  // GUI half below is still queued -- and no command result reaches it.
  CommandRegistry::Instance().RemoveAllFor(module);
  UIModuleManager::GetInstance().UnregisterTranslatorDataReader(module);
  // Here and now too, not queued: a reactivation posted right behind this
  // registers anew, and a queued removal landing after it would take the new
  // registrations with it.
  NativeWidgetRegistry::Instance().RemoveAllFor(module);

  // The runtime and the widgets, on the thread that owns them. Queued rather
  // than waited for when this is a module thread: that thread must never
  // block on the GUI thread. Nothing enters the module meanwhile -- its gate
  // is closed -- and the runtime holds no pointer into module code.
  const auto rest = [module]() {
    auto& lua = Lua::LuaHost::Instance();
    if (auto* rt = lua.Runtime(module)) rt->StopCallbacks();
    lua.Teardown(module);
    NativeInstances::Instance().WithdrawAll(module);
  };

  auto* app = QCoreApplication::instance();
  if (app == nullptr || QThread::currentThread() == app->thread()) {
    rest();
  } else {
    QMetaObject::invokeMethod(app, rest, Qt::QueuedConnection);
  }
}

void ModuleUiReopen(const QString& module) {
  if (module.isEmpty()) return;
  CommandRegistry::Instance().Reopen(module);
}

}  // namespace GpgFrontend::UI
