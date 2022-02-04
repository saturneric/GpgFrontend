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

#include "KeyImportDetailDialog.h"

#include "core/function/GpgKeyGetter.h"

namespace GpgFrontend::UI {
KeyImportDetailDialog::KeyImportDetailDialog(GpgImportInformation result,
                                             bool automatic, QWidget* parent)
    : QDialog(parent), m_result_(std::move(result)) {
  // If no key for import found, just show a message
  if (m_result_.considered == 0) {
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

    this->create_general_info_box();
    mv_box->addWidget(general_info_box_);
    this->create_keys_table();
    mv_box->addWidget(keys_table_);
    this->create_button_box();
    mv_box->addWidget(button_box_);

    this->setLayout(mv_box);
    if (automatic)
      this->setWindowTitle(_("Key Update Details"));
    else
      this->setWindowTitle(_("Key Import Details"));

    auto pos = QPoint(100, 100);
    if (parent) pos += parent->pos();
    this->move(pos);

    this->setMinimumSize(QSize(600, 300));
    this->adjustSize();

    this->setModal(true);
    this->show();
  }
}

void KeyImportDetailDialog::create_general_info_box() {
  // GridBox for general import information
  general_info_box_ = new QGroupBox(_("General key info"));
  auto* generalInfoBoxLayout = new QGridLayout(general_info_box_);

  generalInfoBoxLayout->addWidget(new QLabel(QString(_("Considered")) + ": "),
                                  1, 0);
  generalInfoBoxLayout->addWidget(
      new QLabel(QString::number(m_result_.considered)), 1, 1);
  int row = 2;
  if (m_result_.unchanged != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Public unchanged")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(m_result_.unchanged)), row, 1);
    row++;
  }
  if (m_result_.imported != 0) {
    generalInfoBoxLayout->addWidget(new QLabel(QString(_("Imported")) + ": "),
                                    row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(m_result_.imported)), row, 1);
    row++;
  }
  if (m_result_.not_imported != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Not Imported")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(m_result_.not_imported)), row, 1);
    row++;
  }
  if (m_result_.secret_read != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Private Read")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(m_result_.secret_read)), row, 1);
    row++;
  }
  if (m_result_.secret_imported != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Private Imported")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(m_result_.secret_imported)), row, 1);
    row++;
  }
  if (m_result_.secret_unchanged != 0) {
    generalInfoBoxLayout->addWidget(
        new QLabel(QString(_("Private Unchanged")) + ": "), row, 0);
    generalInfoBoxLayout->addWidget(
        new QLabel(QString::number(m_result_.secret_unchanged)), row, 1);
    row++;
  }
}

void KeyImportDetailDialog::create_keys_table() {
  LOG(INFO) << "KeyImportDetailDialog::create_keys_table() Called";

  keys_table_ = new QTableWidget(this);
  keys_table_->setRowCount(0);
  keys_table_->setColumnCount(4);
  keys_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  // Nothing is selectable
  keys_table_->setSelectionMode(QAbstractItemView::NoSelection);

  QStringList headerLabels;
  headerLabels << _("Name") << _("Email") << _("Status") << _("Fingerprint");
  keys_table_->verticalHeader()->hide();

  keys_table_->setHorizontalHeaderLabels(headerLabels);
  int row = 0;
  for (const auto& imp_key : m_result_.importedKeys) {
    keys_table_->setRowCount(row + 1);
    GpgKey key = GpgKeyGetter::GetInstance().GetKey(imp_key.fpr);
    if (!key.IsGood()) continue;
    keys_table_->setItem(
        row, 0, new QTableWidgetItem(QString::fromStdString(key.GetName())));
    keys_table_->setItem(
        row, 1, new QTableWidgetItem(QString::fromStdString(key.GetEmail())));
    keys_table_->setItem(
        row, 2, new QTableWidgetItem(get_status_string(imp_key.import_status)));
    keys_table_->setItem(
        row, 3, new QTableWidgetItem(QString::fromStdString(imp_key.fpr)));
    row++;
  }
  keys_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  keys_table_->horizontalHeader()->setStretchLastSection(true);
  keys_table_->resizeColumnsToContents();
}

QString KeyImportDetailDialog::get_status_string(int keyStatus) {
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

void KeyImportDetailDialog::create_button_box() {
  button_box_ = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(button_box_, &QDialogButtonBox::accepted, this, &KeyImportDetailDialog::close);
}
}  // namespace GpgFrontend::UI
