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

#include "SettingsAppearance.h"

#include "ui/struct/SettingsObject.h"
#include "ui_AppearanceSettings.h"

namespace GpgFrontend::UI {

AppearanceTab::AppearanceTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_AppearanceSettings>()) {
  ui_->setupUi(this);

  ui_->iconSizeBox->setTitle(_("Icon Size"));
  ui_->smallRadioButton->setText(_("small"));
  ui_->mediumRadioButton->setText(_("medium"));
  ui_->largeRadioButton->setText(_("large"));

  ui_->iconStyleBox->setTitle(_("Icon Style"));
  ui_->justTextRadioButton->setText(_("just text"));
  ui_->justIconRadioButton->setText(_("just icons"));
  ui_->textAndIconsRadioButton->setText(_("text and icons"));

  ui_->windowStateBox->setTitle(_("Window State"));
  ui_->windowStateCheckBox->setText(
      _("Save window size and position on exit."));

  ui_->textEditorBox->setTitle(_("Text Editor"));
  ui_->fontSizeTextEditorLabel->setText(_("Font Size in Text Editor"));

  ui_->informationBoardBox->setTitle(_("Information Board"));
  ui_->fontSizeInformationBoardLabel->setText(
      _("Font Size in Information Board"));

  icon_size_group_ = new QButtonGroup(this);
  icon_size_group_->addButton(ui_->smallRadioButton, 1);
  icon_size_group_->addButton(ui_->mediumRadioButton, 2);
  icon_size_group_->addButton(ui_->largeRadioButton, 3);

  icon_style_group_ = new QButtonGroup(this);
  icon_style_group_->addButton(ui_->justTextRadioButton, 1);
  icon_style_group_->addButton(ui_->justIconRadioButton, 2);
  icon_style_group_->addButton(ui_->textAndIconsRadioButton, 3);

  SetSettings();
}

void AppearanceTab::SetSettings() {
  SettingsObject general_settings_state("general_settings_state");

  int const width =
      general_settings_state.Check("icon_size").Check("width", 24);
  int const height =
      general_settings_state.Check("icon_size").Check("height", 24);

  auto icon_size = QSize(width, height);

  switch (icon_size.width()) {
    case 12:
      ui_->smallRadioButton->setChecked(true);
      break;
    case 24:
      ui_->mediumRadioButton->setChecked(true);
      break;
    case 32:
      ui_->largeRadioButton->setChecked(true);
      break;
  }

  // icon_style
  int const s_icon_style =
      general_settings_state.Check("icon_style", Qt::ToolButtonTextUnderIcon);
  auto icon_style = static_cast<Qt::ToolButtonStyle>(s_icon_style);

  switch (icon_style) {
    case Qt::ToolButtonTextOnly:
      ui_->justTextRadioButton->setChecked(true);
      break;
    case Qt::ToolButtonIconOnly:
      ui_->justIconRadioButton->setChecked(true);
      break;
    case Qt::ToolButtonTextUnderIcon:
      ui_->textAndIconsRadioButton->setChecked(true);
      break;
    default:
      break;
  }

  bool const window_save = general_settings_state.Check("window_save", true);
  if (window_save) ui_->windowStateCheckBox->setCheckState(Qt::Checked);

  auto info_board_info_font_size =
      general_settings_state.Check("info_board").Check("font_size", 10);
  if (info_board_info_font_size < 9 || info_board_info_font_size > 18) {
    info_board_info_font_size = 10;
  }
  ui_->fontSizeInformationBoardSpinBox->setValue(info_board_info_font_size);

  auto text_editor_info_font_size =
      general_settings_state.Check("text_editor").Check("font_size", 10);
  if (text_editor_info_font_size < 9 || text_editor_info_font_size > 18) {
    text_editor_info_font_size = 10;
  }
  ui_->fontSizeTextEditorLabelSpinBox->setValue(text_editor_info_font_size);
}

void AppearanceTab::ApplySettings() {
  SettingsObject general_settings_state("general_settings_state");

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

  general_settings_state["icon_size"]["width"] = icon_size;
  general_settings_state["icon_size"]["height"] = icon_size;

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

  general_settings_state["icon_style"] = icon_style;

  general_settings_state["window_save"] = ui_->windowStateCheckBox->isChecked();

  general_settings_state["info_board"]["font_size"] =
      ui_->fontSizeInformationBoardSpinBox->value();

  general_settings_state["text_editor"]["font_size"] =
      ui_->fontSizeTextEditorLabelSpinBox->value();
}

}  // namespace GpgFrontend::UI
