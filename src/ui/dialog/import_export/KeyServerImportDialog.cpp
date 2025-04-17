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

#include "KeyServerImportDialog.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/model/GpgImportInformation.h"
#include "core/model/SettingsObject.h"
#include "ui/UISignalStation.h"
#include "ui/struct/settings_object/KeyServerSO.h"
#include "ui/thread/KeyServerImportTask.h"
#include "ui/thread/KeyServerSearchTask.h"

namespace GpgFrontend::UI {

KeyServerImportDialog::KeyServerImportDialog(int channel, QWidget* parent)
    : GeneralDialog("key_server_import_dialog", parent),
      current_gpg_context_channel_(channel) {
  auto forbid_all_gnupg_connection =
      GetSettings()
          .value("network/forbid_all_gnupg_connection", false)
          .toBool();
  if (forbid_all_gnupg_connection) {
    QMessageBox::critical(this, "Forbidden", "GnuPG is in offline mode now.");
    this->close();
  }

  // Buttons
  close_button_ = new QPushButton(tr("Close"));
  connect(close_button_, &QPushButton::clicked, this,
          &KeyServerImportDialog::close);
  import_button_ = new QPushButton(tr("Import ALL"));
  connect(import_button_, &QPushButton::clicked, this,
          &KeyServerImportDialog::slot_import);
  import_button_->setDisabled(true);
  search_button_ = new QPushButton(tr("Search"));
  connect(search_button_, &QPushButton::clicked, this,
          &KeyServerImportDialog::slot_search);

  // Line edits for search string
  search_label_ = new QLabel(tr("Search String") + tr(": "));
  search_line_edit_ = new QLineEdit();

  // combobox for keyserver list
  key_server_label_ = new QLabel(tr("Key Server") + tr(": "));
  key_server_combo_box_ = create_combo_box();

  // table containing the keys found
  create_keys_table();
  message_ = new QLabel();
  message_->setFixedHeight(24);
  icon_ = new QLabel();
  icon_->setFixedHeight(24);

  // Layout for messagebox
  message_layout_ = new QHBoxLayout();
  message_layout_->addWidget(icon_);
  message_layout_->addWidget(message_);
  message_layout_->addStretch();

  // Network Waiting
  waiting_bar_ = new QProgressBar();
  waiting_bar_->setVisible(false);
  waiting_bar_->setRange(0, 0);
  waiting_bar_->setFixedWidth(200);

  auto* main_layout = new QGridLayout();

  main_layout->addWidget(search_label_, 1, 0);
  main_layout->addWidget(search_line_edit_, 1, 1);
  main_layout->addWidget(search_button_, 1, 2);
  main_layout->addWidget(key_server_label_, 2, 0);
  main_layout->addWidget(key_server_combo_box_, 2, 1);
  main_layout->addWidget(keys_table_, 3, 0, 1, 3);
  main_layout->addWidget(waiting_bar_, 4, 0, 1, 3);
  main_layout->addLayout(message_layout_, 5, 0, 1, 3);

  // Layout for import and close button
  auto* buttons_layout = new QHBoxLayout();
  buttons_layout->addStretch();
  buttons_layout->addWidget(import_button_);
  buttons_layout->addWidget(close_button_);
  main_layout->addLayout(buttons_layout, 6, 0, 1, 3);

  this->setLayout(main_layout);
  this->setWindowTitle(tr("Import Keys from key server"));
  this->setModal(true);

  movePosition2CenterOfParent();

  connect(this, &KeyServerImportDialog::SignalKeyImported,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
}

auto KeyServerImportDialog::create_combo_box() -> QComboBox* {
  auto* combo_box = new QComboBox;
  combo_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  try {
    KeyServerSO key_server(SettingsObject("key_server"));
    const auto& key_server_list = key_server.server_list;
    for (const auto& key_server : key_server_list) {
      combo_box->addItem(key_server);
    }
    auto target_key_server = key_server.GetTargetServer();
    LOG_D() << "set combo box to key server: " << target_key_server;

    combo_box->setCurrentText(target_key_server);
  } catch (...) {
    FLOG_W("setting operation error server_list default_server");
  }

  return combo_box;
}

void KeyServerImportDialog::create_keys_table() {
  keys_table_ = new QTableWidget();
  keys_table_->setColumnCount(4);

  // always a whole row is marked
  keys_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  keys_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Make just one row selectable
  keys_table_->setSelectionMode(QAbstractItemView::SingleSelection);

  QStringList labels;
  labels << tr("UID") << tr("Creation date") << tr("KeyID") << tr("Tag");
  keys_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  keys_table_->setHorizontalHeaderLabels(labels);
  keys_table_->verticalHeader()->hide();

  connect(keys_table_, &QTableWidget::cellActivated, this,
          &KeyServerImportDialog::slot_import);
}

void KeyServerImportDialog::set_message(const QString& text, bool error) {
  message_->setText(text);
  if (error) {
    icon_->setPixmap(QPixmap(":/icons/error.png")
                         .scaled(QSize(24, 24), Qt::KeepAspectRatio));
  } else {
    icon_->setPixmap(
        QPixmap(":/icons/info.png").scaled(QSize(24, 24), Qt::KeepAspectRatio));
  }
}

void KeyServerImportDialog::slot_search() {
  if (search_line_edit_->text().isEmpty()) {
    set_message("<h4>" + tr("Text is empty.") + "</h4>", false);
    return;
  }

  auto* task = new KeyServerSearchTask(key_server_combo_box_->currentText(),
                                       search_line_edit_->text());

  connect(task, &KeyServerSearchTask::SignalKeyServerSearchResult, this,
          &KeyServerImportDialog::slot_search_finished);

  connect(task, &KeyServerSearchTask::SignalKeyServerSearchResult, this, [=]() {
    this->search_button_->setDisabled(false);
    this->key_server_combo_box_->setDisabled(false);
    this->search_line_edit_->setReadOnly(false);
    this->import_button_->setDisabled(false);
    set_loading(false);
  });

  set_loading(true);
  this->search_button_->setDisabled(true);
  this->key_server_combo_box_->setDisabled(true);
  this->search_line_edit_->setReadOnly(true);
  this->import_button_->setDisabled(true);

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
      ->PostTask(task);

  QEventLoop loop;
  connect(task, &Thread::Task::SignalTaskEnd, &loop, &QEventLoop::quit);
  loop.exec();
}

void KeyServerImportDialog::slot_search_finished(
    QNetworkReply::NetworkError error, QString err_string, QByteArray buffer) {
  keys_table_->clearContents();
  keys_table_->setRowCount(0);

  auto stream = QTextStream(buffer);

  if (error != QNetworkReply::NoError) {
    switch (error) {
      case QNetworkReply::ContentNotFoundError:
        set_message(tr("Not Key Found"), true);
        break;
      case QNetworkReply::TimeoutError:
        set_message(tr("Timeout"), true);
        break;
      case QNetworkReply::HostNotFoundError:
        set_message(tr("Key Server Not Found"), true);
        break;
      default:
        set_message(tr("Connection Error"), true);
        QMessageBox::critical(this, tr("Connection Error"), err_string);
    }
    return;
  }

  if (stream.readLine().contains("Error")) {
    auto text = stream.readLine(1024);

    if (text.contains("Too many responses")) {
      set_message("<h4>" + tr("Too many responses from keyserver!") + "</h4>",
                  true);
      return;
    }

    if (text.contains("No keys found")) {
      auto query = search_line_edit_->text();

      // if string looks like hex string, search again with 0x prepended
      QRegularExpression rx("[0-9A-Fa-f]*");
      if (rx.match(query).hasMatch()) {
        set_message("<h4>" +
                        tr("No keys found, input may be kexId, retrying search "
                           "with 0x.") +
                        "</h4>",
                    true);
        search_line_edit_->setText(query.prepend("0x"));
        this->slot_search();
        return;
      }

      set_message(
          "<h4>" + tr("No keys found containing the search string!") + "</h4>",
          true);
      return;
    }

    if (text.contains("Insufficiently specific words")) {
      set_message(
          "<h4>" + tr("Insufficiently specific search string!") + "</h4>",
          true);
      return;
    }

    set_message(text, true);
    return;
  }

  int row = 0;
  bool strikeout = false;

  // read lines until end of steam
  while (!stream.atEnd()) {
    QStringList line =
        QString::fromUtf8(QByteArray::fromPercentEncoding(
                              stream.readLine().trimmed().toUtf8()))
            .split(":");

    // TODO: have a look at two following pub lines
    if (line[0] == "pub") {
      strikeout = false;

      QString flags = line[line.size() - 1];
      keys_table_->setRowCount(row + 1);

      // flags can be "d" for disabled, "r" for revoked
      // or "e" for expired
      if (flags.contains("r") or flags.contains("d") or flags.contains("e")) {
        strikeout = true;
        if (flags.contains("e")) {
          keys_table_->setItem(row, 3,
                               new QTableWidgetItem(QString("expired")));
        }
        if (flags.contains("r")) {
          keys_table_->setItem(row, 3, new QTableWidgetItem(tr("revoked")));
        }
        if (flags.contains("d")) {
          keys_table_->setItem(row, 3, new QTableWidgetItem(tr("disabled")));
        }
      }

      QStringList line2 = QString(QByteArray::fromPercentEncoding(
                                      stream.readLine().trimmed().toUtf8()))
                              .split(":");

      auto* uid = new QTableWidgetItem();
      if (line2.size() > 1) {
        uid->setText(line2[1]);
        keys_table_->setItem(row, 0, uid);
      }
      auto* creation_date =
          new QTableWidgetItem(QDateTime::fromSecsSinceEpoch(line[4].toInt())
                                   .toString("dd. MMM. yyyy"));
      keys_table_->setItem(row, 1, creation_date);
      auto* keyid = new QTableWidgetItem(line[1]);
      keys_table_->setItem(row, 2, keyid);
      if (strikeout) {
        QFont strike = uid->font();
        strike.setStrikeOut(true);
        uid->setFont(strike);
        creation_date->setFont(strike);
        keyid->setFont(strike);
      }
      row++;
    } else {
      if (line[0] == "uid") {
        QStringList l;
        int height = keys_table_->rowHeight(row - 1);
        keys_table_->setRowHeight(row - 1, height + 16);
        QString tmp = keys_table_->item(row - 1, 0)->text();
        tmp.append(QString("\n") + line[1]);
        auto* tmp1 = new QTableWidgetItem(tmp);
        keys_table_->setItem(row - 1, 0, tmp1);
        if (strikeout) {
          QFont strike = tmp1->font();
          strike.setStrikeOut(true);
          tmp1->setFont(strike);
        }
      }
    }
    set_message(
        QString("<h4>") +
            tr("%1 keys found. Double click a key to import it.").arg(row) +
            "</h4>",
        false);
  }
  keys_table_->resizeColumnsToContents();
  import_button_->setDisabled(keys_table_->size().isEmpty());
}

void KeyServerImportDialog::slot_import() {
  KeyIdArgsList key_ids;
  const int row_count = keys_table_->rowCount();
  for (int i = 0; i < row_count; ++i) {
    if (keys_table_->item(i, 2)->isSelected()) {
      QString keyid = keys_table_->item(i, 2)->text();
      key_ids.push_back(keyid);
    }
  }
  if (!key_ids.empty()) {
    SlotImport(key_ids, key_server_combo_box_->currentText());
  } else {
    QMessageBox::warning(
        this, tr("Warning"),
        tr("Please select one KeyPair before doing this operation."));
  }
}

void KeyServerImportDialog::SlotImport(const KeyIdArgsList& keys) {
  // keyserver host url
  QString target_keyserver;

  if (key_server_combo_box_ != nullptr) {
    target_keyserver = key_server_combo_box_->currentText();
  }
  if (target_keyserver.isEmpty()) {
    KeyServerSO key_server(SettingsObject("key_server"));
    target_keyserver = key_server.GetTargetServer();
  }
  KeyIdArgsList key_ids;
  for (const auto& key_id : keys) {
    key_ids.push_back(key_id);
  }
  SlotImport(key_ids, target_keyserver);
}

void KeyServerImportDialog::SlotImport(QStringList key_ids,
                                       QString keyserver_url) {
  auto* task = new KeyServerImportTask(keyserver_url,
                                       current_gpg_context_channel_, key_ids);

  connect(task, &KeyServerImportTask::SignalKeyServerImportResult, this,
          &KeyServerImportDialog::slot_import_finished);

  set_loading(true);
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
      ->PostTask(task);
}

void KeyServerImportDialog::slot_import_finished(
    int channel, bool success, QString err_msg, QByteArray buffer,
    QSharedPointer<GpgImportInformation> info) {
  set_loading(false);

  if (!success) {
    set_message(err_msg, true);
    return;
  }

  set_message(tr("Key Imported"), false);

  // refresh the key database
  emit SignalKeyImported();

  auto* connection = new QMetaObject::Connection;
  *connection = connect(
      UISignalStation::GetInstance(),
      &UISignalStation::SignalKeyDatabaseRefreshDone, this, [=]() {
        (new KeyImportDetailDialog(current_gpg_context_channel_, info, this));
        QObject::disconnect(*connection);
        delete connection;
      });
}

void KeyServerImportDialog::set_loading(bool status) {
  waiting_bar_->setVisible(status);
  if (status) set_message(tr("Processing ..."), false);
}

}  // namespace GpgFrontend::UI
