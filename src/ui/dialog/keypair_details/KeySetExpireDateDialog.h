/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "core/GpgModel.h"
#include "core/function/gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"
#include "ui/dialog/GeneralDialog.h"

class Ui_ModifiedExpirationDateTime;

namespace GpgFrontend::UI {

class KeySetExpireDateDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Set Expire Date Dialog object
   *
   * @param key_id
   * @param parent
   */
  explicit KeySetExpireDateDialog(const KeyId& key_id,
                                  QWidget* parent = nullptr);

  /**
   * @brief Construct a new Key Set Expire Date Dialog object
   *
   * @param key_id
   * @param subkey_fpr
   * @param parent
   */
  explicit KeySetExpireDateDialog(const KeyId& key_id, std::string subkey_fpr,
                                  QWidget* parent = nullptr);

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyExpireDateUpdated();

 private:
  /**
   * @brief
   *
   */
  void init();

  std::shared_ptr<Ui_ModifiedExpirationDateTime> ui_;  ///<
  const GpgKey m_key_;                                 ///<
  const SubkeyId m_subkey_;                            ///<

 private slots:
  /**
   * @brief
   *
   */
  void slot_confirm();

  /**
   * @brief
   *
   * @param state
   */
  void slot_non_expired_checked(int state);
};

}  // namespace GpgFrontend::UI
