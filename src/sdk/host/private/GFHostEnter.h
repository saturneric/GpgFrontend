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

#include <QByteArray>
#include <QWidget>

#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleSdkBridge.h"

namespace gf_sdk_internal {

/**
 * @brief Enter module code the way every host-to-module call does.
 *
 * Both gates, the process-wide one and the module's own: a call queued
 * before a module was deactivated must not reach it afterwards, and the
 * process-wide gate alone only closes at shutdown.
 *
 * @return false without calling @p fn when the module is not active, or the
 *         host is shutting down.
 */
template <typename Fn>
auto EnterModule(const QByteArray& module_utf8, Fn&& fn) -> bool {
  GpgFrontend::Module::ModuleDispatchScope scope(
      GpgFrontend::Module::GlobalModuleDispatchGate());
  if (!scope.Entered()) return false;
  GpgFrontend::Module::ModuleDispatchScope own(
      GpgFrontend::Module::ModuleEntryGate(QString::fromUtf8(module_utf8)));
  if (!own.Entered()) return false;
  const GpgFrontend::Module::ModuleAttributionScope attribution(
      module_utf8.constData());
  fn();
  return true;
}

/**
 * @brief The widget @p make returns from inside module @p module_utf8.
 *
 * A factory hands back an untyped pointer; whatever it is, if it is not a
 * QWidget it is deleted rather than leaked or used.
 *
 * @return nullptr when the module is not entered or made no widget
 */
template <typename Make>
auto AdoptModuleWidget(const QByteArray& module_utf8, Make&& make) -> QWidget* {
  QWidget* widget = nullptr;
  EnterModule(module_utf8, [&] {
    auto* object = static_cast<QObject*>(make());
    widget = qobject_cast<QWidget*>(object);
    if (widget == nullptr && object != nullptr) delete object;
  });
  return widget;
}

}  // namespace gf_sdk_internal
