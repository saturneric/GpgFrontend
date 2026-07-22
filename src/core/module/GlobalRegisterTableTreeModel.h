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

#include "core/module/GlobalRegisterTable.h"

namespace GpgFrontend::Module {

/**
 * @brief Extra roles exposed by GlobalRegisterTableTreeModel.
 */
enum GRTItemRole {
  kGRTPathRole = Qt::UserRole + 1,  ///< full dotted key path of the node
  kGRTValueRole,                    ///< rendered value, empty for namespaces
};

/**
 * @brief QAbstractItemModel adaptor that exposes a GlobalRegisterTable as a
 * tree.
 *
 * Provides a read-only view of the register table's namespace/key hierarchy
 * suitable for display in a QTreeView. Each tree node corresponds to a
 * GlobalRegisterTable node (namespace, key, or value).
 */
class GF_CORE_EXPORT GlobalRegisterTableTreeModel : public QAbstractItemModel {
 public:
  /**
   * @brief Construct the model backed by @p grt.
   *
   * @param grt pointer to the GlobalRegisterTable to display; must outlive this
   * model
   * @param parent optional parent QObject
   */
  explicit GlobalRegisterTableTreeModel(GlobalRegisterTable* grt,
                                        QObject* parent);

  ~GlobalRegisterTableTreeModel() override;

  /**
   * @brief Rebuild the displayed snapshot from the live register table.
   *
   * Called automatically (coalesced) whenever a module publishes a value.
   */
  void Refresh();

  /**
   * @brief Return the number of child rows under @p parent.
   *
   * @param parent parent index (invalid index = root)
   * @return number of child nodes
   */
  [[nodiscard]] auto rowCount(const QModelIndex& parent) const -> int override;

  /**
   * @brief Return the number of columns (fixed: namespace, key, value, type).
   *
   * @param parent unused
   * @return column count
   */
  [[nodiscard]] auto columnCount(const QModelIndex& parent) const
      -> int override;

  /**
   * @brief Return the data for the given index and role.
   *
   * @param index model index to query
   * @param role Qt item data role
   * @return the data value, or an invalid QVariant if not applicable
   */
  [[nodiscard]] auto data(const QModelIndex& index, int role) const
      -> QVariant override;

  /**
   * @brief Return the model index for the item at (@p row, @p column) under @p
   * parent.
   *
   * @param row row within the parent
   * @param column column index
   * @param parent parent model index
   * @return model index for the requested item
   */
  [[nodiscard]] auto index(int row, int column, const QModelIndex& parent) const
      -> QModelIndex override;

  /**
   * @brief Return the parent index of the given @p index.
   *
   * @param index child model index
   * @return parent model index, or an invalid index if @p index is a root node
   */
  [[nodiscard]] auto parent(const QModelIndex& index) const
      -> QModelIndex override;

  /**
   * @brief Return the header label for the given section and orientation.
   *
   * @param section column or row number
   * @param orientation Qt::Horizontal for column headers
   * @param role Qt item data role
   * @return header label, or an invalid QVariant if not applicable
   */
  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

};  // namespace GpgFrontend::Module
