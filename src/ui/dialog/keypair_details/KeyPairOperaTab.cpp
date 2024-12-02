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

#include "KeyPairOperaTab.h"

#include "KeySetExpireDateDialog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg//GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/model/GpgKey.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/RevocationOptionsDialog.h"
#include "ui/dialog/import_export/KeyUploadDialog.h"
#include "ui/function/SetOwnerTrustLevel.h"

namespace GpgFrontend::UI {

KeyPairOperaTab::KeyPairOperaTab(int channel, const QString& key_id,
                                 QWidget* parent)
    : QWidget(parent),
      current_gpg_context_channel_(channel),
      m_key_(GpgKeyGetter::GetInstance(current_gpg_context_channel_)
                 .GetKey(key_id)) {
  assert(m_key_.IsGood());

  // Set Menu
  CreateOperaMenu();
  auto* m_vbox = new QVBoxLayout(this);

  auto* opera_key_box = new QGroupBox(tr("General Operations"));
  auto* vbox_p_k = new QVBoxLayout();

  auto* export_h_box_layout = new QHBoxLayout();
  vbox_p_k->addLayout(export_h_box_layout);

  auto* export_public_button = new QPushButton(tr("Export Public Key"));
  export_h_box_layout->addWidget(export_public_button);
  connect(export_public_button, &QPushButton::clicked, this,
          &KeyPairOperaTab::slot_export_public_key);

  if (m_key_.IsPrivateKey()) {
    auto* export_private_button = new QPushButton(tr("Export Private Key"));
    export_private_button->setStyleSheet("text-align:center;");
    export_private_button->setMenu(secret_key_export_opera_menu_);
    export_h_box_layout->addWidget(export_private_button);

    if (m_key_.IsHasMasterKey()) {
      auto* edit_expires_button =
          new QPushButton(tr("Modify Expiration Datetime (Primary Key)"));
      connect(edit_expires_button, &QPushButton::clicked, this,
              &KeyPairOperaTab::slot_modify_edit_datetime);
      auto* edit_password_button = new QPushButton(tr("Modify Password"));
      connect(edit_password_button, &QPushButton::clicked, this,
              &KeyPairOperaTab::slot_modify_password);

      vbox_p_k->addWidget(edit_expires_button);
      vbox_p_k->addWidget(edit_password_button);
    }
  }

  auto* advance_h_box_layout = new QHBoxLayout();

  auto settings = GlobalSettingStation::GetInstance().GetSettings();

  // read settings
  bool forbid_all_gnupg_connection =
      settings.value("network/forbid_all_gnupg_connection").toBool();

  auto* key_server_opera_button = new QPushButton(tr("Key Server Operations"));
  key_server_opera_button->setStyleSheet("text-align:center;");
  key_server_opera_button->setMenu(key_server_opera_menu_);
  key_server_opera_button->setDisabled(forbid_all_gnupg_connection);
  advance_h_box_layout->addWidget(key_server_opera_button);

  if (Module::IsModuleActivate(kPaperKeyModuleID)) {
    if (!m_key_.IsPrivateKey()) {
      auto* import_paper_key_button = new QPushButton(tr("Import A Paper Key"));
      import_paper_key_button->setStyleSheet("text-align:center;");
      connect(import_paper_key_button, &QPushButton::clicked, this,
              &KeyPairOperaTab::slot_import_paper_key);
      vbox_p_k->addWidget(import_paper_key_button);
    }
  }

  if (m_key_.IsPrivateKey() && m_key_.IsHasMasterKey()) {
    auto* revoke_cert_opera_button =
        new QPushButton(tr("Revoke Certificate Operation"));
    revoke_cert_opera_button->setStyleSheet("text-align:center;");
    revoke_cert_opera_button->setMenu(rev_cert_opera_menu_);
    advance_h_box_layout->addWidget(revoke_cert_opera_button);
  }

  auto* modify_tofu_button = new QPushButton(tr("Modify TOFU Policy"));
  // do not show, useless
  modify_tofu_button->setHidden(true);
  connect(modify_tofu_button, &QPushButton::clicked, this,
          &KeyPairOperaTab::slot_modify_tofu_policy);

  auto* set_owner_trust_level_button =
      new QPushButton(tr("Set Owner Trust Level"));
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

  auto* upload_key_pair =
      new QAction(tr("Publish Public Key to Key Server"), this);
  connect(upload_key_pair, &QAction::triggered, this,
          &KeyPairOperaTab::slot_publish_key_to_server);
  if (!(m_key_.IsPrivateKey() && m_key_.IsHasMasterKey())) {
    upload_key_pair->setDisabled(true);
  }

  auto* update_key_pair =
      new QAction(tr("Refresh Public Key From Key Server"), this);
  connect(update_key_pair, &QAction::triggered, this,
          &KeyPairOperaTab::slot_update_key_from_server);

  // when a key has primary key, it should always upload to keyserver.
  if (m_key_.IsHasMasterKey()) {
    update_key_pair->setDisabled(true);
  }

  key_server_opera_menu_->addAction(upload_key_pair);
  key_server_opera_menu_->addAction(update_key_pair);

  secret_key_export_opera_menu_ = new QMenu(this);

  auto* export_full_secret_key =
      new QAction(tr("Export Full Secret Key"), this);
  connect(export_full_secret_key, &QAction::triggered, this,
          &KeyPairOperaTab::slot_export_private_key);
  if (!m_key_.IsPrivateKey()) export_full_secret_key->setDisabled(true);

  auto* export_shortest_secret_key =
      new QAction(tr("Export Shortest Secret Key"), this);
  connect(export_shortest_secret_key, &QAction::triggered, this,
          &KeyPairOperaTab::slot_export_short_private_key);

  secret_key_export_opera_menu_->addAction(export_full_secret_key);
  secret_key_export_opera_menu_->addAction(export_shortest_secret_key);

  // only work with RSA
  if (m_key_.GetKeyAlgo() == "RSA" &&
      Module::IsModuleActivate(kPaperKeyModuleID)) {
    auto* export_secret_key_as_paper_key = new QAction(
        tr("Export Secret Key As A Paper Key") + QString(" (BETA)"), this);
    connect(export_secret_key_as_paper_key, &QAction::triggered, this,
            &KeyPairOperaTab::slot_export_paper_key);
    if (!m_key_.IsPrivateKey()) {
      export_secret_key_as_paper_key->setDisabled(true);
    }
    secret_key_export_opera_menu_->addAction(export_secret_key_as_paper_key);
  }

  rev_cert_opera_menu_ = new QMenu(this);

  auto* rev_cert_gen_action =
      new QAction(tr("Generate Revoke Certificate"), this);
  connect(rev_cert_gen_action, &QAction::triggered, this,
          &KeyPairOperaTab::slot_gen_revoke_cert);

  auto* revoke_cert_import_action =
      new QAction(tr("Import Revoke Certificate"));
  connect(revoke_cert_import_action, &QAction::triggered, this,
          &KeyPairOperaTab::slot_import_revoke_cert);

  rev_cert_opera_menu_->addAction(revoke_cert_import_action);
  rev_cert_opera_menu_->addAction(rev_cert_gen_action);
}

void KeyPairOperaTab::slot_export_public_key() {
  auto [err, gf_buffer] =
      GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
          .ExportKey(m_key_, false, true, false);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseMessageBox(this, err);
    return;
  }

