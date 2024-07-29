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

#include "core/function/gpg/GpgContext.h"
#include "core/model/GpgKey.h"
#include "core/typedef/GpgTypedef.h"
#include "ui/GpgFrontendUI.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {
class KeyNewUIDDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key New U I D Dialog object
   *
   * @param key
   * @param parent
   */
  KeyNewUIDDialog(const KeyId& key, QWidget* parent = nullptr);

 signals:
  /**
   * @brief
   *
   */
  void SignalUIDCreated();

 private slots:

  /**
   * @brief
   *
   */
  void slot_create_new_uid();

 private:
  GpgKey m_key_;  ///<

  QLineEdit* name_{};     ///<
  QLineEdit* email_{};    ///<
  QLineEdit* comment_{};  ///<

  QPushButton* create_button_{};  ///<

  QStringList error_messages_;  ///<
  QLabel* error_label_{};       ///<

  QRegularExpression re_email_{
      R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};

  /**
   * @brief
   *
   * @param str
   * @return true
   * @return false
   */
  bool check_email_address(const QString& str);
};
}  // namespace GpgFrontend::UI
