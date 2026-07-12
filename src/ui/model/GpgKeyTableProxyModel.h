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

#include <QFont>
#include <QFontMetrics>

#include "core/model/GpgKeyTableModel.h"

namespace GpgFrontend::UI {

class GpgKeyTableProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
 public:
  using KeyFilter = std::function<bool(const GpgAbstractKey *)>;

  explicit GpgKeyTableProxyModel(QSharedPointer<GpgKeyTableModel> model,
                                 GpgKeyTableDisplayMode display_mode,
                                 GpgKeyTableColumn columns, KeyFilter filter,
                                 QObject *parent);

  void SetSearchKeywords(const QString &keywords);

  void SetFilter(const KeyFilter &filter);

  /**
   * @brief Restrict the view to members of the given key category.
   *
   * An empty id disables category filtering.
   *
   * @param category_id category identifier, or empty to show all
   */
  void SetCategoryFilter(const QString &category_id);

  void ResetGpgKeyTableModel(QSharedPointer<GpgKeyTableModel> model);

  /**
   * @brief Map a visible (proxy) column to its underlying source column.
   *
   * Row-independent, so it is valid even when the model is empty.
   *
   * @param visible_column proxy column index
   * @return the source column index, or -1 if out of range
   */
  [[nodiscard]] auto SourceColumnForVisibleColumn(int visible_column) const
      -> int;

 protected:
  [[nodiscard]] auto filterAcceptsRow(int sourceRow,
                                      const QModelIndex &sourceParent) const
      -> bool override;

  [[nodiscard]] auto filterAcceptsColumn(int sourceColumn,
                                         const QModelIndex &sourceParent) const
      -> bool override;

 signals:

  /**
   * @brief Request a refresh of the cached category membership.
   */
  void SignalCategoriesRefresh();

  /**
   * @brief
   *
   */
  void SignalColumnTypeChange(GpgKeyTableColumn);

 private slots:

  /**
   * @brief Refresh the cached category membership and re-filter.
   */
  void slot_refresh_categories();

  /**
   * @brief
   *
   */
  void slot_update_column_type(GpgKeyTableColumn);

  /**
   * @brief Reload the active category's member ids from the repository.
   *
   */
  void refresh_category_cache();

 private:
  QSharedPointer<GpgKeyTableModel> model_;
  GpgKeyTableDisplayMode display_mode_;
  GpgKeyTableColumn filter_columns_;
  QString filter_keywords_;
  QString category_id_;
  QSet<QString> category_key_ids_;
  KeyFilter custom_filter_;

  QFont default_font_;
  QFontMetrics default_metrics_;
};

}  // namespace GpgFrontend::UI