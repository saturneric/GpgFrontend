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

#include "GFSDKUI.h"

#include <core/utils/CommonUtils.h>

#include <QMap>
#include <QString>

#include "sdk/private/CommonUtils.h"
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
                    int meta_data_array_size, EntryFactory factory) -> int {
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
