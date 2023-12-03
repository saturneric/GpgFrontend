/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "KeyPairOperaTab.h"

#include "KeySetExpireDateDialog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/import_export/KeyUploadDialog.h"
#include "ui/function/SetOwnerTrustLevel.h"

namespace GpgFrontend::UI {

KeyPairOperaTab::KeyPairOperaTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), m_key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  // Set Menu
  CreateOperaMenu();
  auto m_vbox = new QVBoxLayout(this);

  auto* opera_key_box = new QGroupBox(_("General Operations"));
  auto* vbox_p_k = new QVBoxLayout();

  auto export_h_box_layout = new QHBoxLayout();
  vbox_p_k->addLayout(export_h_box_layout);

  auto* export_public_button = new QPushButton(_("Export Public Key"));
  export_h_box_layout->addWidget(export_public_button);
  connect(export_public_button, &QPushButton::clicked, this,
          &KeyPairOperaTab::slot_export_public_key);

  if (m_key_.IsPrivateKey()) {
    auto* export_private_button = new QPushButton(_("Export Private Key"));
    export_private_button->setStyleSheet("text-align:center;");
    export_private_button->setMenu(secret_key_export_opera_menu_);
    export_h_box_layout->addWidget(export_private_button);

    if (m_key_.IsHasMasterKey()) {
      auto* edit_expires_button =
          new QPushButton(_("Modify Expiration Datetime (Primary Key)"));
      connect(edit_expires_button, &QPushButton::clicked, this,
              &KeyPairOperaTab::slot_modify_edit_datetime);
      auto* edit_password_button = new QPushButton(_("Modify Password"));
      connect(edit_password_button, &QPushButton::clicked, this,
              &KeyPairOperaTab::slot_modify_password);

      vbox_p_k->addWidget(edit_expires_button);
      vbox_p_k->addWidget(edit_password_button);
    }
  }

  auto advance_h_box_layout = new QHBoxLayout();

  // get settings
  auto& settings = GlobalSettingStation::GetInstance().GetMainSettings();
  // read settings
  bool forbid_all_gnupg_connection = false;
  try {
    forbid_all_gnupg_connection =
        settings.lookup("network.forbid_all_gnupg_connection");
  } catch (...) {
    SPDLOG_ERROR("setting operation error: forbid_all_gnupg_connection");
  }

  auto* key_server_opera_button =
      new QPushButton(_("Key Server Operation (Pubkey)"));
  key_server_opera_button->setStyleSheet("text-align:center;");
  key_server_opera_button->setMenu(key_server_opera_menu_);
  key_server_opera_button->setDisabled(forbid_all_gnupg_connection);
  advance_h_box_layout->addWidget(key_server_opera_button);

  if (m_key_.IsPrivateKey() && m_key_.IsHasMasterKey()) {
    auto* revoke_cert_gen_button =
        new QPushButton(_("Generate Revoke Certificate"));
    connect(revoke_cert_gen_button, &QPushButton::clicked, this,
            &KeyPairOperaTab::slot_gen_revoke_cert);
    advance_h_box_layout->addWidget(revoke_cert_gen_button);
  }

  auto* modify_tofu_button = new QPushButton(_("Modify TOFU Policy"));
  connect(modify_tofu_button, &QPushButton::clicked, this,
          &KeyPairOperaTab::slot_modify_tofu_policy);

  auto* set_owner_trust_level_button =
      new QPushButton(_("Set Owner Trust Level"));
  connect(set_owner_trust_level_button, &QPushButton::clicked, this,
          &KeyPairOperaTab::slot_set_owner_trust_level);

  vbox_p_k->addLayout(advance_h_box_layout);
  opera_key_box->setLayout(vbox_p_k);
  m_vbox->addWidget(opera_key_box);
  // modify owner trust of public key
  if (!m_key_.IsPrivateKey()) vbox_p_k->addWidget(set_owner_trust_level_button);
  vbox_p_k->addWidget(modify_tofu_button);
  m_vbox->addStretch(0);

  setLayout(m_vbox);

  // set up signal
  connect(this, &KeyPairOperaTab::SignalKeyDatabaseRefresh,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
}

