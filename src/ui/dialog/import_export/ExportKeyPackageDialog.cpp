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

#include "ExportKeyPackageDialog.h"

#include "core/GpgModel.h"
#include "core/function/KeyPackageOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "ui/UserInterfaceUtils.h"
#include "ui_ExportKeyPackageDialog.h"

GpgFrontend::UI::ExportKeyPackageDialog::ExportKeyPackageDialog(
    int channel, KeyIdArgsListPtr key_ids, QWidget* parent)
    : GeneralDialog(typeid(ExportKeyPackageDialog).name(), parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_exportKeyPackageDialog>()),
      current_gpg_context_channel_(channel),
      key_ids_(std::move(key_ids)) {
  ui_->setupUi(this);

  ui_->nameValueLabel->setText(KeyPackageOperator::GenerateKeyPackageName());

  connect(ui_->gnerateNameButton, &QPushButton::clicked, this, [=]() {
    ui_->nameValueLabel->setText(KeyPackageOperator::GenerateKeyPackageName());
  });

  connect(ui_->setOutputPathButton, &QPushButton::clicked, this, [=]() {
    auto file_name = QFileDialog::getSaveFileName(
        this, tr("Export Key Package"),
        ui_->nameValueLabel->text() + ".gfepack",
        tr("Key Package") + " (*.gfepack);;All Files (*)");

    // check path
    if (file_name.isEmpty()) return;

    ui_->outputPathLabel->setText(file_name);
  });

  connect(ui_->generatePassphraseButton, &QPushButton::clicked, this, [=]() {
    auto file_name = QFileDialog::getSaveFileName(
        this, tr("Export Key Package Passphrase"),
        ui_->nameValueLabel->text() + ".key",
        tr("Key File") + " (*.key);;All Files (*)");

    // check path
    if (file_name.isEmpty()) return;

    if (!KeyPackageOperator::GeneratePassphrase(file_name, passphrase_)) {
      QMessageBox::critical(
          this, tr("Error"),
          tr("An error occurred while generating the passphrase file."));
      return;
    }
    ui_->passphraseValueLabel->setText(file_name);
  });

  connect(ui_->button_box_, &QDialogButtonBox::accepted, this, [=]() {
    if (ui_->outputPathLabel->text().isEmpty()) {
      QMessageBox::critical(
          this, tr("Forbidden"),
          tr("Please select an output path before exporting."));
      return;
    }

    if (ui_->passphraseValueLabel->text().isEmpty()) {
      QMessageBox::critical(
          this, tr("Forbidden"),
          tr("Please generate a password to protect your key before exporting, "
             "it is very important. Don't forget to back up your password in a "
             "safe place."));
      return;
    }

    // get suitable key ids
    auto keys = GpgKeyGetter::GetInstance(current_gpg_context_channel_)
                    .GetKeys(key_ids_);
    assert(std::all_of(keys->begin(), keys->end(),
                       [](const auto& key) { return key.IsGood(); }));

    auto keys_new_end =
        std::remove_if(keys->begin(), keys->end(), [this](const auto& key) {
          return ui_->noPublicKeyCheckBox->isChecked() && !key.IsPrivateKey();
        });
    keys->erase(keys_new_end, keys->end());

    if (keys->empty()) {
      QMessageBox::critical(this, tr("Error"),
                            tr("No key is suitable to export."));
      return;
    }

    CommonUtils::WaitForOpera(
        this, tr("Generating"), [this, keys](const OperaWaitingHd& op_hd) {
          KeyPackageOperator::GenerateKeyPackage(
              ui_->outputPathLabel->text(), ui_->nameValueLabel->text(),
              current_gpg_context_channel_, *keys, passphrase_,
              ui_->includeSecretKeyCheckBox->isChecked(),
              [=](GFError err, const DataObjectPtr&) {
                // stop waiting
                op_hd();

                if (err >= 0) {
                  QMessageBox::information(
                      this, tr("Success"),
                      QString(
                          tr("The Key Package has been successfully generated "
                             "and has been protected by encryption "
                             "algorithms(AES-256-ECB). You can safely transfer "
                             "your Key Package.")) +
                          "<br /><br />" + "<b>" +
                          tr("But the key file cannot be leaked under any "
                             "circumstances. Please delete the Key Package and "
                             "key file as soon as possible after completing "
                             "the "
                             "transfer "
                             "operation.") +
                          "</b>");
                  accept();
                } else {
                  QMessageBox::critical(
                      this, tr("Error"),
                      tr("An error occurred while exporting the key package."));
                }
              });
        });
  });

  connect(ui_->button_box_, &QDialogButtonBox::rejected, this,
          [=]() { this->close(); });

  ui_->nameLabel->setText(tr("Key Package Name"));
  ui_->selectOutputPathLabel->setText(tr("Output Path"));
  ui_->passphraseLabel->setText(tr("Passphrase"));
  ui_->tipsLabel->setText(
      tr("Tips: You can use Key Package to safely and conveniently transfer "
         "your public and private keys between devices."));
  ui_->generatePassphraseButton->setText(tr("Generate and Save Passphrase"));
  ui_->gnerateNameButton->setText(tr("Generate Key Package Name"));
  ui_->setOutputPathButton->setText(tr("Select Output Path"));

  ui_->includeSecretKeyCheckBox->setText(
      tr("Include secret key (Think twice before acting)"));
  ui_->noPublicKeyCheckBox->setText(
      tr("Exclude keys that do not have a private key"));

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Export As Key Package"));
}
