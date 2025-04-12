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

#include "core/model/GpgKeyTreeModel.h"

namespace GpgFrontend::UI {

class GpgKeyTreeProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
 public:
  using KeyFilter = std::function<bool(const GpgAbstractKey *)>;

  explicit GpgKeyTreeProxyModel(QSharedPointer<GpgKeyTreeModel> model,
                                GpgKeyTreeDisplayMode display_mode,
                                KeyFilter filter, QObject *parent);

  void SetSearchKeywords(const QString &keywords);

  void ResetGpgKeyTableModel(QSharedPointer<GpgKeyTreeModel> model);

 protected:
  [[nodiscard]] auto filterAcceptsRow(
      int sourceRow, const QModelIndex &sourceParent) const -> bool override;

 signals:

  /**
   * @brief
   *
   */
  void SignalFavoritesChanged();

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
  void slot_update_favorites_cache();

 private:
  QSharedPointer<GpgKeyTreeModel> model_;
  GpgKeyTreeDisplayMode display_mode_;
  QString filter_keywords_;
  QStringList favorite_key_ids_;
  KeyFilter custom_filter_;
};

}  // namespace GpgFrontend::UI