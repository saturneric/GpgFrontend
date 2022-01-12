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
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "KeyPairOperaTab.h"

#include "gpg/function/GpgKeyImportExporter.h"
#include "gpg/function/GpgKeyOpera.h"
#include "ui/KeyUploadDialog.h"
#include "ui/SignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/keypair_details/KeySetExpireDateDialog.h"

namespace GpgFrontend::UI {

KeyPairOperaTab::KeyPairOperaTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), m_key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  // Set Menu
  createOperaMenu();
  auto m_vbox = new QVBoxLayout(this);

  auto* opera_key_box = new QGroupBox(_("General Operations"));
  auto* vbox_p_k = new QVBoxLayout();

  auto export_h_box_layout = new QHBoxLayout();
  vbox_p_k->addLayout(export_h_box_layout);

  auto* export_public_button = new QPushButton(_("Export Public Key"));
  export_h_box_layout->addWidget(export_public_button);
  connect(export_public_button, SIGNAL(clicked()), this,
          SLOT(slotExportPublicKey()));

  if (m_key_.is_private_key()) {
    auto* export_private_button = new QPushButton(_("Export Private Key"));
    export_private_button->setStyleSheet("text-align:center;");
    export_private_button->setMenu(secretKeyExportOperaMenu);
    export_h_box_layout->addWidget(export_private_button);

    if (m_key_.has_master_key()) {
      auto* edit_expires_button =
          new QPushButton(_("Modify Expiration Datetime (Primary Key)"));
      connect(edit_expires_button, SIGNAL(clicked()), this,
              SLOT(slotModifyEditDatetime()));
      auto* edit_password_button = new QPushButton(_("Modify Password"));
      connect(edit_password_button, SIGNAL(clicked()), this,
              SLOT(slotModifyPassword()));

      vbox_p_k->addWidget(edit_expires_button);
      vbox_p_k->addWidget(edit_password_button);
    }
  }

  auto advance_h_box_layout = new QHBoxLayout();
  auto* key_server_opera_button =
      new QPushButton(_("Key Server Operation (Pubkey)"));
  key_server_opera_button->setStyleSheet("text-align:center;");
  key_server_opera_button->setMenu(keyServerOperaMenu);
  advance_h_box_layout->addWidget(key_server_opera_button);

  if (m_key_.is_private_key() && m_key_.has_master_key()) {
    auto* revoke_cert_gen_button =
        new QPushButton(_("Generate Revoke Certificate"));
    connect(revoke_cert_gen_button, SIGNAL(clicked()), this,
            SLOT(slotGenRevokeCert()));
    advance_h_box_layout->addWidget(revoke_cert_gen_button);
  }

  auto* modify_tofu_button = new QPushButton(_("Modify TOFU Policy"));
  connect(modify_tofu_button, SIGNAL(clicked()), this,
          SLOT(slotModifyTOFUPolicy()));

  vbox_p_k->addLayout(advance_h_box_layout);
  opera_key_box->setLayout(vbox_p_k);
  m_vbox->addWidget(opera_key_box);
  vbox_p_k->addWidget(modify_tofu_button);
  m_vbox->addStretch(0);

  setLayout(m_vbox);
}

void KeyPairOperaTab::createOperaMenu() {
  keyServerOperaMenu = new QMenu(this);

  auto* uploadKeyPair = new QAction(_("Upload Key Pair to Key Server"), this);
  connect(uploadKeyPair, SIGNAL(triggered()), this,
          SLOT(slotUploadKeyToServer()));
  if (!(m_key_.is_private_key() && m_key_.has_master_key()))
    uploadKeyPair->setDisabled(true);

  auto* updateKeyPair = new QAction(_("Sync Key Pair From Key Server"), this);
  connect(updateKeyPair, SIGNAL(triggered()), this,
          SLOT(slotUpdateKeyFromServer()));

  // when a key has primary key, it should always upload to keyserver.
  if (m_key_.has_master_key()) {
    updateKeyPair->setDisabled(true);
  }

  keyServerOperaMenu->addAction(uploadKeyPair);
  keyServerOperaMenu->addAction(updateKeyPair);

  secretKeyExportOperaMenu = new QMenu(this);

  auto* exportFullSecretKey = new QAction(_("Export Full Secret Key"), this);
  connect(exportFullSecretKey, SIGNAL(triggered()), this,
          SLOT(slotExportPrivateKey()));
  if (!m_key_.is_private_key()) exportFullSecretKey->setDisabled(true);

  auto* exportShortestSecretKey =
      new QAction(_("Export Shortest Secret Key"), this);
  connect(exportShortestSecretKey, SIGNAL(triggered()), this,
          SLOT(slotExportShortPrivateKey()));

  secretKeyExportOperaMenu->addAction(exportFullSecretKey);
  secretKeyExportOperaMenu->addAction(exportShortestSecretKey);
}

