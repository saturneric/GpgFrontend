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

#include <QReadWriteLock>
#include <QThread>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/TextEdit.h"
#include "ui/widgets/TextEditTabWidget.h"

namespace GpgFrontend::UI {

UIModuleManager::UIModuleManager(int channel)
    : SingletonFunctionObject<UIModuleManager>(channel) {}

UIModuleManager::~UIModuleManager() { clear_installed_translators(); }

auto UIModuleManager::RegisterTranslatorDataReader(
    Module::ModuleIdentifier id, GFTranslatorDataReader reader) -> bool {
  if (reader != nullptr && !id.isEmpty() && Module::IsModuleExists(id)) {
    LOG_D() << "module " << id << "registering translator reader...";
    translator_data_readers_[id] = ModuleTranslatorInfo{reader};
    return true;
  }
  return false;
}

auto UIModuleManager::UnregisterTranslatorDataReader(
    const Module::ModuleIdentifier& id) -> bool {
  return translator_data_readers_.remove(id) > 0;
}

void UIModuleManager::clear_installed_translators() {
  // Order matters: a translator still installed on QCoreApplication keeps
  // reading entry.data, so it has to be uninstalled and destroyed before the
  // entry (and with it the QM bytes) is dropped. deleteLater() would not do --
  // it lets the translator outlive the buffer.
  for (const auto& entry : installed_translators_) {
    if (entry.translator == nullptr) continue;
    QCoreApplication::removeTranslator(entry.translator);
    delete entry.translator;
  }
  installed_translators_.clear();
}

void UIModuleManager::RegisterAllModuleTranslators() {
  clear_installed_translators();

  const auto locale_name = QLocale().name();

  // Borrowed, not donated. This used to pass GFStrDup(locale_name) -- a fresh
  // SDK allocation handed to the module on every reader call, which no module
  // frees and the host never reclaims, so it leaked once per module per locale
  // change. It also contradicted the SDK's own rule that arguments are
  // borrowed, which is the rule every reader is written against.
  const auto locale_utf8 = locale_name.toUtf8();

  for (auto it = translator_data_readers_.keyValueBegin();
       it != translator_data_readers_.keyValueEnd(); ++it) {
    char* data = nullptr;

    auto data_size = it->second.reader_(locale_utf8.constData(), &data);
    LOG_D() << "module " << it->first << "reader, read locale " << locale_name
            << ", data size: " << data_size;

    if (data == nullptr) continue;

    if (data_size <= 0) {
      SMAFree(data);
      continue;
    }

    InstalledModuleTranslator entry;
    entry.data = QByteArray(data, data_size);
    SMAFree(data);

    // Load from the entry's own copy of the bytes, not from a local that goes
    // out of scope: QTranslator reads this buffer for as long as it lives.
    entry.translator = new QTranslator(QCoreApplication::instance());
    auto load = entry.translator->load(
        reinterpret_cast<uchar*>(const_cast<char*>(entry.data.data())),
        static_cast<int>(entry.data.size()));
    if (load && QCoreApplication::installTranslator(entry.translator)) {
      installed_translators_.append(entry);
    } else {
      delete entry.translator;
    }
  }
}

auto UIModuleManager::InstalledTranslators() const
    -> QContainer<QPointer<QTranslator>> {
  QContainer<QPointer<QTranslator>> translators;
  translators.reserve(installed_translators_.size());
  for (const auto& entry : installed_translators_) {
    translators.append(QPointer<QTranslator>(entry.translator));
  }
  return translators;
}

auto UIModuleManager::RegisterQObject(const QString& id, QObject* p)
    -> QString {
  // Null clears the name: there is nothing to watch for destruction.
  if (p == nullptr) {
    registered_qobjects_.remove(id);
    return id;
  }

  QPointer<QObject> ptr = p;

  if (registered_qobjects_.contains(id)) {
    LOG_W() << "QObject with id " << id << " already registered, overwriting";
  }

  registered_qobjects_[id] = ptr;
  // qApp as the context object: the lambda captures this manager, so it must
  // not outlive the application, and the map must only be touched from the
  // main thread even when p is destroyed on another one.
  QObject::connect(p, &QObject::destroyed, QCoreApplication::instance(),
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

auto RegisterNamedQObject(const QString& id, QObject* p) -> QString {
  return UIModuleManager::GetInstance().RegisterQObject(id, p);
}

auto CurrentEditorContent() -> std::optional<QByteArray> {
  auto* edit = qobject_cast<TextEdit*>(
      UIModuleManager::GetInstance().GetQObject("main_window_edit"));
  if (edit == nullptr) return std::nullopt;

  // The document belongs to the GUI thread, and a module's handler does not
  // necessarily run there. Blocking is right when it does not -- the caller
  // asked for the bytes -- but a BlockingQueuedConnection to one's OWN thread
  // is a deadlock, which Qt refuses with "Dead lock detected" and an empty
  // result. A module dialog's button handler is on the GUI thread, so that is
  // the common case, not the exotic one.
  const auto read = [edit]() -> std::optional<QByteArray> {
    if (edit->CurTextPage() == nullptr) return std::nullopt;
    return edit->CurDocumentBytesForOperation();
  };
  if (QThread::currentThread() == edit->thread()) return read();

  std::optional<QByteArray> bytes;
  QMetaObject::invokeMethod(
      edit, [&] { bytes = read(); }, Qt::BlockingQueuedConnection);
  return bytes;
}

auto CurrentDocumentInfo() -> std::optional<QCborMap> {
  auto* edit = qobject_cast<TextEdit*>(
      UIModuleManager::GetInstance().GetQObject("main_window_edit"));
  if (edit == nullptr) return std::nullopt;

  const auto read = [edit]() -> std::optional<QCborMap> {
    auto* page = edit->CurTextPage();
    if (page == nullptr) return std::nullopt;
    QCborMap m;
    m.insert(QStringLiteral("id"), TextEditTabWidget::DocumentIdOf(page));
    m.insert(QStringLiteral("type"), page->property("type").toString());
    m.insert(QStringLiteral("title"), page->property("base_title").toString());
    m.insert(QStringLiteral("path"), page->GetFilePath());
    m.insert(QStringLiteral("modified"),
             page->GetTextPage()->document()->isModified());
    return m;
  };
  if (QThread::currentThread() == edit->thread()) return read();

  std::optional<QCborMap> info;
  QMetaObject::invokeMethod(
      edit, [&] { info = read(); }, Qt::BlockingQueuedConnection);
  return info;
}

}  // namespace GpgFrontend::UI