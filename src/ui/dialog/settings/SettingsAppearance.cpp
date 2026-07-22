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

namespace {

/// The one place the toolbar's crypto actions are enumerated on this page: it
/// pairs each operation bit with the check box that offers it, so a new
/// operation is a single row here instead of an edit in three parallel lists.
/// The order matches the grid in the .ui file.
struct ToolBarOperaEntry {
  GpgOperation opera;
  QCheckBox* Ui_AppearanceSettings::* box;
};

constexpr std::array<ToolBarOperaEntry, 9> kToolBarOperas{{
    {kENCRYPT, &Ui_AppearanceSettings::encrCheckBox},
    {kDECRYPT, &Ui_AppearanceSettings::decrCheckBox},
    {kSIGN, &Ui_AppearanceSettings::signCheckBox},
    {kVERIFY, &Ui_AppearanceSettings::verifyCheckBox},
    {kENCRYPT_SIGN, &Ui_AppearanceSettings::encrSignCheckBox},
    {kDECRYPT_VERIFY, &Ui_AppearanceSettings::decrVerifyCheckBox},
    {kSYMMETRIC_ENCRYPT, &Ui_AppearanceSettings::symmetricEncrCheckBox},
    {kIM_ENCRYPT, &Ui_AppearanceSettings::imEncrCheckBox},
    {kIM_ENCRYPT_SIGN, &Ui_AppearanceSettings::imEncrSignCheckBox},
}};

}  // namespace

AppearanceTab::AppearanceTab(QWidget* parent)
    : QWidget(parent), ui_(SecureCreateSharedObject<Ui_AppearanceSettings>()) {
  ui_->setupUi(this);

  ui_->themeBox->setTitle(tr("Theme"));
  ui_->themaLabel->setText(tr("Theme"));

  ui_->toolbarBox->setTitle(tr("Toolbar"));

  ui_->toolbarIconSizeLabel->setText(tr("Icon Size"));
  ui_->smallRadioButton->setText(tr("small"));
  ui_->mediumRadioButton->setText(tr("medium"));
  ui_->largeRadioButton->setText(tr("large"));

  ui_->toolbarIconStyleLabel->setText(tr("Icon Style"));
  ui_->justTextRadioButton->setText(tr("just text"));
  ui_->justIconRadioButton->setText(tr("just icons"));
  ui_->textAndIconsRadioButton->setText(tr("text and icons"));

  ui_->toolbarOperasLabel->setText(tr("Actions"));
  ui_->toolbarImTipLabel->setText(
      tr("IM actions turn the text into one compact line that is safe to paste "
         "into an instant messenger."));

  ui_->textEditorBox->setTitle(tr("Text Editor"));
  ui_->textEditorFontLabel->setText(tr("Font Family"));
  ui_->fontSizeTextEditorLabel->setText(tr("Font Size"));
  ui_->textEditorTabSizeLabel->setText(tr("Tab Size"));

  ui_->fontSizeBox->setTitle(tr("Status Panel"));
  ui_->fontSizeInformationBoardLabel->setText(tr("Font Size"));

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

  auto info_board_info_font_size = appearance.info_board_font_size;
  if (info_board_info_font_size < 9 || info_board_info_font_size > 18) {
    info_board_info_font_size = 10;
  }
  ui_->fontSizeInformationBoardSpinBox->setValue(info_board_info_font_size);

  ui_->textEditorFontComboBox->clear();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  for (const auto& family : QFontDatabase::families()) {
    if (QFontDatabase::isFixedPitch(family)) {
      ui_->textEditorFontComboBox->addItem(family);
    }
  }
#else
  {
    QFontDatabase db;
    for (const auto& family : db.families()) {
      if (db.isFixedPitch(family)) {
        ui_->textEditorFontComboBox->addItem(family);
      }
    }
  }
#endif

  auto text_editor_font_family = appearance.text_editor_font_family;
  if (text_editor_font_family.isEmpty()) {
    text_editor_font_family =
        QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
  }
  auto font_index =
      ui_->textEditorFontComboBox->findText(text_editor_font_family);
  ui_->textEditorFontComboBox->setCurrentIndex(font_index != -1 ? font_index
                                                                : 0);

  auto text_editor_info_font_size = appearance.text_editor_font_size;
  if (text_editor_info_font_size < 9 || text_editor_info_font_size > 18) {
    text_editor_info_font_size = 10;
  }
  ui_->textEditorFontSizeSpinBox->setValue(text_editor_info_font_size);

  auto text_editor_tab_size = appearance.text_editor_tab_size;
  if (text_editor_tab_size < 1 || text_editor_tab_size > 16) {
    text_editor_tab_size = 4;
  }
  ui_->textEditorTabSizeSpinBox->setValue(text_editor_tab_size);

  // init available styles
  for (const auto& s : QStyleFactory::keys()) {
    ui_->themeComboBox->addItem(s.toLower());
  }

  auto settings = GetSettings();
  auto theme = settings.value("appearance/theme").toString();

  auto target_theme_index = ui_->themeComboBox->findText(theme);
  if (theme.isEmpty() || target_theme_index == -1) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    ui_->themeComboBox->setCurrentIndex(
        ui_->themeComboBox->findText(QApplication::style()->name()));
#else
    ui_->themeComboBox->setCurrentIndex(ui_->themeComboBox->findText(
        QApplication::style()->metaObject()->className()));
#endif
  } else {
    ui_->themeComboBox->setCurrentIndex(target_theme_index);
  }

  for (const auto& entry : kToolBarOperas) {
    (ui_.get()->*entry.box)
        ->setChecked((appearance.tool_bar_crypto_operas_type & entry.opera) !=
                     0);
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
    default:
      icon_size = 24;
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

  appearance.save_window_state = true;
  appearance.info_board_font_size =
      ui_->fontSizeInformationBoardSpinBox->value();
  appearance.text_editor_font_family =
      ui_->textEditorFontComboBox->currentText();
  appearance.text_editor_font_size = ui_->textEditorFontSizeSpinBox->value();
  appearance.text_editor_tab_size = ui_->textEditorTabSizeSpinBox->value();

  appearance.tool_bar_crypto_operas_type = kNONE;
  for (const auto& entry : kToolBarOperas) {
    appearance.tool_bar_crypto_operas_type |=
        (ui_.get()->*entry.box)->isChecked() ? entry.opera : kNONE;
  }

  general_settings_state.Store(appearance.ToJson());

  auto settings = GetSettings();
  settings.setValue("appearance/theme", ui_->themeComboBox->currentText());
}

}  // namespace GpgFrontend::UI
