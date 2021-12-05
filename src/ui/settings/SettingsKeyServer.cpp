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

#include "SettingsKeyServer.h"

#include "GlobalSettingStation.h"

namespace GpgFrontend::UI {

KeyserverTab::KeyserverTab(QWidget* parent) : QWidget(parent) {
  auto generalGroupBox = new QGroupBox(_("General"));
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
  labels << _("No.") << _("Address") << _("Available");
  keyServerTable->setHorizontalHeaderLabels(labels);

  auto* mainLayout = new QVBoxLayout(this);
  auto* label = new QLabel(QString(_("Default Key Server for Import")) + ": ");

  comboBox = new QComboBox;
  comboBox->setEditable(false);
  comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  auto* addKeyServerBox = new QWidget(this);
  auto* addKeyServerLayout = new QHBoxLayout(addKeyServerBox);
  auto* http = new QLabel("URL: ");
  newKeyServerEdit = new QLineEdit(this);
  auto* newKeyServerButton = new QPushButton(_("Add"), this);
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
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    auto& server_list = settings.lookup("keyserver.server_list");
    const auto server_list_size = server_list.getLength();
    for (int i = 0; i < server_list_size; i++) {
      std::string server_url = server_list[i];
      comboBox->addItem(server_url.c_str());
      keyServerStrList.append(server_url.c_str());
    }
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("server_list");
  }

  try {
    std::string default_server = settings.lookup("keyserver.default_server");
    comboBox->setCurrentText(default_server.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("default_server");
  }
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
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("keyserver") ||
      settings.lookup("keyserver").getType() != libconfig::Setting::TypeGroup)
    settings.add("keyserver", libconfig::Setting::TypeGroup);

  auto& keyserver = settings["keyserver"];

  if (keyserver.exists("server_list"))
    keyserver.remove("server_list");

  keyserver.add("server_list", libconfig::Setting::TypeList);

  auto& server_list = keyserver["server_list"];
  for (const auto& key_server_url : keyServerStrList) {
    server_list.add(libconfig::Setting::TypeString) =
        key_server_url.toStdString();
  }

  if (!keyserver.exists("default_server")) {
    keyserver.add("default_server", libconfig::Setting::TypeString) =
        comboBox->currentText().toStdString();
  } else {
    keyserver["default_server"] = comboBox->currentText().toStdString();
  }
}

void KeyserverTab::refreshTable() {
  LOG(INFO) << "Start Refreshing Key Server Table";

  keyServerTable->setRowCount(keyServerStrList.size());

  int index = 0;
  for (const auto& server : keyServerStrList) {
    auto* tmp1 = new QTableWidgetItem(QString::number(index));
    tmp1->setTextAlignment(Qt::AlignCenter);
    keyServerTable->setItem(index, 0, tmp1);
    auto* tmp2 = new QTableWidgetItem(server);
    tmp2->setTextAlignment(Qt::AlignCenter);
    keyServerTable->setItem(index, 1, tmp2);
    auto* tmp3 = new QTableWidgetItem();
    tmp3->setTextAlignment(Qt::AlignCenter);
    keyServerTable->setItem(index, 2, tmp3);
    index++;
  }
}

}  // namespace GpgFrontend::UI
