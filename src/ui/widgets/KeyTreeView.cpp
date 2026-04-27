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

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/model/GpgKeyTreeProxyModel.h"

namespace GpgFrontend::UI {

KeyTreeView::KeyTreeView(QWidget* parent)
    : QTreeView(parent),
      checkable_detector_([](auto) { return false; }),
      key_filter_([](auto) { return true; }),
      model_(SecureCreateSharedObject<GpgKeyTreeModel>(
          channel_, AbstractKeyRepository::GetInstance(channel_).Fetch(),
          checkable_detector_, this)),
      proxy_model_(model_, GpgKeyTreeDisplayMode::kALL, key_filter_, this) {
  init();
}

KeyTreeView::KeyTreeView(int channel,
                         GpgKeyTreeModel::Detector checkable_detector,
                         GpgKeyTreeProxyModel::KeyFilter filter,
                         QWidget* parent)
    : QTreeView(parent),
      channel_(channel),
      checkable_detector_(std::move(checkable_detector)),
      key_filter_(std::move(filter)),
      model_(SecureCreateSharedObject<GpgKeyTreeModel>(
          channel_, AbstractKeyRepository::GetInstance(channel_).Fetch(),
          checkable_detector_, this)),
      proxy_model_(model_, GpgKeyTreeDisplayMode::kALL, key_filter_, this) {
  init();
}

void KeyTreeView::init_view_style() {
  setProperty("gfKeyTreeView", true);

  setRootIsDecorated(true);
  setItemsExpandable(true);
  setExpandsOnDoubleClick(true);
  setAnimated(false);

  setUniformRowHeights(true);
  setAlternatingRowColors(true);
  setAllColumnsShowFocus(true);
  setMouseTracking(true);

  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setFocusPolicy(Qt::NoFocus);

  setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  setSortingEnabled(true);
  sortByColumn(2, Qt::AscendingOrder);

  header()->setStretchLastSection(true);
  header()->setHighlightSections(false);
  header()->setSectionsClickable(true);
  header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  header()->setMinimumHeight(24);

  header()->setSectionResizeMode(0, QHeaderView::Interactive);
  header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  header()->setSectionResizeMode(4, QHeaderView::Stretch);

  setStyleSheet(R"(
QTreeView[gfKeyTreeView="true"] {
  outline: 0;
}

QTreeView[gfKeyTreeView="true"]::item {
  min-height: 24px;
  padding: 2px 4px;
  border: none;
}

QTreeView[gfKeyTreeView="true"]::item:selected {
  background: palette(highlight);
  color: palette(highlighted-text);
}
)");

  style()->unpolish(this);
  style()->polish(this);
}

void KeyTreeView::paintEvent(QPaintEvent* event) {
  QTreeView::paintEvent(event);

  if (!init_) {
    slot_adjust_column_widths();
    init_ = true;
  }
}

void KeyTreeView::slot_adjust_column_widths() {
  if (model_ == nullptr) return;

  for (int i = 1; i < model_->columnCount({}); ++i) {
    resizeColumnToContents(i);
  }

  header()->setStretchLastSection(true);
}

auto KeyTreeView::GetAllCheckedKeyIds() -> KeyIdArgsList {
  return model_->GetAllCheckedKeyIds();
}

auto KeyTreeView::GetAllCheckedSubKey() -> QContainer<GpgSubKey> {
  return model_->GetAllCheckedSubKey();
}

void KeyTreeView::init() {
  setModel(&proxy_model_);

  init_view_style();

  connect(this, &QTreeView::doubleClicked, this,
          [this](const QModelIndex& index) {
            if (!index.isValid()) return;

            auto key = GetKeyByIndex(index);
            if (key == nullptr) return;

            CommonUtils::OpenDetailsDialogByKey(
                this, model_->GetGpgContextChannel(), key);
          });

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyTreeView::reset_model);

  connect(model_.get(), &GpgKeyTreeModel::SignalKeyCheckedChanged, this,
          [this](GpgAbstractKey*, bool) {
            emit SignalKeysChecked(GetAllCheckedKeys());
          });
}

void KeyTreeView::reset_model() {
  init_ = false;

  model_ = SecureCreateSharedObject<GpgKeyTreeModel>(
      channel_, AbstractKeyRepository::GetInstance(channel_).Fetch(),
      checkable_detector_, this);

  proxy_model_.setSourceModel(model_.get());
  proxy_model_.SetKeyFilter(key_filter_);
  proxy_model_.invalidate();

  connect(model_.get(), &GpgKeyTreeModel::SignalKeyCheckedChanged, this,
          [this](GpgAbstractKey*, bool) {
            emit SignalKeysChecked(GetAllCheckedKeys());
          });

  expandToDepth(0);
  slot_adjust_column_widths();
}

void KeyTreeView::SetKeyFilter(const GpgKeyTreeProxyModel::KeyFilter& filter) {
  key_filter_ = filter;
  proxy_model_.SetKeyFilter(key_filter_);
  proxy_model_.invalidate();
}

void KeyTreeView::SetChannel(int channel) {
  if (channel_ == channel) return;

  LOG_D() << "new channel for key tree view: " << channel;

  channel_ = channel;
  reset_model();
}

auto KeyTreeView::GetKeyByIndex(QModelIndex index) -> GpgAbstractKeyPtr {
  const auto source_index = proxy_model_.mapToSource(index);

  auto* item =
      source_index.isValid()
          ? static_cast<GpgKeyTreeItem*>(source_index.internalPointer())
          : nullptr;

  if (item == nullptr) {
    return nullptr;
  }

  return item->SharedKey();
}

void KeyTreeView::Refresh() { reset_model(); }

auto KeyTreeView::GetAllCheckedKeys() -> GpgAbstractKeyPtrList {
  return model_->GetAllCheckedKeys();
}
}  // namespace GpgFrontend::UI
