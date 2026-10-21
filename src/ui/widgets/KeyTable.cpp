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

#include "ui/widgets/KeyTable.h"

#include "ui/UserInterfaceUtils.h"
#include "ui/widgets/KeyTableEmptyState.h"

namespace GpgFrontend::UI {

auto KeyTable::GetCheckedKeys() const -> GpgAbstractKeyPtrList {
  GpgAbstractKeyPtrList ret;

  for (int row = 0; row < GetRowCount(); ++row) {
    if (!IsRowChecked(row)) continue;

    auto key = GetKeyByIndex(model()->index(row, 0));
    if (key == nullptr) continue;

    ret.push_back(key);
  }

  return ret;
}

KeyTable::KeyTable(QWidget* parent, QSharedPointer<GpgKeyTableModel> model,
                   GpgKeyTableDisplayMode select_type,
                   GpgKeyTableColumn column_filter,
                   GpgKeyTableProxyModel::KeyFilter filter)
    : QTableView(parent),
      model_(std::move(model)),
      proxy_model_(model_, select_type, column_filter, std::move(filter), this),
      column_filter_(column_filter) {
  setModel(&proxy_model_);
  InitTableStyle();

  // Repaint whenever the row count can have changed, so the empty-state
  // message appears and disappears with the rows it explains.
  for (const auto signal :
       {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved}) {
    connect(&proxy_model_, signal, this, [this]() { viewport()->update(); });
  }
  connect(&proxy_model_, &QAbstractItemModel::modelReset, this,
          [this]() { viewport()->update(); });
  connect(&proxy_model_, &QAbstractItemModel::layoutChanged, this,
          [this]() { viewport()->update(); });

  connect(CommonUtils::GetInstance(), &CommonUtils::SignalFavoritesChanged,
          &proxy_model_, &GpgKeyTableProxyModel::SignalFavoritesChanged);

  connect(this, &KeyTable::SignalColumnTypeChange, this,
          [this](GpgKeyTableColumn global_column_filter) {
            emit proxy_model_.SignalColumnTypeChange(column_filter_ &
                                                     global_column_filter);
          });

  connect(this, &QTableView::doubleClicked, this,
          [this](const QModelIndex& index) {
            if (!index.isValid()) return;

            auto key = GetKeyByIndex(index);
            if (key == nullptr) return;

            CommonUtils::OpenDetailsDialogByKey(
                this, model_->GetGpgContextChannel(), key);
          });

  connect(&proxy_model_, &GpgKeyTableProxyModel::dataChanged, this,
          [this](const QModelIndex&, const QModelIndex&,
                 const QContainer<int>& roles) {
            if (bulk_checking_) return;
            if (!roles.contains(Qt::CheckStateRole)) return;
            emit SignalKeyChecked();
          });
}

void KeyTable::InitTableStyle() {
  verticalHeader()->hide();
  horizontalHeader()->setStretchLastSection(false);
  horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  setShowGrid(false);
  sortByColumn(2, Qt::AscendingOrder);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);

  // table items not editable
  setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Focusable, so arrow keys move the selection and Enter reaches
  // keyPressEvent(). This was Qt::NoFocus, which made the table unreachable
  // from the keyboard entirely.
  setFocusPolicy(Qt::StrongFocus);
  setAlternatingRowColors(true);
  setSortingEnabled(true);
}

void KeyTable::SetFilterKeyword(const QString& keyword) {
  current_keyword_ = keyword;
  proxy_model_.SetSearchKeywords(keyword);

  // Deliberately not clearSelection(): the search box filters on a timer as the
  // user types, and dropping the selection on every batch meant the key you had
  // just picked was gone before you could act on it. The proxy keeps persistent
  // indexes, so a row filtered out and matched again comes back selected.
  emit SignalKeyChecked();
  viewport()->update();
}

auto KeyTable::selected_key_ids() const -> QStringList {
  QStringList ids;
  for (const auto& key : GetSelectedKeys()) {
    if (key != nullptr) ids << key->ID();
  }
  return ids;
}

