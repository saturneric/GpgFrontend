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

#include "ExportKeyPackageDialog.h"

#include <boost/format.hpp>

#include "core/function/KeyPackageOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "ui_ExportKeyPackageDialog.h"

GpgFrontend::UI::ExportKeyPackageDialog::ExportKeyPackageDialog(
    KeyIdArgsListPtr key_ids, QWidget* parent)
    : GeneralDialog(typeid(ExportKeyPackageDialog).name(), parent),
      ui_(std::make_shared<Ui_exportKeyPackageDialog>()),
      key_ids_(std::move(key_ids)) {
  ui_->setupUi(this);

  ui_->nameValueLabel->setText(
      KeyPackageOperator::GenerateKeyPackageName().c_str());

  connect(ui_->gnerateNameButton, &QPushButton::clicked, this, [=]() {
    ui_->nameValueLabel->setText(
        KeyPackageOperator::GenerateKeyPackageName().c_str());
  });

  connect(ui_->setOutputPathButton, &QPushButton::clicked, this, [=]() {
    auto file_name = QFileDialog::getSaveFileName(
        this, _("Export Key Package"), ui_->nameValueLabel->text() + ".gfepack",
        QString(_("Key Package")) + " (*.gfepack);;All Files (*)");
    ui_->outputPathLabel->setText(file_name);
  });

  connect(ui_->generatePassphraseButton, &QPushButton::clicked, this, [=]() {
    auto file_name = QFileDialog::getSaveFileName(
        this, _("Export Key Package Passphrase"),
        ui_->nameValueLabel->text() + ".key",
        QString(_("Key File")) + " (*.key);;All Files (*)");

    if (!KeyPackageOperator::GeneratePassphrase(file_name.toStdString(),
                                                passphrase_)) {
      QMessageBox::critical(
          this, _("Error"),
          _("An error occurred while generating the passphrase file."));
      return;
    }
    ui_->passphraseValueLabel->setText(file_name);
  });

  connect(ui_->button_box_, &QDialogButtonBox::accepted, this, [=]() {
    if (ui_->outputPathLabel->text().isEmpty()) {
      QMessageBox::critical(
          this, _("Forbidden"),
          _("Please select an output path before exporting."));
      return;
    }

    if (ui_->passphraseValueLabel->text().isEmpty()) {
      QMessageBox::critical(
          this, _("Forbidden"),
          _("Please generate a password to protect your key before exporting, "
            "it is very important. Don't forget to back up your password in a "
            "safe place."));
      return;
    }

    // get suitable key ids
    auto key_id_exported = std::make_unique<KeyIdArgsList>();
    auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids_);
    for (const auto& key : *keys) {
      if (ui_->noPublicKeyCheckBox->isChecked() && !key.IsPrivateKey())
        continue;
      key_id_exported->push_back(key.GetId());
    }

    if (KeyPackageOperator::GenerateKeyPackage(
            ui_->outputPathLabel->text().toStdString(),
            ui_->nameValueLabel->text().toStdString(), key_id_exported,
            passphrase_, ui_->includeSecretKeyCheckBox->isChecked())) {
      QMessageBox::information(
          this, _("Success"),
          QString(
              _("The Key Package has been successfully generated and has been "
                "protected by encryption algorithms(AES-256-ECB). You can "
                "safely transfer your Key Package.")) +
              "<br /><br />" + "<b>" +
              _("But the key file cannot be leaked under any "
                "circumstances. Please delete the Key Package and key file as "
                "soon "
                "as possible after completing the transfer operation.") +
              "</b>");
      accept();
    } else {
      QMessageBox::critical(
          this, _("Error"),
          _("An error occurred while exporting the key package."));
    }
  });

  connect(ui_->button_box_, &QDialogButtonBox::rejected, this,
          [=]() { this->close(); });

  ui_->nameLabel->setText(_("Key Package Name"));
  ui_->selectOutputPathLabel->setText(_("Output Path"));
  ui_->passphraseLabel->setText(_("Passphrase"));
  ui_->tipsLabel->setText(
      _("Tips: You can use Key Package to safely and conveniently transfer "
        "your public and private keys between devices."));
  ui_->generatePassphraseButton->setText(_("Generate and Save Passphrase"));
  ui_->gnerateNameButton->setText(_("Generate Key Package Name"));
  ui_->setOutputPathButton->setText(_("Select Output Path"));

  ui_->includeSecretKeyCheckBox->setText(
      _("Include secret key (Think twice before acting)"));
  ui_->noPublicKeyCheckBox->setText(
      _("Exclude keys that do not have a private key"));

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(_("exportKeyPackageDialog"));
}
