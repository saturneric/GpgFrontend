/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
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
  iconSizeGroup = new QButtonGroup();
  iconSizeSmall = new QRadioButton(_("small"));
  iconSizeMedium = new QRadioButton(_("medium"));
  iconSizeLarge = new QRadioButton(_("large"));

  iconSizeGroup->addButton(iconSizeSmall, 1);
  iconSizeGroup->addButton(iconSizeMedium, 2);
  iconSizeGroup->addButton(iconSizeLarge, 3);

  auto* iconSizeBoxLayout = new QHBoxLayout();
  iconSizeBoxLayout->addWidget(iconSizeSmall);
  iconSizeBoxLayout->addWidget(iconSizeMedium);
  iconSizeBoxLayout->addWidget(iconSizeLarge);

  iconSizeBox->setLayout(iconSizeBoxLayout);

  /*****************************************
   * Icon-Style-Box
   *****************************************/
  auto* iconStyleBox = new QGroupBox(_("Icon Style"));
  iconStyleGroup = new QButtonGroup();
  iconTextButton = new QRadioButton(_("just text"));
  iconIconsButton = new QRadioButton(_("just icons"));
  iconAllButton = new QRadioButton(_("text and icons"));

  iconStyleGroup->addButton(iconTextButton, 1);
  iconStyleGroup->addButton(iconIconsButton, 2);
  iconStyleGroup->addButton(iconAllButton, 3);

  auto* iconStyleBoxLayout = new QHBoxLayout();
  iconStyleBoxLayout->addWidget(iconTextButton);
  iconStyleBoxLayout->addWidget(iconIconsButton);
  iconStyleBoxLayout->addWidget(iconAllButton);

  iconStyleBox->setLayout(iconStyleBoxLayout);

  /*****************************************
   * Window-Size-Box
   *****************************************/
  auto* windowSizeBox = new QGroupBox(_("Window State"));
  auto* windowSizeBoxLayout = new QHBoxLayout();
  windowSizeCheckBox =
      new QCheckBox(_("Save window size and position on exit."), this);
  windowSizeBoxLayout->addWidget(windowSizeCheckBox);
  windowSizeBox->setLayout(windowSizeBoxLayout);

  /*****************************************
   * Info-Board-Font-Size-Box
   *****************************************/

  auto* infoBoardBox = new QGroupBox(_("Information Board"));
  auto* infoBoardLayout = new QHBoxLayout();
  infoBoardFontSizeSpin = new QSpinBox();
  infoBoardFontSizeSpin->setRange(9, 18);
  infoBoardFontSizeSpin->setValue(10);
  infoBoardFontSizeSpin->setSingleStep(1);
  infoBoardLayout->addWidget(new QLabel(_("Font Size in Information Board")));
  infoBoardLayout->addWidget(infoBoardFontSizeSpin);
  infoBoardBox->setLayout(infoBoardLayout);

  auto* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(iconSizeBox);
  mainLayout->addWidget(iconStyleBox);
  mainLayout->addWidget(windowSizeBox);
  mainLayout->addWidget(infoBoardBox);
  mainLayout->addStretch(1);
  setSettings();
  setLayout(mainLayout);
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void AppearanceTab::setSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    int width = settings.lookup("window.icon_size.width");
    int height = settings.lookup("window.icon_size.height");

    auto icon_size = QSize(width, height);

    switch (icon_size.height()) {
      case 12:
        iconSizeSmall->setChecked(true);
        break;
      case 24:
        iconSizeMedium->setChecked(true);
        break;
      case 32:
        iconSizeLarge->setChecked(true);
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
        iconTextButton->setChecked(true);
        break;
      case Qt::ToolButtonIconOnly:
        iconIconsButton->setChecked(true);
        break;
      case Qt::ToolButtonTextUnderIcon:
        iconAllButton->setChecked(true);
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
    if (window_save) windowSizeCheckBox->setCheckState(Qt::Checked);

  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("window_save");
  }

  // info board font size
  try {
    int info_font_size = settings.lookup("window.info_font_size");
    if (info_font_size < 9 || info_font_size > 18) info_font_size = 10;
    infoBoardFontSizeSpin->setValue(info_font_size);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("info_font_size");
  }
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void AppearanceTab::applySettings() {
  auto& settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("window") ||
      settings.lookup("window").getType() != libconfig::Setting::TypeGroup)
    settings.add("window", libconfig::Setting::TypeGroup);

  auto& window = settings["window"];

  int icon_size = 24;
  switch (iconSizeGroup->checkedId()) {
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
  switch (iconStyleGroup->checkedId()) {
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
        windowSizeCheckBox->isChecked();
  } else {
    window["window_save"] = windowSizeCheckBox->isChecked();
  }

  if (!window.exists("info_font_size")) {
    window.add("info_font_size", libconfig::Setting::TypeBoolean) =
        infoBoardFontSizeSpin->value();
  } else {
    window["info_font_size"] = infoBoardFontSizeSpin->value();
  }
}

}  // namespace GpgFrontend::UI