void KeyPairOperaTab::CreateOperaMenu() {
  key_server_opera_menu_ = new QMenu(this);

  auto* uploadKeyPair = new QAction(_("Upload Key Pair to Key Server"), this);
  connect(uploadKeyPair, &QAction::triggered, this,
          &KeyPairOperaTab::slot_upload_key_to_server);
  if (!(m_key_.IsPrivateKey() && m_key_.IsHasMasterKey()))
    uploadKeyPair->setDisabled(true);

  auto* updateKeyPair = new QAction(_("Sync Key Pair From Key Server"), this);
  connect(updateKeyPair, &QAction::triggered, this,
          &KeyPairOperaTab::slot_update_key_from_server);

  // when a key has primary key, it should always upload to keyserver.
  if (m_key_.IsHasMasterKey()) {
    updateKeyPair->setDisabled(true);
  }

  key_server_opera_menu_->addAction(uploadKeyPair);
  key_server_opera_menu_->addAction(updateKeyPair);

  secret_key_export_opera_menu_ = new QMenu(this);

  auto* exportFullSecretKey = new QAction(_("Export Full Secret Key"), this);
  connect(exportFullSecretKey, &QAction::triggered, this,
          &KeyPairOperaTab::slot_export_private_key);
  if (!m_key_.IsPrivateKey()) exportFullSecretKey->setDisabled(true);

  auto* exportShortestSecretKey =
      new QAction(_("Export Shortest Secret Key"), this);
  connect(exportShortestSecretKey, &QAction::triggered, this,
          &KeyPairOperaTab::slot_export_short_private_key);

  secret_key_export_opera_menu_->addAction(exportFullSecretKey);
  secret_key_export_opera_menu_->addAction(exportShortestSecretKey);
}

void KeyPairOperaTab::slot_export_public_key() {
  ByteArrayPtr keyArray = nullptr;

  if (!GpgKeyImportExporter::GetInstance().ExportKey(m_key_, keyArray)) {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during the export operation."));
    return;
  }

  // generate a file name
#ifndef WINDOWS
  auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                     m_key_.GetId() + ")_pub.asc";
#else
  auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                     m_key_.GetId() + ")_pub.asc";
#endif
  std::replace(file_string.begin(), file_string.end(), ' ', '_');

  auto file_name =
      QFileDialog::getSaveFileName(
          this, _("Export Key To File"), QString::fromStdString(file_string),
          QString(_("Key Files")) + " (*.asc *.txt);;All Files (*)")
          .toStdString();

  if (file_name.empty()) return;

  if (!WriteBufferToFile(file_name, *keyArray)) {
    QMessageBox::critical(
        this, _("Export Error"),
        QString(_("Couldn't open %1 for writing")).arg(file_name.c_str()));
    return;
  }
}

void KeyPairOperaTab::slot_export_short_private_key() {
  // Show a information box with explanation about private key
  int ret = QMessageBox::information(
      this, _("Exporting short private Key"),
      "<h3>" + QString(_("You are about to export your")) +
          "<font color=\"red\">" + _(" PRIVATE KEY ") + "</font>!</h3>\n" +
          _("This is NOT your Public Key, so DON'T give it away.") + "<br />" +
          _("Do you REALLY want to export your PRIVATE KEY in a Minimum "
            "Size?") +
          "<br />" +
          _("For OpenPGP keys it removes all signatures except for the latest "
            "self-signatures."),
      QMessageBox::Cancel | QMessageBox::Ok);

  // export key, if ok was clicked
  if (ret == QMessageBox::Ok) {
    ByteArrayPtr keyArray = nullptr;

    if (!GpgKeyImportExporter::GetInstance().ExportSecretKeyShortest(
            m_key_, keyArray)) {
      QMessageBox::critical(
          this, _("Error"),
          _("An error occurred during the export operation."));
      return;
    }

    // generate a file name
#ifndef WINDOWS
    auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                       m_key_.GetId() + ")_short_secret.asc";
#else
    auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                       m_key_.GetId() + ")_short_secret.asc";
#endif
    std::replace(file_string.begin(), file_string.end(), ' ', '_');

    auto file_name =
        QFileDialog::getSaveFileName(
            this, _("Export Key To File"), QString::fromStdString(file_string),
            QString(_("Key Files")) + " (*.asc *.txt);;All Files (*)")
            .toStdString();

    if (file_name.empty()) return;

    if (!WriteBufferToFile(file_name, *keyArray)) {
      QMessageBox::critical(
          this, _("Export Error"),
          QString(_("Couldn't open %1 for writing")).arg(file_name.c_str()));
      return;
    }
  }
}

