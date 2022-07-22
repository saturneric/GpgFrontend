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

#include "KeyServerImportDialog.h"

#include <string>
#include <utility>

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "thread/KeyServerImportTask.h"
#include "ui/SignalStation.h"
#include "ui/struct/SettingsObject.h"
#include "ui/thread/KeyServerSearchTask.h"

namespace GpgFrontend::UI {

KeyServerImportDialog::KeyServerImportDialog(bool automatic, QWidget* parent)
    : GeneralDialog("key_server_import_dialog", parent),
      m_automatic_(automatic) {
  // Layout for messagebox
  auto* message_layout = new QHBoxLayout();

  if (automatic) {
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  } else {
    // Buttons

    close_button_ = new QPushButton(_("Close"));
    connect(close_button_, &QPushButton::clicked, this,
            &KeyServerImportDialog::close);
    import_button_ = new QPushButton(_("Import ALL"));
    connect(import_button_, &QPushButton::clicked, this,
            &KeyServerImportDialog::slot_import);
    import_button_->setDisabled(true);
    search_button_ = new QPushButton(_("Search"));
    connect(search_button_, &QPushButton::clicked, this,
            &KeyServerImportDialog::slot_search);

    // Line edits for search string
    search_label_ = new QLabel(QString(_("Search String")) + _(": "));
    search_line_edit_ = new QLineEdit();

    // combobox for keyserver list
    key_server_label_ = new QLabel(QString(_("Key Server")) + _(": "));
    key_server_combo_box_ = create_comboBox();

    // table containing the keys found
    create_keys_table();
    message_ = new QLabel();
    message_->setFixedHeight(24);
    icon_ = new QLabel();
    icon_->setFixedHeight(24);

    message_layout->addWidget(icon_);
    message_layout->addWidget(message_);
    message_layout->addStretch();
  }

  // Network Waiting
  waiting_bar_ = new QProgressBar();
  waiting_bar_->setVisible(false);
  waiting_bar_->setRange(0, 0);
  waiting_bar_->setFixedWidth(200);
  message_layout->addWidget(waiting_bar_);

  auto* mainLayout = new QGridLayout;

  // 自动化调用界面布局
  if (automatic) {
    mainLayout->addLayout(message_layout, 0, 0, 1, 3);
  } else {
    mainLayout->addWidget(search_label_, 1, 0);
    mainLayout->addWidget(search_line_edit_, 1, 1);
    mainLayout->addWidget(search_button_, 1, 2);
    mainLayout->addWidget(key_server_label_, 2, 0);
    mainLayout->addWidget(key_server_combo_box_, 2, 1);
    mainLayout->addWidget(keys_table_, 3, 0, 1, 3);
    mainLayout->addLayout(message_layout, 4, 0, 1, 3);

    // Layout for import and close button
    auto* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(import_button_);
    buttonsLayout->addWidget(close_button_);
    mainLayout->addLayout(buttonsLayout, 6, 0, 1, 3);
  }

  this->setLayout(mainLayout);
  if (automatic)
    this->setWindowTitle(_("Update Keys from Keyserver"));
  else
    this->setWindowTitle(_("Import Keys from Keyserver"));

  if (automatic) {
    this->setFixedSize(240, 42);
  }

  this->setModal(true);

  connect(this, &KeyServerImportDialog::SignalKeyImported,
          SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh);
}

KeyServerImportDialog::KeyServerImportDialog(QWidget* parent)
    : GeneralDialog("key_server_import_dialog", parent), m_automatic_(true) {
  setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

  // Network Waiting
  waiting_bar_ = new QProgressBar();
  waiting_bar_->setVisible(false);
  waiting_bar_->setRange(0, 0);
  waiting_bar_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  waiting_bar_->setTextVisible(false);

  // Layout for messagebox
  auto* layout = new QHBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(waiting_bar_);

  key_server_combo_box_ = create_comboBox();

  this->setLayout(layout);
  this->setWindowTitle(_("Update Keys from Keyserver"));
  this->setFixedSize(240, 42);
  this->setModal(true);
}

QComboBox* KeyServerImportDialog::create_comboBox() {
  auto* comboBox = new QComboBox;
  comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  try {
    SettingsObject key_server_json("key_server");

    const auto key_server_list =
        key_server_json.Check("server_list", nlohmann::json::array());

    for (const auto& key_server : key_server_list) {
      const auto key_server_str = key_server.get<std::string>();
      comboBox->addItem(key_server_str.c_str());
    }

    int default_key_server_index = key_server_json.Check("default_server", 0);
    if (default_key_server_index >= key_server_list.size()) {
      throw std::runtime_error("default_server index out of range");
    }
    std::string default_key_server =
        key_server_list[default_key_server_index].get<std::string>();

    comboBox->setCurrentText(default_key_server.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << "server_list"
               << "default_server";
  }

  return comboBox;
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
  labels << _("UID") << _("Creation date") << _("KeyID") << _("Tag");
  keys_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  keys_table_->setHorizontalHeaderLabels(labels);
  keys_table_->verticalHeader()->hide();

  connect(keys_table_, &QTableWidget::cellActivated, this,
          &KeyServerImportDialog::slot_import);
}

void KeyServerImportDialog::set_message(const QString& text, bool error) {
  if (m_automatic_) return;

  message_->setText(text);
  if (error) {
    icon_->setPixmap(
        QPixmap(":error.png").scaled(QSize(24, 24), Qt::KeepAspectRatio));
  } else {
    icon_->setPixmap(
        QPixmap(":info.png").scaled(QSize(24, 24), Qt::KeepAspectRatio));
  }
}

void KeyServerImportDialog::slot_search() {
  if (search_line_edit_->text().isEmpty()) {
    set_message("<h4>" + QString(_("Text is empty.")) + "</h4>", false);
    return;
  }

  auto* task = new KeyServerSearchTask(
      key_server_combo_box_->currentText().toStdString(),
      search_line_edit_->text().toStdString());

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
}

void KeyServerImportDialog::slot_search_finished(
    QNetworkReply::NetworkError error, QByteArray buffer) {
  LOG(INFO) << "Called" << error << buffer.size();
  LOG(INFO) << buffer.toStdString();

  keys_table_->clearContents();
  keys_table_->setRowCount(0);

  auto stream = QTextStream(buffer);

  if (error != QNetworkReply::NoError) {
    LOG(INFO) << "Error From Reply" << error;

    switch (error) {
      case QNetworkReply::ContentNotFoundError:
        set_message(_("Not Key Found"), true);
        break;
      case QNetworkReply::TimeoutError:
        set_message(_("Timeout"), true);
        break;
      case QNetworkReply::HostNotFoundError:
        set_message(_("Key Server Not Found"), true);
        break;
      default:
        set_message(_("Connection Error"), true);
    }
    return;
  }

  if (stream.readLine().contains("Error")) {
    auto text = stream.readLine(1024);

    if (text.contains("Too many responses")) {
      set_message(
          "<h4>" + QString(_("Too many responses from keyserver!")) + "</h4>",
          true);
      return;
    } else if (text.contains("No keys found")) {
      // if string looks like hex string, search again with 0x prepended
      QRegExp rx("[0-9A-Fa-f]*");
      QString query = search_line_edit_->text();
      if (rx.exactMatch(query)) {
        set_message(
            "<h4>" +
                QString(_("No keys found, input may be kexId, retrying search "
                          "with 0x.")) +
                "</h4>",
            true);
        search_line_edit_->setText(query.prepend("0x"));
        this->slot_search();
        return;
      } else {
        set_message(
            "<h4>" + QString(_("No keys found containing the search string!")) +
                "</h4>",
            true);
        return;
      }
    } else if (text.contains("Insufficiently specific words")) {
      set_message("<h4>" +
                      QString(_("Insufficiently specific search string!")) +
                      "</h4>",
                  true);
      return;
    } else {
      set_message(text, true);
      return;
    }
  } else {
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
            keys_table_->setItem(row, 3,
                                 new QTableWidgetItem(QString(_("revoked"))));
          }
          if (flags.contains("d")) {
            keys_table_->setItem(row, 3,
                                 new QTableWidgetItem(QString(_("disabled"))));
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
        auto* creation_date = new QTableWidgetItem(
            QDateTime::fromTime_t(line[4].toInt()).toString("dd. MMM. yyyy"));
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
              QString(_("%1 keys found. Double click a key to import it."))
                  .arg(row) +
              "</h4>",
          false);
    }
    keys_table_->resizeColumnsToContents();
    import_button_->setDisabled(keys_table_->size().isEmpty());
  }
}

