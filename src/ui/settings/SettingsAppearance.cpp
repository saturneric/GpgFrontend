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

#include "GlobalSettingStation.h"

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
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    int width = settings.lookup("window.icon_size.width");
    int height = settings.lookup("window.icon_size.height");

    auto icon_size = QSize(width, height);

    switch (icon_size.height()) {
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

  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("icon_size");
  }

  // icon_style
  try {
    int s_icon_style = settings.lookup("window.icon_style");
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

  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("icon_style");
  }

  // Window Save and Position
  try {
    bool window_save = settings.lookup("window.window_save");
    if (window_save) window_size_check_box_->setCheckState(Qt::Checked);

  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("window_save");
  }

  // info board font size
  try {
    int info_font_size = settings.lookup("window.info_font_size");
    if (info_font_size < 9 || info_font_size > 18) info_font_size = 10;
    info_board_font_size_spin_->setValue(info_font_size);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("info_font_size");
  }
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void AppearanceTab::ApplySettings() {
  auto& settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("window") ||
      settings.lookup("window").getType() != libconfig::Setting::TypeGroup)
    settings.add("window", libconfig::Setting::TypeGroup);

  auto& window = settings["window"];

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

  if (!window.exists("icon_size")) {
    auto& icon_size_settings =
        window.add("icon_size", libconfig::Setting::TypeGroup);
    icon_size_settings.add("width", libconfig::Setting::TypeInt) = icon_size;
    icon_size_settings.add("height", libconfig::Setting::TypeInt) = icon_size;
  } else {
    window["icon_size"]["width"] = icon_size;
    window["icon_size"]["height"] = icon_size;
  }

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

  if (!window.exists("icon_style")) {
    window.add("icon_style", libconfig::Setting::TypeInt) = icon_style;
  } else {
    window["icon_style"] = icon_style;
  }

  if (!window.exists("window_save")) {
    window.add("window_save", libconfig::Setting::TypeBoolean) =
        window_size_check_box_->isChecked();
  } else {
    window["window_save"] = window_size_check_box_->isChecked();
  }

  if (!window.exists("info_font_size")) {
    window.add("info_font_size", libconfig::Setting::TypeBoolean) =
        info_board_font_size_spin_->value();
  } else {
    window["info_font_size"] = info_board_font_size_spin_->value();
  }
}

}  // namespace GpgFrontend::UI
