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

#include "core/function/CacheManager.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/widgets/KeyTableEmptyState.h"

namespace GpgFrontend::UI {

namespace {

/**
 * @brief Durable-cache key holding the column widths of one host window.
 *
 * Widths are stored per source-column index, so the key is versioned: inserting
 * a column shifts every index after it and stale widths would land on the wrong
 * columns. Bump the suffix whenever the source column order changes.
 */
auto ColumnWidthsCacheKey(const QString& scope) -> QString {
  return QString("key_table_column_widths_v2:%1").arg(scope);
}

/**
 * @brief Source columns whose text can be long: Name (2), Email (3),
 * Comment (11).
 *
 * These share the leftover width and elide; every other visible column sizes to
 * its content. Shared by the full sizing pass and the cheap resize pass, which
 * have to agree — when they were two separate literals they silently drifted
 * apart and the resize path stretched Subkeys instead of Comment.
 */
auto StretchSourceColumns() -> const QSet<int>& {
  static const QSet<int> kColumns = {2, 3, 11};
  return kColumns;
}

constexpr int kStretchMinWidth = 80;

}  // namespace

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

auto KeyTable::HasCheckedKeys() const -> bool {
  for (int row = 0; row < GetRowCount(); ++row) {
    if (IsRowChecked(row)) return true;
  }

  return false;
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
  has_category_filter_ = !category_id.isEmpty();
  load_column_widths();
  init_table_style();

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

  connect(horizontalHeader(), &QHeaderView::sectionResized, this,
          [this](int visible_col, int /*old_size*/, int new_size) {
            if (applying_sizing_) return;

            const auto source_col =
                proxy_model_.SourceColumnForVisibleColumn(visible_col);
            if (source_col < 0) return;

            saved_widths_[source_col] = new_size;
            // A drag emits a resize per pixel, so leave the entry dirty and let
            // the cache manager's periodic flush write it out.
            save_column_widths();
            emit SignalColumnWidthChanged();
          });

  connect(CommonUtils::GetInstance(), &CommonUtils::SignalCategoriesChanged,
          &proxy_model_, &GpgKeyTableProxyModel::SignalCategoriesRefresh);

  connect(this, &KeyTable::SignalColumnTypeChange, this,
          [this](GpgKeyTableColumn global_column_filter) {
            emit proxy_model_.SignalColumnTypeChange(column_filter_ &
                                                     global_column_filter);
            apply_column_sizing();
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

  // Survives a refresh: RefreshModel() goes through ResetGpgKeyTableModel() ->
  // setSourceModel(), not setModel(), so the view keeps this selection model
  // and this connection. Anything that starts calling setModel() here has to
  // reconnect, and the failure would be silent — the per-key actions would
  // simply stop responding to the selection.
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          [this]() { emit SignalSelectionChanged(); });
}

void KeyTable::init_table_style() {
  setProperty("gfKeyTable", true);

  verticalHeader()->hide();

  horizontalHeader()->setStretchLastSection(false);
  // Per-section modes are assigned in apply_column_sizing(); Interactive is the
  // baseline so every divider is draggable.
  horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  horizontalHeader()->setHighlightSections(false);
  horizontalHeader()->setSectionsClickable(true);
  horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  setShowGrid(false);
  setAlternatingRowColors(true);
  setSortingEnabled(true);
  apply_default_sort();

  setSelectionBehavior(QAbstractItemView::SelectRows);
  // Ctrl/Shift-click selects several keys at once. The checkboxes stay for
  // building a set across category tabs and across refreshes; the selection is
  // for acting on what is in front of you right now.
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Focusable, so arrow keys move the selection and Enter/Space/Delete reach
  // keyPressEvent(). This was Qt::NoFocus, which made the table unreachable
  // from the keyboard entirely.
  setFocusPolicy(Qt::StrongFocus);
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
  padding: 2px 3px;
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

  apply_column_sizing();
}

void KeyTable::apply_column_sizing() {
  auto* header = horizontalHeader();
  if (header == nullptr) return;

  const auto columns = proxy_model_.columnCount({});
  if (columns <= 0) return;

  const QScopedValueRollback<bool> guard(applying_sizing_, true);

  header->setStretchLastSection(false);

  // Content-fit widths first; the stretch columns get their share of whatever
  // room is left over afterwards.
  QVector<int> widths(columns, 0);
  QVector<int> stretch_cols;
  int fixed_total = 0;

  for (int col = 0; col < columns; ++col) {
    const auto source_col = proxy_model_.SourceColumnForVisibleColumn(col);

    // The checkbox column is not a meaningful thing to drag.
    header->setSectionResizeMode(
        col, source_col == 0 ? QHeaderView::Fixed : QHeaderView::Interactive);

    if (const auto saved = saved_widths_.constFind(source_col);
        saved != saved_widths_.constEnd() && *saved > 0) {
      widths[col] = *saved;
      fixed_total += widths[col];
      continue;
    }

    if (source_col > 0 && StretchSourceColumns().contains(source_col)) {
      stretch_cols.push_back(col);
      continue;
    }

    widths[col] =
        qMax(sizeHintForColumn(col), header->sectionSizeHint(col)) + 1;
    fixed_total += widths[col];
  }

  if (!stretch_cols.isEmpty()) {
    const auto available = viewport()->width() - fixed_total;
    const auto share = static_cast<int>(available / stretch_cols.size());
    for (const auto col : stretch_cols) {
      widths[col] = qMax(share, kStretchMinWidth);
    }
  }

  for (int col = 0; col < columns; ++col) {
    if (widths[col] > 0) header->resizeSection(col, widths[col]);
  }
}

void KeyTable::redistribute_stretch_columns() {
  auto* header = horizontalHeader();
  if (header == nullptr) return;

  const auto columns = proxy_model_.columnCount({});
  if (columns <= 0) return;

  // Cheap counterpart of apply_column_sizing() for the resize path: the
  // content-fit columns keep the width they already have, so no row scan is
  // needed; only the leftover space is shared out again.
  QVector<int> stretch_cols;
  int fixed_total = 0;

  for (int col = 0; col < columns; ++col) {
    const auto source_col = proxy_model_.SourceColumnForVisibleColumn(col);
    if (source_col > 0 && StretchSourceColumns().contains(source_col)) {
      stretch_cols.push_back(col);
      continue;
    }
    fixed_total += header->sectionSize(col);
  }

  if (stretch_cols.isEmpty()) return;

  const QScopedValueRollback<bool> guard(applying_sizing_, true);

  const auto available = viewport()->width() - fixed_total;
  const auto share = static_cast<int>(available / stretch_cols.size());
  for (const auto col : stretch_cols) {
    header->resizeSection(col, qMax(share, kStretchMinWidth));
  }
}

void KeyTable::load_column_widths() {
  saved_widths_.clear();

  const auto object =
      CacheManager::GetInstance()
          .LoadDurableCache(ColumnWidthsCacheKey(column_widths_scope_))
          .object();

  for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
    bool ok = false;
    const auto source_col = it.key().toInt(&ok);
    if (!ok) continue;

    const auto width = it.value().toInt();
    if (width <= 0) continue;

    saved_widths_.insert(source_col, width);
  }
}

void KeyTable::save_column_widths(bool flush) {
  QJsonObject object;
  for (auto it = saved_widths_.constBegin(); it != saved_widths_.constEnd();
       ++it) {
    object.insert(QString::number(it.key()), it.value());
  }

  CacheManager::GetInstance().SaveDurableCache(
      ColumnWidthsCacheKey(column_widths_scope_), QJsonDocument(object), flush);
}

void KeyTable::SetColumnWidthsScope(const QString& scope) {
  if (scope.isEmpty() || scope == column_widths_scope_) return;

  column_widths_scope_ = scope;
  ReloadColumnWidths();
}

void KeyTable::ReloadColumnWidths() {
  load_column_widths();
  apply_column_sizing();
}

void KeyTable::ResetColumnWidths() {
  saved_widths_.clear();
  save_column_widths(true);
  apply_column_sizing();
}

void KeyTable::showEvent(QShowEvent* event) {
  QTableView::showEvent(event);

  // The viewport width is not usable before the first show, so the automatic
  // layout can only be computed here.
  if (saved_widths_.isEmpty()) apply_column_sizing();
}

void KeyTable::resizeEvent(QResizeEvent* event) {
  QTableView::resizeEvent(event);

  // Keep redistributing the leftover width until the user takes control; once
  // they have dragged a divider the layout is theirs.
  if (saved_widths_.isEmpty()) redistribute_stretch_columns();
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

void KeyTable::apply_default_sort() {
  // Deliberately mapped from the *source* column: the Type column can be
  // hidden from the Columns menu, and visible index 2 then lands on Email, so
  // the default sort would silently change with an unrelated setting.
  constexpr int kNameSourceColumn = 2;
  const auto visible =
      proxy_model_.VisibleColumnForSourceColumn(kNameSourceColumn);
  if (visible >= 0) sortByColumn(visible, Qt::AscendingOrder);
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
    apply_default_sort();
    sort_initialised_ = true;
  }

  restore_selection_by_ids(previous_ids);
  apply_column_sizing();
  viewport()->update();

  emit SignalKeyChecked();
  emit SignalSelectionChanged();
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

void KeyTable::ToggleSelectedRowsChecked() {
  const auto rows = selectionModel() != nullptr
                        ? selectionModel()->selectedRows()
                        : QModelIndexList{};
  if (rows.isEmpty()) return;

  // One decision for the whole selection: if anything in it is unchecked,
  // Space checks all of it. Toggling each row independently would scatter the
  // check marks and give the user nothing they asked for.
  const bool target = std::any_of(
      rows.cbegin(), rows.cend(),
      [this](const QModelIndex& index) { return !IsRowChecked(index.row()); });

  bulk_checking_ = true;
  for (const auto& index : rows) {
    const auto check_index = model()->index(index.row(), 0);
    if (!check_index.isValid()) continue;

    model()->setData(check_index, target ? Qt::Checked : Qt::Unchecked,
                     Qt::CheckStateRole);
  }
  bulk_checking_ = false;

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

  // Emitted even with nothing selected: right-clicking empty space is exactly
  // when an empty keyring needs to offer Generate and Import.
  emit SignalRequestContextMenu(event);
}

void KeyTable::paintEvent(QPaintEvent* event) {
  QTableView::paintEvent(event);

  const auto reason = ClassifyKeyTableEmptyState(
      model() != nullptr ? model()->rowCount() : 0,
      model_ != nullptr ? model_->rowCount({}) : 0, !current_keyword_.isEmpty(),
      has_category_filter_);
  if (reason == KeyTableEmptyReason::kNotEmpty) return;

  QPainter painter(viewport());
  painter.setPen(palette().color(QPalette::Disabled, QPalette::Text));
  painter.drawText(viewport()->rect().adjusted(24, 24, -24, -24),
                   Qt::AlignCenter | Qt::TextWordWrap,
                   DescribeKeyTableEmptyState(reason, current_keyword_));
}

void KeyTable::keyPressEvent(QKeyEvent* event) {
  if (event == nullptr) {
    QTableView::keyPressEvent(event);
    return;
  }

  switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
      // Matches double-click, which has always opened the details dialog.
      if (GetRowSelected() >= 0) {
        emit SignalRequestShowDetails();
        event->accept();
        return;
      }
      break;
    case Qt::Key_Space:
      ToggleSelectedRowsChecked();
      event->accept();
      return;
    default:
      break;
  }

  QTableView::keyPressEvent(event);
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