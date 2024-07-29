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

#include "GRTTreeView.h"

#include "core/module/GlobalRegisterTableTreeModel.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend::UI {

GRTTreeView::GRTTreeView(QWidget* parent) : QTreeView(parent) {
  setModel(new Module::GlobalRegisterTableTreeModel(
      Module::ModuleManager::GetInstance().GRT(), this));

  connect(model(), &QFileSystemModel::layoutChanged, this,
          &GRTTreeView::slot_adjust_column_widths);
  connect(model(), &QFileSystemModel::dataChanged, this,
          &GRTTreeView::slot_adjust_column_widths);
  connect(this, &GRTTreeView::expanded, this,
          &GRTTreeView::slot_adjust_column_widths);
  connect(this, &GRTTreeView::collapsed, this,
          &GRTTreeView::slot_adjust_column_widths);
}

GRTTreeView::~GRTTreeView() = default;

void GRTTreeView::paintEvent(QPaintEvent* event) {
  QTreeView::paintEvent(event);

  if (!initial_resize_done_) {
    slot_adjust_column_widths();
    initial_resize_done_ = true;
  }
}

void GRTTreeView::slot_adjust_column_widths() {
  for (int i = 0; i < model()->columnCount(); ++i) {
    this->resizeColumnToContents(i);
  }
}
}  // namespace GpgFrontend::UI