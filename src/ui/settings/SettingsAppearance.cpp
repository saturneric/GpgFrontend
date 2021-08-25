/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#include "ui/SettingsDialog.h"

AppearanceTab::AppearanceTab(QWidget *parent)
: QWidget(parent), appPath(qApp->applicationDirPath()),
settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
         QSettings::IniFormat) {
    /*****************************************
     * Icon-Size-Box
     *****************************************/
    auto *iconSizeBox = new QGroupBox(tr("Iconsize"));
    iconSizeGroup = new QButtonGroup();
    iconSizeSmall = new QRadioButton(tr("small"));
    iconSizeMedium = new QRadioButton(tr("medium"));
    iconSizeLarge = new QRadioButton(tr("large"));

    iconSizeGroup->addButton(iconSizeSmall, 1);
    iconSizeGroup->addButton(iconSizeMedium, 2);
    iconSizeGroup->addButton(iconSizeLarge, 3);

    auto *iconSizeBoxLayout = new QHBoxLayout();
    iconSizeBoxLayout->addWidget(iconSizeSmall);
    iconSizeBoxLayout->addWidget(iconSizeMedium);
    iconSizeBoxLayout->addWidget(iconSizeLarge);

    iconSizeBox->setLayout(iconSizeBoxLayout);

    /*****************************************
     * Icon-Style-Box
     *****************************************/
    auto *iconStyleBox = new QGroupBox(tr("Iconstyle"));
    iconStyleGroup = new QButtonGroup();
    iconTextButton = new QRadioButton(tr("just text"));
    iconIconsButton = new QRadioButton(tr("just icons"));
    iconAllButton = new QRadioButton(tr("text and icons"));

    iconStyleGroup->addButton(iconTextButton, 1);
    iconStyleGroup->addButton(iconIconsButton, 2);
    iconStyleGroup->addButton(iconAllButton, 3);

    auto *iconStyleBoxLayout = new QHBoxLayout();
    iconStyleBoxLayout->addWidget(iconTextButton);
    iconStyleBoxLayout->addWidget(iconIconsButton);
    iconStyleBoxLayout->addWidget(iconAllButton);

    iconStyleBox->setLayout(iconStyleBoxLayout);

    /*****************************************
     * Window-Size-Box
     *****************************************/
    auto *windowSizeBox = new QGroupBox(tr("Windowstate"));
    auto *windowSizeBoxLayout = new QHBoxLayout();
    windowSizeCheckBox =
            new QCheckBox(tr("Save window size and position on exit."), this);
    windowSizeBoxLayout->addWidget(windowSizeCheckBox);
    windowSizeBox->setLayout(windowSizeBoxLayout);

    /*****************************************
     * Info-Board-Font-Size-Box
     *****************************************/

    auto *infoBoardBox = new QGroupBox(tr("Information Board"));
    auto *infoBoardLayout = new QHBoxLayout();
    infoBoardFontSizeSpin = new QSpinBox();
    infoBoardFontSizeSpin->setRange(9, 18);
    infoBoardFontSizeSpin->setValue(10);
    infoBoardFontSizeSpin->setSingleStep(1);
    infoBoardLayout->addWidget(new QLabel(tr(" Front Size")));
    infoBoardLayout->addWidget(infoBoardFontSizeSpin);
    infoBoardBox->setLayout(infoBoardLayout);

    auto *mainLayout = new QVBoxLayout;
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

    // Iconsize
    QSize iconSize = settings.value("toolbar/iconsize", QSize(24, 24)).toSize();
    switch (iconSize.height()) {
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
    // Iconstyle
    Qt::ToolButtonStyle iconStyle = static_cast<Qt::ToolButtonStyle>(
            settings.value("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon)
            .toUInt());
    switch (iconStyle) {
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

    // Window Save and Position
    if (settings.value("window/windowSave").toBool())
        windowSizeCheckBox->setCheckState(Qt::Checked);

    // infoBoardFontSize
    auto infoBoardFontSize = settings.value("informationBoard/fontSize", 10).toInt();
    if (infoBoardFontSize < 9 || infoBoardFontSize > 18)
        infoBoardFontSize = 10;
    infoBoardFontSizeSpin->setValue(infoBoardFontSize);
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void AppearanceTab::applySettings() {
    switch (iconSizeGroup->checkedId()) {
        case 1:
            settings.setValue("toolbar/iconsize", QSize(12, 12));
            break;
            case 2:
                settings.setValue("toolbar/iconsize", QSize(24, 24));
                break;
                case 3:
                    settings.setValue("toolbar/iconsize", QSize(32, 32));
                    break;
    }

    switch (iconStyleGroup->checkedId()) {
        case 1:
            settings.setValue("toolbar/iconstyle", Qt::ToolButtonTextOnly);
            break;
            case 2:
                settings.setValue("toolbar/iconstyle", Qt::ToolButtonIconOnly);
                break;
                case 3:
                    settings.setValue("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon);
                    break;
    }

    settings.setValue("window/windowSave", windowSizeCheckBox->isChecked());

    settings.setValue("informationBoard/fontSize", infoBoardFontSizeSpin->value());
}

