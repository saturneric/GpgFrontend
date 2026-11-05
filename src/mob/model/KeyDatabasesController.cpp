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

#include "model/KeyDatabasesController.h"

#include <QDir>
#include <QPointer>
#include <optional>
#include <utility>

#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/KeyDatabaseRuntime.h"
#include "core/model/SettingsObject.h"
#include "core/profile/ProfileSession.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "shell/models/KeyDatabaseSettingsModel.h"
#include "shell/rules/KeyDatabaseNames.h"

namespace GpgFrontend::Mobile {

namespace {

/// The database the application works in now.
auto CurrentId() -> QString {
  return LookupChannelHandle(UsableDefaultChannelHandle()).database_id;
}

}  // namespace

KeyDatabasesController::KeyDatabasesController(QObject* parent)
    : QObject(parent) {
  // Whoever switched, the list says what is in use now.
  connect(CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalKeyDatabaseRuntimePublished, this,
          &KeyDatabasesController::SignalChanged, Qt::QueuedConnection);
}

auto KeyDatabasesController::Databases() const -> QVariantList {
  if (!ProfileSession::Loaded()) return {};
  const auto current = CurrentId();
  QVariantList list;
  for (const auto& info : GetGpgKeyDatabaseInfos()) {
    list.append(QVariantMap{
        {QStringLiteral("id"), info.id},
        {QStringLiteral("name"), KeyDatabaseDisplayName(info.id, info.name)},
        {QStringLiteral("path"), info.path},
        {QStringLiteral("current"), info.id == current},
    });
  }
  return list;
}

auto KeyDatabasesController::CurrentName() const -> QString {
  for (const auto& d : Databases()) {
    if (d.toMap().value(QStringLiteral("current")).toBool()) {
      return d.toMap().value(QStringLiteral("name")).toString();
    }
  }
  return {};
}

auto KeyDatabasesController::Busy() const -> bool { return busy_; }

void KeyDatabasesController::switchTo(const QString& id) {
  if (busy_ || !ProfileSession::Loaded()) return;
  if (id == CurrentId()) {
    emit SignalSwitched(true, {});
    return;
  }

  // Stored first, on this thread, as the desktop's Settings store it: the
  // core reads the choice from there.
  {
    SettingsObject so(QString::fromLatin1(kKeyDatabaseListObject));
    if (so.LoadFailed()) {
      emit SignalSwitched(false, tr("The key database settings could not be "
                                    "read."));
      return;
    }
    KeyDatabaseListSO list(so);
    KeyDatabaseSettingsModel model;
    model.Load(GetAllKeyDatabaseInfoBySettings(), list.default_database_id, {});
    // Only one that is listed: a stale id from the page would leave the
    // choice to the fallback.
    if (!model.Contains(id)) {
      emit SignalSwitched(false, tr("This key database is gone."));
      return;
    }
    model.choice_id = id;
    model.ApplyTo(list);
    so.Store(list.ToJson());
  }

  apply(id);
}

auto KeyDatabasesController::nameProblem(const QString& name) const -> QString {
  const auto trimmed = name.trimmed();
  if (trimmed.isEmpty()) return tr("Give the key database a name.");
  // The name becomes its folder: the characters the desktop accepts.
  if (!IsKeyDatabaseNameText(trimmed)) {
    return tr("Use only letters A to Z, digits, spaces, \"-\" and \"_\".");
  }
  if (IsReservedKeyDatabaseName(trimmed)) {
    return tr("This name is reserved for the built-in key database.");
  }
  if (!ProfileSession::Loaded()) return {};
  if (IsKeyDatabaseNameTaken(trimmed, GetAllKeyDatabaseInfoBySettings())) {
    return tr("A key database with this name already exists.");
  }
  return {};
}

auto KeyDatabasesController::suggestName() const -> QString {
  // One whose folder a removed database left behind would be refused when
  // made: the phone has no Recycle Bin to empty it from.
  return SuggestKeyDatabaseName(
      QStringLiteral("Key DB"),
      ProfileSession::Loaded() ? GetAllKeyDatabaseInfoBySettings()
                               : QList<KeyDatabaseInfo>{},
      ProfileSession::Loaded()
          ? GlobalSettingStation::GetInstance().GetAppDataPath()
          : QString());
}

void KeyDatabasesController::create(const QString& name) {
  if (busy_ || !ProfileSession::Loaded()) return;
  const auto trimmed = name.trimmed();
  if (const auto problem = nameProblem(trimmed); !problem.isEmpty()) {
    emit SignalCreated(false, problem);
    return;
  }

  const auto all = GetAllKeyDatabaseInfoBySettings();
  if (all.size() >= kMaxKeyDatabases) {
    emit SignalCreated(
        false,
        tr("There can be at most %1 key databases.").arg(kMaxKeyDatabases));
    return;
  }

  // As the desktop adds one kept in the profile: a folder of its own, made
  // now and never one that was already there (a removed database's, say).
  const auto app_data = GlobalSettingStation::GetInstance().GetAppDataPath();
  const auto creation =
      CreateManagedKeyDatabaseFolderExclusively(app_data, trimmed);
  if (creation == ManagedFolderCreation::kEXISTED) {
    emit SignalCreated(false, tr("A folder named \"%1\" is already there "
                                 "from an earlier key database. Choose "
                                 "another name.")
                                  .arg(trimmed));
    return;
  }
  if (creation == ManagedFolderCreation::kFAILED) {
    emit SignalCreated(false,
                       tr("The folder of the key database could not be made."));
    return;
  }
  const auto path = ManagedKeyDatabasePath(app_data, trimmed);

  KeyDatabaseInfo info;
  info.id = NewKeyDatabaseId();
  info.name = trimmed;
  info.backend_type = QStringLiteral("rpgp");
  info.path = path;
  info.origin_path = ToProfileRelativeKeyDatabasePath(path, app_data);
  info.kind = KeyDatabaseKind::kMANAGED;

  {
    SettingsObject so(QString::fromLatin1(kKeyDatabaseListObject));
    if (so.LoadFailed()) {
      QDir().rmdir(path);
      emit SignalCreated(false, tr("The key database settings could not be "
                                   "read."));
      return;
    }
    KeyDatabaseListSO list(so);
    KeyDatabaseSettingsModel model;
    model.Load(all, list.default_database_id, {});
    model.managed.append(info);
    // Made to work in: what one makes a new database for.
    model.choice_id = info.id;
    model.ApplyTo(list);
    so.Store(list.ToJson());
  }

  creating_ = true;
  apply(info.id);
}

void KeyDatabasesController::apply(const QString& id) {
  busy_ = true;
  emit SignalBusyChanged();
  auto result = QSharedPointer<KeyDatabaseApplyResult>::create();
  QPointer<KeyDatabasesController> self(this);
  auto* task = new Thread::Task(
      [result](const DataObjectPtr&) -> int {
        // Opens every database again: seconds, maybe, and never on the
        // interface thread.
        *result = ApplyKeyDatabaseRuntime();
        return 0;
      },
      "mobile_switch_key_database", TransferParams(),
      [self, result, id](int, const DataObjectPtr&) {
        if (self == nullptr) return;
        self->busy_ = false;
        emit self->SignalBusyChanged();
        emit self->SignalChanged();
        // A database that would not open is passed over for another.
        const auto ok = CurrentId() == id;
        const auto error = ok ? QString{}
                           : result->errors.isEmpty()
                               ? tr("This key database could not be opened.")
                               : result->errors.join(QLatin1Char('\n'));
        if (std::exchange(self->creating_, false)) {
          emit self->SignalCreated(ok, error);
        } else {
          emit self->SignalSwitched(ok, error);
        }
      });
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(task);
}

}  // namespace GpgFrontend::Mobile
