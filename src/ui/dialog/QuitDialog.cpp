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

#include "QuitDialog.h"

namespace GpgFrontend::UI {

QuitDialog::QuitDialog(QWidget* parent, const QHash<int, QString>& unsavedDocs)
    : GeneralDialog("quit_dialog", parent) {
  setWindowTitle(tr("Unsaved Files"));
  setModal(true);
  discarded_ = false;

  /*
   * Table of unsaved documents
   */
  QHashIterator<int, QString> i(unsavedDocs);
  int row = 0;
  m_fileList_ = new QTableWidget(this);
  m_fileList_->horizontalHeader()->hide();
  m_fileList_->setColumnCount(3);
  m_fileList_->setColumnWidth(0, 20);
  m_fileList_->setColumnHidden(2, true);
  m_fileList_->verticalHeader()->hide();
  m_fileList_->setShowGrid(false);
  m_fileList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_fileList_->setFocusPolicy(Qt::NoFocus);
  m_fileList_->horizontalHeader()->setStretchLastSection(true);
  // fill the table
  i.toFront();  // jump to the end of list to fill the table backwards
  while (i.hasNext()) {
    i.next();
    m_fileList_->setRowCount(m_fileList_->rowCount() + 1);

    // checkbox in front of filename
    auto* tmp0 = new QTableWidgetItem();
    tmp0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    tmp0->setCheckState(Qt::Checked);
    m_fileList_->setItem(row, 0, tmp0);

    // filename
    auto* tmp1 = new QTableWidgetItem(i.value());
    m_fileList_->setItem(row, 1, tmp1);

    // tab-index in hidden column
    auto* tmp2 = new QTableWidgetItem(QString::number(i.key()));
    m_fileList_->setItem(row, 2, tmp2);
    ++row;
  }
  /*
   *  Warnbox with icon and text
   */
  auto pixmap = QPixmap(":/icons/error.png");
  pixmap = pixmap.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  auto* warn_icon = new QLabel();
  warn_icon->setPixmap(pixmap);

  const auto info = tr("%1 files contain unsaved information.<br/>Save the "
                       "changes before closing?")
                        .arg(row);
  auto* warn_label = new QLabel(info);
  auto* warn_box_layout = new QHBoxLayout();
  warn_box_layout->addWidget(warn_icon);
  warn_box_layout->addWidget(warn_label);
  warn_box_layout->setAlignment(Qt::AlignLeft);
  auto* warn_box = new QWidget(this);
  warn_box->setLayout(warn_box_layout);

  /*
   *  Two labels on top and under the filelist
   */
  auto* check_label = new QLabel(tr("Check the files you want to save:"));
  auto* note_label = new QLabel(
      "<b>" + tr("Note") + ":</b>" +
      tr("If you don't save these files, all changes are lost.") + "<br/>");

  /*
   *  Buttonbox
   */
  auto* button_box =
      new QDialogButtonBox(QDialogButtonBox::Discard | QDialogButtonBox::Save |
                           QDialogButtonBox::Cancel);
  connect(button_box, &QDialogButtonBox::accepted, this, &QuitDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &QuitDialog::reject);
  QPushButton* btn_no_key = button_box->button(QDialogButtonBox::Discard);
  connect(btn_no_key, &QPushButton::clicked, this,
          &QuitDialog::slot_my_discard);

  /*
   *  Set the layout
   */
  auto* vbox = new QVBoxLayout();
  vbox->addWidget(warn_box);
  vbox->addWidget(check_label);
  vbox->addWidget(m_fileList_);
  vbox->addWidget(note_label);
  vbox->addWidget(button_box);
  this->setLayout(vbox);

  this->movePosition2CenterOfParent();
}

void QuitDialog::slot_my_discard() {
  discarded_ = true;
  reject();
}

auto QuitDialog::IsDiscarded() const -> bool { return discarded_; }

auto QuitDialog::GetTabIdsToSave() -> QList<int> {
  QList<int> tab_ids_to_save;
  for (int i = 0; i < m_fileList_->rowCount(); i++) {
    if (m_fileList_->item(i, 0)->checkState() == Qt::Checked) {
      tab_ids_to_save << m_fileList_->item(i, 2)->text().toInt();
    }
  }
  return tab_ids_to_save;
}

}  // namespace GpgFrontend::UI
