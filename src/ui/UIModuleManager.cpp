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

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend::UI {

UIModuleManager::UIModuleManager(int channel)
    : SingletonFunctionObject<UIModuleManager>(channel),
      settings_(GpgFrontend::GetSettings()) {}

UIModuleManager::~UIModuleManager() = default;

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

  for (auto it = translator_data_readers_.keyValueBegin();
       it != translator_data_readers_.keyValueEnd(); ++it) {
    char* data = nullptr;

    auto data_size = it->second.reader_(GFStrDup(locale_name), &data);
    LOG_D() << "module " << it->first << "reader, read locale " << locale_name
            << ", data size: " << data_size;

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

auto UIModuleManager::RegisterQObject(QObject* p) -> QString {
  const QString id = QString::number(reinterpret_cast<quintptr>(p), 16);
  QPointer<QObject> ptr = p;
  registered_qobjects_[id] = ptr;
  QObject::connect(p, &QObject::destroyed,
                   [this, id]() { registered_qobjects_.remove(id); });
  return id;
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

auto UIModuleManager::GetSettings() const -> const QSettings* {
  return &settings_;
}

auto GF_UI_EXPORT RegisterQObject(QObject* p) -> QString {
  return UIModuleManager::GetInstance().RegisterQObject(p);
}

}  // namespace GpgFrontend::UI