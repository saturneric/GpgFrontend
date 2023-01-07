/*
 * Copyright (c) 2022. Saturneric
 *
 *  This file is part of GpgFrontend.
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
 */

//
// Created by eric on 2022/7/23.
//

#ifndef GPGFRONTEND_GNUPGTAB_H
#define GPGFRONTEND_GNUPGTAB_H

#include "core/GpgContext.h"
#include "ui/GpgFrontendUI.h"

class Ui_GnuPGInfo;
namespace GpgFrontend::UI {
class GnupgTab : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Info Tab object
   *
   * @param parent
   */
  explicit GnupgTab(QWidget* parent = nullptr);

 private:
  std::shared_ptr<Ui_GnuPGInfo> ui_;  ///<

  void process_software_info();
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_GNUPGTAB_H
