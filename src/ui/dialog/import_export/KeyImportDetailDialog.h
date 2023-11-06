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

#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "ui/GpgFrontendUI.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyImportDetailDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key Import Detail Dialog object
   *
   * @param result
   * @param automatic
   * @param parent
   */
  KeyImportDetailDialog(GpgImportInformation result, bool automatic,
                        QWidget* parent = nullptr);

 private:
  /**
   * @brief Create a general info box object
   *
   */
  void create_general_info_box();

  /**
   * @brief Create a keys table object
   *
   */
  void create_keys_table();

  /**
   * @brief Create a button box object
   *
   */
  void create_button_box();

  /**
   * @brief Get the status string object
   *
   * @param keyStatus
   * @return QString
   */
  static QString get_status_string(int keyStatus);

  QTableWidget* keys_table_{};      ///<
  QGroupBox* general_info_box_{};   ///<
  QGroupBox* key_info_box_{};       ///<
  QDialogButtonBox* button_box_{};  ///<
  GpgImportInformation m_result_;   ///<
};
}  // namespace GpgFrontend::UI
