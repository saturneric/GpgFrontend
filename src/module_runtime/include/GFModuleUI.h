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

#pragma once

#include <GFSDKContext.h>
#include <GFSDKUI.h>

#include <QObject>
#include <QString>

#include "GFModuleConvert.h"

/// This module's SDK context, from the runtime.
auto GFModuleSdkContext() -> GFSDKContext*;

/**
 * @file GFModuleUI.h
 * @brief Reaching host GUI objects from a module.
 *
 * A host GUI object crosses the boundary as an opaque handle, because a
 * QObject* cannot. GFUIObject<T> resolves one and checks it really is a T.
 *
 * Most handlers should not call this directly: GFEvent::RequireGui<T>() does
 * the same resolution AND produces the failure to return, which is what the
 * 22 call sites of the old spelling each wrote out by hand.
 */

// --------------------------------------------------------------- gui objects

/// Resolve a GUI handle to an object of the type it must be, or nullptr.
template <typename T>
auto GFUIObject(const QString& handle) -> T* {
  auto* obj = static_cast<QObject*>(
      GFUIGetGUIObject(GFModuleSdkContext(), handle.toUtf8().constData()));
  if (obj == nullptr) return nullptr;
  return qobject_cast<T*>(obj);
}

#define Q_VARIANT_Q_OBJECT_FACTORY_DECLARE(name) \
  auto name(void* data_raw_ptr) -> void*;

#define Q_VARIANT_Q_OBJECT_FACTORY_DEFINE(name, func)   \
  auto name(void* data_raw_ptr) -> void* {              \
    auto data = ConvertVoidPtrToQVariant(data_raw_ptr); \
    return func(data);                                  \
  }

#define GUI_OBJECT(factory, data)                      \
  GFUICreateGUIObject(GFModuleSdkContext(), (factory), \
                      ConvertQVariantToVoidPtr(data))
