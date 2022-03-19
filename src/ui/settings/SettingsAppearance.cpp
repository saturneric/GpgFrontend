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

#include "SettingsAppearance.h"

#include "core/function/GlobalSettingStation.h"
#include "ui/struct/SettingsObject.h"

namespace GpgFrontend::UI {

AppearanceTab::AppearanceTab(QWidget* parent) : QWidget(parent) {
  /*****************************************
   * Icon-Size-Box
   *****************************************/
  auto* iconSizeBox = new QGroupBox(_("Icon Size"));
  icon_size_group_ = new QButtonGroup();
  icon_size_small_ = new QRadioButton(_("small"));
  icon_size_medium_ = new QRadioButton(_("medium"));
  icon_size_large_ = new QRadioButton(_("large"));

  icon_size_group_->addButton(icon_size_small_, 1);
  icon_size_group_->addButton(icon_size_medium_, 2);
  icon_size_group_->addButton(icon_size_large_, 3);

  auto* iconSizeBoxLayout = new QHBoxLayout();
  iconSizeBoxLayout->addWidget(icon_size_small_);
  iconSizeBoxLayout->addWidget(icon_size_medium_);
  iconSizeBoxLayout->addWidget(icon_size_large_);

  iconSizeBox->setLayout(iconSizeBoxLayout);

  /*****************************************
   * Icon-Style-Box
   *****************************************/
  auto* iconStyleBox = new QGroupBox(_("Icon Style"));
  icon_style_group_ = new QButtonGroup();
  icon_text_button_ = new QRadioButton(_("just text"));
  icon_icons_button_ = new QRadioButton(_("just icons"));
  icon_all_button_ = new QRadioButton(_("text and icons"));

  icon_style_group_->addButton(icon_text_button_, 1);
  icon_style_group_->addButton(icon_icons_button_, 2);
  icon_style_group_->addButton(icon_all_button_, 3);

  auto* iconStyleBoxLayout = new QHBoxLayout();
  iconStyleBoxLayout->addWidget(icon_text_button_);
  iconStyleBoxLayout->addWidget(icon_icons_button_);
  iconStyleBoxLayout->addWidget(icon_all_button_);

  iconStyleBox->setLayout(iconStyleBoxLayout);

  /*****************************************
   * Window-Size-Box
   *****************************************/
  auto* windowSizeBox = new QGroupBox(_("Window State"));
  auto* windowSizeBoxLayout = new QHBoxLayout();
  window_size_check_box_ =
      new QCheckBox(_("Save window size and position on exit."), this);
  windowSizeBoxLayout->addWidget(window_size_check_box_);
  windowSizeBox->setLayout(windowSizeBoxLayout);

  /*****************************************
   * Info-Board-Font-Size-Box
   *****************************************/

  auto* infoBoardBox = new QGroupBox(_("Information Board"));
  auto* infoBoardLayout = new QHBoxLayout();
  info_board_font_size_spin_ = new QSpinBox();
  info_board_font_size_spin_->setRange(9, 18);
  info_board_font_size_spin_->setValue(10);
  info_board_font_size_spin_->setSingleStep(1);
  infoBoardLayout->addWidget(new QLabel(_("Font Size in Information Board")));
  infoBoardLayout->addWidget(info_board_font_size_spin_);
  infoBoardBox->setLayout(infoBoardLayout);

  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(iconSizeBox);
  mainLayout->addWidget(iconStyleBox);
  mainLayout->addWidget(windowSizeBox);
  mainLayout->addWidget(infoBoardBox);
  mainLayout->addStretch(1);
  SetSettings();
  setLayout(mainLayout);
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void AppearanceTab::SetSettings() {

  SettingsObject main_windows_state("main_windows_state");

  int width = main_windows_state.Check("icon_size").Check("width", 24),
      height = main_windows_state.Check("icon_size").Check("height", 24);

  auto icon_size = QSize(width, height);

  switch (icon_size.width()) {
    case 12:
      icon_size_small_->setChecked(true);
      break;
    case 24:
      icon_size_medium_->setChecked(true);
      break;
    case 32:
      icon_size_large_->setChecked(true);
      break;
  }

  // icon_style
  int s_icon_style =
      main_windows_state.Check("icon_style", Qt::ToolButtonTextUnderIcon);
  auto icon_style = static_cast<Qt::ToolButtonStyle>(s_icon_style);

  switch (icon_style) {
    case Qt::ToolButtonTextOnly:
      icon_text_button_->setChecked(true);
      break;
    case Qt::ToolButtonIconOnly:
      icon_icons_button_->setChecked(true);
      break;
    case Qt::ToolButtonTextUnderIcon:
      icon_all_button_->setChecked(true);
      break;
    default:
      break;
  }

  bool window_save = main_windows_state.Check("window_save", true);
  if (window_save) window_size_check_box_->setCheckState(Qt::Checked);

  auto info_font_size = main_windows_state.Check("info_font_size", 10);
  if (info_font_size < 9 || info_font_size > 18) info_font_size = 10;
  info_board_font_size_spin_->setValue(info_font_size);
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void AppearanceTab::ApplySettings() {

  SettingsObject main_windows_state("main_windows_state");

  int icon_size = 24;
  switch (icon_size_group_->checkedId()) {
    case 1:
      icon_size = 12;
      break;
    case 2:
      icon_size = 24;
      break;
    case 3:
      icon_size = 32;
      break;
  }

  main_windows_state["icon_size"]["width"] = icon_size;
  main_windows_state["icon_size"]["height"] = icon_size;

  auto icon_style = Qt::ToolButtonTextUnderIcon;
  switch (icon_style_group_->checkedId()) {
    case 1:
      icon_style = Qt::ToolButtonTextOnly;
      break;
    case 2:
      icon_style = Qt::ToolButtonIconOnly;
      break;
    case 3:
      icon_style = Qt::ToolButtonTextUnderIcon;
      break;
  }

  main_windows_state["icon_style"] = icon_style;

  main_windows_state["window_save"] = window_size_check_box_->isChecked();

  main_windows_state["info_font_size"] = info_board_font_size_spin_->value();

}

}  // namespace GpgFrontend::UI
