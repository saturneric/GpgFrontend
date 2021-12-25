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

#include "ui/KeyServerImportDialog.h"

#include <utility>

#include "gpg/function/GpgKeyImportExporter.h"
#include "ui/SignalStation.h"
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

KeyServerImportDialog::KeyServerImportDialog(bool automatic, QWidget* parent)
    : QDialog(parent), mAutomatic(automatic) {
  // Layout for messagebox
  auto* messageLayout = new QHBoxLayout;

  if (automatic) {
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  } else {
    // Buttons
    closeButton = createButton(_("Close"), SLOT(close()));
    importButton = createButton(_("Import ALL"), SLOT(slotImport()));
    importButton->setDisabled(true);
    searchButton = createButton(_("Search"), SLOT(slotSearch()));

    // Line edit for search string
    searchLabel = new QLabel(QString(_("Search String")) + _(": "));
    searchLineEdit = new QLineEdit();

    // combobox for keyserverlist
    keyServerLabel = new QLabel(QString(_("Key Server")) + _(": "));
    keyServerComboBox = createComboBox();

    // table containing the keys found
    createKeysTable();
    message = new QLabel;
    message->setFixedHeight(24);
    icon = new QLabel;
    icon->setFixedHeight(24);

    messageLayout->addWidget(icon);
    messageLayout->addWidget(message);
    messageLayout->addStretch();
  }

  // Network Waiting
  waitingBar = new QProgressBar();
  waitingBar->setVisible(false);
  waitingBar->setRange(0, 0);
  waitingBar->setFixedWidth(200);
  messageLayout->addWidget(waitingBar);

  auto* mainLayout = new QGridLayout;

  // 自动化调用界面布局
  if (automatic) {
    mainLayout->addLayout(messageLayout, 0, 0, 1, 3);
  } else {
    mainLayout->addWidget(searchLabel, 1, 0);
    mainLayout->addWidget(searchLineEdit, 1, 1);
    mainLayout->addWidget(searchButton, 1, 2);
    mainLayout->addWidget(keyServerLabel, 2, 0);
    mainLayout->addWidget(keyServerComboBox, 2, 1);
    mainLayout->addWidget(keysTable, 3, 0, 1, 3);
    mainLayout->addLayout(messageLayout, 4, 0, 1, 3);

    // Layout for import and close button
    auto* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(importButton);
    buttonsLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonsLayout, 6, 0, 1, 3);
  }

  this->setLayout(mainLayout);
  if (automatic)
    this->setWindowTitle(_("Update Keys from Keyserver"));
  else
    this->setWindowTitle(_("Import Keys from Keyserver"));

  if (automatic) {
    this->setFixedSize(240, 42);
  } else {
    auto pos = QPoint(150, 150);
    LOG(INFO) << "parent" << parent;
    if (parent) pos += parent->pos();
    LOG(INFO) << "pos default" << pos.x() << pos.y();
    auto size = QSize(800, 500);

    try {
      auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

      int x, y, width, height;
      x = settings.lookup("window.import_from_keyserver.position.x");
      y = settings.lookup("window.import_from_keyserver.position.y");
      width = settings.lookup("window.import_from_keyserver.size.width");
      height = settings.lookup("window.import_from_keyserver.size.height");
      pos = QPoint(x, y);
      size = QSize(width, height);

    } catch (...) {
      LOG(WARNING) << "cannot read pos or size from settings";
    }

    this->resize(size);
    this->move(pos);
  }

  this->setModal(true);

  connect(this, SIGNAL(signalKeyImported()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));

  // save window pos and size to configure file
  connect(this, SIGNAL(finished(int)), this, SLOT(slotSaveWindowState()));
}

QPushButton* KeyServerImportDialog::createButton(const QString& text,
                                                 const char* member) {
  auto* button = new QPushButton(text);
  connect(button, SIGNAL(clicked()), this, member);
  return button;
}

QComboBox* KeyServerImportDialog::createComboBox() {
  auto* comboBox = new QComboBox;
  comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    auto& server_list = settings.lookup("keyserver.server_list");
    const auto server_list_size = server_list.getLength();
    for (int i = 0; i < server_list_size; i++) {
      std::string server_url = server_list[i];
      comboBox->addItem(server_url.c_str());
    }
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("server_list");
  }

  // set default keyserver in combobox
  try {
    std::string default_server = settings.lookup("keyserver.default_server");
    comboBox->setCurrentText(default_server.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("default_server");
  }

  return comboBox;
}

