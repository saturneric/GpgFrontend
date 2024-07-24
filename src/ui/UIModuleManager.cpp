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

#include "UIModuleManager.h"

#include <utility>

#include "core/module/ModuleManager.h"

namespace GpgFrontend::UI {

UIModuleManager::UIModuleManager(int channel)
    : SingletonFunctionObject<UIModuleManager>(channel) {}

UIModuleManager::~UIModuleManager() = default;

auto UIModuleManager::DeclareMountPoint(const QString& id,
                                        const QString& entry_type,
                                        QMap<QString, QVariant> meta_data_desc)
    -> bool {
  if (id.isEmpty() || mount_points_.contains(id)) return false;

  UIMountPoint point;
  point.id = id;
  point.entry_type = entry_type;
  point.meta_data_desc = std::move(meta_data_desc);

  mount_points_[id] = point;

  auto grt_key = QString("mount_points.%1").arg(id);
  GpgFrontend::Module::UpsertRTValue(
      "ui", grt_key, QString(QJsonDocument(point.ToJson()).toJson()));

  return true;
}

auto UIModuleManager::MountEntry(const QString& id,
                                 QMap<QString, QString> meta_data,
                                 EntryFactory factory) -> bool {
  if (id.isEmpty() || !mount_points_.contains(id)) return false;

  if (factory == nullptr) return false;

  MountedUIEntry m_entry;
  m_entry.id_ = id;

  for (const auto& meta : meta_data.asKeyValueRange()) {
    meta_data[meta.first] =
        QApplication::translate("GTrC", meta.second.toUtf8());
  }

  m_entry.meta_data_ = std::move(meta_data);
  m_entry.factory_ = factory;

  mounted_entries_[id].append(m_entry);
  return true;
}

auto UIModuleManager::QueryMountedEntries(QString id) -> QList<MountedUIEntry> {
  if (id.isEmpty() || !mount_points_.contains(id)) return {};
  return mounted_entries_[id];
}

auto MountedUIEntry::GetWidget() const -> QWidget* {
  return qobject_cast<QWidget*>(static_cast<QObject*>(factory_(id_.toUtf8())));
}

auto MountedUIEntry::GetMetaDataByDefault(const QString& key,
                                          QString default_value) const
    -> QString {
  if (!meta_data_.contains(key)) return default_value;
  return meta_data_[key];
}
}  // namespace GpgFrontend::UI