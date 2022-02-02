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

#include "ui/keypair_details/KeySetExpireDateDialog.h"

#include <utility>

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyOpera.h"
#include "ui/SignalStation.h"
#include "ui/settings/GlobalSettingStation.h"
#include "ui_ModifiedExpirationDateTime.h"

namespace GpgFrontend::UI {

KeySetExpireDateDialog::KeySetExpireDateDialog(const KeyId& key_id,
                                               QWidget* parent)
    : QDialog(parent),
      ui_(std::make_shared<Ui_ModifiedExpirationDateTime>()),
      m_key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  init();
}

KeySetExpireDateDialog::KeySetExpireDateDialog(const KeyId& key_id,
                                               std::string subkey_fpr,
                                               QWidget* parent)
    : QDialog(parent),
      ui_(std::make_shared<Ui_ModifiedExpirationDateTime>()),
      m_key_(GpgKeyGetter::GetInstance().GetKey(key_id)),
      m_subkey_(std::move(subkey_fpr)) {
  init();
}

void KeySetExpireDateDialog::slot_confirm() {
  LOG(INFO) << "Called" << ui_->dateEdit->date().toString().toStdString()
            << ui_->timeEdit->time().toString().toStdString();
  auto datetime = QDateTime(ui_->dateEdit->date(), ui_->timeEdit->time());
  std::unique_ptr<boost::posix_time::ptime> expires = nullptr;
  if (ui_->noExpirationCheckBox->checkState() == Qt::Unchecked) {
    expires = std::make_unique<boost::posix_time::ptime>(
        boost::posix_time::from_time_t(datetime.toLocalTime().toTime_t()));
    LOG(INFO) << "keyid" << m_key_.GetId() << m_subkey_ << *expires;
  } else {
    LOG(INFO) << "keyid" << m_key_.GetId() << m_subkey_ << "Non Expired";
  }

  auto err = GpgKeyOpera::GetInstance().SetExpire(m_key_, m_subkey_, expires);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR) {
    auto* msg_box = new QMessageBox(nullptr);
    msg_box->setAttribute(Qt::WA_DeleteOnClose);
    msg_box->setStandardButtons(QMessageBox::Ok);
    msg_box->setWindowTitle(_("Success"));
    msg_box->setText(_("The expire date of the key pair has been updated."));
    msg_box->setModal(false);
    msg_box->open();

    emit SignalKeyExpireDateUpdated();

  } else {
    QMessageBox::critical(this, _("Failure"), _(gpgme_strerror(err)));
  }

  this->close();
}

void KeySetExpireDateDialog::init() {
  ui_->setupUi(this);

  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  bool longer_expiration_date = false;
  try {
    longer_expiration_date = settings.lookup("general.longer_expiration_date");
    LOG(INFO) << "longer_expiration_date" << longer_expiration_date;

  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("longer_expiration_date");
  }

  auto max_date_time =
      longer_expiration_date
          ? QDateTime::currentDateTime().toLocalTime().addYears(30)
          : QDateTime::currentDateTime().toLocalTime().addYears(2);

  auto min_date_time = QDateTime::currentDateTime().addDays(7);

  ui_->dateEdit->setMaximumDateTime(max_date_time);
  ui_->dateEdit->setMinimumDateTime(min_date_time);

  ui_->dateEdit->setDateTime(max_date_time);
  ui_->timeEdit->setDateTime(max_date_time);

  connect(ui_->noExpirationCheckBox, &QCheckBox::stateChanged, this,
          &KeySetExpireDateDialog::slot_non_expired_checked);
  connect(ui_->button_box_, &QDialogButtonBox::accepted, this,
          &KeySetExpireDateDialog::slot_confirm);
  connect(this, &KeySetExpireDateDialog::SignalKeyExpireDateUpdated,
          SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh);

  ui_->titleLabel->setText(_("Modified Expiration Date (Local Time)"));
  ui_->label->setText(
      _("Tips: For the sake of security, the key is valid for up to two years. "
        "If you are an expert user, please unlock it for a longer time in the "
        "settings."));
  ui_->noExpirationCheckBox->setText(_("No Expiration"));
  this->setWindowTitle(_("Modified Expiration Date"));
}

void KeySetExpireDateDialog::slot_non_expired_checked(int state) {
  ui_->dateEdit->setDisabled(state == Qt::Checked);
  ui_->timeEdit->setDisabled(state == Qt::Checked);
}

}  // namespace GpgFrontend::UI