void KeyTable::restore_selection_by_ids(const QStringList& ids) {
  if (ids.isEmpty()) return;

  auto* selection = selectionModel();
  if (selection == nullptr) return;

  const auto rows = GetRowCount();
  for (int row = 0; row < rows; ++row) {
    auto key = GetKeyByIndex(model()->index(row, 0));
    if (key == nullptr || !ids.contains(key->ID())) continue;

    selection->select(model()->index(row, 0),
                      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
}

void KeyTable::RefreshModel(QSharedPointer<GpgKeyTableModel> model) {
  // A refresh fires on every keyring change, including ones the user did not
  // ask for. Throwing away their sort order and their selection each time made
  // the window feel like it was fighting them.
  const auto sort_column = horizontalHeader()->sortIndicatorSection();
  const auto sort_order = horizontalHeader()->sortIndicatorOrder();
  const auto previous_ids = selected_key_ids();

  clearSelection();

  model_ = std::move(model);
  proxy_model_.ResetGpgKeyTableModel(model_);

  if (sort_initialised_) {
    sortByColumn(sort_column, sort_order);
  } else {
    sortByColumn(2, Qt::AscendingOrder);
    sort_initialised_ = true;
  }

  restore_selection_by_ids(previous_ids);
  resizeColumnsToContents();
  viewport()->update();

  emit SignalKeyChecked();
}

void KeyTable::contextMenuEvent(QContextMenuEvent* event) {
  if (event == nullptr) return;

  // Standard behaviour everywhere else: right-clicking a row you had not
  // selected acts on that row, not on whatever was selected before.
  const auto index = indexAt(event->pos());
  if (index.isValid() && (selectionModel() == nullptr ||
                          !selectionModel()->isRowSelected(index.row(), {}))) {
    selectRow(index.row());
  }

  emit SignalRequestContextMenu(event);
}

void KeyTable::paintEvent(QPaintEvent* event) {
  QTableView::paintEvent(event);

  const auto reason =
      ClassifyKeyTableEmptyState(model() != nullptr ? model()->rowCount() : 0,
                                 model_ != nullptr ? model_->rowCount({}) : 0,
                                 !current_keyword_.isEmpty());
  if (reason == KeyTableEmptyReason::kNotEmpty) return;

  QPainter painter(viewport());
  painter.setPen(palette().color(QPalette::Disabled, QPalette::Text));
  painter.drawText(viewport()->rect().adjusted(24, 24, -24, -24),
                   Qt::AlignCenter | Qt::TextWordWrap,
                   DescribeKeyTableEmptyState(reason, current_keyword_));
}

void KeyTable::keyPressEvent(QKeyEvent* event) {
  if (event != nullptr &&
      (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
    // Matches double-click, which has always opened the details dialog.
    const auto row = GetRowSelected();
    if (row >= 0) {
      auto key = GetKeyByIndex(model()->index(row, 0));
      if (key != nullptr) {
        CommonUtils::OpenDetailsDialogByKey(
            this, model_->GetGpgContextChannel(), key);
      }
      event->accept();
      return;
    }
  }

  QTableView::keyPressEvent(event);
}

auto KeyTable::IsRowChecked(int row) const -> bool {
  if (row < 0 || row >= model()->rowCount()) return false;

  const auto index = model()->index(row, 0);
  if (!index.isValid()) return false;

  return index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
}

auto KeyTable::GetRowCount() const -> int { return model()->rowCount(); }

auto KeyTable::GetKeyByIndex(QModelIndex index) const -> GpgAbstractKeyPtr {
  if (!index.isValid()) return nullptr;

  const auto source_index = proxy_model_.mapToSource(index);
  if (!source_index.isValid()) return nullptr;

  auto* item = static_cast<GpgKeyTableItem*>(source_index.internalPointer());
  if (item == nullptr) return nullptr;

  return item->SharedKey();
}

auto KeyTable::GetSelectedKeys() const -> GpgAbstractKeyPtrList {
  GpgAbstractKeyPtrList ret;

  const auto* select = selectionModel();
  if (select == nullptr) return ret;

  for (const auto& index : select->selectedRows()) {
    auto key = GetKeyByIndex(index);
    if (key == nullptr) continue;

    ret.push_back(key);
  }

  return ret;
}

void KeyTable::SetRowChecked(int row) const {
  if (row < 0 || row >= model()->rowCount()) return;
  model()->setData(model()->index(row, 0), Qt::Checked, Qt::CheckStateRole);
}

void KeyTable::CheckAll() {
  bulk_checking_ = true;

  for (int row = 0; row < model()->rowCount(); ++row) {
    const auto index = model()->index(row, 0);
    if (!index.isValid()) continue;

    model()->setData(index, Qt::Checked, Qt::CheckStateRole);
  }

  bulk_checking_ = false;
  emit SignalKeyChecked();
}

void KeyTable::UncheckAll() {
  bulk_checking_ = true;

  for (int row = 0; row < model()->rowCount(); ++row) {
    const auto index = model()->index(row, 0);
    if (!index.isValid()) continue;

    model()->setData(index, Qt::Unchecked, Qt::CheckStateRole);
  }

  bulk_checking_ = false;
  emit SignalKeyChecked();
}

[[nodiscard]] auto KeyTable::GetRowSelected() const -> int {
  auto selected_indexes = selectedIndexes();
  if (selected_indexes.empty()) return -1;

  return selected_indexes.first().row();
}

void KeyTable::SetFilter(const GpgKeyTableProxyModel::KeyFilter& filter) {
  proxy_model_.SetFilter(filter);
}

void KeyTable::RefreshProxyModel() { proxy_model_.invalidate(); }
}  // namespace GpgFrontend::UI