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

#include "SettingsKeyServer.h"

#include "GlobalSettingStation.h"
#include "ui/thread/ListedKeyServerTestThread.h"
#include "ui_KeyServerSettings.h"

namespace GpgFrontend::UI {

KeyserverTab::KeyserverTab(QWidget* parent)
    : QWidget(parent), ui_(std::make_shared<Ui_KeyServerSettings>()) {
  ui_->setupUi(this);
  ui_->keyServerListTable->setSizeAdjustPolicy(
      QAbstractScrollArea::AdjustToContents);

  connect(ui_->addKeyServerPushButton, &QPushButton::clicked, this,
          &KeyserverTab::slot_add_key_server);
  connect(ui_->testKeyServerButton, &QPushButton::clicked, this,
          &KeyserverTab::slot_test_listed_key_server);

  ui_->keyServerListGroupBox->setTitle(_("Keyserver List"));
  ui_->operationsGroupBox->setTitle(_("Operations"));

  ui_->keyServerListTable->horizontalHeaderItem(0)->setText(_("Default"));
  ui_->keyServerListTable->horizontalHeaderItem(1)->setText(
      _("Keyserver Address"));
  ui_->keyServerListTable->horizontalHeaderItem(2)->setText(_("Security"));
  ui_->keyServerListTable->horizontalHeaderItem(3)->setText(_("Available"));

  ui_->addKeyServerPushButton->setText(_("Add"));
  ui_->testKeyServerButton->setText(_("Test Listed Keyserver"));

  ui_->tipsLabel->setText(
      _("Tips: Please Double-click table item to edit it."));
  ui_->actionDelete_Selected_Key_Server->setText(_("Delete Selected"));
  ui_->actionDelete_Selected_Key_Server->setToolTip(
      _("Delete Selected Key Server"));
  ui_->actionSet_As_Default->setText(_("Set As Default"));
  ui_->actionSet_As_Default->setToolTip(_("Set As Default"));

  popup_menu_ = new QMenu(this);
  popup_menu_->addAction(ui_->actionSet_As_Default);
  popup_menu_->addAction(ui_->actionDelete_Selected_Key_Server);

  connect(ui_->keyServerListTable, &QTableWidget::itemChanged,
          [=](QTableWidgetItem* item) {
            LOG(INFO) << "item edited" << item->column();
            if (item->column() != 1) return;
            const auto row_size = ui_->keyServerListTable->rowCount();
            // Update Actions
            if (row_size > 0) {
              key_server_str_list_.clear();
              for (int i = 0; i < row_size; i++) {
                const auto key_server =
                    ui_->keyServerListTable->item(i, 1)->text();
                key_server_str_list_.append(key_server);
              }
            }
          });

  connect(ui_->actionSet_As_Default, &QAction::triggered, [=]() {
    const auto row_size = ui_->keyServerListTable->rowCount();
    for (int i = 0; i < row_size; i++) {
      const auto item = ui_->keyServerListTable->item(i, 1);
      if (!item->isSelected()) continue;
      this->default_key_server_ = item->text();
    }
    this->slot_refresh_table();
  });

  connect(ui_->actionDelete_Selected_Key_Server, &QAction::triggered, [=]() {
    const auto row_size = ui_->keyServerListTable->rowCount();
    for (int i = 0; i < row_size; i++) {
      const auto item = ui_->keyServerListTable->item(i, 1);
      if (!item->isSelected()) continue;
      this->key_server_str_list_.removeAt(i);
      break;
    }
    this->slot_refresh_table();
  });

  // Read key-list from ini-file and fill it into combobox
  SetSettings();
  slot_refresh_table();
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void KeyserverTab::SetSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    auto& server_list = settings.lookup("keyserver.server_list");
    const auto server_list_size = server_list.getLength();
    for (int i = 0; i < server_list_size; i++) {
      std::string server_url = server_list[i];
      key_server_str_list_.append(server_url.c_str());
    }
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("server_list");
  }

  try {
    std::string default_server = settings.lookup("keyserver.default_server");
    if (!key_server_str_list_.contains(default_server.c_str()))
      key_server_str_list_.append(default_server.c_str());
    default_key_server_ = QString::fromStdString(default_server);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("default_server");
  }
}

void KeyserverTab::slot_add_key_server() {
  auto target_url = ui_->addKeyServerEdit->text();
  if (url_reg_.match(target_url).hasMatch()) {
    if (target_url.startsWith("https://")) {
      ;
    } else if (target_url.startsWith("http://")) {
      QMessageBox::warning(
          this, _("Insecure keyserver address"),
          _("For security reasons, using HTTP as the communication protocol "
            "with "
            "the key server is not recommended. It is recommended to use "
            "HTTPS."));
    }
    key_server_str_list_.append(ui_->addKeyServerEdit->text());
  } else {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("You may not use HTTPS or HTTP as the protocol for communicating "
          "with the key server, which may not be wrong. But please check the "
          "address you entered again to make sure it is correct. Are you "
          "sure "
          "that want to add it into the keyserver list?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel)
      return;
    else
      key_server_str_list_.append(ui_->addKeyServerEdit->text());
  }
  slot_refresh_table();
}

void KeyserverTab::ApplySettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("keyserver") ||
      settings.lookup("keyserver").getType() != libconfig::Setting::TypeGroup)
    settings.add("keyserver", libconfig::Setting::TypeGroup);

  auto& keyserver = settings["keyserver"];

  if (keyserver.exists("server_list")) keyserver.remove("server_list");

  keyserver.add("server_list", libconfig::Setting::TypeList);

  const auto row_size = ui_->keyServerListTable->rowCount();
  auto& server_list = keyserver["server_list"];
  for (int i = 0; i < row_size; i++) {
    const auto key_server = ui_->keyServerListTable->item(i, 1)->text();
    server_list.add(libconfig::Setting::TypeString) = key_server.toStdString();
  }

  if (!keyserver.exists("default_server")) {
    keyserver.add("default_server", libconfig::Setting::TypeString) =
        default_key_server_.toStdString();
  } else {
    keyserver["default_server"] = default_key_server_.toStdString();
  }
}