void KeyServerImportDialog::createKeysTable() {
  keysTable = new QTableWidget();
  keysTable->setColumnCount(4);

  // always a whole row is marked
  keysTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  keysTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Make just one row selectable
  keysTable->setSelectionMode(QAbstractItemView::SingleSelection);

  QStringList labels;
  labels << _("UID") << _("Creation date") << _("KeyID") << _("Tag");
  keysTable->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  keysTable->setHorizontalHeaderLabels(labels);
  keysTable->verticalHeader()->hide();

  connect(keysTable, SIGNAL(cellActivated(int, int)), this, SLOT(slotImport()));
}

void KeyServerImportDialog::setMessage(const QString& text, bool error) {
  if (mAutomatic) return;

  message->setText(text);
  if (error) {
    icon->setPixmap(
        QPixmap(":error.png").scaled(QSize(24, 24), Qt::KeepAspectRatio));
  } else {
    icon->setPixmap(
        QPixmap(":info.png").scaled(QSize(24, 24), Qt::KeepAspectRatio));
  }
}

void KeyServerImportDialog::slotSearch() {
  if (searchLineEdit->text().isEmpty()) {
    setMessage("<h4>" + QString(_("Text is empty.")) + "</h4>", false);
    return;
  }

  QUrl url_from_remote = keyServerComboBox->currentText() +
                         "/pks/lookup?search=" + searchLineEdit->text() +
                         "&op=index&options=mr";
  qnam = new QNetworkAccessManager(this);
  QNetworkReply* reply = qnam->get(QNetworkRequest(url_from_remote));

  connect(reply, SIGNAL(finished()), this, SLOT(slotSearchFinished()));

  setLoading(true);
  this->searchButton->setDisabled(true);
  this->keyServerComboBox->setDisabled(true);
  this->searchLineEdit->setReadOnly(true);
  this->importButton->setDisabled(true);

  while (reply->isRunning()) {
    QApplication::processEvents();
  }

  this->searchButton->setDisabled(false);
  this->keyServerComboBox->setDisabled(false);
  this->searchLineEdit->setReadOnly(false);
  this->importButton->setDisabled(false);
  setLoading(false);
}

void KeyServerImportDialog::slotSearchFinished() {
  LOG(INFO) << "KeyServerImportDialog::slotSearchFinished Called";

  auto* reply = qobject_cast<QNetworkReply*>(sender());

  keysTable->clearContents();
  keysTable->setRowCount(0);
  QString first_line = QString(reply->readLine(1024));

  auto error = reply->error();
  if (error != QNetworkReply::NoError) {
    qDebug() << "Error From Reply" << reply->errorString();
    switch (error) {
      case QNetworkReply::ContentNotFoundError:
        setMessage(_("Not Key Found"), true);
        break;
      case QNetworkReply::TimeoutError:
        setMessage(_("Timeout"), true);
        break;
      case QNetworkReply::HostNotFoundError:
        setMessage(_("Key Server Not Found"), true);
        break;
      default:
        setMessage(_("Connection Error"), true);
    }
    return;
  }

  if (first_line.contains("Error")) {
    QString text = QString(reply->readLine(1024));
    if (text.contains("Too many responses")) {
      setMessage(
          "<h4>" + QString(_("Too many responses from keyserver!")) + "</h4>",
          true);
      return;
    } else if (text.contains("No keys found")) {
      // if string looks like hex string, search again with 0x prepended
      QRegExp rx("[0-9A-Fa-f]*");
      QString query = searchLineEdit->text();
      if (rx.exactMatch(query)) {
        setMessage(
            "<h4>" +
                QString(_("No keys found, input may be kexId, retrying search "
                          "with 0x.")) +
                "</h4>",
            true);
        searchLineEdit->setText(query.prepend("0x"));
        this->slotSearch();
        return;
      } else {
        setMessage(
            "<h4>" + QString(_("No keys found containing the search string!")) +
                "</h4>",
            true);
        return;
      }
    } else if (text.contains("Insufficiently specific words")) {
      setMessage("<h4>" + QString(_("Insufficiently specific search string!")) +
                     "</h4>",
                 true);
      return;
    } else {
      setMessage(text, true);
      return;
    }
  } else {
    int row = 0;
    bool strikeout = false;
    while (reply->canReadLine()) {
      auto line_buff = reply->readLine().trimmed();
      QString decoded =
          QString::fromUtf8(line_buff.constData(), line_buff.size());
      QStringList line = decoded.split(":");
      // TODO: have a look at two following pub lines
      if (line[0] == "pub") {
        strikeout = false;

        QString flags = line[line.size() - 1];
        keysTable->setRowCount(row + 1);

        // flags can be "d" for disabled, "r" for revoked
        // or "e" for expired
        if (flags.contains("r") or flags.contains("d") or flags.contains("e")) {
          strikeout = true;
          if (flags.contains("e")) {
            keysTable->setItem(row, 3,
                               new QTableWidgetItem(QString("expired")));
          }
          if (flags.contains("r")) {
            keysTable->setItem(row, 3,
                               new QTableWidgetItem(QString(_("revoked"))));
          }
          if (flags.contains("d")) {
            keysTable->setItem(row, 3,
                               new QTableWidgetItem(QString(_("disabled"))));
          }
        }

        QStringList line2 = QString(reply->readLine()).split(":");

        auto* uid = new QTableWidgetItem();
        if (line2.size() > 1) {
          uid->setText(line2[1]);
          keysTable->setItem(row, 0, uid);
        }
        auto* creation_date = new QTableWidgetItem(
            QDateTime::fromTime_t(line[4].toInt()).toString("dd. MMM. yyyy"));
        keysTable->setItem(row, 1, creation_date);
        auto* keyid = new QTableWidgetItem(line[1]);
        keysTable->setItem(row, 2, keyid);
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
          int height = keysTable->rowHeight(row - 1);
          keysTable->setRowHeight(row - 1, height + 16);
          QString tmp = keysTable->item(row - 1, 0)->text();
          tmp.append(QString("\n") + line[1]);
          auto* tmp1 = new QTableWidgetItem(tmp);
          keysTable->setItem(row - 1, 0, tmp1);
          if (strikeout) {
            QFont strike = tmp1->font();
            strike.setStrikeOut(true);
            tmp1->setFont(strike);
          }
        }
      }
      setMessage(
          QString("<h4>") +
              QString(_("%1 keys found. Double click a key to import it."))
                  .arg(row) +
              "</h4>",
          false);
    }
    keysTable->resizeColumnsToContents();
    importButton->setDisabled(keysTable->size().isEmpty());
  }
  reply->deleteLater();
}

