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
#include "model/GpgKeyTreeProxyModel.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyTreeView : public QTreeView {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Table object
   *
   * @param _key_list
   * @param _select_type
   * @param _info_type
   * @param _filter
   */
  explicit KeyTreeView(QWidget* parent = nullptr);

  /**
   * @brief Construct a new Key Table object
   *
   * @param _key_list
   * @param _select_type
   * @param _info_type
   * @param _filter
   */
  explicit KeyTreeView(int channel,
                       GpgKeyTreeModel::Detector checkable_detector,
                       GpgKeyTreeProxyModel::KeyFilter filter,
                       QWidget* parent = nullptr);

  /**
   * @brief Get the Key By Index object
   *
   * @param index
   * @return GpgAbstractKeyPtr
   */
  static auto GetKeyByIndex(QModelIndex index) -> GpgAbstractKeyPtr;

  /**
   * @brief Get the All Checked Key Ids object
   *
   * @return KeyIdArgsList
   */
  auto GetAllCheckedKeyIds() -> KeyIdArgsList;

  /**
   * @brief Get the All Checked Sub Key object
   *
   * @return QContainer<GpgSubKey>
   */
  auto GetAllCheckedSubKey() -> QContainer<GpgSubKey>;

  /**
   * @brief Set the Key Filter object
   *
   * @param filter
   */
  void SetKeyFilter(const GpgKeyTreeProxyModel::KeyFilter& filter);

  /**
   * @brief
   *
   * @param channel
   */
  void SetChannel(int channel);

 protected:
  /**
   * @brief
   *
   */
  void paintEvent(QPaintEvent* event) override;

 private:
  bool init_;
  int channel_;
  QSharedPointer<GpgKeyTreeModel> model_;  ///<
  GpgKeyTreeProxyModel proxy_model_;

  void slot_adjust_column_widths();

  void init();
};

}  // namespace GpgFrontend::UI