void KeyServerImportDialog::slot_import() {
  std::vector<std::string> key_ids;
  const int row_count = keys_table_->rowCount();
  for (int i = 0; i < row_count; ++i) {
    if (keys_table_->item(i, 2)->isSelected()) {
      QString keyid = keys_table_->item(i, 2)->text();
      key_ids.push_back(keyid.toStdString());
    }
  }
  if (!key_ids.empty())
    SlotImport(key_ids, key_server_combo_box_->currentText().toStdString());
}

void KeyServerImportDialog::SlotImport(const KeyIdArgsListPtr& keys) {
  // keyserver host url
  std::string target_keyserver;

  if (key_server_combo_box_ != nullptr) {
    target_keyserver = key_server_combo_box_->currentText().toStdString();
  }
  if (target_keyserver.empty()) {
    try {
      SettingsObject key_server_json("key_server");
      const auto key_server_list =
          key_server_json.Check("server_list", nlohmann::json::array());

      int default_key_server_index = key_server_json.Check("default_server", 0);
      if (default_key_server_index >= key_server_list.size()) {
        throw std::runtime_error("default_server index out of range");
      }
      std::string default_key_server =
          key_server_list[default_key_server_index].get<std::string>();

      target_keyserver = default_key_server;
    } catch (...) {
      LOG(ERROR) << _("Setting Operation Error") << "server_list"
                 << "default_server";
      QMessageBox::critical(
          nullptr, _("Default Keyserver Not Found"),
          _("Cannot read default keyserver from your settings, "
            "please set a default keyserver first"));
      return;
    }
  }
  std::vector<std::string> key_ids;
  for (const auto& key_id : *keys) {
    key_ids.push_back(key_id);
  }
  SlotImport(key_ids, target_keyserver);
}