void KeyserverTab::slot_refresh_table() {
  LOG(INFO) << "Start Refreshing Key Server Table";

  ui_->keyServerListTable->blockSignals(true);
  ui_->keyServerListTable->setRowCount(key_server_str_list_.size());

  int index = 0;
  for (const auto& server : key_server_str_list_) {
    auto* tmp1 =
        new QTableWidgetItem(server == default_key_server_ ? "*" : QString{});
    tmp1->setTextAlignment(Qt::AlignCenter);
    ui_->keyServerListTable->setItem(index, 0, tmp1);
    tmp1->setFlags(tmp1->flags() ^ Qt::ItemIsEditable);

    auto* tmp2 = new QTableWidgetItem(server);
    tmp2->setTextAlignment(Qt::AlignCenter);
    ui_->keyServerListTable->setItem(index, 1, tmp2);

    auto* tmp3 = new QTableWidgetItem(server.startsWith("https") ? _("true")
                                                                 : _("false"));
    tmp3->setTextAlignment(Qt::AlignCenter);
    ui_->keyServerListTable->setItem(index, 2, tmp3);
    tmp3->setFlags(tmp3->flags() ^ Qt::ItemIsEditable);

    auto* tmp4 = new QTableWidgetItem(_("unknown"));
    tmp4->setTextAlignment(Qt::AlignCenter);
    ui_->keyServerListTable->setItem(index, 3, tmp4);
    tmp4->setFlags(tmp3->flags() ^ Qt::ItemIsEditable);
    index++;
  }
  const auto column_count = ui_->keyServerListTable->columnCount();
  for (int i = 0; i < column_count; i++) {
    ui_->keyServerListTable->resizeColumnToContents(i);
  }
  ui_->keyServerListTable->blockSignals(false);
}

void KeyserverTab::slot_test_listed_key_server() {
  auto timeout =
      QInputDialog::getInt(this, _("Set TCP Timeout"), tr("timeout(ms): "),
                           QLineEdit::Normal, 500, 2000);

  QStringList urls;
  const auto row_size = ui_->keyServerListTable->rowCount();
  for (int i = 0; i < row_size; i++) {
    const auto keyserver_url = ui_->keyServerListTable->item(i, 1)->text();
    urls.push_back(keyserver_url);
  }

  auto thread = new ListedKeyServerTestThread(urls, timeout, this);
  connect(thread,
          &GpgFrontend::UI::ListedKeyServerTestThread::
              SignalKeyServerListTestResult,
          this, [=](const QStringList& result) {
            const auto row_size = ui_->keyServerListTable->rowCount();
            if (result.size() != row_size) return;
            ui_->keyServerListTable->blockSignals(true);
            for (int i = 0; i < row_size; i++) {
              const auto status = result[i];
              auto status_iem = ui_->keyServerListTable->item(i, 3);
              if (status == "Reachable") {
                status_iem->setText(_("Reachable"));
                status_iem->setForeground(QBrush(QColor::fromRgb(0, 255, 0)));
              } else {
                status_iem->setText(_("Not Reachable"));
                status_iem->setForeground(QBrush(QColor::fromRgb(255, 0, 0)));
              }
            }
            ui_->keyServerListTable->blockSignals(false);
          });
  connect(thread, &QThread::finished, thread, &QThread::deleteLater);

  // Waiting Dialog
  auto* waiting_dialog = new QProgressDialog(this);
  waiting_dialog->setMaximum(0);
  waiting_dialog->setMinimum(0);
  auto waiting_dialog_label =
      new QLabel(QString(_("Test Key Server Connection...")) + "<br /><br />" +
                 _("This test only tests the network connectivity of the key "
                   "server. Passing the test does not mean that the key server "
                   "is functionally available."));
  waiting_dialog_label->setWordWrap(true);
  waiting_dialog->setLabel(waiting_dialog_label);
  waiting_dialog->resize(420, 120);
  connect(thread, &QThread::finished, [=]() {
    waiting_dialog->finished(0);
    waiting_dialog->deleteLater();
  });
  connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    LOG(INFO) << "cancel clicked";
    if (thread->isRunning()) thread->terminate();
  });

  // Show Waiting Dialog
  waiting_dialog->show();
  waiting_dialog->setFocus();

  thread->start();
  QEventLoop loop;
  connect(thread, &QThread::finished, &loop, &QEventLoop::quit);
  loop.exec();
}

void KeyserverTab::contextMenuEvent(QContextMenuEvent* event) {
  QWidget::contextMenuEvent(event);
  if (ui_->keyServerListTable->selectedItems().length() > 0) {
    popup_menu_->exec(event->globalPos());
  }
}

}  // namespace GpgFrontend::UI