void KeyServerImportDialog::slotImport() {
  LOG(INFO) << _("Current Row") << keysTable->currentRow();
  if (keysTable->currentRow() > -1) {
    QString keyid = keysTable->item(keysTable->currentRow(), 2)->text();
    slotImport(QStringList(keyid), keyServerComboBox->currentText());
  }
}

void KeyServerImportDialog::slotImport(const KeyIdArgsListPtr& keys) {
  std::string target_keyserver;
  if (keyServerComboBox != nullptr) {
    target_keyserver = keyServerComboBox->currentText().toStdString();
  }
  if (target_keyserver.empty()) {
    try {
      auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

      target_keyserver = settings.lookup("keyserver.default_server").c_str();

      LOG(INFO) << _("Set target Key Server to default Key Server")
                << target_keyserver;
    } catch (...) {
      LOG(ERROR) << _("Cannot read default_keyserver From Settings");
      QMessageBox::critical(
          nullptr, _("Default Keyserver Not Found"),
          _("Cannot read default keyserver from your settings, "
            "please set a default keyserver first"));
      return;
    }
  }
  auto key_ids = QStringList();
  for (const auto& key_id : *keys)
    key_ids.append(QString::fromStdString(key_id));
  slotImport(key_ids, QUrl(target_keyserver.c_str()));
}

void KeyServerImportDialog::slotImport(const QStringList& keyIds,
                                       const QUrl& keyServerUrl) {
  for (const auto& keyId : keyIds) {
    QUrl req_url(keyServerUrl.scheme() + "://" + keyServerUrl.host() +
                 "/pks/lookup?op=get&search=0x" + keyId + "&options=mr");

    LOG(INFO) << "request url" << req_url.toString().toStdString();
    auto manager = new QNetworkAccessManager(this);

    QNetworkReply* reply = manager->get(QNetworkRequest(req_url));
    connect(reply, &QNetworkReply::finished, this,
            [&, keyId]() { this->slotImportFinished(keyId); });
    LOG(INFO) << "loading start";
    setLoading(true);
    while (reply->isRunning()) QApplication::processEvents();
    setLoading(false);
    LOG(INFO) << "loading done";
  }
}