  // generate a file name
#if defined(_WIN32) || defined(WIN32)

  auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                     m_key_.GetId() + ")_pub.asc";
#else
  auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                     m_key_.GetId() + ")_pub.asc";
#endif
  std::replace(file_string.begin(), file_string.end(), ' ', '_');

  auto file_name = QFileDialog::getSaveFileName(
      this, tr("Export Key To File"), file_string,
      tr("Key Files") + " (*.asc *.txt);;All Files (*)");

  if (file_name.isEmpty()) return;

  if (!WriteFileGFBuffer(file_name, gf_buffer)) {
    QMessageBox::critical(this, tr("Export Error"),
                          tr("Couldn't open %1 for writing").arg(file_name));
    return;
  }
}

void KeyPairOperaTab::slot_export_short_private_key() {
  QString warning_message =
      "<h3><b>" + tr("WARNING: You are about to export your") + " " +
      "<font color=\"red\">" + tr("PRIVATE KEY") + "</font>!</b></h3>" + "<p>" +
      tr("This is NOT your Public Key, so <b>DO NOT</b> share it with "
         "anyone.") +
      "</p>" + "<p>" +
      tr("You are exporting a <b>minimum size</b> private key, which "
         "removes all signatures except for the latest self-signatures.") +
      "</p>" + "<p>" + tr("Do you <b>REALLY</b> want to proceed?") + "</p>";

  int ret = QMessageBox::warning(this, tr("Exporting Short Private Key"),
                                 warning_message,
                                 QMessageBox::Cancel | QMessageBox::Ok);

  // export key, if ok was clicked
  if (ret == QMessageBox::Ok) {
    auto [err, gf_buffer] =
        GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
            .ExportKey(m_key_, true, true, true);
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      CommonUtils::RaiseMessageBox(this, err);
      return;
    }

    // generate a file name
#if defined(_WIN32) || defined(WIN32)

    auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                       m_key_.GetId() + ")_short_secret.asc";
#else
    auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                       m_key_.GetId() + ")_short_secret.asc";
#endif
    std::replace(file_string.begin(), file_string.end(), ' ', '_');

    auto file_name = QFileDialog::getSaveFileName(
        this, tr("Export Key To File"), file_string,
        tr("Key Files") + " (*.asc *.txt);;All Files (*)");

    if (file_name.isEmpty()) return;

    if (!WriteFileGFBuffer(file_name, gf_buffer)) {
      QMessageBox::critical(this, tr("Export Error"),
                            tr("Couldn't open %1 for writing").arg(file_name));
      return;
    }
  }
}

