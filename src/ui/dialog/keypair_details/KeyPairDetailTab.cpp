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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "KeyPairDetailTab.h"

#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "dialog/WaitingDialog.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {
KeyPairDetailTab::KeyPairDetailTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  LOG(INFO) << key_.GetEmail() << key_.IsPrivateKey() << key_.IsHasMasterKey()
            << key_.GetSubKeys()->front().IsPrivateKey();

  owner_box_ = new QGroupBox(_("Owner"));
  key_box_ = new QGroupBox(_("Primary Key"));
  fingerprint_box_ = new QGroupBox(_("Fingerprint"));
  additional_uid_box_ = new QGroupBox(_("Additional UIDs"));

  name_var_label_ = new QLabel();
  name_var_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  email_var_label_ = new QLabel();
  email_var_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  comment_var_label_ = new QLabel();
  comment_var_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  key_id_var_label = new QLabel();
  key_id_var_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

  usage_var_label_ = new QLabel();
  actual_usage_var_label_ = new QLabel();

  key_size_var_label_ = new QLabel();
  expire_var_label_ = new QLabel();
  created_var_label_ = new QLabel();
  last_update_var_label_ = new QLabel();
  algorithm_var_label_ = new QLabel();
  primary_key_exist_var_label_ = new QLabel();

  auto* mvbox = new QVBoxLayout();
  auto* vboxKD = new QGridLayout();
  auto* vboxOD = new QGridLayout();

  vboxOD->addWidget(new QLabel(QString(_("Name")) + ": "), 0, 0);
  vboxOD->addWidget(new QLabel(QString(_("Email Address")) + ": "), 1, 0);
  vboxOD->addWidget(new QLabel(QString(_("Comment")) + ": "), 2, 0);
  vboxOD->addWidget(name_var_label_, 0, 1);
  vboxOD->addWidget(email_var_label_, 1, 1);
  vboxOD->addWidget(comment_var_label_, 2, 1);

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

  key_id_var_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  vboxKD->addWidget(key_id_var_label, 0, 1, 1, 1);
  vboxKD->addWidget(algorithm_var_label_, 1, 1, 1, 2);
  vboxKD->addWidget(key_size_var_label_, 2, 1, 1, 2);
  vboxKD->addWidget(usage_var_label_, 3, 1, 1, 2);
  vboxKD->addWidget(actual_usage_var_label_, 4, 1, 1, 2);
  vboxKD->addWidget(created_var_label_, 5, 1, 1, 2);
  vboxKD->addWidget(expire_var_label_, 6, 1, 1, 2);
  vboxKD->addWidget(last_update_var_label_, 7, 1, 1, 2);
  vboxKD->addWidget(primary_key_exist_var_label_, 8, 1, 1, 2);

  auto* copyKeyIdButton = new QPushButton(_("Copy"));
  copyKeyIdButton->setFlat(true);
  copyKeyIdButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  vboxKD->addWidget(copyKeyIdButton, 0, 2);
  connect(copyKeyIdButton, &QPushButton::clicked, this, [=]() {
    QString fpr = key_id_var_label->text().trimmed();
    QClipboard* cb = QApplication::clipboard();
    cb->setText(fpr);
  });

  owner_box_->setLayout(vboxOD);
  mvbox->addWidget(owner_box_);
  key_box_->setLayout(vboxKD);
  mvbox->addWidget(key_box_);

  fingerprint_var_label_ = new QLabel();
  fingerprint_var_label_->setWordWrap(false);
  fingerprint_var_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  fingerprint_var_label_->setStyleSheet("margin-left: 0; margin-right: 5;");
  fingerprint_var_label_->setAlignment(Qt::AlignCenter);
  auto* hboxFP = new QHBoxLayout();

  hboxFP->addStretch();
  hboxFP->addWidget(fingerprint_var_label_);

  auto* copyFingerprintButton = new QPushButton(_("Copy"));
  copyFingerprintButton->setFlat(true);
  copyFingerprintButton->setToolTip(_("copy fingerprint to clipboard"));
  connect(copyFingerprintButton, &QPushButton::clicked, this,
          &KeyPairDetailTab::slot_copy_fingerprint);

  hboxFP->addWidget(copyFingerprintButton);
  hboxFP->addStretch();

  fingerprint_box_->setLayout(hboxFP);
  mvbox->addWidget(fingerprint_box_);
  mvbox->addStretch();

  auto* expBox = new QHBoxLayout();
  QPixmap pixmap(":warning.png");

  exp_label_ = new QLabel();
  icon_label_ = new QLabel();

  icon_label_->setPixmap(pixmap.scaled(24, 24, Qt::KeepAspectRatio));
  exp_label_->setAlignment(Qt::AlignCenter);
  expBox->addStretch();
  expBox->addWidget(icon_label_);
  expBox->addWidget(exp_label_);
  expBox->addStretch();
  mvbox->addLayout(expBox);
  mvbox->setContentsMargins(0, 0, 0, 0);

  // when key database updated
  connect(SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyPairDetailTab::slot_refresh_key);

  slot_refresh_key_info();
  setAttribute(Qt::WA_DeleteOnClose, true);
  setLayout(mvbox);
}

