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

#include "ui/KeyImportDetailDialog.h"

#include "gpg/function/GpgKeyGetter.h"

namespace GpgFrontend::UI {
KeyImportDetailDialog::KeyImportDetailDialog(GpgImportInformation result,
                                             bool automatic, QWidget* parent)
    : QDialog(parent), mResult(std::move(result)) {
  // If no key for import found, just show a message
  if (mResult.considered == 0) {
    if (automatic)
      QMessageBox::information(parent, _("Key Update Details"),
                               _("No keys found"));
    else
      QMessageBox::information(parent, _("Key Import Details"),
                               _("No keys found to import"));
    emit finished(0);
    this->close();
    this->deleteLater();
  } else {
    auto* mv_box = new QVBoxLayout();

    this->createGeneralInfoBox();
    mv_box->addWidget(generalInfoBox);

    this->createKeysTable();
    mv_box->addWidget(keysTable);

    this->createButtonBox();
    mv_box->addWidget(buttonBox);

    this->setLayout(mv_box);
    if (automatic)
      this->setWindowTitle(_("Key Update Details"));
    else
      this->setWindowTitle(_("Key Import Details"));

    auto pos = QPoint(100, 100);
    LOG(INFO) << "parent" << parent;
    if (parent) pos += parent->pos();
    this->move(pos);
    this->resize(QSize(600, 300));
    this->setModal(true);
    this->show();
  }
}

void KeyImportDetailDialog::createGeneralInfoBox() {
  // GridBox for general import information
  generalInfoBox = new QGroupBox(_("General key info"));
  auto* generalInfoBoxLayout = new QGridLayout(generalInfoBox);

  generalInfoBoxLayout->addWidget(new QLabel(QString(_("Considered")) + ": "),
                                  1, 0);
  generalInfoBoxLayout->addWidget(
      new QLabel(QString::number(mResult.considered)), 1, 1);
  int row = 2;
  if (mResult.unchanged != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Public unchanged")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(mResult.unchanged)), row, 1);
    row++;
  }
  if (mResult.imported != 0) {
    generalInfoBoxLayout->addWidget(new QLabel(QString(_("Imported")) + ": "),
                                    row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(mResult.imported)), row, 1);
    row++;
  }
  if (mResult.not_imported != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Not Imported")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(mResult.not_imported)), row, 1);
    row++;
  }
  if (mResult.secret_read != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Private Read")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(mResult.secret_read)), row, 1);
    row++;
  }
  if (mResult.secret_imported != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Private Imported")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(mResult.secret_imported)), row, 1);
    row++;
  }
  if (mResult.secret_unchanged != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Private Unchanged")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(mResult.secret_unchanged)), row, 1);
    row++;
  }
}

void KeyImportDetailDialog::createKeysTable() {
  LOG(INFO) << "KeyImportDetailDialog::createKeysTable() Called";

  keysTable = new QTableWidget(this);
  keysTable->setRowCount(0);
  keysTable->setColumnCount(4);
  keysTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  // Nothing is selectable
  keysTable->setSelectionMode(QAbstractItemView::NoSelection);

  QStringList headerLabels;
  headerLabels << _("Name") << _("Email") << _("Status") << _("Fingerprint");
  keysTable->verticalHeader()->hide();

  keysTable->setHorizontalHeaderLabels(headerLabels);
  int row = 0;
  for (const auto& imp_key : mResult.importedKeys) {
    keysTable->setRowCount(row + 1);
    GpgKey key = GpgKeyGetter::GetInstance().GetKey(imp_key.fpr);
    if (!key.good()) continue;
    keysTable->setItem(
        row, 0, new QTableWidgetItem(QString::fromStdString(key.name())));
    keysTable->setItem(
        row, 1, new QTableWidgetItem(QString::fromStdString(key.email())));
    keysTable->setItem(
        row, 2, new QTableWidgetItem(getStatusString(imp_key.import_status)));
    keysTable->setItem(
        row, 3, new QTableWidgetItem(QString::fromStdString(imp_key.fpr)));
    row++;
  }
  keysTable->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  keysTable->horizontalHeader()->setStretchLastSection(true);
  keysTable->resizeColumnsToContents();
}

QString KeyImportDetailDialog::getStatusString(int keyStatus) {
  QString statusString;
  // keystatus is greater than 15, if key is private
  if (keyStatus > 15) {
    statusString.append(_("Private"));
    keyStatus = keyStatus - 16;
  } else {
    statusString.append(_("Public"));
  }
  if (keyStatus == 0) {
    statusString.append(", " + QString(_("Unchanged")));
  } else {
    if (keyStatus == 1) {
      statusString.append(", " + QString(_("New Key")));
    } else {
      if (keyStatus > 7) {
        statusString.append(", " + QString(_("New Subkey")));
        keyStatus = keyStatus - 8;
      }
      if (keyStatus > 3) {
        statusString.append(", " + QString(_("New Signature")));
        keyStatus = keyStatus - 4;
      }
      if (keyStatus > 1) {
        statusString.append(", " + QString(_("New UID")));
        keyStatus = keyStatus - 2;
      }
    }
  }
  return statusString;
}

void KeyImportDetailDialog::createButtonBox() {
  buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(close()));
}
}  // namespace GpgFrontend::UI
