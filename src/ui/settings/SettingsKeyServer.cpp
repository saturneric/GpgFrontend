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

KeyserverTab::KeyserverTab(QWidget *parent)
: QWidget(parent), appPath(qApp->applicationDirPath()),
settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
         QSettings::IniFormat) {

    auto keyServerList = settings.value("keyserver/keyServerList").toStringList();

    auto *mainLayout = new QVBoxLayout(this);

    auto *label = new QLabel(tr("Default Key Server for import:"));
    comboBox = new QComboBox;
    comboBox->setEditable(false);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    for (const auto &keyServer : keyServerList) {
        comboBox->addItem(keyServer);
        qDebug() << "KeyserverTab Get ListItemText" << keyServer;
    }

    comboBox->setCurrentText(
            settings.value("keyserver/defaultKeyServer").toString());

    auto *addKeyServerBox = new QWidget(this);
    auto *addKeyServerLayout = new QHBoxLayout(addKeyServerBox);
    auto *http = new QLabel("URL: ");
    newKeyServerEdit = new QLineEdit(this);
    auto *newKeyServerButton = new QPushButton(tr("Add"), this);
    connect(newKeyServerButton, SIGNAL(clicked()), this, SLOT(addKeyServer()));
    addKeyServerLayout->addWidget(http);
    addKeyServerLayout->addWidget(newKeyServerEdit);
    addKeyServerLayout->addWidget(newKeyServerButton);

    mainLayout->addWidget(label);
    mainLayout->addWidget(comboBox);
    mainLayout->addWidget(addKeyServerBox);
    mainLayout->addStretch(1);

    // Read keylist from ini-file and fill it into combobox
    setSettings();
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void KeyserverTab::setSettings() {
    auto *keyServerList = new QStringList();
    for (int i = 0; i < comboBox->count(); i++) {
        keyServerList->append(comboBox->itemText(i));
        qDebug() << "KeyserverTab ListItemText" << comboBox->itemText(i);
    }
    settings.setValue("keyserver/keyServerList", *keyServerList);
    delete keyServerList;
    settings.setValue("keyserver/defaultKeyServer", comboBox->currentText());
}

void KeyserverTab::addKeyServer() {
    if (newKeyServerEdit->text().startsWith("http://") ||
    newKeyServerEdit->text().startsWith("https://")) {
        comboBox->addItem(newKeyServerEdit->text());
    } else {
        comboBox->addItem("http://" + newKeyServerEdit->text());
    }
    comboBox->setCurrentIndex(comboBox->count() - 1);
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void KeyserverTab::applySettings() {
    settings.setValue("keyserver/defaultKeyServer", comboBox->currentText());
}
