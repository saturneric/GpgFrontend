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

#include "crypto/PassphraseBroker.h"
#include "lifecycle/AppLifecycle.h"
#include "model/KeyListController.h"
#include "startup/InteractionBridge.h"
#include "startup/StartupController.h"

namespace GpgFrontend::Mobile {

/**
 * @brief The objects behind the application's QML singletons, made and
 * connected once.
 *
 * The application, its test runner and its QML tests all make their objects
 * here, so the types QML resolves are the same everywhere: a singleton added
 * to one and forgotten in another cannot happen.
 */
class AppServices {
 public:
  explicit AppServices(QSharedPointer<BootContext> ctx);
  ~AppServices();

  AppServices(const AppServices&) = delete;
  auto operator=(const AppServices&) -> AppServices& = delete;

  /**
   * @brief Refuse every passphrase still asked or asked from now on, then
   * StartupController::Shutdown(): an operation waiting at a prompt ends,
   * and the core can stop.
   */
  auto Shutdown(int timeout_ms = 5000) -> bool;

  InteractionBridge bridge;
  StartupController startup;
  PassphraseBroker passphrases;
  AppLifecycle lifecycle;
  KeyListController keys;
};

}  // namespace GpgFrontend::Mobile
