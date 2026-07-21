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

#pragma once

#include "core/model/GpgKey.h"
#include "core/model/GpgKeyTableModel.h"
#include "ui/model/GpgKeyTableProxyModel.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
struct KeyTable : public QTableView {
  Q_OBJECT
 public:
  using KeyTableFilter = std::function<bool(const GpgKey&, const KeyTable&)>;

  /**
   * @brief Construct a new Key Table object
   *
   * @param _key_list
   * @param _select_type
   * @param _info_type
   * @param _filter
   */
  KeyTable(
      QWidget* parent, QSharedPointer<GpgKeyTableModel> model,
      GpgKeyTableDisplayMode _select_type, GpgKeyTableColumn _info_type,
      GpgKeyTableProxyModel::KeyFilter _filter =
          [](const GpgAbstractKey*) -> bool { return true; },
      const QString& category_id = {});

  /**
   * @brief
   *
   * @param model
   */
  void RefreshModel(QSharedPointer<GpgKeyTableModel> model);

  /**
   * @brief Get the Checked object
   *
   * @return KeyIdArgsListPtr&
   */
  [[nodiscard]] auto GetCheckedKeys() const -> GpgAbstractKeyPtrList;

  /**
   * @brief
   *
   */
  void SetFilterKeyword(const QString& keyword);

  /**
   * @brief
   *
   * @param row
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsRowChecked(int row) const -> bool;

  /**
   * @brief Set the Row Checked object
   *
   * @param row
   */
  void SetRowChecked(int row) const;

  /**
   * @brief Set the Row Checked object
   *
   * @param row
   */
  [[nodiscard]] auto GetRowSelected() const -> int;

  /**
   * @brief Get the Row Count object
   *
   * @return auto
   */
  [[nodiscard]] auto GetRowCount() const -> int;

  /**
   * @brief
   *
   * @param index
   * @return GpgAbstractKeyPtr
   */
  [[nodiscard]] auto GetKeyByIndex(QModelIndex index) const
      -> GpgAbstractKeyPtr;

  /**
   * @brief Get the Selected Keys object
   *
   * @param index
   * @return GpgAbstractKeyPtrList
   */
  [[nodiscard]] auto GetSelectedKeys() const -> GpgAbstractKeyPtrList;

  /**
   * @brief
   *
   */
  void CheckAll();

  /**
   * @brief
   *
   */
  void UncheckAll();

  /**
   * @brief
   *
   */
  void SetFilter(const GpgKeyTableProxyModel::KeyFilter&);

  /**
   * @brief
   *
   */
  void RefreshProxyModel();

  /**
   * @brief Set the settings key under which this table's column widths are
   * persisted, then reload and apply them.
   *
   * Widths are stored per host window (Key Management, main-window dock, ...)
   * and shared by every category tab of that window.
   *
   * @param settings_key group key, e.g. "keys/keymgmt_column_widths"
   */
  void SetColumnWidthsSettingsKey(const QString& settings_key);

  /**
   * @brief Re-read the persisted column widths and apply them. Used to keep
   * sibling tabs of the same key list in sync after a resize.
   */
  void ReloadColumnWidths();

  /**
   * @brief Discard the persisted column widths and fall back to the automatic
   * layout.
   */
  void ResetColumnWidths();

 signals:

  /**
   * @brief
   *
   */
  void SignalColumnTypeChange(GpgKeyTableColumn);

  /**
   * @brief
   *
   */
  void SignalGpgContextChannelChange(int);

  /**
   * @brief
   *
   */
  void SignalKeyChecked();

  /**
   * @brief Emitted after the user drags a section divider and the new width is
   * persisted.
   */
  void SignalColumnWidthChanged();

 protected:
  void showEvent(QShowEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  QSharedPointer<GpgKeyTableModel> model_;
  GpgKeyTableProxyModel proxy_model_;
  GpgKeyTableColumn column_filter_;
  bool bulk_checking_ = false;

  ///< Settings group holding this table's column widths.
  QString column_widths_settings_key_ = "keys/global_column_widths";

  ///< Persisted widths, keyed by *source* column index. The proxy hides columns
  ///< dynamically, so visible indices are not stable across column toggles.
  QHash<int, int> saved_widths_;

  ///< True while apply_column_sizing() drives the header, so its own
  ///< resizeSection() calls are not mistaken for user drags.
  bool applying_sizing_ = false;

  /**
   * @brief Construct a new Init Table Style object
   *
   */
  void init_table_style();

  /**
   * @brief Apply per-column sizing so long identity columns elide and share the
   * available width instead of expanding to fit and pushing the short,
   * decision-relevant columns (Type, Usage) off-screen.
   *
   * Every column is Interactive so the user can drag any divider. Widths the
   * user has chosen (saved_widths_) win; the rest fall back to the automatic
   * layout: Name / Email / Comment share the leftover width (and elide with an
   * ellipsis), every other visible column sizes to its content. Re-applied
   * whenever the visible column set or the model changes.
   */
  void apply_column_sizing();

  /**
   * @brief Cheap counterpart of apply_column_sizing() used on the resize path:
   * shares the leftover width out among the stretch columns while leaving the
   * content-fit columns at their current width, avoiding a full row scan on
   * every frame of a window drag-resize.
   */
  void redistribute_stretch_columns();

  /**
   * @brief Load saved_widths_ from the settings group.
   */
  void load_column_widths();

  /**
   * @brief Persist one column's width, keyed by its source column index.
   */
  void save_column_width(int source_column, int width);
};

}  // namespace GpgFrontend::UI