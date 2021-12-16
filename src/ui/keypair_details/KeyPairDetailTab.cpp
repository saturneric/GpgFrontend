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

#include "ui/keypair_details/KeyPairDetailTab.h"

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExportor.h"
#include "gpg/function/GpgKeyOpera.h"
#include "ui/SignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/WaitingDialog.h"

namespace GpgFrontend::UI {
KeyPairDetailTab::KeyPairDetailTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), mKey(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  ownerBox = new QGroupBox(_("Owner"));
  keyBox = new QGroupBox(_("Master Key"));
  fingerprintBox = new QGroupBox(_("Fingerprint"));
  additionalUidBox = new QGroupBox(_("Additional UIDs"));

  nameVarLabel = new QLabel();
  nameVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  emailVarLabel = new QLabel();
  emailVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  commentVarLabel = new QLabel();
  commentVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  keyidVarLabel = new QLabel();
  keyidVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  usageVarLabel = new QLabel();
  actualUsageVarLabel = new QLabel();

  keySizeVarLabel = new QLabel();
  expireVarLabel = new QLabel();
  createdVarLabel = new QLabel();
  lastUpdateVarLabel = new QLabel();
  algorithmVarLabel = new QLabel();
  masterKeyExistVarLabel = new QLabel();

  auto* mvbox = new QVBoxLayout();
  auto* vboxKD = new QGridLayout();
  auto* vboxOD = new QGridLayout();

  vboxOD->addWidget(new QLabel(QString(_("Name")) + ": "), 0, 0);
  vboxOD->addWidget(new QLabel(QString(_("Email Address")) + ": "), 1, 0);
  vboxOD->addWidget(new QLabel(QString(_("Comment")) + ": "), 2, 0);
  vboxOD->addWidget(nameVarLabel, 0, 1);
  vboxOD->addWidget(emailVarLabel, 1, 1);
  vboxOD->addWidget(commentVarLabel, 2, 1);

  vboxKD->addWidget(new QLabel(QString(_("Key ID")) + ": "), 0, 0);
  vboxKD->addWidget(new QLabel(QString(_("Algorithm")) + ": "), 1, 0);
  vboxKD->addWidget(new QLabel(QString(_("Key Size")) + ": "), 2, 0);
  vboxKD->addWidget(new QLabel(QString(_("Nominal Usage")) + ": "), 3, 0);
  vboxKD->addWidget(new QLabel(QString(_("Actual Usage")) + ": "), 4, 0);
  vboxKD->addWidget(new QLabel(QString(_("Create Date (Local Time)")) + ": "),
                    5, 0);
  vboxKD->addWidget(new QLabel(QString(_("Expires on (Local Time)")) + ": "), 6,
                    0);
  vboxKD->addWidget(new QLabel(QString(_("Last Update (Local Time)")) + ": "),
                    7, 0);
  vboxKD->addWidget(new QLabel(QString(_("Master Key Existence")) + ": "), 8,
                    0);

  keyidVarLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  vboxKD->addWidget(keyidVarLabel, 0, 1, 1, 1);
  vboxKD->addWidget(algorithmVarLabel, 1, 1, 1, 2);
  vboxKD->addWidget(keySizeVarLabel, 2, 1, 1, 2);
  vboxKD->addWidget(usageVarLabel, 3, 1, 1, 2);
  vboxKD->addWidget(actualUsageVarLabel, 4, 1, 1, 2);
  vboxKD->addWidget(createdVarLabel, 5, 1, 1, 2);
  vboxKD->addWidget(expireVarLabel, 6, 1, 1, 2);
  vboxKD->addWidget(lastUpdateVarLabel, 7, 1, 1, 2);
  vboxKD->addWidget(masterKeyExistVarLabel, 8, 1, 1, 2);

  auto* copyKeyIdButton = new QPushButton(_("Copy"));
  copyKeyIdButton->setFlat(true);
  copyKeyIdButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  vboxKD->addWidget(copyKeyIdButton, 0, 2);
  connect(copyKeyIdButton, &QPushButton::clicked, this, [=]() {
    QString fpr = keyidVarLabel->text().trimmed();
    QClipboard* cb = QApplication::clipboard();
    cb->setText(fpr);
  });

  ownerBox->setLayout(vboxOD);
  mvbox->addWidget(ownerBox);
  keyBox->setLayout(vboxKD);
  mvbox->addWidget(keyBox);

  fingerPrintVarLabel = new QLabel();
  fingerPrintVarLabel->setWordWrap(false);
  fingerPrintVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  fingerPrintVarLabel->setStyleSheet("margin-left: 0; margin-right: 5;");
  fingerPrintVarLabel->setAlignment(Qt::AlignCenter);
  auto* hboxFP = new QHBoxLayout();

  hboxFP->addStretch();
  hboxFP->addWidget(fingerPrintVarLabel);

  auto* copyFingerprintButton = new QPushButton(_("Copy"));
  copyFingerprintButton->setFlat(true);
  copyFingerprintButton->setToolTip(_("copy fingerprint to clipboard"));
  connect(copyFingerprintButton, SIGNAL(clicked()), this,
          SLOT(slotCopyFingerprint()));

  hboxFP->addWidget(copyFingerprintButton);
  hboxFP->addStretch();

  fingerprintBox->setLayout(hboxFP);
  mvbox->addWidget(fingerprintBox);
  mvbox->addStretch();

  // Set Menu
  createOperaMenu();

  auto* opera_key_box = new QGroupBox(_("Operations"));
  auto* vbox_p_k = new QVBoxLayout();

  auto export_h_box_layout = new QHBoxLayout();
  vbox_p_k->addLayout(export_h_box_layout);

  auto* export_public_button = new QPushButton(_("Export Public Key"));
  export_h_box_layout->addWidget(export_public_button);
  connect(export_public_button, SIGNAL(clicked()), this,
          SLOT(slotExportPublicKey()));

  if (mKey.is_private_key()) {
    auto* export_private_button = new QPushButton(_("Export Private Key"));
    export_private_button->setStyleSheet("text-align:center;");
    export_private_button->setMenu(secretKeyExportOperaMenu);
    export_h_box_layout->addWidget(export_private_button);

    if (mKey.has_master_key()) {
      auto* edit_expires_button =
          new QPushButton(_("Modify Expiration Datetime (Master Key)"));
      connect(edit_expires_button, SIGNAL(clicked()), this,
              SLOT(slotModifyEditDatetime()));
      auto* edit_password_button = new QPushButton(_("Modify Password"));
      connect(edit_password_button, SIGNAL(clicked()), this,
              SLOT(slotModifyPassword()));

      auto edit_h_box_layout = new QHBoxLayout();
      edit_h_box_layout->addWidget(edit_expires_button);
      edit_h_box_layout->addWidget(edit_password_button);
      vbox_p_k->addLayout(edit_h_box_layout);
    }
  }

  auto advance_h_box_layout = new QHBoxLayout();
  auto* key_server_opera_button =
      new QPushButton(_("Key Server Operation (Pubkey)"));
  key_server_opera_button->setStyleSheet("text-align:center;");
  key_server_opera_button->setMenu(keyServerOperaMenu);
  advance_h_box_layout->addWidget(key_server_opera_button);

  if (mKey.is_private_key() && mKey.has_master_key()) {
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
  mvbox->addWidget(opera_key_box);
  vbox_p_k->addWidget(modify_tofu_button);

  auto* expBox = new QHBoxLayout();
  QPixmap pixmap(":warning.png");

  expLabel = new QLabel();
  iconLabel = new QLabel();

  iconLabel->setPixmap(pixmap.scaled(24, 24, Qt::KeepAspectRatio));
  expLabel->setAlignment(Qt::AlignCenter);
  expBox->addStretch();
  expBox->addWidget(iconLabel);
  expBox->addWidget(expLabel);
  expBox->addStretch();
  mvbox->addLayout(expBox);
  mvbox->setContentsMargins(0, 0, 0, 0);

  // when key database updated
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slotRefreshKey()));

  slotRefreshKeyInfo();
  setAttribute(Qt::WA_DeleteOnClose, true);
  setLayout(mvbox);
}