void KeyServerImportDialog::SlotImport(std::vector<std::string> key_ids,
                                       std::string keyserver_url) {
  auto* task = new KeyServerImportTask(keyserver_url, key_ids);

  connect(task, &KeyServerImportTask::SignalKeyServerImportResult, this,
          &KeyServerImportDialog::slot_import_finished);

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
      ->PostTask(task);
}

void KeyServerImportDialog::slot_import_finished(
    QNetworkReply::NetworkError error, QByteArray buffer) {
  LOG(INFO) << _("Called");

  if (error != QNetworkReply::NoError) {
    LOG(ERROR) << "Error From Reply" << buffer.toStdString();
    if (!m_automatic_) {
      switch (error) {
        case QNetworkReply::ContentNotFoundError:
          set_message(_("Key Not Found"), true);
          break;
        case QNetworkReply::TimeoutError:
          set_message(_("Timeout"), true);
          break;
        case QNetworkReply::HostNotFoundError:
          set_message(_("Key Server Not Found"), true);
          break;
        default:
          set_message(_("Connection Error"), true);
      }
    } else {
      switch (error) {
        case QNetworkReply::ContentNotFoundError:
          QMessageBox::critical(nullptr, _("Key Not Found"),
                                QString(_("key not found in the Keyserver")));
          break;
        case QNetworkReply::TimeoutError:
          QMessageBox::critical(nullptr, _("Timeout"), "Connection timeout");
          break;
        case QNetworkReply::HostNotFoundError:
          QMessageBox::critical(nullptr, _("Host Not Found"),
                                "cannot resolve the default Keyserver");
          break;
        default:
          QMessageBox::critical(nullptr, _("Connection Error"),
                                _("General Connection Error"));
      }
    }
    if (m_automatic_) {
      setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                     Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    }
    return;
  }

  this->import_keys(
      std::make_unique<ByteArray>(buffer.constData(), buffer.length()));

  if (!m_automatic_) {
    set_message(QString("<h4>") + _("Key Imported") + "</h4>", false);
  }
}

void KeyServerImportDialog::import_keys(ByteArrayPtr in_data) {
  GpgImportInformation result =
      GpgKeyImportExporter::GetInstance().ImportKey(std::move(in_data));

  // refresh the key database
  emit SignalKeyImported();

  QWidget* _parent = qobject_cast<QWidget*>(parent());
  if (m_automatic_) {
    auto dialog = new KeyImportDetailDialog(result, true, _parent);
    dialog->show();
    this->accept();
  } else {
    auto dialog = new KeyImportDetailDialog(result, false, this);
    dialog->exec();
  }
}

void KeyServerImportDialog::set_loading(bool status) {
  waiting_bar_->setVisible(status);
  if (!m_automatic_) {
    icon_->setVisible(!status);
    message_->setVisible(!status);
  }
}

}  // namespace GpgFrontend::UI
