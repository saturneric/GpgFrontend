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

#include "KeyDatabaseEditDialog.h"

#include <utility>

#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/utils/MemoryUtils.h"
#include "ui_KeyDatabaseEditDialog.h"

namespace GpgFrontend::UI {
KeyDatabaseEditDialog::KeyDatabaseEditDialog(
    QList<KeyDatabaseInfo> key_db_infos, QWidget* parent)
    : GeneralDialog("KeyDatabaseEditDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabaseEditDialog>()),
      key_database_infos_(std::move(key_db_infos)) {
  ui_->setupUi(this);

  ui_->keyDBPathShowLabel->setHidden(true);

  ui_->keyDBNameLabel->setText(tr("Key Database Name"));
  ui_->keyDBPathLabel->setText(tr("Key Database Path"));
  ui_->selectKeyDBButton->setText(tr("Select A Key Database Path"));

  this->setWindowTitle(tr("Key Database Info"));

  connect(ui_->selectKeyDBButton, &QPushButton::clicked, this, [this](bool) {
    auto path = QFileDialog::getExistingDirectory(
        this, tr("Open Directory"), {},
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!check_custom_gnupg_key_database_path(path)) {
      QMessageBox::critical(this, tr("Illegal GnuPG Key Database Path"),
                            tr("Target GnuPG Key Database Path is not an "
                               "exists readable directory."));
    }

    if (!path.trimmed().isEmpty() && path != path_) {
      path_ = QFileInfo(path).absoluteFilePath();

      ui_->keyDBPathShowLabel->setText(path_);
      ui_->keyDBPathShowLabel->setHidden(false);
    }
  });

  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this,
          &KeyDatabaseEditDialog::slot_button_box_accepted);
  connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);
}
void KeyDatabaseEditDialog::slot_button_box_accepted() {
  name_ = ui_->keyDBNameLineEdit->text();

  if (name_.isEmpty()) {
    slot_show_err_msg(tr("The key database name cannot be empty."));
    return;
  }

  if (path_.isEmpty()) {
    slot_show_err_msg(tr("The key database path cannot be empty."));
    return;
  }

  for (const auto& info : key_database_infos_) {
    if (default_name_ != name_ && info.name == name_) {
      slot_show_err_msg(tr("A key database with the name '%1' already exists. "
                           "Please choose a different name.")
                            .arg(name_));
      return;
    }
  }

  slot_clear_err_msg();
  emit SignalKeyDatabaseInfoAccepted(name_, path_);
  this->accept();
}

auto KeyDatabaseEditDialog::check_custom_gnupg_key_database_path(
    const QString& path) -> bool {
  if (path.isEmpty()) return false;

  QFileInfo const dir_info(path);
  return dir_info.exists() && dir_info.isReadable() && dir_info.isDir();
}

void KeyDatabaseEditDialog::SetDefaultName(QString name) {
  name_ = std::move(name);
  default_name_ = name_;

  ui_->keyDBNameLineEdit->setText(name_);
}

void KeyDatabaseEditDialog::SetDefaultPath(const QString& path) {
  path_ = QFileInfo(path).absoluteFilePath();
  default_path_ = path_;

  ui_->keyDBPathShowLabel->setText(path_);
  ui_->keyDBPathShowLabel->setHidden(path_.isEmpty());
}
void KeyDatabaseEditDialog::slot_show_err_msg(const QString& error_msg) {
  ui_->errorLabel->setText(error_msg);
  ui_->errorLabel->setStyleSheet("color: red;");
  ui_->errorLabel->setHidden(false);
}

void KeyDatabaseEditDialog::slot_clear_err_msg() {
  ui_->errorLabel->setText({});
  ui_->errorLabel->setHidden(true);
}
};  // namespace GpgFrontend::UI