void KeyPairDetailTab::slotExportPublicKey() {
  ByteArrayPtr keyArray = nullptr;

  if (!GpgKeyImportExportor::GetInstance().ExportKey(mKey, keyArray)) {
    QMessageBox::critical(this, _("Error"),
                          _("An error occurred during the export operation."));
    return;
  }
  auto file_string =
      mKey.name() + " " + mKey.email() + "(" + mKey.id() + ")_pub.asc";
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

void KeyPairDetailTab::slotExportShortPrivateKey() {
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

    if (!GpgKeyImportExportor::GetInstance().ExportSecretKeyShortest(
            mKey, keyArray)) {
      QMessageBox::critical(
          this, _("Error"),
          _("An error occurred during the export operation."));
      return;
    }
    auto file_string = mKey.name() + " " + mKey.email() + "(" + mKey.id() +
                       ")_short_secret.asc";
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

void KeyPairDetailTab::slotExportPrivateKey() {
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

    if (!GpgKeyImportExportor::GetInstance().ExportSecretKey(mKey, keyArray)) {
      QMessageBox::critical(
          this, _("Error"),
          _("An error occurred during the export operation."));
      return;
    }
    auto file_string = mKey.name() + " " + mKey.email() + "(" + mKey.id() +
                       ")_full_secret.asc";
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

void KeyPairDetailTab::slotCopyFingerprint() {
  QString fpr = fingerPrintVarLabel->text().trimmed().replace(" ", QString());
  QClipboard* cb = QApplication::clipboard();
  cb->setText(fpr);
}

void KeyPairDetailTab::slotModifyEditDatetime() {
  auto dialog = new KeySetExpireDateDialog(mKey.id(), this);
  dialog->show();
}

void KeyPairDetailTab::slotRefreshKeyInfo() {
  // Show the situation that master key not exists.
  masterKeyExistVarLabel->setText(mKey.has_master_key() ? _("Exists")
                                                        : _("Not Exists"));
  if (!mKey.has_master_key()) {
    auto paletteExpired = masterKeyExistVarLabel->palette();
    paletteExpired.setColor(masterKeyExistVarLabel->foregroundRole(), Qt::red);
    masterKeyExistVarLabel->setPalette(paletteExpired);
  } else {
    auto paletteValid = masterKeyExistVarLabel->palette();
    paletteValid.setColor(masterKeyExistVarLabel->foregroundRole(),
                          Qt::darkGreen);
    masterKeyExistVarLabel->setPalette(paletteValid);
  }

  if (mKey.expired()) {
    auto paletteExpired = expireVarLabel->palette();
    paletteExpired.setColor(expireVarLabel->foregroundRole(), Qt::red);
    expireVarLabel->setPalette(paletteExpired);
  } else {
    auto paletteValid = expireVarLabel->palette();
    paletteValid.setColor(expireVarLabel->foregroundRole(), Qt::darkGreen);
    expireVarLabel->setPalette(paletteValid);
  }

  nameVarLabel->setText(QString::fromStdString(mKey.name()));
  emailVarLabel->setText(QString::fromStdString(mKey.email()));

  commentVarLabel->setText(QString::fromStdString(mKey.comment()));
  keyidVarLabel->setText(QString::fromStdString(mKey.id()));

  std::stringstream usage_steam;

  if (mKey.can_certify()) usage_steam << _("Certificate") << " ";
  if (mKey.can_encrypt()) usage_steam << _("Encrypt") << " ";
  if (mKey.can_sign()) usage_steam << _("Sign") << " ";
  if (mKey.can_authenticate()) usage_steam << _("Auth") << " ";

  usageVarLabel->setText(usage_steam.str().c_str());

  std::stringstream actual_usage_steam;

  if (mKey.CanCertActual()) actual_usage_steam << _("Certificate") << " ";
  if (mKey.CanEncrActual()) actual_usage_steam << _("Encrypt") << " ";
  if (mKey.CanSignActual()) actual_usage_steam << _("Sign") << " ";
  if (mKey.CanAuthActual()) actual_usage_steam << _("Auth") << " ";

  actualUsageVarLabel->setText(actual_usage_steam.str().c_str());

  std::string key_size_val, key_expire_val, key_create_time_val, key_algo_val,
      key_last_update_val;

  key_size_val = std::to_string(mKey.length());

  if (to_time_t(boost::posix_time::ptime(mKey.expires())) == 0) {
    expireVarLabel->setText(_("Never Expire"));
  } else {
    expireVarLabel->setText(QLocale::system().toString(
        QDateTime::fromTime_t(to_time_t(mKey.expires()))));
  }

  key_algo_val = mKey.pubkey_algo();

  createdVarLabel->setText(QLocale::system().toString(
      QDateTime::fromTime_t(to_time_t(mKey.create_time()))));

  if (to_time_t(boost::posix_time::ptime(mKey.last_update())) == 0) {
    lastUpdateVarLabel->setText(_("No Data"));
  } else {
    lastUpdateVarLabel->setText(QLocale::system().toString(
        QDateTime::fromTime_t(to_time_t(mKey.last_update()))));
  }

  keySizeVarLabel->setText(key_size_val.c_str());
  algorithmVarLabel->setText(key_algo_val.c_str());
  fingerPrintVarLabel->setText(beautify_fingerprint(mKey.fpr()).c_str());

  iconLabel->hide();
  expLabel->hide();

  if (mKey.expired()) {
    iconLabel->show();
    expLabel->show();
    expLabel->setText(_("Warning: The Master Key has expired."));
  }
  if (mKey.revoked()) {
    iconLabel->show();
    expLabel->show();
    expLabel->setText(_("Warning: The Master Key has been revoked."));
  }
}

void KeyPairDetailTab::createOperaMenu() {
  keyServerOperaMenu = new QMenu(this);

  auto* uploadKeyPair = new QAction(_("Upload Key Pair to Key Server"), this);
  connect(uploadKeyPair, SIGNAL(triggered()), this,
          SLOT(slotUploadKeyToServer()));
  if (!(mKey.is_private_key() && mKey.has_master_key()))
    uploadKeyPair->setDisabled(true);

  auto* updateKeyPair = new QAction(_("Sync Key Pair From Key Server"), this);
  connect(updateKeyPair, SIGNAL(triggered()), this,
          SLOT(slotUpdateKeyFromServer()));

  // when a key has master key, it should always upload to keyserver.
  if (mKey.has_master_key()) {
    updateKeyPair->setDisabled(true);
  }

  keyServerOperaMenu->addAction(uploadKeyPair);
  keyServerOperaMenu->addAction(updateKeyPair);

  secretKeyExportOperaMenu = new QMenu(this);

  auto* exportFullSecretKey = new QAction(_("Export Full Secret Key"), this);
  connect(exportFullSecretKey, SIGNAL(triggered()), this,
          SLOT(slotExportPrivateKey()));
  if (!mKey.is_private_key()) exportFullSecretKey->setDisabled(true);

  auto* exportShortestSecretKey =
      new QAction(_("Export Shortest Secret Key"), this);
  connect(exportShortestSecretKey, SIGNAL(triggered()), this,
          SLOT(slotExportShortPrivateKey()));

  secretKeyExportOperaMenu->addAction(exportFullSecretKey);
  secretKeyExportOperaMenu->addAction(exportShortestSecretKey);
}

void KeyPairDetailTab::slotUploadKeyToServer() {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(mKey.id());
  auto* dialog = new KeyUploadDialog(keys, this);
  dialog->show();
  dialog->slotUpload();
}

void KeyPairDetailTab::slotUpdateKeyFromServer() {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(mKey.id());
  auto* dialog = new KeyServerImportDialog(this);
  dialog->show();
  dialog->slotImport(keys);
}

void KeyPairDetailTab::slotGenRevokeCert() {
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
         m_output_file_name, "--gen-revoke", mKey.fpr().c_str()},
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
void KeyPairDetailTab::slotRefreshKey() {
  LOG(INFO) << _("Called");
  this->mKey = GpgKeyGetter::GetInstance().GetKey(mKey.id());
  this->slotRefreshKeyInfo();
}

void KeyPairDetailTab::slotModifyPassword() {
  auto err = GpgKeyOpera::GetInstance().ModifyPassword(mKey);
  if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) {
    QMessageBox::critical(this, _("Not Successful"),
                          QString(_("Modify password not successfully.")));
  }
}

void KeyPairDetailTab::slotModifyTOFUPolicy() {
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
    auto err = GpgKeyOpera::GetInstance().ModifyTOFUPolicy(mKey, tofu_policy);
    if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) {
      QMessageBox::critical(this, _("Not Successful"),
                            QString(_("Modify TOFU policy not successfully.")));
    }
  }
}

}  // namespace GpgFrontend::UI
