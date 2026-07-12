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
                   GpgKeyTableProxyModel::KeyFilter filter,
                   const QString& category_id)
    : QTableView(parent),
      model_(std::move(model)),
      proxy_model_(model_, select_type, column_filter, std::move(filter), this),
      column_filter_(column_filter) {
  setModel(&proxy_model_);
  proxy_model_.SetCategoryFilter(category_id);
  init_table_style();

  connect(CommonUtils::GetInstance(), &CommonUtils::SignalCategoriesChanged,
          &proxy_model_, &GpgKeyTableProxyModel::SignalCategoriesRefresh);

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

void KeyTable::init_table_style() {
  setProperty("gfKeyTable", true);

  verticalHeader()->hide();

  horizontalHeader()->setStretchLastSection(false);
  horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  horizontalHeader()->setStretchLastSection(true);
  horizontalHeader()->setHighlightSections(false);
  horizontalHeader()->setSectionsClickable(true);
  horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  setShowGrid(false);
  setAlternatingRowColors(true);
  setSortingEnabled(true);
  sortByColumn(2, Qt::AscendingOrder);

  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setEditTriggers(QAbstractItemView::NoEditTriggers);

  setFocusPolicy(Qt::NoFocus);
  setMouseTracking(true);
  setWordWrap(false);
  setTextElideMode(Qt::ElideRight);

  setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  verticalHeader()->setDefaultSectionSize(28);

  setStyleSheet(R"(
QTableView[gfKeyTable="true"] {
  outline: 0;
}

QTableView[gfKeyTable="true"]::item {
  min-height: 24px;
  padding: 2px 5px;
  border: none;
}

QTableView[gfKeyTable="true"]::item:selected {
  background: palette(highlight);
  color: palette(highlighted-text);
}

QTableView[gfKeyTable="true"] QHeaderView::section {
  padding: 4px 6px;
  border: none;
}

QTableView[gfKeyTable="true"] QTableCornerButton::section {
  border: none;
}
)");

  style()->unpolish(this);
  style()->polish(this);
}

void KeyTable::SetFilterKeyword(const QString& keyword) {
  proxy_model_.SetSearchKeywords(keyword);
  clearSelection();
  emit SignalKeyChecked();
}

void KeyTable::RefreshModel(QSharedPointer<GpgKeyTableModel> model) {
  clearSelection();

  model_ = std::move(model);
  proxy_model_.ResetGpgKeyTableModel(model_);

  sortByColumn(2, Qt::AscendingOrder);
  resizeColumnsToContents();

  emit SignalKeyChecked();
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