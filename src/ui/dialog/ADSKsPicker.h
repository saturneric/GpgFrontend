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

#include "core/typedef/GpgTypedef.h"
#include "ui/dialog//GeneralDialog.h"
#include "ui/model/GpgKeyTreeProxyModel.h"

namespace GpgFrontend::UI {

class KeyTreeView;

/**
 * @brief
 *
 */
class ADSKsPicker : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Signers Picker object
   *
   * @param parent
   */
  explicit ADSKsPicker(int channel, GpgKeyPtr key,
                       const GpgKeyTreeProxyModel::KeyFilter& filter,
                       QWidget* parent = nullptr);

 private slots:
  /**
   * @brief
   *
   * @param s_keys
   */
  void slot_add_adsk(const QContainer<GpgSubKey>& s_keys);

 private:
  KeyTreeView* tree_view_;  ///<
  bool accepted_ = false;
  int channel_;
  GpgKeyPtr key_;
};

}  // namespace GpgFrontend::UI