void KeyPairOperaTab::slot_export_private_key() {
  // Show a information box with explanation about private key
  QString warning_message =
      "<h3><b>" + tr("WARNING: You are about to export your") + " " +
      "<font color=\"red\">" + tr("PRIVATE KEY") + "</font>!</b></h3>" + "<p>" +
      tr("This operation will export your <b>private key</b>, including both "
         "the main key and all subkeys, "
         "into an external file. This key is extremely sensitive, and anyone "
         "with access to it can impersonate you. "
         "DO NOT share this file with anyone!") +
      "</p>" + "<p>" +
      tr("Are you <b>ABSOLUTELY SURE</b> you want to proceed?") + "</p>";

  int ret =
      QMessageBox::warning(this, tr("Exporting Private Key"), warning_message,
                           QMessageBox::Cancel | QMessageBox::Ok);

  // export key, if ok was clicked
  if (ret == QMessageBox::Ok) {
    auto [err, gf_buffer] =
        GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
            .ExportKey(m_key_, true, true, false);
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      CommonUtils::RaiseMessageBox(this, err);
      return;
    }

    // generate a file name
#if defined(_WIN32) || defined(WIN32)
    auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                       m_key_.GetId() + ")_full_secret.asc";
#else
    auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                       m_key_.GetId() + ")_full_secret.asc";
#endif
    std::replace(file_string.begin(), file_string.end(), ' ', '_');

    auto file_name = QFileDialog::getSaveFileName(
        this, tr("Export Key To File"), file_string,
        tr("Key Files") + " (*.asc *.txt);;All Files (*)");

    if (file_name.isEmpty()) return;

    if (!WriteFileGFBuffer(file_name, gf_buffer)) {
      QMessageBox::critical(this, tr("Export Error"),
                            tr("Couldn't open %1 for writing").arg(file_name));
      return;
    }
  }
}

void KeyPairOperaTab::slot_modify_edit_datetime() {
  auto* dialog = new KeySetExpireDateDialog(current_gpg_context_channel_,
                                            m_key_.GetId(), this);
  dialog->show();
}

