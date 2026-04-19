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

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgContext.h"
#include "core/utils/MemoryUtils.h"
#include "ui_KeyDatabaseEditDialog.h"

namespace GpgFrontend::UI {
KeyDatabaseEditDialog::KeyDatabaseEditDialog(
    QContainer<KeyDatabaseInfo> key_db_infos, QWidget* parent)
    : GeneralDialog("KeyDatabaseEditDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabaseEditDialog>()),
      channel_(-1),
      key_database_infos_(std::move(key_db_infos)) {
  ui_->setupUi(this);

  init_ui();
}

KeyDatabaseEditDialog::KeyDatabaseEditDialog(
    QContainer<KeyDatabaseInfo> key_db_infos, int index, QWidget* parent)
    : GeneralDialog("KeyDatabaseEditDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabaseEditDialog>()),
      channel_(-1),
      key_database_infos_(std::move(key_db_infos)) {
  ui_->setupUi(this);

  if (index < 0 || index >= key_database_infos_.size()) {
    throw std::out_of_range("Index out of range in KeyDatabaseEditDialog");
  }

  const auto& key_db_info = key_database_infos_[index];
  default_name_ = key_db_info.name;
  default_path_ = key_db_info.origin_path;
  channel_ = index;

  LOG_D() << "edit key database, index: " << index << "name: " << default_name_
          << "path: " << default_path_ << "channel: " << channel_;

  init_ui();
}

void KeyDatabaseEditDialog::init_ui() {
  ui_->keyDBPathShowLabel->setHidden(true);
  ui_->convert2RelativePathCheckBox->setChecked(
      GlobalSettingStation::GetInstance().IsProtableMode());
  ui_->keyDBNameLineEdit->setText(default_name_);
  if (!default_path_.isEmpty()) {
    path_ = QFileInfo(default_path_).absoluteFilePath();
    ui_->keyDBPathShowLabel->setText(path_);
    ui_->keyDBPathShowLabel->setHidden(false);
  }

  ui_->keyDBBackendTypeComboBox->clear();
  ui_->keyDBBackendTypeComboBox->addItem("GnuPG", "GNUPG");
  ui_->keyDBBackendTypeComboBox->addItem("rPGP", "RPGP");
  if (channel_ != -1) {
    auto backend_types = GpgContext::GetInstance(channel_).Engine();
    ui_->keyDBBackendTypeComboBox->setCurrentIndex(
        backend_types == OpenPGPEngine::kRPGP ? 1 : 0);
  } else {
    ui_->keyDBBackendTypeComboBox->setCurrentIndex(0);
  }

  if (channel_ != -1) {
    ui_->keyDBBackendTypeComboBox->setEnabled(false);
  }

  ui_->keyDBNameLabel->setText(tr("Key Database Name"));
  ui_->keyDBPathLabel->setText(tr("Key Database Path"));
  ui_->keyDBBackendTypeLabel->setText(tr("Key Database Engine"));
  ui_->selectKeyDBButton->setText(tr("Select A Key Database Path"));
  ui_->convert2RelativePathCheckBox->setText(tr("Convert to Relative Path"));

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

  connect(ui_->globalButtonBox, &QDialogButtonBox::accepted, this,
          &KeyDatabaseEditDialog::slot_button_box_accepted);
  connect(ui_->globalButtonBox, &QDialogButtonBox::rejected, this,
          &QDialog::reject);
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

  if (ui_->convert2RelativePathCheckBox->isChecked()) {
    path_ = QDir(GlobalSettingStation::GetInstance().GetAppDir())
                .relativeFilePath(path_);
  }

  backend_type_ =
      ui_->keyDBBackendTypeComboBox->currentText().toLower().trimmed();
  if (backend_type_.isEmpty()) {
    backend_type_ = "gnupg";
  }

  slot_clear_err_msg();
  emit SignalKeyDatabaseInfoAccepted(name_, backend_type_, path_);
  this->accept();
}

auto KeyDatabaseEditDialog::check_custom_gnupg_key_database_path(
    const QString& path) -> bool {
  if (path.isEmpty()) return false;

  QFileInfo const dir_info(path);
  return dir_info.exists() && dir_info.isReadable() && dir_info.isDir();
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