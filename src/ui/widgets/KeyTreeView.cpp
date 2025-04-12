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

#include "ui/widgets/KeyTreeView.h"

#include <utility>

#include "core/function/gpg/GpgKeyGetter.h"
#include "model/GpgKeyTreeProxyModel.h"

namespace GpgFrontend::UI {

KeyTreeView::KeyTreeView(int channel,
                         GpgKeyTreeModel::Detector checkable_detector,
                         GpgKeyTreeProxyModel::KeyFilter filter,
                         QWidget* parent)
    : QTreeView(parent),
      channel_(channel),
      model_(QSharedPointer<GpgKeyTreeModel>::create(
          channel, GpgKeyGetter::GetInstance(channel_).FetchKey(),
          checkable_detector, this)),
      proxy_model_(model_, GpgKeyTreeDisplayMode::kALL, std::move(filter),
                   this) {
  setModel(&proxy_model_);

  sortByColumn(2, Qt::AscendingOrder);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);

  setEditTriggers(QAbstractItemView::NoEditTriggers);
  header()->resizeSections(QHeaderView::Interactive);
  header()->setDefaultAlignment(Qt::AlignCenter);
  header()->setMinimumHeight(20);

  setUniformRowHeights(true);
  setExpandsOnDoubleClick(true);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

  setFocusPolicy(Qt::NoFocus);
  setAlternatingRowColors(true);
  setSortingEnabled(true);
}

void KeyTreeView::paintEvent(QPaintEvent* event) {
  QTreeView::paintEvent(event);

  if (!init_) {
    slot_adjust_column_widths();
    init_ = true;
  }
}

void KeyTreeView::slot_adjust_column_widths() {
  for (int i = 1; i < model_->columnCount({}); ++i) {
    this->resizeColumnToContents(i);
  }
}

auto KeyTreeView::GetAllCheckedKeyIds() -> KeyIdArgsList {
  return model_->GetAllCheckedKeyIds();
}

auto KeyTreeView::GetAllCheckedSubKey() -> QContainer<GpgSubKey> {
  return model_->GetAllCheckedSubKey();
}

}  // namespace GpgFrontend::UI
