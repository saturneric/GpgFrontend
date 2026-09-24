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

#include <QThread>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleSdkBridge.h"
#include "core/utils/CommonUtils.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/TextEdit.h"
#include "ui/widgets/TextEditTabWidget.h"

namespace GpgFrontend::UI {

namespace {

auto OnGuiThread() -> bool {
  auto* app = QCoreApplication::instance();
  return app == nullptr || QThread::currentThread() == app->thread();
}

/**
 * @brief Run @p fn on the GUI thread and return what it returns.
 *
 * Blocking is right when the caller is elsewhere -- it asked for the answer --
 * but a BlockingQueuedConnection to one's OWN thread is a deadlock, so the GUI
 * thread runs @p fn directly. Once shutdown has begun the GUI event loop is
 * gone and a blocking hop would never return, so the answer is then "none".
 */
template <typename Fn>
auto AskGui(Fn fn, decltype(fn()) fallback) -> decltype(fn()) {
  if (OnGuiThread()) return fn();
  if (Module::GlobalModuleDispatchGate().IsClosed()) return fallback;
  auto out = fallback;
  QMetaObject::invokeMethod(
      QCoreApplication::instance(), [&] { out = fn(); },
      Qt::BlockingQueuedConnection);
  return out;
}

auto MainEditor() -> TextEdit* {
  return qobject_cast<TextEdit*>(
      UIModuleManager::GetInstance().GetQObject("main_window_edit"));
}

}  // namespace

UIModuleManager::UIModuleManager(int channel)
    : SingletonFunctionObject<UIModuleManager>(channel) {}

UIModuleManager::~UIModuleManager() {
  for (const auto& id : installed_translators_.keys()) uninstall_translator(id);
}

auto UIModuleManager::RegisterTranslatorDataReader(
    Module::ModuleIdentifier id, GFTranslatorDataReader reader) -> bool {
  if (reader == nullptr || id.isEmpty() || !Module::IsModuleExists(id)) {
    return false;
  }
  LOG_D() << "module " << id << "registering translator reader...";
  {
    const QMutexLocker lock(&readers_mutex_);
    translator_data_readers_[id] = reader;
  }
  auto* app = QCoreApplication::instance();
  if (app == nullptr) return true;
  QMetaObject::invokeMethod(
      app, [this, id]() { install_translator(id); }, Qt::QueuedConnection);
  return true;
}

auto UIModuleManager::UnregisterTranslatorDataReader(
    const Module::ModuleIdentifier& id) -> bool {
  bool removed = false;
  {
    const QMutexLocker lock(&readers_mutex_);
    removed = translator_data_readers_.remove(id) > 0;
  }
  if (!removed) return false;
  if (OnGuiThread()) {
    uninstall_translator(id);
  } else {
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [this, id]() { uninstall_translator(id); }, Qt::QueuedConnection);
  }
  return true;
}

void UIModuleManager::uninstall_translator(const QString& id) {
  // Order matters: a translator still installed on QCoreApplication keeps
  // reading entry.data, so it has to be uninstalled and destroyed before the
  // entry (and with it the QM bytes) is dropped. deleteLater() would not do --
  // it lets the translator outlive the buffer.
  auto it = installed_translators_.find(id);
  if (it == installed_translators_.end()) return;
  if (it->translator != nullptr) {
    QCoreApplication::removeTranslator(it->translator);
    delete it->translator;
  }
  installed_translators_.erase(it);
}