void KeyPairOperaTab::slotExportPublicKey() {
  ByteArrayPtr keyArray = nullptr;

  if (!GpgKeyImportExporter::GetInstance().ExportKey(m_key_, keyArray)) {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during the export operation."));
    return;
  }
  auto file_string =
      m_key_.name() + " " + m_key_.email() + "(" + m_key_.id() + ")_pub.asc";
  auto file_name =
      QFileDialog::getSaveFileName(
          this, _("Export Key To File"), QString::fromStdString(file_string),
          QString(_("Key Files")) + " (*.asc *.txt);;All Files (*)")
          .toStdString();

  if (file_name.empty()) return;

  if (!write_buffer_to_file(file_name, *keyArray)) {
    QMessageBox::critical(
        this, _("Export Error"),
        QString(_("Couldn't open %1 for writing")).arg(file_name.c_str()));
    return;
  }
}

void KeyPairOperaTab::slotExportShortPrivateKey() {
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
    auto file_string = m_key_.name() + " " + m_key_.email() + "(" +
                       m_key_.id() + ")_short_secret.asc";
    auto file_name =
        QFileDialog::getSaveFileName(
            this, _("Export Key To File"), QString::fromStdString(file_string),
            QString(_("Key Files")) + " (*.asc *.txt);;All Files (*)")
            .toStdString();

    if (file_name.empty()) return;

    if (!write_buffer_to_file(file_name, *keyArray)) {
      QMessageBox::critical(
          this, _("Export Error"),
          QString(_("Couldn't open %1 for writing")).arg(file_name.c_str()));
      return;
    }
  }
}

void KeyPairOperaTab::slotExportPrivateKey() {
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
    auto file_string = m_key_.name() + " " + m_key_.email() + "(" +
                       m_key_.id() + ")_full_secret.asc";
    auto file_name =
        QFileDialog::getSaveFileName(
            this, _("Export Key To File"), QString::fromStdString(file_string),
            QString(_("Key Files")) + " (*.asc *.txt);;All Files (*)")
            .toStdString();

    if (file_name.empty()) return;

    if (!write_buffer_to_file(file_name, *keyArray)) {
      QMessageBox::critical(
          this, _("Export Error"),
          QString(_("Couldn't open %1 for writing")).arg(file_name.c_str()));
      return;
    }
  }
}

void KeyPairOperaTab::slotModifyEditDatetime() {
  auto dialog = new KeySetExpireDateDialog(m_key_.id(), this);
  dialog->show();
}

void KeyPairOperaTab::slotUploadKeyToServer() {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(m_key_.id());
  auto* dialog = new KeyUploadDialog(keys, this);
  dialog->show();
  dialog->slotUpload();
}

void KeyPairOperaTab::slotUpdateKeyFromServer() {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(m_key_.id());
  auto* dialog = new KeyServerImportDialog(this);
  dialog->show();
  dialog->slotImport(keys);
}

void KeyPairOperaTab::slotGenRevokeCert() {
  auto literal = QString("%1 (*.rev)").arg(_("Revocation Certificates"));
  QString m_output_file_name;

  QFileDialog dialog(this, "Generate revocation certificate", QString(),
                     literal);
  dialog.setDefaultSuffix(".rev");
  dialog.setAcceptMode(QFileDialog::AcceptSave);

  if (dialog.exec()) m_output_file_name = dialog.selectedFiles().front();

  if (!m_output_file_name.isEmpty())
    CommonUtils::GetInstance()->slotExecuteGpgCommand(
        {"--command-fd", "0", "--status-fd", "1", "--no-tty", "-o",
         m_output_file_name, "--gen-revoke", m_key_.fpr().c_str()},
        [](QProcess* proc) -> void {
          // Code From Gpg4Win
          while (proc->canReadLine()) {
            const QString line = QString::fromUtf8(proc->readLine()).trimmed();
            LOG(INFO) << "line" << line.toStdString();
            if (line == QLatin1String("[GNUPG:] GET_BOOL gen_revoke.okay")) {
              proc->write("y\n");
            } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                             "ask_revocation_reason.code")) {
              proc->write("0\n");
            } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                             "ask_revocation_reason.text")) {
              proc->write("\n");
            } else if (line ==
                       QLatin1String(
                           "[GNUPG:] GET_BOOL openfile.overwrite.okay")) {
              // We asked before
              proc->write("y\n");
            } else if (line == QLatin1String("[GNUPG:] GET_BOOL "
                                             "ask_revocation_reason.okay")) {
              proc->write("y\n");
            }
          }
        });
}

void KeyPairOperaTab::slotModifyPassword() {
  auto err = GpgKeyOpera::GetInstance().ModifyPassword(m_key_);
  if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) {
    QMessageBox::critical(this, _("Not Successful"),
                          QString(_("Modify password not successfully.")));
  }
}

void KeyPairOperaTab::slotModifyTOFUPolicy() {
  QStringList items;
  items << _("Policy Auto") << _("Policy Good") << _("Policy Bad")
        << _("Policy Ask") << _("Policy Unknown");

  bool ok;
  QString item = QInputDialog::getItem(
      this, _("Modify TOFU Policy(Default is Auto)"),
      _("Policy for the Key Pair:"), items, 0, false, &ok);
  if (ok && !item.isEmpty()) {
    LOG(INFO) << "selected policy" << item.toStdString();
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
    if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) {
      QMessageBox::critical(this, _("Not Successful"),
                            QString(_("Modify TOFU policy not successfully.")));
    }
  }
}

}  // namespace GpgFrontend::UI
