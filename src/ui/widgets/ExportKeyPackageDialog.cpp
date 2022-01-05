/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ExportKeyPackageDialog.h"

#include <boost/format.hpp>

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExporter.h"
#include "ui/aes/qaesencryption.h"
#include "ui_ExportKeyPackageDialog.h"

GpgFrontend::UI::ExportKeyPackageDialog::ExportKeyPackageDialog(
    KeyIdArgsListPtr key_ids, QWidget* parent)
    : QDialog(parent),
      ui(std::make_shared<Ui_exportKeyPackageDialog>()),
      key_ids_(std::move(key_ids)),
      mt(rd()) {
  ui->setupUi(this);

  generate_key_package_name();

  connect(ui->gnerateNameButton, &QPushButton::clicked, this,
          [=]() { generate_key_package_name(); });

  connect(ui->setOutputPathButton, &QPushButton::clicked, this, [=]() {
    auto file_name = QFileDialog::getSaveFileName(
        this, _("Export Key Package"), ui->nameValueLabel->text() + ".gfepack",
        QString(_("Key Package")) + " (*.gfepack);;All Files (*)");
    ui->outputPathLabel->setText(file_name);
  });

  connect(ui->generatePassphraseButton, &QPushButton::clicked, this, [=]() {
    passphrase_ = generate_passphrase(256);
    auto file_name = QFileDialog::getSaveFileName(
        this, _("Export Key Package Passphrase"),
        ui->nameValueLabel->text() + ".key",
        QString(_("Key File")) + " (*.key);;All Files (*)");
    ui->passphraseValueLabel->setText(file_name);
    write_buffer_to_file(file_name.toStdString(), passphrase_);
  });

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [=]() {
    if (ui->outputPathLabel->text().isEmpty()) {
      QMessageBox::critical(
          this, _("Forbidden"),
          _("Please select an output path before exporting."));
      return;
    }

    if (ui->passphraseValueLabel->text().isEmpty()) {
      QMessageBox::critical(
          this, _("Forbidden"),
          _("Please generate a password to protect your key before exporting, "
            "it is very important. Don't forget to back up your password in a "
            "safe place."));
      return;
    }

    auto key_id_exported = std::make_unique<KeyIdArgsList>();
    auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids_);
    for (const auto& key : *keys) {
      if (ui->noPublicKeyCheckBox->isChecked() && !key.is_private_key()) {
        continue;
      }
      key_id_exported->push_back(key.id());
    }

    ByteArrayPtr key_export_data = nullptr;
    if (!GpgKeyImportExporter::GetInstance().ExportKeys(
            key_ids_, key_export_data,
            ui->includeSecretKeyCheckBox->isChecked())) {
      QMessageBox::critical(this, _("Error"), _("Export Key(s) Failed."));
      this->close();
      return;
    }

    auto key = QByteArray::fromStdString(passphrase_),
         data =
             QString::fromStdString(*key_export_data).toLocal8Bit().toBase64();

    auto hash_key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                              QAESEncryption::Padding::ISO);
    auto encoded = encryption.encode(data, hash_key);

    write_buffer_to_file(ui->outputPathLabel->text().toStdString(),
                         encoded.toStdString());

    QMessageBox::information(
        this, _("Success"),
        QString(_(
            "The Key Package has been successfully generated and has been "
            "protected by encryption algorithms. You can safely transfer your "
            "Key Package.")) +
            "<br />" + "<b>" +
            _("But the key file cannot be leaked under any "
              "circumstances. Please delete the Key Package and key file as "
              "soon "
              "as possible after completing the transfer operation.") +
            "</b>");
  });

  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          [=]() { this->close(); });

  ui->nameLabel->setText(_("Key Package Name"));
  ui->selectOutputPathLabel->setText(_("Output Path"));
  ui->passphraseLabel->setText(_("Passphrase"));
  ui->tipsLabel->setText(
      _("Tips: You can use Key Package to safely and conveniently transfer "
        "your public and private keys between devices."));
  ui->generatePassphraseButton->setText(_("Generate and Save Passphrase"));
  ui->gnerateNameButton->setText(_("Generate Key Package Name"));
  ui->setOutputPathButton->setText(_("Select Output Path"));

  ui->includeSecretKeyCheckBox->setText(
      _("Include secret key (Think twice before acting)"));
  ui->noPublicKeyCheckBox->setText(
      _("Exclude keys that do not have a private key"));

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(_("exportKeyPackageDialog"));
}

std::string GpgFrontend::UI::ExportKeyPackageDialog::generate_passphrase(
    const int len) {
  std::uniform_int_distribution<int> dist(999, 99999);
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_str;
  tmp_str.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_str += alphanum[dist(mt) % (sizeof(alphanum) - 1)];
  }

  return tmp_str;
}

void GpgFrontend::UI::ExportKeyPackageDialog::generate_key_package_name() {
  std::uniform_int_distribution<int> dist(999, 99999);
  auto file_string = boost::format("KeyPackage_%1%") % dist(mt);
  ui->nameValueLabel->setText(file_string.str().c_str());
  ui->outputPathLabel->clear();
  ui->passphraseValueLabel->clear();
}
