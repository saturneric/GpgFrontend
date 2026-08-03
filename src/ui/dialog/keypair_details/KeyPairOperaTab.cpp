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

#include <utility>

#include "KeySetExpireDateDialog.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/model/GpgKey.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/function/ExportKey.h"
#include "ui/function/GenerateRevocationCert.h"
#include "ui/function/SetOwnerTrustLevel.h"

namespace GpgFrontend::UI {

KeyPairOperaTab::KeyPairOperaTab(int channel, GpgKeyPtr key, QWidget* parent)
    : QWidget(parent),
      current_gpg_context_channel_(channel),
      m_key_(std::move(key)) {
  assert(m_key_ != nullptr);

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

  if (m_key_->IsPrivateKey()) {
    auto* export_private_button = new QPushButton(tr("Export Private Key"));
    export_private_button->setStyleSheet("text-align:center;");
    export_private_button->setMenu(secret_key_export_opera_menu_);
    export_h_box_layout->addWidget(export_private_button);

    if (m_key_->IsHasMasterKey()) {
      auto if_expire_options_supported = IsOpSupported<SetExpireOpTag>(channel);

      if (if_expire_options_supported) {
        auto* edit_expires_button =
            new QPushButton(tr("Modify Expiration Datetime (Primary Key)"));
        connect(edit_expires_button, &QPushButton::clicked, this,
                &KeyPairOperaTab::slot_modify_edit_datetime);

        vbox_p_k->addWidget(edit_expires_button);
      }

      auto* edit_password_button = new QPushButton(tr("Modify Password"));
      connect(edit_password_button, &QPushButton::clicked, this,
              &KeyPairOperaTab::slot_modify_password);
      vbox_p_k->addWidget(edit_password_button);
    }
  }

  if (m_key_->IsPrivateKey() && m_key_->IsHasMasterKey()) {
    auto* revoke_cert_opera_button =
        new QPushButton(tr("Revoke Certificate Operation"));
    revoke_cert_opera_button->setStyleSheet("text-align:center;");
    revoke_cert_opera_button->setMenu(rev_cert_opera_menu_);
    vbox_p_k->addWidget(revoke_cert_opera_button);
  }

  auto if_owner_trust_level_supported =
      IsOpSupported<SetOwnerTrustLevelOpTag>(channel);
  auto* set_owner_trust_level_button =
      new QPushButton(tr("Set Owner Trust Level"));
  connect(set_owner_trust_level_button, &QPushButton::clicked, this,
          &KeyPairOperaTab::slot_set_owner_trust_level);

  opera_key_box->setLayout(vbox_p_k);
  m_vbox->addWidget(opera_key_box);
  // modify owner trust of public key
  if (!m_key_->IsPrivateKey() && if_owner_trust_level_supported) {
    vbox_p_k->addWidget(set_owner_trust_level_button);
  }
  m_vbox->addStretch(0);

  setLayout(m_vbox);

  Module::TriggerEvent(
      "KEY_PAIR_OPERA_MENU_CREATED",
      {
          {"tab", GFBuffer(RegisterQObject(this))},
          {"opera_layout", GFBuffer(RegisterQObject(vbox_p_k))},
          {"channel", GFBuffer(QString::number(current_gpg_context_channel_))},
          {"key_id", GFBuffer(m_key_->ID())},
          {"fpr", GFBuffer(m_key_->Fingerprint())},
          {"has_master_key", GFBuffer(QString::number(
                                 static_cast<int>(m_key_->IsHasMasterKey())))},
          {"is_private_key",
           GFBuffer(QString::number(static_cast<int>(m_key_->IsPrivateKey())))},
      });

  // set up signal
  connect(this, &KeyPairOperaTab::SignalKeyDatabaseRefresh,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
}

void KeyPairOperaTab::CreateOperaMenu() {
  secret_key_export_opera_menu_ = new QMenu(this);

  auto* export_full_secret_key =
      new QAction(tr("Export Full Secret Key"), this);
  connect(export_full_secret_key, &QAction::triggered, this,
          &KeyPairOperaTab::slot_export_private_key);
  if (!m_key_->IsPrivateKey()) export_full_secret_key->setDisabled(true);

  secret_key_export_opera_menu_->addAction(export_full_secret_key);

  auto* export_shortest_secret_key =
      new QAction(tr("Export Shortest Secret Key"), this);
  connect(export_shortest_secret_key, &QAction::triggered, this,
          &KeyPairOperaTab::slot_export_short_private_key);
  secret_key_export_opera_menu_->addAction(export_shortest_secret_key);

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

// The three export entries here and the ones in the key list are the same
// operation, so they go through the same function object rather than through
// two copies of the confirmation text and the save dialog. It deletes itself
// once its task is done — see the warning on the class.

void KeyPairOperaTab::slot_export_public_key() {
  (new ExportKey(this))->ExecPublic(current_gpg_context_channel_, m_key_);
}

void KeyPairOperaTab::slot_export_short_private_key() {
  (new ExportKey(this))->ExecShortPrivate(current_gpg_context_channel_, m_key_);
}

void KeyPairOperaTab::slot_export_private_key() {
  (new ExportKey(this))->ExecPrivate(current_gpg_context_channel_, m_key_);
}

void KeyPairOperaTab::slot_modify_edit_datetime() {
  auto* dialog =
      new KeySetExpireDateDialog(current_gpg_context_channel_, m_key_, this);
  dialog->show();
}

void KeyPairOperaTab::slot_gen_revoke_cert() {
  auto* function = new GenerateRevocationCert(this);
  function->Exec(current_gpg_context_channel_, m_key_);
}

void KeyPairOperaTab::slot_modify_password() {
  KeyManagementOperation::GetInstance(current_gpg_context_channel_)
      .ModifyPassword(m_key_, [this](GpgError err, const DataObjectPtr&) {
        CommonUtils::RaiseMessageBox(this, err);
      });
}

void KeyPairOperaTab::slot_set_owner_trust_level() {
  auto* function = new SetOwnerTrustLevel(this);
  function->Exec(current_gpg_context_channel_, m_key_);
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

  auto [succ, buffer] = ReadFileGFBuffer(rev_file_info.absoluteFilePath());
  if (!succ) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  emit UISignalStation::GetInstance() -> SignalKeyRevoked(m_key_->ID());

  // import revocation certificate
  CommonUtils::GetInstance()->SlotImportKeys(
      nullptr, current_gpg_context_channel_, buffer, true);
}

}  // namespace GpgFrontend::UI
