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

#include "AppServices.h"

#include <QStandardPaths>

#include "core/function/CoreSignalStation.h"
#include "files/FileOperations.h"
#include "model/KeyCategoriesController.h"
#include "model/ProfilesController.h"
#include "plat/common/PlatformClipboard.h"
#include "plat/common/PlatformRestart.h"
#include "plat/common/PlatformShare.h"

namespace GpgFrontend::Mobile {

namespace {

/// Runs before the staging area is made, so this run's own is never taken.
auto CleanStagingRoot() -> QString {
  StagingArea::RemoveOrphans(AppServices::StagingRoot());
  return AppServices::StagingRoot();
}

}  // namespace

auto AppServices::StagingRoot() -> QString {
  return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
         QStringLiteral("/staging");
}

auto AppServices::RetentionIndexFile() -> QString {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QStringLiteral("/share-retention.json");
}

AppServices::AppServices(QSharedPointer<BootContext> ctx)
    : bridge(nullptr),
      startup(std::move(ctx), &bridge),
      passphrases(nullptr),
      lifecycle(&bridge, &passphrases),
      operations(&passphrases, &lifecycle),
      keys(nullptr),
      selection(keys.Keys()),
      staging(CleanStagingRoot()),
      retention(SharedFilesRoot(), RetentionIndexFile(), {},
                [](const QString& path) { RevokeSharedFile(path); }),
      files(&operations, &lifecycle) {
  InteractionBridge::SetInstance(&bridge);
  StartupController::SetInstance(&startup);
  PassphraseBroker::SetInstance(&passphrases);
  AppLifecycle::SetInstance(&lifecycle);
  OperationQueue::SetInstance(&operations);
  KeyListController::SetInstance(&keys);
  WorkSelection::SetInstance(&selection);
  SetFileStores(&staging, &retention);
  FileWorkspace::SetInstance(&files);

  // The core has just read every key database as it started: what it holds
  // is shown as it is, not read a second time.
  QObject::connect(&startup, &StartupController::SignalReady, &keys,
                   &KeyListController::ShowLoaded);
  // The first listing of a profile's keys; later ones the selection hears
  // from the list itself.
  QObject::connect(
      &keys, &KeyListController::SignalRefreshed, &selection,
      [this]() { selection.SetKeysReady(); }, Qt::SingleShotConnection);
  // Every time the list is read again, the quick path is checked against it.
  QObject::connect(&keys, &KeyListController::SignalRefreshed, &selection,
                   [this]() { selection.KeysRefreshed(); });

  // Another key database in use (a switch, here or by the core): its keys
  // and its categories, at once. The old channel's table goes as the new one
  // comes in; it is reclaimed soon after.
  QObject::connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalKeyDatabaseRuntimePublished, &keys,
      [this](int, int) {
        keys.refresh();
        KeyCategoriesController::NotifyChanged();
      },
      Qt::QueuedConnection);

  // Opening another profile: the core is shut down, then the application
  // starts again and opens it. Quit either way: the core is gone, and the
  // next start opens the profile chosen.
  ProfilesController::SetRestarter([this]() {
    Shutdown();
    const auto restarted = RestartApplication();
    QMetaObject::invokeMethod(
        QCoreApplication::instance(), []() { QCoreApplication::quit(); },
        Qt::QueuedConnection);
    return restarted;
  });

  // Start and every return are the store's chances to clean up.
  retention.Cleanup(QDateTime::currentDateTimeUtc());
  QObject::connect(&lifecycle, &AppLifecycle::SignalResumed, [this]() {
    retention.Cleanup(QDateTime::currentDateTimeUtc());
  });
}

AppServices::~AppServices() {
  ProfilesController::SetRestarter({});
  FileWorkspace::SetInstance(nullptr);
  SetFileStores(nullptr, nullptr);
  WorkSelection::SetInstance(nullptr);
  KeyListController::SetInstance(nullptr);
  OperationQueue::SetInstance(nullptr);
  AppLifecycle::SetInstance(nullptr);
  PassphraseBroker::SetInstance(nullptr);
  StartupController::SetInstance(nullptr);
  InteractionBridge::SetInstance(nullptr);
}

auto AppServices::Shutdown(int timeout_ms) -> bool {
  operations.Close();
  passphrases.Close();
  // Decrypted text copied a moment ago does not outlive the application.
  ExpireSensitiveCopy(true);
  return startup.Shutdown(timeout_ms);
}

}  // namespace GpgFrontend::Mobile