void KeyPairOperaTab::slot_publish_key_to_server() {
  if (Module::IsModuleActivate(kKeyServerSyncModuleID)) {
    auto [err, gf_buffer] =
        GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
            .ExportKey(m_key_, false, true, false);

    auto fpr = m_key_.GetFingerprint();
    auto key_text = gf_buffer.ConvertToQByteArray();

    Module::TriggerEvent(
        "REQUEST_UPLOAD_PUBLIC_KEY",
        {
            {"key_text", QString::fromUtf8(key_text)},
        },
        [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
            Module::Event::Params p) {
          LOG_D() << "REQUEST_UPLOAD_PUBLIC_KEY "
                     "callback: "
                  << i << ei;

          if (p["ret"] != "0" || !p["error_msg"].isEmpty()) {
            LOG_E() << "An error occurred trying to get data "
                       "from key:"
                    << fpr << "error message: " << p["error_msg"]
                    << "reply data: " << p["reply_data"];

            // Notify user of the error
            QString error_message = p["error_msg"];
            QMessageBox::critical(
                this, tr("Key Upload Failed"),
                tr("Failed to upload public key to the server.\n"
                   "Fingerprint: %1\n"
                   "Error: %2")
                    .arg(fpr, error_message));
          } else if (p.contains("token") && p.contains("status") &&
                     p.contains("fingerprint")) {
            const auto token = p["token"];
            const auto status = p["status"];
            const auto reply_fpr = p["fingerprint"];
            LOG_D() << "got key data of key " << fpr
                    << "from key server, token: " << token << "fpr: " << fpr
                    << "status: " << status;

            // Handle successful response
            QString status_message =
                tr("The following email addresses have status:\n");
            QJsonDocument json_doc = QJsonDocument::fromJson(status.toUtf8());
            QStringList email_list;
            if (!json_doc.isNull() && json_doc.isObject()) {
              QJsonObject json_obj = json_doc.object();
              for (auto it = json_obj.constBegin(); it != json_obj.constEnd();
                   ++it) {
                status_message +=
                    QString("%1: %2\n").arg(it.key(), it.value().toString());
                email_list.append(it.key());
              }
            } else {
              status_message += tr("Could not parse status information.");
            }

            // Notify user of successful upload and status details
            QMessageBox::information(
                this, tr("Public Key Upload Successful"),
                tr("The public key was successfully uploaded to the "
                   "key server keys.openpgp.org.\n"
                   "Fingerprint: %1\n\n"
                   "%2\n"
                   "Please check your email (%3) for further "
                   "verification from keys.openpgp.org.\n\n"
                   "Note: For verification, you can find more "
                   "information here: "
                   "https://keys.openpgp.org/about")
                    .arg(fpr, status_message, email_list.join(", ")));
          }
        });
    return;
  }

  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(m_key_.GetId());
  auto* dialog = new KeyUploadDialog(current_gpg_context_channel_, keys, this);
  dialog->show();
  dialog->SlotUpload();
}

void KeyPairOperaTab::slot_update_key_from_server() {
  if (Module::IsModuleActivate(kKeyServerSyncModuleID)) {
    CommonUtils::GetInstance()->ImportKeyByKeyServerSyncModule(
        this, current_gpg_context_channel_, {m_key_.GetFingerprint()});
    return;
  }
  CommonUtils::GetInstance()->ImportKeyFromKeyServer(
      current_gpg_context_channel_, {m_key_.GetId()});
}

void KeyPairOperaTab::slot_gen_revoke_cert() {
  QStringList codes;
  codes << tr("0 -> No Reason.") << tr("1 -> This key is no more safe.")
        << tr("2 -> Key is outdated.") << tr("3 -> Key is no longer used");
  auto* revocation_options_dialog = new RevocationOptionsDialog(codes, this);

  connect(revocation_options_dialog,
          &RevocationOptionsDialog::SignalRevokeOptionAccepted, this,
          [this](int code, const QString& text) {
            auto literal =
                QString("%1 (*.rev)").arg(tr("Revocation Certificates"));
            QString m_output_file_name;

#if defined(_WIN32) || defined(WIN32)
            auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() +
                               "](" + m_key_.GetId() + ").rev";
#else
            auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() +
                               ">(" + m_key_.GetId() + ").rev";
#endif

            QFileDialog dialog(this, tr("Generate revocation certificate"),
                               file_string, literal);
            dialog.setDefaultSuffix(".rev");
            dialog.setAcceptMode(QFileDialog::AcceptSave);

            if (dialog.exec() != QFileDialog::Reject) {
              m_output_file_name = dialog.selectedFiles().front();
            }

            if (!m_output_file_name.isEmpty()) {
              GpgKeyOpera::GetInstance(current_gpg_context_channel_)
                  .GenerateRevokeCert(m_key_, m_output_file_name, code, text);
            }
          });

  revocation_options_dialog->show();
}

