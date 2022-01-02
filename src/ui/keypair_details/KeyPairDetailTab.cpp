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
#include "gpg/function/GpgKeyImportExporter.h"
#include "ui/SignalStation.h"
#include "ui/WaitingDialog.h"

namespace GpgFrontend::UI {
KeyPairDetailTab::KeyPairDetailTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), mKey(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  ownerBox = new QGroupBox(_("Owner"));
  keyBox = new QGroupBox(_("Primary Key"));
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
  vboxKD->addWidget(new QLabel(QString(_("Primary Key Existence")) + ": "), 8,
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

void KeyPairDetailTab::slotCopyFingerprint() {
  QString fpr = fingerPrintVarLabel->text().trimmed().replace(" ", QString());
  QClipboard* cb = QApplication::clipboard();
  cb->setText(fpr);
}

void KeyPairDetailTab::slotRefreshKeyInfo() {
  // Show the situation that primary key not exists.
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
    expLabel->setText(_("Warning: The primary key has expired."));
  }
  if (mKey.revoked()) {
    iconLabel->show();
    expLabel->show();
    expLabel->setText(_("Warning: The primary key has been revoked."));
  }
}

void KeyPairDetailTab::slotRefreshKey() {
  LOG(INFO) << _("Called");
  this->mKey = GpgKeyGetter::GetInstance().GetKey(mKey.id());
  this->slotRefreshKeyInfo();
}

}  // namespace GpgFrontend::UI
