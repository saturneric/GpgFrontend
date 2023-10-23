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

#include "core/GpgContext.h"
#include "ui/GpgFrontendUI.h"
#include "ui/dialog/GeneralDialog.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

class KeyUIDSignDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key U I D Sign Dialog object
   *
   * @param key
   * @param uid
   * @param parent
   */
  explicit KeyUIDSignDialog(const GpgKey& key, UIDArgsListPtr uid,
                            QWidget* parent = nullptr);

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyUIDSignUpdate();

 private:
  KeyList* m_key_list_;           ///<
  QPushButton* sign_key_button_;  ///<
  QDateTimeEdit* expires_edit_;   ///<
  QCheckBox* non_expire_check_;   ///<
  UIDArgsListPtr m_uids_;         ///<

  const GpgKey& m_key_;  ///<

 private slots:
  /**
   * @brief
   *
   * @param clicked
   */
  void slot_sign_key(bool clicked);
};

}  // namespace GpgFrontend::UI
