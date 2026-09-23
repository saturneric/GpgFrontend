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

#include <QCoreApplication>

#include "GFHostImpl.h"
#include "GFSDKHostApi.h"
#include "private/GFHostContext.h"
#include "private/GFHostGate.h"
#include "private/GFHostTransfer.h"
#include "private/GFSDKPrivate.h"
#include "ui/lua/LuaHost.h"

/**
 * @file GFHostScript.cpp
 * @brief A module hands the Host a UI script; the Host runs it, sandboxed.
 *
 * The bytes are copied here, on the caller's thread, and the load itself is
 * queued to the GUI thread, which owns every module's Lua state. Nothing
 * waits for it: a module thread must never block on the GUI thread.
 */

namespace {

auto ScriptLoad(GFHostContextRef ctx, const char* chunk_name,
                GFBufferView source) -> int {
  GATE(ctx, GF_HOST_CAP_UI, "script.load", -1);
  if (chunk_name == nullptr) return -1;
  const auto bytes = gf_sdk_internal::ReadBuffer(source, "script.load");
  if (!bytes.has_value()) return -1;

  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  const auto caps = gf_sdk_internal::ContextGranted(ctx);
  const auto chunk = QString::fromUtf8(chunk_name);
  const auto text = bytes->ConvertToQByteArray();  // a script is not secret

  auto* app = QCoreApplication::instance();
  if (app == nullptr) return -1;
  QMetaObject::invokeMethod(
      app,
      [module, caps, chunk, text]() {
        QString error;
        if (!GpgFrontend::UI::Lua::LuaHost::Instance().Load(module, caps, text,
                                                            chunk, &error)) {
          LOG_W() << "module" << module << "UI script" << chunk
                  << "did not load:" << error;
        }
      },
      Qt::QueuedConnection);
  return 0;
}

}  // namespace

namespace gf_sdk_internal {

const GFHostScriptApi kScriptApi = {sizeof(GFHostScriptApi), &ScriptLoad};

}  // namespace gf_sdk_internal
