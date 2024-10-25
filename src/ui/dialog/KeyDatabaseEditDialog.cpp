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

#include "core/utils/MemoryUtils.h"
#include "ui_KeyDatabaseEditDialog.h"

namespace GpgFrontend::UI {
KeyDatabaseEditDialog::KeyDatabaseEditDialog(QWidget* parent)
    : GeneralDialog("KeyDatabaseEditDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabaseEditDialog>()) {
  ui_->setupUi(this);

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

    path_ = QFileInfo(path).absoluteFilePath();
    ui_->keyDBPathShowLabel->setText(path_);
  });

  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this,
          &KeyDatabaseEditDialog::slot_button_box_accepted);
  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);
}

void KeyDatabaseEditDialog::slot_button_box_accepted() {
  name_ = ui_->keyDBNameLineEdit->text();
  emit SignalKeyDatabaseInfoAccepted(name_, path_);
}

auto KeyDatabaseEditDialog::check_custom_gnupg_key_database_path(
    const QString& path) -> bool {
  if (path.isEmpty()) return false;

  QFileInfo const dir_info(path);
  return dir_info.exists() && dir_info.isReadable() && dir_info.isDir();
}

void KeyDatabaseEditDialog::SetDefaultName(QString name) {
  name_ = std::move(name);
}

void KeyDatabaseEditDialog::SetDefaultPath(QString path) {
  path_ = QFileInfo(path).absoluteFilePath();
  ui_->keyDBPathShowLabel->setText(path_);
}
};  // namespace GpgFrontend::UI