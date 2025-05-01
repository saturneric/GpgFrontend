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

auto MetaDataArrayToQMap(MetaData** meta_data_array, int size)
    -> QMap<QString, QString> {
  QMap<QString, QString> map;

  for (int i = 0; i < size; ++i) {
    QString const key = GFUnStrDup(meta_data_array[i]->key);
    QString const value = GFUnStrDup(meta_data_array[i]->value);
    map.insert(key, value);

    GpgFrontend::SecureFree(meta_data_array[i]);
  }

  GpgFrontend::SecureFree(meta_data_array);
  return map;
}

auto GFUIMountEntry(const char* id, MetaData** meta_data_array,
                    int meta_data_array_size, QObjectFactory factory) -> int {
  if (id == nullptr || factory == nullptr) return -1;

  auto meta_data = MetaDataArrayToQMap(meta_data_array, meta_data_array_size);
  auto qid = GFUnStrDup(id);

  QMetaObject::invokeMethod(
      QApplication::instance()->thread(), [qid, meta_data, factory]() -> int {
        return GpgFrontend::UI::UIModuleManager::GetInstance().MountEntry(
                   qid, meta_data, factory)
                   ? 0
                   : -1;
      });

  return 0;
}

auto GF_SDK_EXPORT GFUIMainWindowPtr() -> void* {
  return GpgFrontend::UI::UIModuleManager::GetInstance().GetQObject(
      "main_window");
}

auto GF_SDK_EXPORT GFUIShowDialog(void* dialog_raw_ptr, void* parent_raw_ptr)
    -> bool {
  if (dialog_raw_ptr == nullptr) {
    LOG_E() << "dialog raw ptr is nullptr";
    return false;
  }

  auto* q_obj = static_cast<QObject*>(dialog_raw_ptr);
  QPointer<QDialog> dialog = qobject_cast<QDialog*>(q_obj);

  if (dialog == nullptr) {
    LOG_E() << "convert dialog raw ptr to qdialog failed";
    return false;
  }

  QPointer<QWidget> parent = nullptr;
  if (parent_raw_ptr != nullptr) {
    auto* qp_obj = static_cast<QObject*>(parent_raw_ptr);
    parent = qobject_cast<QWidget*>(qp_obj);

    if (parent == nullptr) {
      LOG_E() << "convert parent raw ptr to qwidget failed";
      return false;
    }
  }

  auto* main_thread = QApplication::instance()->thread();

  LOG_D() << "before entering into main thread, current thread id:"
          << QThread::currentThreadId()
          << ", dialog thread: " << dialog->thread()
          << "main thread: " << main_thread;

  if (dialog->thread() != main_thread) {
    LOG_E() << "dialog must be created on main thread";
    return false;
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

  return true;
}

auto GF_SDK_EXPORT GFUICreateGUIObject(QObjectFactory factory, void* data)
    -> void* {
  QEventLoop loop;
  void* object = nullptr;

  QMetaObject::invokeMethod(QApplication::instance(), [&]() -> int {
    LOG_D() << "create gui object, current thread id:"
            << QThread::currentThreadId();
    object = factory(data);
    loop.quit();
    return 0;
  });

  loop.exec();
  return object;
}
