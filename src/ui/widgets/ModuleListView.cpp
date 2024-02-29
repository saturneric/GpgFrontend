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

#include "ModuleListView.h"

#include "core/module/ModuleManager.h"

namespace GpgFrontend::UI {

ModuleListView::ModuleListView(QWidget *parent)
    : QListView(parent), model_(new QStringListModel(this)) {
  setModel(model_);

  load_module_informations();
}

void ModuleListView::currentChanged(const QModelIndex &current,
                                    const QModelIndex &previous) {
  QListView::currentChanged(current, previous);
  emit this->SignalSelectModule(model_->data(current).toString());
}

void ModuleListView::load_module_informations() {
  auto module_ids =
      Module::ModuleManager::GetInstance().ListAllRegisteredModuleID();

  QStringList model_data;
  for (const auto &module_id : module_ids) {
    model_data.append(module_id);
  }

  model_->setStringList(model_data);
}

auto ModuleListView::GetCurrentModuleID() -> Module::ModuleIdentifier {
  return model_->data(currentIndex()).toString();
}
};  // namespace GpgFrontend::UI
