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

#include "core/model/GpgKeyGroup.h"
#include "ui/dialog//GeneralDialog.h"

class Ui_KeyGroupManageDialog;

namespace GpgFrontend::UI {

class KeyList;

/**
 * @brief
 *
 */
class KeyGroupManageDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Signers Picker object
   *
   * @param parent
   */
  explicit KeyGroupManageDialog(int channel,
                                const QSharedPointer<GpgKeyGroup>& key_group,
                                QWidget* parent = nullptr);

 private slots:

  /**
   * @brief
   *
   */
  void slot_add_to_key_group();

  /**
   * @brief
   *
   */
  void slot_remove_from_key_group();

  /**
   * @brief
   *
   */
  void slot_notify_invalid_key_ids();

  /**
   * @brief
   *
   */
  void slot_set_add_button_state();

  /**
   * @brief
   *
   */
  void slot_set_remove_button_state();

 private:
  QSharedPointer<Ui_KeyGroupManageDialog> ui_;  ///<
  int channel_;
  QSharedPointer<GpgKeyGroup> key_group_;
};

}  // namespace GpgFrontend::UI
