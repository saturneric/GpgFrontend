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

#ifndef GPGFRONTEND_SETTINGSAPPEARANCE_H
#define GPGFRONTEND_SETTINGSAPPEARANCE_H

#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {

class AppearanceTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Appearance Tab object
   *
   * @param parent
   */
  explicit AppearanceTab(QWidget* parent = nullptr);

  /**
   * @brief Set the Settings object
   *
   */
  void SetSettings();

  /**
   * @brief
   *
   */
  void ApplySettings();

 private:
  QButtonGroup* icon_style_group_;       ///<
  QRadioButton* icon_size_small_;        ///<
  QRadioButton* icon_size_medium_;       ///<
  QRadioButton* icon_size_large_;        ///<
  QButtonGroup* icon_size_group_;        ///<
  QRadioButton* icon_text_button_;       ///<
  QRadioButton* icon_icons_button_;      ///<
  QRadioButton* icon_all_button_;        ///<
  QSpinBox* info_board_font_size_spin_;  ///<
  QCheckBox* window_size_check_box_;     ///<

 signals:

  /**
   * @brief
   *
   * @param needed
   */
  void signalRestartNeeded(bool needed);
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSAPPEARANCE_H