void KeyPairOperaTab::slot_export_private_key() {
  // Show a information box with explanation about private key
  int ret = QMessageBox::information(
      this, _("Exporting private Key"),
      "<h3>" + QString(_("You are about to export your")) +
          "<font color=\"red\">" + _(" PRIVATE KEY ") + "</font>!</h3>\n" +
          _("This is NOT your Public Key, so DON'T give it away.") + "<br />" +
          _("Do you REALLY want to export your PRIVATE KEY?"),
      QMessageBox::Cancel | QMessageBox::Ok);

  // export key, if ok was clicked
  if (ret == QMessageBox::Ok) {
    ByteArrayPtr keyArray = nullptr;

    if (!GpgKeyImportExporter::GetInstance().ExportSecretKey(m_key_,
                                                             keyArray)) {
      QMessageBox::critical(
          this, _("Error"),
          _("An error occurred during the export operation."));
      return;
    }

    // generate a file name
#ifndef WINDOWS
    auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                       m_key_.GetId() + ")_full_secret.asc";
#else
    auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                       m_key_.GetId() + ")_full_secret.asc";
#endif
    std::replace(file_string.begin(), file_string.end(), ' ', '_');

    auto file_name =
        QFileDialog::getSaveFileName(
            this, _("Export Key To File"), QString::fromStdString(file_string),
            QString(_("Key Files")) + " (*.asc *.txt);;All Files (*)")
            .toStdString();

    if (file_name.empty()) return;

    if (!WriteBufferToFile(file_name, *keyArray)) {
      QMessageBox::critical(
          this, _("Export Error"),
          QString(_("Couldn't open %1 for writing")).arg(file_name.c_str()));
      return;
    }
  }
}

void KeyPairOperaTab::slot_modify_edit_datetime() {
  auto* dialog = new KeySetExpireDateDialog(m_key_.GetId(), this);
  dialog->show();
}

void KeyPairOperaTab::slot_upload_key_to_server() {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(m_key_.GetId());
  auto* dialog = new KeyUploadDialog(keys, this);
  dialog->show();
  dialog->SlotUpload();
}

void KeyPairOperaTab::slot_update_key_from_server() {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(m_key_.GetId());
  auto* dialog = new KeyServerImportDialog(this);
  dialog->show();
  dialog->SlotImport(keys);
}

void KeyPairOperaTab::slot_gen_revoke_cert() {
  auto literal = QString("%1 (*.rev)").arg(_("Revocation Certificates"));
  QString m_output_file_name;

#ifndef WINDOWS
  auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                     m_key_.GetId() + ").rev";
#else
  auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                     m_key_.GetId() + ").rev";
#endif

  QFileDialog dialog(this, "Generate revocation certificate",
                     QString::fromStdString(file_string), literal);
  dialog.setDefaultSuffix(".rev");
  dialog.setAcceptMode(QFileDialog::AcceptSave);

  if (dialog.exec()) m_output_file_name = dialog.selectedFiles().front();

  if (!m_output_file_name.isEmpty()) {
    GpgKeyOpera::GetInstance().GenerateRevokeCert(
        m_key_, m_output_file_name.toStdString());
  }
}

void KeyPairOperaTab::slot_modify_password() {
  GpgKeyOpera::GetInstance().ModifyPassword(
      m_key_, [this](GpgError err, const DataObjectPtr&) {
        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          QMessageBox::critical(
              this, _("Not Successful"),
              QString(_("Modify password not successfully.")));
        }
      });
}

void KeyPairOperaTab::slot_modify_tofu_policy() {
  QStringList items;
  items << _("Policy Auto") << _("Policy Good") << _("Policy Bad")
        << _("Policy Ask") << _("Policy Unknown");

  bool ok;
  QString item = QInputDialog::getItem(
      this, _("Modify TOFU Policy(Default is Auto)"),
      _("Policy for the Key Pair:"), items, 0, false, &ok);
  if (ok && !item.isEmpty()) {
    SPDLOG_DEBUG("selected policy: {}", item.toStdString());
    gpgme_tofu_policy_t tofu_policy = GPGME_TOFU_POLICY_AUTO;
    if (item == _("Policy Auto")) {
      tofu_policy = GPGME_TOFU_POLICY_AUTO;
    } else if (item == _("Policy Good")) {
      tofu_policy = GPGME_TOFU_POLICY_GOOD;
    } else if (item == _("Policy Bad")) {
      tofu_policy = GPGME_TOFU_POLICY_BAD;
    } else if (item == _("Policy Ask")) {
      tofu_policy = GPGME_TOFU_POLICY_ASK;
    } else if (item == _("Policy Unknown")) {
      tofu_policy = GPGME_TOFU_POLICY_UNKNOWN;
    }
    auto err = GpgKeyOpera::GetInstance().ModifyTOFUPolicy(m_key_, tofu_policy);
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      QMessageBox::critical(this, _("Not Successful"),
                            QString(_("Modify TOFU policy not successfully.")));
    }
  }
}

void KeyPairOperaTab::slot_set_owner_trust_level() {
  auto* function = new SetOwnerTrustLevel(this);
  function->Exec(m_key_.GetId());
  function->deleteLater();
}

}  // namespace GpgFrontend::UI
