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

#include "EmailListEditor.h"

#include "ui_EmailListEditor.h"

GpgFrontend::UI::EmailListEditor::EmailListEditor(const QString& email_list,
                                                  QWidget* parent)
    : QDialog(parent), ui_(std::make_shared<Ui_EmailListEditorDialog>()) {
  ui_->setupUi(this);

  QStringList email_string_list = email_list.split(';');

  if (!email_string_list.isEmpty()) {
    for (const auto& recipient : email_string_list) {
      auto _recipient = recipient.trimmed();
      if (check_email_address(_recipient)) {
        auto item = new QListWidgetItem(_recipient);
        ui_->emaillistWidget->addItem(item);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
      }
    }
  }

  connect(ui_->addEmailAddressButton, &QPushButton::clicked, this, [=]() {
    auto item = new QListWidgetItem("new email address");
    ui_->emaillistWidget->addItem(item);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
  });

  connect(ui_->actionDelete_Selected_Email_Address, &QAction::triggered, this,
          [=]() {
            const auto row_size = ui_->emaillistWidget->count();
            for (int i = 0; i < row_size; i++) {
              auto item = ui_->emaillistWidget->item(i);
              if (!item->isSelected()) continue;
              delete ui_->emaillistWidget->takeItem(
                  ui_->emaillistWidget->row(item));
              break;
            }
          });

  ui_->titleLabel->setText(_("Email List:"));
  ui_->tipsLabel->setText(
      _("Tips: You can double-click the email address in the edit list, or "
        "click the email to pop up the option menu."));
  ui_->addEmailAddressButton->setText(_("Add An Email Address"));
  this->setWindowTitle(_("Email List Editor"));
  ui_->actionDelete_Selected_Email_Address->setText(_("Delete"));

  popup_menu_ = new QMenu(this);
  popup_menu_->addAction(ui_->actionDelete_Selected_Email_Address);

  this->exec();
}

bool GpgFrontend::UI::EmailListEditor::check_email_address(
    const QString& email_address) {
  return re_email_.match(email_address).hasMatch();
}

QString GpgFrontend::UI::EmailListEditor::GetEmailList() {
  QString email_list;
  for (int i = 0; i < ui_->emaillistWidget->count(); ++i) {
    QListWidgetItem* item = ui_->emaillistWidget->item(i);
    if (check_email_address(item->text())) {
      email_list.append(item->text());
      email_list.append("; ");
    }
  }
  return email_list;
}

void GpgFrontend::UI::EmailListEditor::contextMenuEvent(
    QContextMenuEvent* event) {
  QWidget::contextMenuEvent(event);
  if (ui_->emaillistWidget->selectedItems().length() > 0) {
    popup_menu_->exec(event->globalPos());
  }
}
