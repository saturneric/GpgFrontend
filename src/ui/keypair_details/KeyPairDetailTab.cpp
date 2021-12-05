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
#include "ui/SignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/WaitingDialog.h"

namespace GpgFrontend::UI {
KeyPairDetailTab::KeyPairDetailTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), mKey(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  keyid = mKey.id();

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
  algorithmVarLabel = new QLabel();

  // Show the situation that master key not exists.
  masterKeyExistVarLabel =
      new QLabel(mKey.has_master_key() ? _("Exists") : _("Not Exists"));
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
  vboxKD->addWidget(new QLabel(QString(_("Expires on")) + ": "), 5, 0);
  vboxKD->addWidget(new QLabel(QString(_("Last Update")) + ": "), 6, 0);
  vboxKD->addWidget(new QLabel(QString(_("Secret Key Existence")) + ": "), 7,
                    0);

  vboxKD->addWidget(keySizeVarLabel, 2, 1);
  vboxKD->addWidget(expireVarLabel, 5, 1);
  vboxKD->addWidget(algorithmVarLabel, 1, 1);
  vboxKD->addWidget(createdVarLabel, 6, 1);
  vboxKD->addWidget(keyidVarLabel, 0, 1);
  vboxKD->addWidget(usageVarLabel, 3, 1);
  vboxKD->addWidget(actualUsageVarLabel, 4, 1);
  vboxKD->addWidget(masterKeyExistVarLabel, 7, 1);

  ownerBox->setLayout(vboxOD);
  mvbox->addWidget(ownerBox);
  keyBox->setLayout(vboxKD);
  mvbox->addWidget(keyBox);

  fingerPrintVarLabel =
      new QLabel(beautifyFingerprint(QString::fromStdString(mKey.fpr())));
  fingerPrintVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  fingerPrintVarLabel->setStyleSheet("margin-left: 0; margin-right: 5;");
  auto* hboxFP = new QHBoxLayout();

  hboxFP->addWidget(fingerPrintVarLabel);

  auto* copyFingerprintButton = new QPushButton(_("Copy"));
  copyFingerprintButton->setFlat(true);
  copyFingerprintButton->setToolTip(_("copy fingerprint to clipboard"));
  connect(copyFingerprintButton, SIGNAL(clicked()), this,
          SLOT(slotCopyFingerprint()));

  hboxFP->addWidget(copyFingerprintButton);

  fingerprintBox->setLayout(hboxFP);
  mvbox->addWidget(fingerprintBox);
  mvbox->addStretch();

  if (mKey.is_private_key()) {
    auto* privKeyBox = new QGroupBox(_("Operations"));
    auto* vboxPK = new QVBoxLayout();

    auto* exportButton =
        new QPushButton(_("Export Private Key (Include Subkey)"));
    vboxPK->addWidget(exportButton);
    connect(exportButton, SIGNAL(clicked()), this,
            SLOT(slotExportPrivateKey()));

    if (mKey.has_master_key()) {
      auto* editExpiresButton =
          new QPushButton(_("Modify Expiration Datetime (Master Key)"));
      vboxPK->addWidget(editExpiresButton);
      connect(editExpiresButton, SIGNAL(clicked()), this,
              SLOT(slotModifyEditDatetime()));

      auto hBoxLayout = new QHBoxLayout();
      auto* keyServerOperaButton =
          new QPushButton(_("Key Server Operation (Pubkey)"));
      keyServerOperaButton->setStyleSheet("text-align:center;");

      auto* revokeCertGenButton =
          new QPushButton(_("Generate Revoke Certificate"));
      connect(revokeCertGenButton, SIGNAL(clicked()), this,
              SLOT(slotGenRevokeCert()));

      hBoxLayout->addWidget(keyServerOperaButton);
      hBoxLayout->addWidget(revokeCertGenButton);

      vboxPK->addLayout(hBoxLayout);
      connect(keyServerOperaButton, SIGNAL(clicked()), this,
              SLOT(slotModifyEditDatetime()));

      // Set Menu
      createKeyServerOperaMenu();
      keyServerOperaButton->setMenu(keyServerOperaMenu);
    }

    privKeyBox->setLayout(vboxPK);
    mvbox->addWidget(privKeyBox);
  }

  if ((mKey.expired()) || (mKey.revoked())) {
    auto* expBox = new QHBoxLayout();
    QPixmap pixmap(":warning.png");

    auto* expLabel = new QLabel();
    auto* iconLabel = new QLabel();
    if (mKey.expired()) {
      expLabel->setText(_("Warning: The Master Key has expired."));
    }
    if (mKey.revoked()) {
      expLabel->setText(_("Warning: The Master Key has been revoked"));
    }

    iconLabel->setPixmap(pixmap.scaled(24, 24, Qt::KeepAspectRatio));
    QFont font = expLabel->font();
    font.setBold(true);
    expLabel->setFont(font);
    expLabel->setAlignment(Qt::AlignCenter);
    expBox->addWidget(iconLabel);
    expBox->addWidget(expLabel);
    mvbox->addLayout(expBox);
  }

  // when key database updated
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slotRefreshKey()));

  mvbox->setContentsMargins(0, 0, 0, 0);

  slotRefreshKeyInfo();
  setAttribute(Qt::WA_DeleteOnClose, true);
  setLayout(mvbox);
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

    auto key = GpgKeyGetter::GetInstance().GetKey(keyid);
    if (!key.good()) {
      QMessageBox::critical(nullptr, _("Error"), _("Key Not Found."));
      return;
    }
    auto fileString =
        key.name() + " " + key.email() + "(" + key.id() + ")_secret.asc";
    auto fileName =
        QFileDialog::getSaveFileName(
            this, _("Export Key To File"), QString::fromStdString(fileString),
            QString(_("Key Files")) + " (*.asc *.txt);;All Files (*)")
            .toStdString();

    if (!write_buffer_to_file(fileName, *keyArray)) {
      QMessageBox::critical(
          nullptr, _("Export Error"),
          QString(_("Couldn't open %1 for writing")).arg(fileName.c_str()));
      return;
    }
  }
}

