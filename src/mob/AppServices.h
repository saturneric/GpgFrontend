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

#include "crypto/OperationQueue.h"
#include "crypto/PassphraseBroker.h"
#include "crypto/WorkSelection.h"
#include "files/FileWorkspace.h"
#include "files/ShareRetentionStore.h"
#include "files/StagingArea.h"
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
 *
 * It also owns the files the application stages and shares: this run's
 * staging area, and the store of shared files, which is cleaned when the
 * application starts and whenever it returns to the foreground.
 */
class AppServices {
 public:
  explicit AppServices(QSharedPointer<BootContext> ctx);
  ~AppServices();

  AppServices(const AppServices&) = delete;
  auto operator=(const AppServices&) -> AppServices& = delete;

  /**
   * @brief Drop the operations still waiting, refuse every passphrase still
   * asked or asked from now on, then StartupController::Shutdown(): an
   * operation waiting at a prompt ends, none starts after it, and the core
   * can stop.
   */
  auto Shutdown(int timeout_ms = 5000) -> bool;

  InteractionBridge bridge;
  StartupController startup;
  PassphraseBroker passphrases;
  AppLifecycle lifecycle;
  OperationQueue operations;
  KeyListController keys;
  WorkSelection selection;
  StagingArea staging;
  ShareRetentionStore retention;
  /// After the stores and the queue it uses, so it goes before them.
  FileWorkspace files;

  /// <cache>/staging, where every run's staging area lives.
  static auto StagingRoot() -> QString;
  /// Where the store keeps its record.
  static auto RetentionIndexFile() -> QString;
};

}  // namespace GpgFrontend::Mobile