void KeyPairOperaTab::slot_modify_password() {
  GpgKeyOpera::GetInstance(current_gpg_context_channel_)
      .ModifyPassword(m_key_, [this](GpgError err, const DataObjectPtr&) {
        CommonUtils::RaiseMessageBox(this, err);
      });
}

void KeyPairOperaTab::slot_modify_tofu_policy() {
  QStringList items;
  items << tr("Policy Auto") << tr("Policy Good") << tr("Policy Bad")
        << tr("Policy Ask") << tr("Policy Unknown");

  bool ok;
  QString item = QInputDialog::getItem(
      this, tr("Modify TOFU Policy(Default is Auto)"),
      tr("Policy for the Key Pair:"), items, 0, false, &ok);
  if (ok && !item.isEmpty()) {
    gpgme_tofu_policy_t tofu_policy = GPGME_TOFU_POLICY_AUTO;
    if (item == tr("Policy Auto")) {
      tofu_policy = GPGME_TOFU_POLICY_AUTO;
    } else if (item == tr("Policy Good")) {
      tofu_policy = GPGME_TOFU_POLICY_GOOD;
    } else if (item == tr("Policy Bad")) {
      tofu_policy = GPGME_TOFU_POLICY_BAD;
    } else if (item == tr("Policy Ask")) {
      tofu_policy = GPGME_TOFU_POLICY_ASK;
    } else if (item == tr("Policy Unknown")) {
      tofu_policy = GPGME_TOFU_POLICY_UNKNOWN;
    }
    auto err = GpgKeyOpera::GetInstance(current_gpg_context_channel_)
                   .ModifyTOFUPolicy(m_key_, tofu_policy);
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      QMessageBox::critical(this, tr("Not Successful"),
                            tr("Modify TOFU policy not successfully."));
    }
  }
}

void KeyPairOperaTab::slot_set_owner_trust_level() {
  auto* function = new SetOwnerTrustLevel(this);
  function->Exec(current_gpg_context_channel_, m_key_.GetId());
  function->deleteLater();
}

void KeyPairOperaTab::slot_import_revoke_cert() {
  // Show a information box with explanation about private key
  int ret = QMessageBox::information(
      this, tr("Import Key Revocation Certificate"),
      "<h3>" + tr("You are about to import the") + "<font color=\"red\">" +
          " " + tr("REVOCATION CERTIFICATE") + " " + "</font>!</h3>\n" +
          tr("A successful import will result in the key being irreversibly "
             "revoked.") +
          "<br />" + tr("Do you REALLY want to execute this operation?"),
      QMessageBox::Cancel | QMessageBox::Ok);

  // export key, if ok was clicked
  if (ret != QMessageBox::Ok) return;

  auto rev_file_name = QFileDialog::getOpenFileName(
      this, tr("Import Key Revocation Certificate"), {},
      tr("Revocation Certificates") + " (*.rev)");

  if (rev_file_name.isEmpty()) return;

  QFileInfo rev_file_info(rev_file_name);

  if (!rev_file_info.isFile() || !rev_file_info.isReadable()) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  // max file size is 1 mb
  if (rev_file_info.size() > static_cast<qint64>(1024 * 1024)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("The target file is too large for a key revocation certificate."));
    return;
  }

  QFile rev_file(rev_file_info.absoluteFilePath());

  if (!rev_file.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  emit UISignalStation::GetInstance() -> SignalKeyRevoked(m_key_.GetId());
  CommonUtils::GetInstance()->SlotImportKeys(
      nullptr, current_gpg_context_channel_, rev_file.readAll());
}

