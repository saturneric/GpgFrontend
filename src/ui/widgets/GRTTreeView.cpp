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

GRTTreeView::GRTTreeView(QWidget* parent)
    : QTreeView(parent),
      model_(new Module::GlobalRegisterTableTreeModel(
          Module::ModuleManager::GetInstance().GRT(), this)),
      proxy_model_(new QSortFilterProxyModel(this)) {
  proxy_model_->setSourceModel(model_);
  proxy_model_->setRecursiveFilteringEnabled(true);
  proxy_model_->setFilterKeyColumn(-1);
  proxy_model_->setFilterCaseSensitivity(Qt::CaseInsensitive);
  setModel(proxy_model_);

  setUniformRowHeights(true);
  setAlternatingRowColors(true);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setTextElideMode(Qt::ElideRight);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(proxy_model_, &QAbstractItemModel::layoutChanged, this,
          &GRTTreeView::slot_adjust_column_widths);
  connect(proxy_model_, &QAbstractItemModel::dataChanged, this,
          &GRTTreeView::slot_adjust_column_widths);
  connect(proxy_model_, &QAbstractItemModel::modelReset, this,
          &GRTTreeView::slot_adjust_column_widths);
  connect(this, &GRTTreeView::expanded, this,
          &GRTTreeView::slot_adjust_column_widths);
  connect(this, &GRTTreeView::collapsed, this,
          &GRTTreeView::slot_adjust_column_widths);
  connect(this, &GRTTreeView::customContextMenuRequested, this,
          &GRTTreeView::slot_show_context_menu);

  // the model refreshes itself whenever a module publishes a value, keep the
  // user's expansion state across those resets
  connect(model_, &QAbstractItemModel::modelAboutToBeReset, this,
          [this]() { expanded_paths_cache_ = save_expanded_paths(); });
  connect(model_, &QAbstractItemModel::modelReset, this,
          [this]() { restore_expanded_paths(expanded_paths_cache_); });
}

GRTTreeView::~GRTTreeView() = default;

void GRTTreeView::SetFilter(const QString& text) {
  proxy_model_->setFilterFixedString(text);
  if (!text.isEmpty()) expandAll();
  slot_adjust_column_widths();
}

void GRTTreeView::ExpandAll() { expandAll(); }

void GRTTreeView::CollapseAll() { collapseAll(); }

void GRTTreeView::Refresh() { model_->Refresh(); }

void GRTTreeView::paintEvent(QPaintEvent* event) {
  QTreeView::paintEvent(event);

  if (!initial_resize_done_) {
    slot_adjust_column_widths();
    initial_resize_done_ = true;
  }

  if (proxy_model_->rowCount({}) == 0) {
    QPainter painter(viewport());
    auto color = palette().color(QPalette::Text);
    color.setAlpha(140);
    painter.setPen(color);
    painter.drawText(viewport()->rect(), Qt::AlignCenter,
                     tr("No runtime values published yet."));
  }
}

void GRTTreeView::slot_adjust_column_widths() {
  for (int i = 0; i < model()->columnCount(); ++i) {
    this->resizeColumnToContents(i);
  }
}

void GRTTreeView::slot_show_context_menu(const QPoint& pos) {
  const auto index = indexAt(pos);
  if (!index.isValid()) return;

  QMenu menu(this);

  menu.addAction(tr("Copy Key Path"), this, [=]() {
    QGuiApplication::clipboard()->setText(
        index.data(Module::kGRTPathRole).toString());
  });

  const auto value = index.data(Module::kGRTValueRole);
  auto* copy_value = menu.addAction(tr("Copy Value"), this, [=]() {
    QGuiApplication::clipboard()->setText(value.toString());
  });
  copy_value->setEnabled(value.isValid() && !value.toString().isEmpty());

  menu.addSeparator();
  menu.addAction(tr("Expand All"), this, [=]() { expandAll(); });
  menu.addAction(tr("Collapse All"), this, [=]() { collapseAll(); });

  menu.exec(viewport()->mapToGlobal(pos));
}

auto GRTTreeView::save_expanded_paths() const -> QSet<QString> {
  QSet<QString> paths;

  QContainer<QModelIndex> pending;
  for (auto row = 0; row < proxy_model_->rowCount({}); ++row) {
    pending.push_back(proxy_model_->index(row, 0, {}));
  }

  while (!pending.isEmpty()) {
    const auto index = pending.takeLast();
    if (!isExpanded(index)) continue;

    paths.insert(index.data(Module::kGRTPathRole).toString());
    for (auto row = 0; row < proxy_model_->rowCount(index); ++row) {
      pending.push_back(proxy_model_->index(row, 0, index));
    }
  }

  return paths;
}

void GRTTreeView::restore_expanded_paths(const QSet<QString>& paths) {
  if (paths.isEmpty()) return;

  QContainer<QModelIndex> pending;
  for (auto row = 0; row < proxy_model_->rowCount({}); ++row) {
    pending.push_back(proxy_model_->index(row, 0, {}));
  }

  while (!pending.isEmpty()) {
    const auto index = pending.takeLast();
    if (!paths.contains(index.data(Module::kGRTPathRole).toString())) continue;

    setExpanded(index, true);
    for (auto row = 0; row < proxy_model_->rowCount(index); ++row) {
      pending.push_back(proxy_model_->index(row, 0, index));
    }
  }
}
}  // namespace GpgFrontend::UI
