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

#include "SettingsKeyServer.h"

namespace GpgFrontend::UI {

KeyserverTab::KeyserverTab(QWidget* parent)
    : QWidget(parent),
      appPath(qApp->applicationDirPath()),
      settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
               QSettings::IniFormat) {
  auto generalGroupBox = new QGroupBox(tr("General"));
  auto generalLayout = new QVBoxLayout();

  keyServerTable = new QTableWidget();
  keyServerTable->setColumnCount(3);
  keyServerTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  keyServerTable->horizontalHeader()->setStretchLastSection(false);
  keyServerTable->verticalHeader()->hide();
  keyServerTable->setShowGrid(false);
  keyServerTable->sortByColumn(0, Qt::AscendingOrder);
  keyServerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  keyServerTable->setSelectionMode(QAbstractItemView::SingleSelection);

  // tableitems not editable
  keyServerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  keyServerTable->setFocusPolicy(Qt::NoFocus);
  keyServerTable->setAlternatingRowColors(true);

  QStringList labels;
  labels << tr("No.") << tr("Address") << tr("Available");
  keyServerTable->setHorizontalHeaderLabels(labels);

  auto* mainLayout = new QVBoxLayout(this);
  auto* label = new QLabel(tr("Default Key Server for Import:"));

  comboBox = new QComboBox;
  comboBox->setEditable(false);
  comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  auto* addKeyServerBox = new QWidget(this);
  auto* addKeyServerLayout = new QHBoxLayout(addKeyServerBox);
  auto* http = new QLabel("URL: ");
  newKeyServerEdit = new QLineEdit(this);
  auto* newKeyServerButton = new QPushButton(tr("Add"), this);
  connect(newKeyServerButton, SIGNAL(clicked()), this, SLOT(addKeyServer()));
  addKeyServerLayout->addWidget(http);
  addKeyServerLayout->addWidget(newKeyServerEdit);
  addKeyServerLayout->addWidget(newKeyServerButton);

  generalLayout->addWidget(label);
  generalLayout->addWidget(comboBox);
  generalLayout->addWidget(keyServerTable);
  generalLayout->addWidget(addKeyServerBox);
  generalLayout->addStretch(0);

  generalGroupBox->setLayout(generalLayout);
  mainLayout->addWidget(generalGroupBox);
  mainLayout->addStretch(0);

  setLayout(mainLayout);
  // Read keylist from ini-file and fill it into combobox
  setSettings();
  refreshTable();
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void KeyserverTab::setSettings() {
  keyServerStrList = settings.value("keyserver/keyServerList").toStringList();

  for (const auto& keyServer : keyServerStrList) {
    comboBox->addItem(keyServer);
    qDebug() << "KeyserverTab Get ListItemText" << keyServer;
  }

  comboBox->setCurrentText(
      settings.value("keyserver/defaultKeyServer").toString());
}

void KeyserverTab::addKeyServer() {
  QString targetUrl;
  if (newKeyServerEdit->text().startsWith("http://") ||
      newKeyServerEdit->text().startsWith("https://"))
    targetUrl = newKeyServerEdit->text();
  else
    targetUrl = "http://" + newKeyServerEdit->text();
  keyServerStrList.append(targetUrl);
  comboBox->addItem(targetUrl);
  refreshTable();
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void KeyserverTab::applySettings() {
  settings.setValue("keyserver/keyServerList", keyServerStrList);
  settings.setValue("keyserver/defaultKeyServer", comboBox->currentText());
}

void KeyserverTab::refreshTable() {
  qDebug() << "Start Refreshing Key Server Table";

  keyServerTable->setRowCount(keyServerStrList.size());

  int index = 0;
  for (const auto& server : keyServerStrList) {
    auto* tmp1 = new QTableWidgetItem(QString::number(index));
    tmp1->setTextAlignment(Qt::AlignCenter);
    keyServerTable->setItem(index, 0, tmp1);
    auto* tmp2 = new QTableWidgetItem(server);
    tmp2->setTextAlignment(Qt::AlignCenter);
    keyServerTable->setItem(index, 1, tmp2);
    auto* tmp3 = new QTableWidgetItem("");
    tmp3->setTextAlignment(Qt::AlignCenter);
    keyServerTable->setItem(index, 2, tmp3);
    index++;
  }
}

}  // namespace GpgFrontend::UI