void KeyPairOperaTab::slot_export_paper_key() {
  if (!Module::IsModuleActivate(kPaperKeyModuleID)) return;

  QString warning_message =
      "<h3><b>" + tr("WARNING: You are about to export your") + " " +
      "<font color=\"red\">" + tr("PRIVATE KEY") + "</font>!</b></h3>" + "<p>" +
      tr("This is NOT your Public Key, so <b>DO NOT</b> share it with "
         "anyone.") +
      "</p>" + "<p>" +
      tr("A <b>PaperKey</b> is a human-readable printout of your private key, "
         "which can be used to recover your key if you lose access to your "
         "digital copy. ") +
      "</p>" + "<p>" +
      tr("Keep this paper copy in a safe and secure place, such as a fireproof "
         "safe or a trusted vault.") +
      "</p>" + "<p>" +
      tr("Are you <b>ABSOLUTELY SURE</b> you want to proceed?") + "</p>";

  int ret = QMessageBox::warning(
      this, tr("Exporting Private Key as a PaperKey"), warning_message,
      QMessageBox::Cancel | QMessageBox::Ok);

  // export key, if ok was clicked
  if (ret == QMessageBox::Ok) {
    auto [err, gf_buffer] =
        GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
            .ExportKey(m_key_, true, false, true);
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      CommonUtils::RaiseMessageBox(this, err);
      return;
    }

    // generate a file name
#if defined(_WIN32) || defined(WIN32)
    auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                       m_key_.GetId() + ")_paper_key.txt";
#else
    auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                       m_key_.GetId() + ")_paper_key.txt";
#endif
    std::replace(file_string.begin(), file_string.end(), ' ', '_');

    auto file_name = QFileDialog::getSaveFileName(
        this, tr("Export Key To File"), file_string,
        tr("Key Files") + " (*.txt);;All Files (*)");

    if (file_name.isEmpty()) return;

    Module::TriggerEvent(
        "REQUEST_TRANS_KEY_2_PAPER_KEY",
        {
            {"secret_key", QString(gf_buffer.ConvertToQByteArray().toBase64())},
        },
        [this, file_name](Module::EventIdentifier i,
                          Module::Event::ListenerIdentifier ei,
                          Module::Event::Params p) {
          LOG_D() << "REQUEST_TRANS_KEY_2_PAPER_KEY callback: " << i << ei;

          if (p["ret"] != "0" || p["paper_key"].isEmpty()) {
            QMessageBox::critical(
                this, tr("Error"),
                tr("An error occurred trying to generate Paper Key."));
            return;
          }

          if (!WriteFile(file_name, p["paper_key"].toLatin1())) {
            QMessageBox::critical(
                this, tr("Export Error"),
                tr("Couldn't open %1 for writing").arg(file_name));
            return;
          }
        });
  }
}

void KeyPairOperaTab::slot_import_paper_key() {
  if (!Module::IsModuleActivate(kPaperKeyModuleID)) return;

  auto [err, gf_buffer] =
      GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
          .ExportKey(m_key_, false, false, true);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseMessageBox(this, err);
    return;
  }

  // generate a file name
#if defined(_WIN32) || defined(WIN32)
  auto file_string = m_key_.GetName() + "[" + m_key_.GetEmail() + "](" +
                     m_key_.GetId() + ")_paper_key.txt";
#else
  auto file_string = m_key_.GetName() + "<" + m_key_.GetEmail() + ">(" +
                     m_key_.GetId() + ")_paper_key.txt";
#endif
  std::replace(file_string.begin(), file_string.end(), ' ', '_');

  auto file_name = QFileDialog::getOpenFileName(
      this, tr("Import A Paper Key"), file_string,
      tr("Paper Key File") + " (*.txt);;All Files (*)");

  if (file_name.isEmpty()) return;

  QFileInfo file_info(file_name);

  if (!file_info.isFile() || !file_info.isReadable()) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  if (file_info.size() > static_cast<qint64>(1024 * 1024)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("The target file is too large for a paper key keyring."));
    return;
  }

  auto [succ, gf_in_buff] = ReadFileGFBuffer(file_name);
  if (!succ) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  Module::TriggerEvent(
      "REQUEST_TRANS_PAPER_KEY_2_KEY",
      {
          {"public_key", QString(gf_buffer.ConvertToQByteArray().toBase64())},
          {"paper_key_secrets",
           QString(gf_in_buff.ConvertToQByteArray().toBase64())},
      },
      [this](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
             Module::Event::Params p) {
        LOG_D() << "REQUEST_TRANS_PAPER_KEY_2_KEY callback: " << i << ei;

        if (p["ret"] != "0" || p["secret_key"].isEmpty()) {
          QMessageBox::critical(this, tr("Error"),
                                tr("An error occurred trying to recover the "
                                   "Paper Key back to the private key."));
          return;
        }

        CommonUtils::GetInstance()->SlotImportKeys(
            this, current_gpg_context_channel_,
            QByteArray::fromBase64(p["secret_key"].toLatin1()));
      });
}

}  // namespace GpgFrontend::UI
