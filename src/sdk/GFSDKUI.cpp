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

#include "GFSDKUI.h"

#include <core/utils/CommonUtils.h>

#include <QMap>
#include <QObject>
#include <QString>

#include "private/GFSDKPrivat.h"
#include "ui/UIModuleManager.h"
#include "ui/function/FilePanelPath.h"
#include "ui/function/UIStyle.h"

auto GFUIShowDialog(void* dialog_raw_ptr, void* parent_raw_ptr) -> int {
  if (dialog_raw_ptr == nullptr) {
    LOG_E() << "dialog raw ptr is nullptr";
    return 0;
  }

  auto* q_obj = static_cast<QObject*>(dialog_raw_ptr);
  QPointer<QDialog> dialog = qobject_cast<QDialog*>(q_obj);

  if (dialog == nullptr) {
    LOG_E() << "convert dialog raw ptr to qdialog failed";
    return 0;
  }

  QPointer<QWidget> parent = nullptr;
  if (parent_raw_ptr != nullptr) {
    auto* qp_obj = static_cast<QObject*>(parent_raw_ptr);
    parent = qobject_cast<QWidget*>(qp_obj);

    if (parent == nullptr) {
      LOG_E() << "convert parent raw ptr to qwidget failed";
      return 0;
    }
  }

  auto* main_thread = QApplication::instance()->thread();

  LOG_D() << "before entering into main thread, current thread id:"
          << QThread::currentThreadId()
          << ", dialog thread: " << dialog->thread()
          << "main thread: " << main_thread;

  if (dialog->thread() != main_thread) {
    LOG_E() << "dialog must be created on main thread";
    return 0;
  }

  QMetaObject::invokeMethod(
      parent == nullptr ? QPointer<QObject>(QApplication::instance()) : parent,
      [dialog, parent]() -> int {
        LOG_D() << "show qdialog, current thread id:"
                << QThread::currentThreadId();
        dialog->setParent(parent);
        dialog->show();
        return 0;
      });

  return 1;
}

auto GFUICreateGUIObject(QObjectFactory factory, void* data) -> void* {
  void* object = nullptr;

  if (QThread::currentThread() == QApplication::instance()->thread()) {
    object = factory(data);
  } else {
    QMetaObject::invokeMethod(
        QApplication::instance(),
        [&]() {
          LOG_D() << "create gui object, current thread id:"
                  << QThread::currentThreadId();
          object = factory(data);
        },
        Qt::BlockingQueuedConnection);
  }

  return object;
}

auto GF_SDK_EXPORT GFUIGetGUIObject(const char* id) -> void* {
  if (id == nullptr) {
    LOG_W() << "gui object id is nullptr";
    return nullptr;
  }

  auto* object = GpgFrontend::UI::UIModuleManager::GetInstance().GetQObject(
      GFStrView(id));

  return object;
}

auto GFUIGlobalSettings() -> void* {
  const auto* settings =
      GpgFrontend::UI::UIModuleManager::GetInstance().GetSettings();
  return static_cast<void*>(const_cast<QSettings*>(settings));
}

auto GFUIRegisterFileExtensionHandleEvent(const char* extension,
                                          const char* event_prefix) -> int {
  if (extension == nullptr || event_prefix == nullptr) {
    LOG_W() << "extension or event prefix is nullptr";
    return -1;
  }

  GpgFrontend::UI::UIModuleManager::GetInstance()
      .RegisterFileExtensionHandleEvent(GFStrView(extension),
                                        GFStrView(event_prefix));
  return 0;
}

auto GFUIRegisterSettingsPage(const char* page_id, const char* section_id,
                              const char* title, const char* keywords,
                              QObjectFactory factory, void* data) -> int {
  // Every string is consumed first: GFUnStrDup frees what it is given, so
  // returning early on one null argument would leak the others.
  GpgFrontend::UI::SettingsPageRegistration reg;
  reg.id = page_id == nullptr ? QString() : GFStrView(page_id);
  reg.section_id = section_id == nullptr ? QString() : GFStrView(section_id);
  reg.title = title == nullptr ? QString() : GFStrView(title);
  reg.keywords = keywords == nullptr
                     ? QStringList{}
                     : GFStrView(keywords).split('\n', Qt::SkipEmptyParts);
  reg.factory = factory;
  reg.data = data;

  return GpgFrontend::UI::UIModuleManager::GetInstance().RegisterSettingsPage(
             reg)
             ? 0
             : -1;
}

auto GFUIUnregisterSettingsPage(const char* page_id) -> int {
  if (page_id == nullptr) {
    LOG_W() << "settings page id is nullptr";
    return -1;
  }

  return GpgFrontend::UI::UIModuleManager::GetInstance().UnregisterSettingsPage(
             GFStrView(page_id))
             ? 0
             : -1;
}
auto GFUIRegisterTabPageView(const char* tab_type, QObjectFactory factory,
                             void* data) -> int {
  // Consumed first: GFUnStrDup frees what it is given, so returning early on a
  // null factory would leak the type string.
  GpgFrontend::UI::TabPageViewRegistration reg;
  reg.tab_type = tab_type == nullptr ? QString() : GFStrView(tab_type);
  reg.factory = factory;
  reg.data = data;

  return GpgFrontend::UI::UIModuleManager::GetInstance().RegisterTabPageView(
             reg)
             ? 0
             : -1;
}

auto GFUIUnregisterTabPageView(const char* tab_type) -> int {
  if (tab_type == nullptr) {
    LOG_W() << "tab page view type is nullptr";
    return -1;
  }

  return GpgFrontend::UI::UIModuleManager::GetInstance().UnregisterTabPageView(
             GFStrView(tab_type))
             ? 0
             : -1;
}

auto GFUIDefaultUserFilePath() -> char* {
  return GFStrDup(GpgFrontend::UI::GetDefaultUserFilePath());
}

namespace {

// Every colour getter is the same shape: resolve the widget, hand its palette
// to the shared UI style, and return the result in a form that survives the C
// boundary. 0 is "not a widget", which callers treat as "use the default".
auto ColorOf(void* widget_raw_ptr,
             const std::function<QColor(const QPalette&)>& pick) -> uint32_t {
  auto* widget = qobject_cast<QWidget*>(static_cast<QObject*>(widget_raw_ptr));
  if (widget == nullptr) {
    LOG_W() << "colour requested for something that is not a QWidget";
    return 0;
  }
  return pick(widget->palette()).rgba();
}

}  // namespace

auto GFUIMutedTextColor(void* widget) -> uint32_t {
  return ColorOf(widget, [](const QPalette& p) {
    return GpgFrontend::UI::MutedTextColor(p);
  });
}

auto GFUIBorderColor(void* widget) -> uint32_t {
  return ColorOf(widget, [](const QPalette& p) {
    return GpgFrontend::UI::BorderColor(p);
  });
}

auto GFUIWarningColor(void* widget) -> uint32_t {
  return ColorOf(widget, [](const QPalette& p) {
    return GpgFrontend::UI::WarningColor(p);
  });
}

auto GFUIDangerColor(void* widget) -> uint32_t {
  return ColorOf(widget, [](const QPalette& p) {
    return GpgFrontend::UI::DangerColor(p);
  });
}

auto GFUIAccentColor(void* widget, int positive) -> uint32_t {
  return ColorOf(widget, [positive](const QPalette& p) {
    return GpgFrontend::UI::AccentColor(p, positive != 0);
  });
}

auto GFUIHumanSize(int64_t bytes) -> char* {
  return GFStrDup(GpgFrontend::UI::HumanSize(static_cast<qint64>(bytes)));
}
