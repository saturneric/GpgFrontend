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

#include "core/function/gpg/GpgAbstractKeyGetter.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/model/GpgKeyTreeProxyModel.h"

namespace GpgFrontend::UI {

KeyTreeView::KeyTreeView(QWidget* parent)
    : QTreeView(parent),
      channel_(kGpgFrontendDefaultChannel),
      model_(SecureCreateSharedObject<GpgKeyTreeModel>(
          channel_, GpgAbstractKeyGetter::GetInstance(channel_).Fetch(),
          [](auto) { return false; }, this)),
      proxy_model_(
          model_, GpgKeyTreeDisplayMode::kALL, [](auto) { return false; },
          this) {
  init();
}

KeyTreeView::KeyTreeView(int channel,
                         GpgKeyTreeModel::Detector checkable_detector,
                         GpgKeyTreeProxyModel::KeyFilter filter,
                         QWidget* parent)
    : QTreeView(parent),
      channel_(channel),
      model_(SecureCreateSharedObject<GpgKeyTreeModel>(
          channel_, GpgAbstractKeyGetter::GetInstance(channel_).Fetch(),
          checkable_detector, this)),
      proxy_model_(model_, GpgKeyTreeDisplayMode::kALL, std::move(filter),
                   this) {
  init();
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

void KeyTreeView::init() {
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

  connect(this, &QTableView::doubleClicked, this,
          [this](const QModelIndex& index) {
            if (!index.isValid()) return;

            auto key = GetKeyByIndex(index);
            if (key == nullptr) return;

            CommonUtils::OpenDetailsDialogByKey(
                this, model_->GetGpgContextChannel(), key);
          });

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this, [=] {
            model_ = SecureCreateSharedObject<GpgKeyTreeModel>(
                channel_, GpgAbstractKeyGetter::GetInstance(channel_).Fetch(),
                [](auto) { return false; }, this);
            proxy_model_.setSourceModel(model_.get());
            proxy_model_.invalidate();
          });

  connect(model_.get(), &GpgKeyTreeModel::SignalKeyCheckedChanged, this,
          [=](GpgAbstractKey*, bool) {
            emit SignalKeysChecked(GetAllCheckedKeys());
          });
}

void KeyTreeView::SetKeyFilter(const GpgKeyTreeProxyModel::KeyFilter& filter) {
  proxy_model_.SetKeyFilter(filter);
}

void KeyTreeView::SetChannel(int channel) {
  if (channel_ == channel) return;
  LOG_D() << "new channel for key tree view: " << channel;

  channel_ = channel;
  init_ = false;
  model_ = SecureCreateSharedObject<GpgKeyTreeModel>(
      channel_, GpgAbstractKeyGetter::GetInstance(channel_).Fetch(),
      [](auto) { return false; }, this);
  proxy_model_.setSourceModel(model_.get());
  proxy_model_.invalidate();
}

auto KeyTreeView::GetKeyByIndex(QModelIndex index) -> GpgAbstractKeyPtr {
  auto s_index = proxy_model_.mapToSource(index);
  auto* i = s_index.isValid()
                ? static_cast<GpgKeyTreeItem*>(s_index.internalPointer())
                : nullptr;
  assert(i != nullptr);

  return i->SharedKey();
}

void KeyTreeView::Refresh() { SetChannel(channel_); }

auto KeyTreeView::GetAllCheckedKeys() -> GpgAbstractKeyPtrList {
  return model_->GetAllCheckedKeys();
}
}  // namespace GpgFrontend::UI