void KeyPairDetailTab::slot_copy_fingerprint() {
  QString fpr =
      fingerprint_var_label_->text().trimmed().replace(" ", QString());
  QClipboard* cb = QApplication::clipboard();
  cb->setText(fpr);
}

void KeyPairDetailTab::slot_refresh_key_info() {
  // Show the situation that primary key not exists.
  primary_key_exist_var_label_->setText(
      key_.IsHasMasterKey() ? _("Exists") : _("Not Exists"));
  if (!key_.IsHasMasterKey()) {
    auto palette_expired = primary_key_exist_var_label_->palette();
    palette_expired.setColor(primary_key_exist_var_label_->foregroundRole(),
                             Qt::red);
    primary_key_exist_var_label_->setPalette(palette_expired);
  } else {
    auto palette_valid = primary_key_exist_var_label_->palette();
    palette_valid.setColor(primary_key_exist_var_label_->foregroundRole(),
                           Qt::darkGreen);
    primary_key_exist_var_label_->setPalette(palette_valid);
  }

  if (key_.IsExpired()) {
    auto paletteExpired = expire_var_label_->palette();
    paletteExpired.setColor(expire_var_label_->foregroundRole(), Qt::red);
    expire_var_label_->setPalette(paletteExpired);
  } else {
    auto paletteValid = expire_var_label_->palette();
    paletteValid.setColor(expire_var_label_->foregroundRole(), Qt::darkGreen);
    expire_var_label_->setPalette(paletteValid);
  }

  name_var_label_->setText(QString::fromStdString(key_.GetName()));
  email_var_label_->setText(QString::fromStdString(key_.GetEmail()));

  comment_var_label_->setText(QString::fromStdString(key_.GetComment()));
  key_id_var_label->setText(QString::fromStdString(key_.GetId()));

  std::stringstream usage_steam;

  if (key_.IsHasCertificationCapability())
    usage_steam << _("Certificate") << " ";
  if (key_.IsHasEncryptionCapability()) usage_steam << _("Encrypt") << " ";
  if (key_.IsHasSigningCapability()) usage_steam << _("Sign") << " ";
  if (key_.IsHasAuthenticationCapability()) usage_steam << _("Auth") << " ";

  usage_var_label_->setText(usage_steam.str().c_str());

  std::stringstream actual_usage_steam;

  if (key_.IsHasActualCertificationCapability())
    actual_usage_steam << _("Certificate") << " ";
  if (key_.IsHasActualEncryptionCapability())
    actual_usage_steam << _("Encrypt") << " ";
  if (key_.IsHasActualSigningCapability())
    actual_usage_steam << _("Sign") << " ";
  if (key_.IsHasActualAuthenticationCapability())
    actual_usage_steam << _("Auth") << " ";

  actual_usage_var_label_->setText(actual_usage_steam.str().c_str());

  std::string key_size_val, key_expire_val, key_create_time_val, key_algo_val,
      key_last_update_val;

  key_size_val = std::to_string(key_.GetPrimaryKeyLength());

  if (to_time_t(boost::posix_time::ptime(key_.GetExpireTime())) == 0) {
    expire_var_label_->setText(_("Never Expire"));
  } else {
    expire_var_label_->setText(QLocale::system().toString(
        QDateTime::fromTime_t(to_time_t(key_.GetExpireTime()))));
  }

  key_algo_val = key_.GetPublicKeyAlgo();

  created_var_label_->setText(QLocale::system().toString(
      QDateTime::fromTime_t(to_time_t(key_.GetCreateTime()))));

  if (to_time_t(boost::posix_time::ptime(key_.GetLastUpdateTime())) == 0) {
    last_update_var_label_->setText(_("No Data"));
  } else {
    last_update_var_label_->setText(QLocale::system().toString(
        QDateTime::fromTime_t(to_time_t(key_.GetLastUpdateTime()))));
  }

  key_size_var_label_->setText(key_size_val.c_str());
  algorithm_var_label_->setText(key_algo_val.c_str());
  fingerprint_var_label_->setText(
      beautify_fingerprint(key_.GetFingerprint()).c_str());

  icon_label_->hide();
  exp_label_->hide();

  if (key_.IsExpired()) {
    icon_label_->show();
    exp_label_->show();
    exp_label_->setText(_("Warning: The primary key has expired."));
  }
  if (key_.IsRevoked()) {
    icon_label_->show();
    exp_label_->show();
    exp_label_->setText(_("Warning: The primary key has been revoked."));
  }
}

void KeyPairDetailTab::slot_refresh_key() {
  LOG(INFO) << _("Called");
  this->key_ = GpgKeyGetter::GetInstance().GetKey(key_.GetId());
  this->slot_refresh_key_info();
}

}  // namespace GpgFrontend::UI