void UIModuleManager::install_translator(const QString& id) {
  uninstall_translator(id);

  GFTranslatorDataReader reader = nullptr;
  {
    const QMutexLocker lock(&readers_mutex_);
    reader = translator_data_readers_.value(id, nullptr);
  }
  if (reader == nullptr) return;

  // Borrowed, not donated: the reader must not own the locale string.
  const auto locale_utf8 = QLocale().name().toUtf8();
  char* data = nullptr;
  int data_size = 0;

  // A reader is module code, entered the way every host-to-module call is:
  // only while the module is active, and attributed to it.
  {
    Module::ModuleDispatchScope global(Module::GlobalModuleDispatchGate());
    if (!global.Entered()) return;
    Module::ModuleDispatchScope own(Module::ModuleEntryGate(id));
    if (!own.Entered()) return;
    const auto id_utf8 = id.toUtf8();
    const Module::ModuleAttributionScope attributed(id_utf8.constData());
    data_size = reader(locale_utf8.constData(), &data);
  }
  LOG_D() << "module " << id << "reader, read locale " << QLocale().name()
          << ", data size: " << data_size;

  if (data == nullptr) return;
  if (data_size <= 0) {
    SMAFree(data);
    return;
  }

  InstalledModuleTranslator entry;
  entry.data = QByteArray(data, data_size);
  SMAFree(data);

  // Load from the entry's own copy of the bytes, not from a local that goes
  // out of scope: QTranslator reads this buffer for as long as it lives.
  entry.translator = new QTranslator(QCoreApplication::instance());
  const auto loaded = entry.translator->load(
      reinterpret_cast<uchar*>(const_cast<char*>(entry.data.data())),
      static_cast<int>(entry.data.size()));
  if (loaded && QCoreApplication::installTranslator(entry.translator)) {
    installed_translators_.insert(id, entry);
  } else {
    delete entry.translator;
  }
}

void UIModuleManager::RegisterAllModuleTranslators() {
  QStringList ids;
  {
    const QMutexLocker lock(&readers_mutex_);
    ids = translator_data_readers_.keys();
  }
  for (const auto& id : installed_translators_.keys()) uninstall_translator(id);
  for (const auto& id : ids) install_translator(id);
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

  if (registered_qobjects_.contains(id)) {
    LOG_W() << "QObject with id " << id << " already registered, overwriting";
  }
  registered_qobjects_[id] = QPointer<QObject>(p);

  // qApp as the context object: the lambda captures this manager, so it must
  // not outlive the application, and the map is only touched on the GUI
  // thread even when p is destroyed on another one. Only an entry that no
  // longer points anywhere is dropped: an object that was replaced under the
  // same name must not take its replacement's entry with it.
  QObject::connect(p, &QObject::destroyed, QCoreApplication::instance(),
                   [this, id]() {
                     const auto it = registered_qobjects_.find(id);
                     if (it != registered_qobjects_.end() && it->isNull()) {
                       registered_qobjects_.erase(it);
                     }
                   });
  return id;
}

auto UIModuleManager::GetQObject(const QString& id) -> QObject* {
  Q_ASSERT(OnGuiThread());
  return registered_qobjects_.value(id, nullptr);
}

auto RegisterNamedQObject(const QString& id, QObject* p) -> QString {
  return UIModuleManager::GetInstance().RegisterQObject(id, p);
}

auto CurrentEditorContent() -> std::optional<QByteArray> {
  // The document belongs to the GUI thread, and a module's handler does not
  // necessarily run there -- so does the lookup of the editor itself.
  return AskGui(
      []() -> std::optional<QByteArray> {
        auto* edit = MainEditor();
        if (edit == nullptr || edit->CurTextPage() == nullptr) {
          return std::nullopt;
        }
        return edit->CurDocumentBytesForOperation();
      },
      std::nullopt);
}

auto CurrentGpgContextChannel() -> int {
  return AskGui(
      []() -> int {
        auto* window = qobject_cast<MainWindow*>(
            UIModuleManager::GetInstance().GetQObject("main_window"));
        return window == nullptr ? -1 : window->GetCurrentGpgContextChannel();
      },
      -1);
}

auto CurrentDocumentInfo() -> std::optional<QCborMap> {
  return AskGui(
      []() -> std::optional<QCborMap> {
        auto* edit = MainEditor();
        if (edit == nullptr) return std::nullopt;
        auto* page = edit->CurTextPage();
        if (page == nullptr) return std::nullopt;
        QCborMap m;
        m.insert(QStringLiteral("id"), TextEditTabWidget::DocumentIdOf(page));
        m.insert(QStringLiteral("type"), page->property("type").toString());
        m.insert(QStringLiteral("title"),
                 page->property("base_title").toString());
        m.insert(QStringLiteral("path"), page->GetFilePath());
        m.insert(QStringLiteral("modified"),
                 page->GetTextPage()->document()->isModified());
        return m;
      },
      std::nullopt);
}

}  // namespace GpgFrontend::UI
