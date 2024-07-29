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

#include "KeyImportDetailDialog.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/GpgImportInformation.h"

namespace GpgFrontend::UI {
KeyImportDetailDialog::KeyImportDetailDialog(
    std::shared_ptr<GpgImportInformation> result, QWidget* parent)
    : GeneralDialog(typeid(KeyImportDetailDialog).name(), parent),
      m_result_(std::move(result)) {
  this->setAttribute(Qt::WA_DeleteOnClose);

  // If no key for import found, just show a message
  if (m_result_ == nullptr || m_result_->considered == 0) {
    QMessageBox::information(parent, tr("Key Import Details"),
                             tr("No keys found to import"));

    this->close();
    return;
  }

  auto* mv_box = new QVBoxLayout();

  this->create_general_info_box();
  mv_box->addWidget(general_info_box_);
  this->create_keys_table();
  mv_box->addWidget(keys_table_);
  this->create_button_box();
  mv_box->addWidget(button_box_);

  this->setLayout(mv_box);
  this->setWindowTitle(tr("Key Import Details"));

  this->setMinimumSize(QSize(600, 300));
  this->adjustSize();

  movePosition2CenterOfParent();
  this->setModal(true);
  this->show();
}

void KeyImportDetailDialog::create_general_info_box() {
  // GridBox for general import information
  general_info_box_ = new QGroupBox(tr("General key info"));
  auto* general_info_box_layout = new QGridLayout(general_info_box_);

  general_info_box_layout->addWidget(new QLabel(tr("Considered") + ": "), 1, 0);
  general_info_box_layout->addWidget(
      new QLabel(QString::number(m_result_->considered)), 1, 1);
  int row = 2;
  if (m_result_->unchanged != 0) {
    general_info_box_layout->addWidget(
        new QLabel(tr("Public unchanged") + ": "), row, 0);
    general_info_box_layout->addWidget(
        new QLabel(QString::number(m_result_->unchanged)), row, 1);
    row++;
  }
  if (m_result_->imported != 0) {
    general_info_box_layout->addWidget(new QLabel(tr("Imported") + ": "), row,
                                       0);
    general_info_box_layout->addWidget(
        new QLabel(QString::number(m_result_->imported)), row, 1);
    row++;
  }
  if (m_result_->not_imported != 0) {
    general_info_box_layout->addWidget(new QLabel(tr("Not Imported") + ": "),
                                       row, 0);
    general_info_box_layout->addWidget(
        new QLabel(QString::number(m_result_->not_imported)), row, 1);
    row++;
  }
  if (m_result_->secret_read != 0) {
    general_info_box_layout->addWidget(new QLabel(tr("Private Read") + ": "),
                                       row, 0);
    general_info_box_layout->addWidget(
        new QLabel(QString::number(m_result_->secret_read)), row, 1);
    row++;
  }
  if (m_result_->secret_imported != 0) {
    general_info_box_layout->addWidget(
        new QLabel(tr("Private Imported") + ": "), row, 0);
    general_info_box_layout->addWidget(
        new QLabel(QString::number(m_result_->secret_imported)), row, 1);
    row++;
  }
  if (m_result_->secret_unchanged != 0) {
    general_info_box_layout->addWidget(
        new QLabel(tr("Private Unchanged") + ": "), row, 0);
    general_info_box_layout->addWidget(
        new QLabel(QString::number(m_result_->secret_unchanged)), row, 1);
  }

  if (m_result_->new_revocations != 0) {
    general_info_box_layout->addWidget(new QLabel(tr("New Revocations") + ": "),
                                       row, 0);
    general_info_box_layout->addWidget(
        new QLabel(QString::number(m_result_->new_revocations)), row, 1);
  }
}

void KeyImportDetailDialog::create_keys_table() {
  keys_table_ = new QTableWidget(this);
  keys_table_->setRowCount(0);
  keys_table_->setColumnCount(4);
  keys_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  // Nothing is selectable
  keys_table_->setSelectionMode(QAbstractItemView::NoSelection);

  QStringList header_labels;
  header_labels << tr("Name") << tr("Email") << tr("Status")
                << tr("Fingerprint");
  keys_table_->verticalHeader()->hide();

  keys_table_->setHorizontalHeaderLabels(header_labels);
  int row = 0;
  for (const auto& imp_key : m_result_->imported_keys) {
    keys_table_->setRowCount(row + 1);
    auto key = GpgKeyGetter::GetInstance().GetKey(imp_key.fpr);
    if (!key.IsGood()) continue;
    keys_table_->setItem(row, 0, new QTableWidgetItem(key.GetName()));
    keys_table_->setItem(row, 1, new QTableWidgetItem(key.GetEmail()));
    keys_table_->setItem(
        row, 2, new QTableWidgetItem(get_status_string(imp_key.import_status)));
    keys_table_->setItem(row, 3, new QTableWidgetItem(imp_key.fpr));
    row++;
  }
  keys_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  keys_table_->horizontalHeader()->setStretchLastSection(true);
  keys_table_->resizeColumnsToContents();
}

auto KeyImportDetailDialog::get_status_string(int key_status) -> QString {
  QString status_string;
  // keystatus is greater than 15, if key is private
  if (key_status > 15) {
    status_string.append(tr("Private"));
    key_status = key_status - 16;
  } else {
    status_string.append(tr("Public"));
  }
  if (key_status == 0) {
    status_string.append(", " + tr("Unchanged"));
  } else {
    if (key_status == 1) {
      status_string.append(", " + tr("New Key"));
    } else {
      if (key_status > 7) {
        status_string.append(", " + tr("New Subkey"));
        return status_string;
      }
      if (key_status > 3) {
        status_string.append(", " + tr("New Signature"));
        return status_string;
      }
      if (key_status > 1) {
        status_string.append(", " + tr("New UID"));
        return status_string;
      }
    }
  }
  return status_string;
}

void KeyImportDetailDialog::create_button_box() {
  button_box_ = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &KeyImportDetailDialog::close);
}
}  // namespace GpgFrontend::UI
