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

#include "KeyGroupCreationDialog.h"

#include "core/function/gpg/GpgKeyGroupGetter.h"
#include "core/model/GpgKeyGroup.h"
#include "ui/UISignalStation.h"

namespace GpgFrontend::UI {
KeyGroupCreationDialog::KeyGroupCreationDialog(int channel, QStringList key_ids,
                                               QWidget* parent)
    : GeneralDialog(typeid(KeyGroupCreationDialog).name(), parent),
      current_gpg_context_channel_(channel),
      key_ids_(key_ids) {
  assert(!key_ids.isEmpty());

  name_ = new QLineEdit();
  name_->setMinimumWidth(240);
  email_ = new QLineEdit();
  email_->setMinimumWidth(240);
  comment_ = new QLineEdit();
  comment_->setMinimumWidth(240);
  create_button_ = new QPushButton("Create");
  error_label_ = new QLabel();

  auto* grid_layout = new QGridLayout();
  grid_layout->addWidget(new QLabel(tr("Name")), 0, 0);
  grid_layout->addWidget(new QLabel(tr("Email")), 1, 0);
  grid_layout->addWidget(new QLabel(tr("Comment")), 2, 0);

  grid_layout->addWidget(name_, 0, 1);
  grid_layout->addWidget(email_, 1, 1);
  grid_layout->addWidget(comment_, 2, 1);

  grid_layout->addWidget(create_button_, 3, 0, 1, 2);
  grid_layout->addWidget(error_label_, 4, 0, 1, 2);

  connect(create_button_, &QPushButton::clicked, this,
          &KeyGroupCreationDialog::slot_create_new_uid);

  connect(this, &KeyGroupCreationDialog::SignalCreated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);

  this->setLayout(grid_layout);
  this->setWindowTitle(tr("Create New Key Group"));
  this->setAttribute(Qt::WA_DeleteOnClose, true);
  this->setModal(true);
}

void KeyGroupCreationDialog::slot_create_new_uid() {
  QString buffer;
  QTextStream error_stream(&buffer);

  /**
   * check for errors in keygen dialog input
   */
  if ((name_->text()).size() < 5) {
    error_stream << "  " << tr("Name must contain at least five characters.")
                 << Qt::endl;
  }
  if (email_->text().isEmpty() || !check_email_address(email_->text())) {
    error_stream << "  " << tr("Please give a email address.") << Qt::endl;
  }
  auto error_string = error_stream.readAll();
  if (error_string.isEmpty()) {
    auto p_kg =
        GpgKeyGroup{name_->text(), email_->text(), comment_->text(), key_ids_};
    GpgKeyGroupGetter::GetInstance(current_gpg_context_channel_)
        .AddKeyGroup(p_kg);

    emit SignalCreated();
    this->close();
  } else {
    /**
     * create error message
     */
    error_label_->setAutoFillBackground(true);
    QPalette error = error_label_->palette();
    error.setColor(QPalette::Window, "#ff8080");
    error_label_->setPalette(error);
    error_label_->setText(error_string);

    this->show();
    this->raise();
    this->activateWindow();
  }
}

auto KeyGroupCreationDialog::check_email_address(const QString& str) -> bool {
  return re_email_.match(str).hasMatch();
}
}  // namespace GpgFrontend::UI
