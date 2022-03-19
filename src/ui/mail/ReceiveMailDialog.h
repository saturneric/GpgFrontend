/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef GPGFRONTEND_RECEIVEMAILDIALOG_H
#define GPGFRONTEND_RECEIVEMAILDIALOG_H

#include "ui/GpgFrontendUI.h"

class Ui_ReceiveMailDialog;

namespace vmime::net {
class folder;
};

namespace GpgFrontend::UI {

class IMAPFolder;

/**
 * @brief
 *
 */
class ReceiveMailDialog : public QDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Receive Mail Dialog object
   *
   * @param parent
   */
  explicit ReceiveMailDialog(QWidget* parent);

 private slots:
  /**
   * @brief
   *
   */
  void slot_refresh_data();

 private:
  std::shared_ptr<Ui_ReceiveMailDialog> ui_;          ///<
  std::vector<std::shared_ptr<IMAPFolder>> folders_;  ///<

  /**
   * @brief
   *
   * @param parent_folder
   */
  void list_sub_folders(IMAPFolder* parent_folder,
                        const std::shared_ptr<vmime::net::folder>&);
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_RECEIVEMAILDIALOG_H
