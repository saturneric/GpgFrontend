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

#include "SettingsAppearance.h"

#include "core/function/GlobalSettingStation.h"
#include "core/model/SettingsObject.h"
#include "core/utils/MemoryUtils.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui_AppearanceSettings.h"

namespace GpgFrontend::UI {

AppearanceTab::AppearanceTab(QWidget* parent)
    : QWidget(parent), ui_(SecureCreateSharedObject<Ui_AppearanceSettings>()) {
  ui_->setupUi(this);

  ui_->generalBox->setTitle(tr("General"));

  ui_->themaLabel->setText(tr("Theme"));
  ui_->windowStateCheckBox->setText(
      tr("Save window size and position on exit."));

  ui_->toolbarIconBox->setTitle(tr("Toolbar Icon"));

  ui_->toolbarIconSizeLabel->setText(tr("Size"));
  ui_->smallRadioButton->setText(tr("small"));
  ui_->mediumRadioButton->setText(tr("medium"));
  ui_->largeRadioButton->setText(tr("large"));

  ui_->toolbarIconStyleLabel->setText(tr("Style"));
  ui_->justTextRadioButton->setText(tr("just text"));
  ui_->justIconRadioButton->setText(tr("just icons"));
  ui_->textAndIconsRadioButton->setText(tr("text and icons"));

  ui_->fontSizeBox->setTitle(tr("Font Size"));

  ui_->fontSizeTextEditorLabel->setText(tr("Text Editor"));
  ui_->fontSizeInformationBoardLabel->setText(tr("Information Board"));

  icon_size_group_ = new QButtonGroup(this);
  icon_size_group_->addButton(ui_->smallRadioButton, 1);
  icon_size_group_->addButton(ui_->mediumRadioButton, 2);
  icon_size_group_->addButton(ui_->largeRadioButton, 3);

  icon_style_group_ = new QButtonGroup(this);
  icon_style_group_->addButton(ui_->justTextRadioButton, 1);
  icon_style_group_->addButton(ui_->justIconRadioButton, 2);
  icon_style_group_->addButton(ui_->textAndIconsRadioButton, 3);

  SetSettings();

  connect(ui_->themeComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &AppearanceTab::SignalRestartNeeded);
}

void AppearanceTab::SetSettings() {
  AppearanceSO const appearance(SettingsObject("general_settings_state"));

  auto icon_size =
      QSize(appearance.tool_bar_icon_width, appearance.tool_bar_icon_height);

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
    default:
      ui_->smallRadioButton->setChecked(true);
      break;
  }

  switch (appearance.tool_bar_button_style) {
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

  if (appearance.save_window_state) {
    ui_->windowStateCheckBox->setCheckState(Qt::Checked);
  }

  auto info_board_info_font_size = appearance.info_board_font_size;
  if (info_board_info_font_size < 9 || info_board_info_font_size > 18) {
    info_board_info_font_size = 10;
  }
  ui_->fontSizeInformationBoardSpinBox->setValue(info_board_info_font_size);

  auto text_editor_info_font_size = appearance.text_editor_font_size;
  if (text_editor_info_font_size < 9 || text_editor_info_font_size > 18) {
    text_editor_info_font_size = 10;
  }
  ui_->fontSizeTextEditorLabelSpinBox->setValue(text_editor_info_font_size);

  // init available styles
  for (const auto& s : QStyleFactory::keys()) {
    ui_->themeComboBox->addItem(s.toLower());
  }

  auto settings = GlobalSettingStation::GetInstance().GetSettings();
  auto theme = settings.value("appearance/theme").toString();

  auto target_theme_index = ui_->themeComboBox->findText(theme);
  if (theme.isEmpty() || target_theme_index == -1) {
#ifdef QT5_BUILD
    ui_->themeComboBox->setCurrentIndex(ui_->themeComboBox->findText(
        QApplication::style()->metaObject()->className()));
#else
    ui_->themeComboBox->setCurrentIndex(
        ui_->themeComboBox->findText(QApplication::style()->name()));
#endif
  } else {
    ui_->themeComboBox->setCurrentIndex(target_theme_index);
  }
}

void AppearanceTab::ApplySettings() {
  SettingsObject general_settings_state("general_settings_state");
  AppearanceSO appearance(general_settings_state);

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

  appearance.tool_bar_icon_height = icon_size;
  appearance.tool_bar_icon_width = icon_size;

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
    default:
      icon_style = Qt::ToolButtonTextOnly;
      break;
  }
  appearance.tool_bar_button_style = icon_style;

  appearance.save_window_state = ui_->windowStateCheckBox->isChecked();
  appearance.info_board_font_size =
      ui_->fontSizeInformationBoardSpinBox->value();
  appearance.text_editor_font_size =
      ui_->fontSizeTextEditorLabelSpinBox->value();

  general_settings_state.Store(appearance.ToJson());

  auto settings = GlobalSettingStation::GetInstance().GetSettings();
  settings.setValue("appearance/theme", ui_->themeComboBox->currentText());
}

}  // namespace GpgFrontend::UI
