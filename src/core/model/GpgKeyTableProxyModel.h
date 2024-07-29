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

#include "core/model/GpgKeyTableModel.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT GpgKeyTableProxyModel
    : public QSortFilterProxyModel {
  Q_OBJECT
 public:
  using KeyFilter = std::function<bool(const GpgKey &)>;

  explicit GpgKeyTableProxyModel(QSharedPointer<GpgKeyTableModel> model,
                                 GpgKeyTableDisplayMode display_mode,
                                 GpgKeyTableColumn columns, KeyFilter filter,
                                 QObject *parent);

  void SetSearchKeywords(const QString &keywords);

 protected:
  [[nodiscard]] auto filterAcceptsRow(
      int sourceRow, const QModelIndex &sourceParent) const -> bool override;

  [[nodiscard]] auto filterAcceptsColumn(
      int sourceColumn, const QModelIndex &sourceParent) const -> bool override;

 signals:

  /**
   * @brief
   *
   */
  void SignalFavoritesChanged();

  /**
   * @brief
   *
   */
  void SignalColumnTypeChange(GpgKeyTableColumn);

 private slots:

  /**
   * @brief
   *
   */
  void slot_update_favorites();

  /**
   * @brief
   *
   */
  void slot_update_column_type(GpgKeyTableColumn);

 private:
  QSharedPointer<GpgKeyTableModel> model_;
  GpgKeyTableDisplayMode display_mode_;
  GpgKeyTableColumn filter_columns_;
  QString filter_keywords_;
  QList<QString> favorite_fingerprints_;
  KeyFilter custom_filter_;
};

}  // namespace GpgFrontend