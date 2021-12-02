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

#include "gpg/function/GpgKeyImportExportor.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {

KeyServerImportDialog::KeyServerImportDialog(KeyList* keyList, bool automatic,
                                             QWidget* parent)
    : QDialog(parent),
      mAutomatic(automatic),
      appPath(qApp->applicationDirPath()),
      settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
               QSettings::IniFormat),
      mKeyList(keyList) {
  if (automatic) {
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  }

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

  // Network Waiting
  waitingBar = new QProgressBar();
  waitingBar->setVisible(false);
  waitingBar->setRange(0, 0);
  waitingBar->setFixedWidth(200);

  // Layout for messagebox
  auto* messageLayout = new QHBoxLayout;
  messageLayout->addWidget(icon);
  messageLayout->addWidget(message);
  messageLayout->addWidget(waitingBar);
  messageLayout->addStretch();

  // Layout for import and close button
  auto* buttonsLayout = new QHBoxLayout;
  buttonsLayout->addStretch();
  if (!automatic) buttonsLayout->addWidget(importButton);
  buttonsLayout->addWidget(closeButton);

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
    // Restore window size & location
    if (this->settings.value("ImportKeyFromServer/setWindowSize").toBool()) {
      QPoint pos =
          settings.value("ImportKeyFromServer/pos", QPoint(150, 150)).toPoint();
      QSize size =
          settings.value("ImportKeyFromServer/size", QSize(800, 500)).toSize();
      qDebug() << "Settings size" << size << "pos" << pos;
      this->setMinimumSize(size);
      this->move(pos);
    } else {
      qDebug() << "Use default min windows size and pos";
      QPoint defaultPoint(150, 150);
      QSize defaultMinSize(500, 300);
      this->setMinimumSize(defaultMinSize);
      this->move(defaultPoint);
      this->settings.setValue("ImportKeyFromServer/pos", defaultPoint);
      this->settings.setValue("ImportKeyFromServer/size", defaultMinSize);
      this->settings.setValue("ImportKeyFromServer/setWindowSize", true);
    }
  }

  this->setModal(true);

  connect(this, SIGNAL(signalKeyImported()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));
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

  // Read keylist from ini-file and fill it into combobox
  comboBox->addItems(settings.value("keyserver/keyServerList").toStringList());

  // set default keyserver in combobox
  QString keyserver = settings.value("keyserver/defaultKeyServer").toString();
  comboBox->setCurrentIndex(comboBox->findText(keyserver));

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
  LOG(INFO) << "KeyServerImportDialog::slotImport currentRow"
            << keysTable->currentRow();
  if (keysTable->currentRow() > -1) {
    QString keyid = keysTable->item(keysTable->currentRow(), 2)->text();
    slotImport(QStringList(keyid), keyServerComboBox->currentText());
  }
}

void KeyServerImportDialog::slotImport(const KeyIdArgsListPtr& keys) {
  QString keyserver = settings.value("keyserver/defaultKeyServer").toString();
  qDebug() << "Select Key Server" << keyserver;
  auto key_ids = QStringList();
  for (const auto& key_id : *keys) {
    key_ids.append(QString::fromStdString(key_id));
  }
  slotImport(key_ids, QUrl(keyserver));
}

void KeyServerImportDialog::slotImportKey(const KeyIdArgsListPtr& keys) {
  QString keyserver = settings.value("keyserver/defaultKeyServer").toString();
  qDebug() << "Select Key Server" << keyserver;
  auto key_ids = QStringList();
  for (const auto& key_id : *keys) {
    key_ids.append(QString::fromStdString(key_id));
  }
  slotImport(key_ids, QUrl(keyserver));
}

void KeyServerImportDialog::slotImport(const QStringList& keyIds,
                                       const QUrl& keyServerUrl) {
  for (const auto& keyId : keyIds) {
    QUrl reqUrl(keyServerUrl.scheme() + "://" + keyServerUrl.host() +
                "/pks/lookup?op=get&search=0x" + keyId + "&options=mr");
    qDebug() << "slotImport reqUrl" << reqUrl;
    auto pManager = new QNetworkAccessManager(this);

    QNetworkReply* reply = pManager->get(QNetworkRequest(reqUrl));

    connect(reply, SIGNAL(finished()), this, SLOT(slotImportFinished()));

    setLoading(true);

    while (reply->isRunning()) {
      QApplication::processEvents();
    }

    setLoading(false);
  }
}

void KeyServerImportDialog::slotImportFinished() {
  auto* reply = qobject_cast<QNetworkReply*>(sender());

  QByteArray key = reply->readAll();

  QVariant redirectionTarget =
      reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

  auto error = reply->error();
  if (error != QNetworkReply::NoError) {
    qDebug() << "Error From Reply" << reply->errorString();
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
    if (mAutomatic) {
      setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                     Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    }
    return;
  }

  // Add keyserver to list in config-file, if it isn't contained
  QStringList keyServerList =
      settings.value("keyserver/keyServerList").toStringList();
  if (!keyServerList.contains(keyServerComboBox->currentText())) {
    keyServerList.append(keyServerComboBox->currentText());
    settings.setValue("keyserver/keyServerList", keyServerList);
  }
  reply->deleteLater();

  this->importKeys(std::make_unique<ByteArray>(key.constData(), key.length()));

  if (mAutomatic) {
    setMessage(QString("<h4>") + _("Key Updated") + "</h4>", false);
  } else {
    setMessage(QString("<h4>") + _("Key Imported") + "</h4>", false);
  }
}

void KeyServerImportDialog::importKeys(ByteArrayPtr in_data) {
  GpgImportInformation result =
      GpgKeyImportExportor::GetInstance().ImportKey(std::move(in_data));
  emit signalKeyImported();
  if (mAutomatic) {
    new KeyImportDetailDialog(result, true, this);
    this->accept();
  } else {
    new KeyImportDetailDialog(result, false, this);
  }
}

void KeyServerImportDialog::setLoading(bool status) {
  if (status) {
    waitingBar->setVisible(true);
    icon->setVisible(false);
    message->setVisible(false);
  } else {
    waitingBar->setVisible(false);
    icon->setVisible(true);
    message->setVisible(true);
  }
}

KeyServerImportDialog::KeyServerImportDialog(QWidget* parent)
    : QDialog(parent),
      mAutomatic(true),
      appPath(qApp->applicationDirPath()),
      settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
               QSettings::IniFormat) {
  setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

  message = new QLabel;
  message->setFixedHeight(24);
  icon = new QLabel;
  icon->setFixedHeight(24);

  // Network Waiting
  waitingBar = new QProgressBar();
  waitingBar->setVisible(false);
  waitingBar->setRange(0, 0);
  waitingBar->setFixedHeight(24);
  waitingBar->setFixedWidth(200);

  // Layout for messagebox
  auto* messageLayout = new QHBoxLayout;
  messageLayout->addWidget(icon);
  messageLayout->addWidget(message);
  messageLayout->addWidget(waitingBar);
  messageLayout->addStretch();

  keyServerComboBox = createComboBox();

  auto* mainLayout = new QGridLayout;

  mainLayout->addLayout(messageLayout, 0, 0, 1, 3);

  this->setLayout(mainLayout);
  this->setWindowTitle(_("Upload Keys from Keyserver"));
  this->setFixedSize(200, 42);
  this->setModal(true);
}

}  // namespace GpgFrontend::UI
