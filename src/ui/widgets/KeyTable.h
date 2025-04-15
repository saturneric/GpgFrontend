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

 private:
  QSharedPointer<GpgKeyTableModel> model_;
  GpgKeyTableProxyModel proxy_model_;
  GpgKeyTableColumn column_filter_;
};

}  // namespace GpgFrontend::UI