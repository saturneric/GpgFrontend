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

#include "UIModuleManager.h"

#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"

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
                                 QObjectFactory factory) -> bool {
  if (id.isEmpty() || !mount_points_.contains(id)) return false;

  if (factory == nullptr) return false;

  MountedUIEntry m_entry;
  m_entry.id_ = id;

  m_entry.meta_data_ = std::move(meta_data);
  m_entry.factory_ = factory;

  mounted_entries_[id].append(m_entry);
  return true;
}

auto UIModuleManager::QueryMountedEntries(QString id)
    -> QContainer<MountedUIEntry> {
  if (id.isEmpty() || !mount_points_.contains(id)) return {};
  return mounted_entries_[id];
}

auto MountedUIEntry::GetWidget() const -> QWidget* {
  return qobject_cast<QWidget*>(static_cast<QObject*>(factory_(nullptr)));
}

auto MountedUIEntry::GetMetaDataByDefault(const QString& key,
                                          QString default_value) const
    -> QString {
  if (meta_data_translated_.contains(key)) return meta_data_translated_[key];
  if (!meta_data_.contains(key)) return default_value;
  return meta_data_[key];
}

auto UIModuleManager::RegisterTranslatorDataReader(
    Module::ModuleIdentifier id, GFTranslatorDataReader reader) -> bool {
  if (reader != nullptr && !id.isEmpty() && Module::IsModuleExists(id)) {
    LOG_D() << "module " << id << "registering translator reader...";
    translator_data_readers_[id] = ModuleTranslatorInfo{reader};
    return true;
  }
  return false;
}

void UIModuleManager::RegisterAllModuleTranslators() {
  registered_translators_.clear();
  read_translator_data_list_.clear();

  const auto locale_name = QLocale().name();

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
  for (const auto& reader : translator_data_readers_.asKeyValueRange()) {
    char* data = nullptr;

    auto data_size = reader.second.reader_(GFStrDup(locale_name), &data);
    LOG_D() << "module " << reader.first << "reader, read locale "
            << locale_name << ", data size: " << data_size;
#else
  for (auto it = translator_data_readers_.keyValueBegin();
       it != translator_data_readers_.keyValueEnd(); ++it) {
    char* data = nullptr;

    auto data_size = it->second.reader_(GFStrDup(locale_name), &data);
    LOG_D() << "module " << it->first << "reader, read locale " << locale_name
            << ", data size: " << data_size;
#endif

    if (data == nullptr) continue;

    if (data_size <= 0) {
      SMAFree(data);
      continue;
    }

    QByteArray b(data, data_size);
    SMAFree(data);

    auto* translator = new QTranslator(QCoreApplication::instance());
    auto load = translator->load(
        reinterpret_cast<uchar*>(const_cast<char*>(b.data())), b.size());
    if (load && QCoreApplication::installTranslator(translator)) {
      registered_translators_.append(translator);
      read_translator_data_list_.append(b);
    } else {
      translator->deleteLater();
    }
  }
}

void UIModuleManager::TranslateAllModulesParams() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
  for (auto entry : mounted_entries_.asKeyValueRange()) {
    for (auto& m_entry : entry.second) {
      m_entry.meta_data_translated_.clear();
      for (auto param : m_entry.meta_data_.asKeyValueRange()) {
        m_entry.meta_data_translated_[param.first] =
            QApplication::translate("GTrC", param.second.toUtf8());
        LOG_D() << "module entry metadata key: " << param.first
                << "value: " << param.second
                << "translated: " << m_entry.meta_data_translated_[param.first];
      }
    }
  }
#else
  for (auto it = mounted_entries_.keyValueBegin();
       it != mounted_entries_.keyValueEnd(); ++it) {
    for (auto& m_entry : it->second) {
      m_entry.meta_data_translated_.clear();
      for (auto it_p = m_entry.meta_data_.keyValueBegin();
           it_p != m_entry.meta_data_.keyValueEnd(); ++it_p) {
        m_entry.meta_data_translated_[it_p->first] =
            QApplication::translate("GTrC", it_p->second.toUtf8());
      }
    }
  }
#endif
}

auto UIModuleManager::RegisterQObject(const QString& id, QObject* p) -> bool {
  if (id.isEmpty() || registered_qobjects_.contains(id)) return false;

  registered_qobjects_[id] = p;
  return true;
}

auto UIModuleManager::GetQObject(const QString& id) -> QObject* {
  return registered_qobjects_.value(id, nullptr);
}

auto UIModuleManager::GetCapsule(const QString& uuid) -> std::any {
  return capsule_.take(uuid);
}

auto UIModuleManager::MakeCapsule(std::any v) -> QString {
  auto uuid = QUuid::createUuid().toString();
  capsule_[uuid] = std::move(v);
  return uuid;
}

}  // namespace GpgFrontend::UI