QString KeyPairDetailTab::beautifyFingerprint(QString fingerprint) {
  uint len = fingerprint.length();
  if ((len > 0) && (len % 4 == 0))
    for (uint n = 0; 4 * (n + 1) < len; ++n)
      fingerprint.insert(static_cast<int>(5u * n + 4u), ' ');
  return fingerprint;
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
  nameVarLabel->setText(QString::fromStdString(mKey.name()));
  emailVarLabel->setText(QString::fromStdString(mKey.email()));

  commentVarLabel->setText(QString::fromStdString(mKey.comment()));
  keyidVarLabel->setText(QString::fromStdString(mKey.id()));

  QString usage;
  QTextStream usage_steam(&usage);

  if (mKey.can_certify()) usage_steam << _("Cert") << " ";
  if (mKey.can_encrypt()) usage_steam << _("Encr") << " ";
  if (mKey.can_sign()) usage_steam << _("Sign") << " ";
  if (mKey.can_authenticate()) usage_steam << _("Auth") << " ";

  usageVarLabel->setText(usage);

  QString actualUsage;
  QTextStream actual_usage_steam(&actualUsage);

  if (mKey.CanCertActual()) actual_usage_steam << _("Cert") << " ";
  if (mKey.CanEncrActual()) actual_usage_steam << _("Encr") << " ";
  if (mKey.CanSignActual()) actual_usage_steam << _("Sign") << " ";
  if (mKey.CanAuthActual()) actual_usage_steam << _("Auth") << " ";

  actualUsageVarLabel->setText(actualUsage);

  QString keySizeVal, keyExpireVal, keyCreateTimeVal, keyAlgoVal;

  keySizeVal = QString::number(mKey.length());

  if (to_time_t(boost::posix_time::ptime(mKey.expires())) == 0) {
    keyExpireVal = _("Never Expire");
  } else {
    keyExpireVal =
        QString::fromStdString(boost::gregorian::to_iso_string(mKey.expires()));
  }

  keyAlgoVal = QString::fromStdString(mKey.pubkey_algo());
  keyCreateTimeVal = QString::fromStdString(to_iso_string(mKey.create_time()));

  keySizeVarLabel->setText(keySizeVal);
  expireVarLabel->setText(keyExpireVal);
  createdVarLabel->setText(keyCreateTimeVal);
  algorithmVarLabel->setText(keyAlgoVal);

  auto key_fpr = mKey.fpr();
  fingerPrintVarLabel->setText(
      QString::fromStdString(beautify_fingerprint(key_fpr)));
}

void KeyPairDetailTab::createKeyServerOperaMenu() {
  keyServerOperaMenu = new QMenu(this);

  auto* uploadKeyPair = new QAction(_("Upload Key Pair to Key Server"), this);
  connect(uploadKeyPair, SIGNAL(triggered()), this,
          SLOT(slotUploadKeyToServer()));
  auto* updateKeyPair = new QAction(_("Update Key Pair"), this);
  connect(updateKeyPair, SIGNAL(triggered()), this,
          SLOT(slotUpdateKeyToServer()));

  keyServerOperaMenu->addAction(uploadKeyPair);
  keyServerOperaMenu->addAction(updateKeyPair);
}

void KeyPairDetailTab::slotUploadKeyToServer() {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(mKey.id());
  auto* dialog = new KeyUploadDialog(keys, this);
  dialog->show();
  dialog->slotUpload();
}

void KeyPairDetailTab::slotUpdateKeyToServer() {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(mKey.id());
  auto* dialog = new KeyServerImportDialog(this);
  dialog->show();
  dialog->slotImport(keys);
}

void KeyPairDetailTab::slotGenRevokeCert() {
  auto literal = QStringLiteral("%1 (*.rev)").arg(_("Revocation Certificates"));
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

}  // namespace GpgFrontend::UI
