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
          [](const GpgAbstractKey*) -> bool { return true; });

  /**
   * @brief Apply the table's view settings.
   *
   */
  void InitTableStyle();

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
  void UncheckALL() const;

  /**
   * @brief
   *
   */
  void CheckALL() const;

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
   * @brief The user asked for the context menu, on a row or on empty space.
   * A row under the cursor that was not selected has been selected first.
   *
   * @param event
   */
  void SignalRequestContextMenu(QContextMenuEvent* event);

 protected:
  /**
   * @brief Select the row under the cursor, then ask for the context menu.
   *
   * @param event
   */
  void contextMenuEvent(QContextMenuEvent* event) override;

  /**
   * @brief Draw a message explaining why the table is empty.
   *
   * @param event
   */
  void paintEvent(QPaintEvent* event) override;

  /**
   * @brief Enter opens the details of the selected key.
   *
   * @param event
   */
  void keyPressEvent(QKeyEvent* event) override;

 private:
  QSharedPointer<GpgKeyTableModel> model_;
  GpgKeyTableProxyModel proxy_model_;
  GpgKeyTableColumn column_filter_;
  bool bulk_checking_ = false;
  bool sort_initialised_ = false;
  QString current_keyword_;

  /**
   * @brief IDs of the selected keys, to restore them after a refresh.
   */
  [[nodiscard]] auto selected_key_ids() const -> QStringList;

  /**
   * @brief Select the rows of the given key IDs.
   */
  void restore_selection_by_ids(const QStringList& ids);
};

}  // namespace GpgFrontend::UI