void KeyServerImportDialog::slotImportFinished(QString keyid) {
  LOG(INFO) << _("Called");

  auto* reply = qobject_cast<QNetworkReply*>(sender());

  QByteArray key = reply->readAll();

  auto error = reply->error();
  if (error != QNetworkReply::NoError) {
    LOG(ERROR) << "Error From Reply" << reply->errorString().toStdString();
    if (!mAutomatic) {
      switch (error) {
        case QNetworkReply::ContentNotFoundError:
          setMessage(_("Key Not Found"), true);
          break;
        case QNetworkReply::TimeoutError:
          setMessage(_("Timeout"), true);
          break;
        case QNetworkReply::HostNotFoundError:
          setMessage(_("Key Server Not Found"), true);
          break;
        default:
          setMessage(_("Connection Error"), true);
      }
    } else {
      switch (error) {
        case QNetworkReply::ContentNotFoundError:
          QMessageBox::critical(
              nullptr, _("Public key Not Found"),
              QString(_("Public key fingerprint %1 not found in the Keyserver"))
                  .arg(keyid));
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
    if (mAutomatic) {
      setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                     Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    }
    return;
  }

  reply->deleteLater();

  this->importKeys(std::make_unique<ByteArray>(key.constData(), key.length()));

  if (!mAutomatic) {
    setMessage(QString("<h4>") + _("Key Imported") + "</h4>", false);
  }
}

void KeyServerImportDialog::importKeys(ByteArrayPtr in_data) {
  GpgImportInformation result =
      GpgKeyImportExportor::GetInstance().ImportKey(std::move(in_data));
  emit signalKeyImported();
  QWidget* _parent = qobject_cast<QWidget*>(parent());
  if (mAutomatic) {
    auto dialog = new KeyImportDetailDialog(result, true, _parent);
    dialog->show();
    this->accept();
  } else {
    auto dialog = new KeyImportDetailDialog(result, false, _parent);
    dialog->exec();
  }
}

void KeyServerImportDialog::setLoading(bool status) {
  waitingBar->setVisible(status);
  if (!mAutomatic) {
    icon->setVisible(!status);
    message->setVisible(!status);
  }
}

KeyServerImportDialog::KeyServerImportDialog(QWidget* parent)
    : QDialog(parent), mAutomatic(true) {
  setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

  // Network Waiting
  waitingBar = new QProgressBar();
  waitingBar->setVisible(false);
  waitingBar->setRange(0, 0);
  waitingBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  waitingBar->setTextVisible(false);

  // Layout for messagebox
  auto* layout = new QHBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(waitingBar);

  keyServerComboBox = createComboBox();

  this->setLayout(layout);
  this->setWindowTitle(_("Update Keys from Keyserver"));
  this->setFixedSize(240, 42);
  this->setModal(true);
}

void KeyServerImportDialog::slotSaveWindowState() {
  LOG(INFO) << _("Called");

  if (mAutomatic) return;

  auto& settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("window") ||
      settings.lookup("window").getType() != libconfig::Setting::TypeGroup)
    settings.add("window", libconfig::Setting::TypeGroup);

  auto& window = settings["window"];

  if (!window.exists("import_from_keyserver") ||
      window.lookup("import_from_keyserver").getType() !=
          libconfig::Setting::TypeGroup)
    window.add("import_from_keyserver", libconfig::Setting::TypeGroup);

  auto& import_from_keyserver = window["import_from_keyserver"];

  if (!import_from_keyserver.exists("position") ||
      import_from_keyserver.lookup("position").getType() !=
          libconfig::Setting::TypeGroup) {
    auto& position =
        import_from_keyserver.add("position", libconfig::Setting::TypeGroup);
    position.add("x", libconfig::Setting::TypeInt) = pos().x();
    position.add("y", libconfig::Setting::TypeInt) = pos().y();
  } else {
    import_from_keyserver["position"]["x"] = pos().x();
    import_from_keyserver["position"]["y"] = pos().y();
  }

  if (!import_from_keyserver.exists("size") ||
      import_from_keyserver.lookup("size").getType() !=
          libconfig::Setting::TypeGroup) {
    auto& size =
        import_from_keyserver.add("size", libconfig::Setting::TypeGroup);
    size.add("width", libconfig::Setting::TypeInt) = QWidget::width();
    size.add("height", libconfig::Setting::TypeInt) = QWidget::height();
  } else {
    import_from_keyserver["size"]["width"] = QWidget::width();
    import_from_keyserver["size"]["height"] = QWidget::height();
  }

  GlobalSettingStation::GetInstance().Sync();
}

}  // namespace GpgFrontend::UI
