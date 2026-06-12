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

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/model/GpgImportInformation.h"

namespace GpgFrontend::UI {
KeyImportDetailDialog::KeyImportDetailDialog(
    int channel, QSharedPointer<GpgImportInformation> result, QWidget* parent)
    : GeneralDialog(typeid(KeyImportDetailDialog).name(), parent),
      current_gpg_context_channel_(channel),
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
  mv_box->setContentsMargins(16, 16, 16, 12);
  mv_box->setSpacing(12);

  this->create_header_box();
  mv_box->addWidget(header_box_);
  this->create_general_info_box();
  mv_box->addWidget(general_info_box_);
  this->create_keys_table();
  mv_box->addWidget(keys_table_);
  this->create_button_box();
  mv_box->addWidget(button_box_);

  this->setLayout(mv_box);
  this->setWindowTitle(tr("Key Import Details"));

  this->setMinimumSize(QSize(680, 420));
  this->adjustSize();

  this->setModal(true);

  this->show();
  this->raise();
  this->activateWindow();
}

void KeyImportDetailDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);
  this->movePosition2CenterOfParent();
}

void KeyImportDetailDialog::create_header_box() {
  header_box_ = new QFrame(this);
  header_box_->setObjectName("importSummaryFrame");
  header_box_->setFrameShape(QFrame::StyledPanel);
  header_box_->setFrameShadow(QFrame::Plain);

  auto* header_layout = new QHBoxLayout(header_box_);
  header_layout->setContentsMargins(14, 12, 14, 12);
  header_layout->setSpacing(14);

  const auto imported_total =
      m_result_->imported + m_result_->secret_imported;
  const auto updated_total = m_result_->new_user_ids +
                             m_result_->new_sub_keys +
                             m_result_->new_signatures;

  QString icon_path = ":/icons/info.png";
  QString title;
  QString summary;

  if (m_result_->not_imported != 0) {
    icon_path = ":/icons/warning.png";
    title = tr("Import Completed with Issues");
    summary = tr("%1 of %2 key(s) could not be imported.")
                  .arg(m_result_->not_imported)
                  .arg(m_result_->considered);
  } else if (imported_total != 0 || updated_total != 0) {
    icon_path = ":/icons/check.png";
    title = tr("Import Successful");
    summary =
        tr("Successfully processed %1 key(s).").arg(m_result_->considered);
  } else {
    title = tr("Nothing to Import");
    summary = tr("All %1 key(s) are already up to date.")
                  .arg(m_result_->considered);
  }

  auto* icon_label = new QLabel(header_box_);
  icon_label->setPixmap(QPixmap(icon_path).scaled(
      40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  icon_label->setFixedSize(40, 40);

  auto* text_layout = new QVBoxLayout();
  text_layout->setSpacing(4);

  auto* title_label = new QLabel(title, header_box_);
  auto title_font = title_label->font();
  title_font.setPointSize(title_font.pointSize() + 2);
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* summary_label = new QLabel(summary, header_box_);
  summary_label->setWordWrap(true);

  text_layout->addWidget(title_label);
  text_layout->addWidget(summary_label);

  header_layout->addWidget(icon_label, 0, Qt::AlignTop);
  header_layout->addLayout(text_layout, 1);
}

void KeyImportDetailDialog::create_general_info_box() {
  general_info_box_ = new QGroupBox(tr("Summary"), this);

  auto* layout = new QGridLayout(general_info_box_);
  layout->setHorizontalSpacing(28);
  layout->setVerticalSpacing(10);

  struct StatEntry {
    QString label;
    int value;
    QString color;
  };

  static const auto kGreen = QStringLiteral("#43a047");
  static const auto kBlue = QStringLiteral("#1e88e5");
  static const auto kGray = QStringLiteral("#757575");
  static const auto kRed = QStringLiteral("#e53935");

  const QList<StatEntry> entries = {
      {tr("Considered"), m_result_->considered, kGray},
      {tr("Imported"), m_result_->imported, kGreen},
      {tr("Unchanged"), m_result_->unchanged, kGray},
      {tr("Not Imported"), m_result_->not_imported, kRed},
      {tr("Private Keys Read"), m_result_->secret_read, kBlue},
      {tr("Private Keys Imported"), m_result_->secret_imported, kGreen},
      {tr("Private Keys Unchanged"), m_result_->secret_unchanged, kGray},
      {tr("New Revocations"), m_result_->new_revocations, kRed},
  };

  static const int kColumns = 4;
  int col = 0;
  int row = 0;
  for (const auto& entry : entries) {
    if (entry.value == 0 && entry.label != tr("Considered")) continue;

    auto* cell = new QWidget(general_info_box_);
    auto* cell_layout = new QVBoxLayout(cell);
    cell_layout->setContentsMargins(0, 0, 0, 0);
    cell_layout->setSpacing(2);

    auto* value_label = new QLabel(QString::number(entry.value), cell);
    auto value_font = value_label->font();
    value_font.setPointSize(value_font.pointSize() + 4);
    value_font.setBold(true);
    value_label->setFont(value_font);
    value_label->setStyleSheet(QStringLiteral("color: %1;").arg(entry.color));

    auto* caption_label = new QLabel(entry.label, cell);
    caption_label->setStyleSheet(QStringLiteral("color: #888888;"));

    cell_layout->addWidget(value_label);
    cell_layout->addWidget(caption_label);

    layout->addWidget(cell, row, col);
    col++;
    if (col >= kColumns) {
      col = 0;
      row++;
    }
  }
}

void KeyImportDetailDialog::create_keys_table() {
  keys_table_ = new QTableWidget(this);
  keys_table_->setRowCount(0);
  keys_table_->setColumnCount(4);
  keys_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  keys_table_->setSelectionMode(QAbstractItemView::SingleSelection);
  keys_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  keys_table_->setAlternatingRowColors(true);

  QStringList header_labels;
  header_labels << tr("Name") << tr("Email") << tr("Status")
                << tr("Fingerprint");
  keys_table_->verticalHeader()->hide();

  keys_table_->setHorizontalHeaderLabels(header_labels);

  QFont monospace_font(QStringLiteral("monospace"));

  int row = 0;
  for (const auto& imp_key : m_result_->imported_keys) {
    auto key = AbstractKeyRepository::GetInstance(current_gpg_context_channel_)
                   .GetKey(imp_key.fpr);
    if (key == nullptr) continue;

    keys_table_->setRowCount(row + 1);

    keys_table_->setItem(row, 0, new QTableWidgetItem(key->Name()));
    keys_table_->setItem(row, 1, new QTableWidgetItem(key->Email()));

    auto* status_item =
        new QTableWidgetItem(get_status_string(imp_key.import_status));
    status_item->setForeground(get_status_color(imp_key.import_status));
    keys_table_->setItem(row, 2, status_item);

    auto* fpr_item = new QTableWidgetItem(imp_key.fpr);
    fpr_item->setFont(monospace_font);
    keys_table_->setItem(row, 3, fpr_item);

    row++;
  }

  keys_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  keys_table_->horizontalHeader()->setSectionResizeMode(1,
                                                        QHeaderView::Stretch);
  keys_table_->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  keys_table_->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::ResizeToContents);
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

auto KeyImportDetailDialog::get_status_color(int key_status) -> QColor {
  // keystatus is greater than 15, if key is private
  if (key_status > 15) {
    key_status = key_status - 16;
  }
  if (key_status == 0) {
    return {0x9e, 0x9e, 0x9e};  // unchanged -> gray
  }
  if (key_status == 1) {
    return {0x43, 0xa0, 0x47};  // new key -> green
  }
  return {0x1e, 0x88, 0xe5};  // new uid/signature/subkey -> blue
}

void KeyImportDetailDialog::create_button_box() {
  button_box_ = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &KeyImportDetailDialog::close);
}
}  // namespace GpgFrontend::UI
