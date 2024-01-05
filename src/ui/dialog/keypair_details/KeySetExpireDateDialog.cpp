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

#include "KeySetExpireDateDialog.h"

#include <utility>

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui_ModifiedExpirationDateTime.h"

namespace GpgFrontend::UI {

KeySetExpireDateDialog::KeySetExpireDateDialog(const KeyId& key_id,
                                               QWidget* parent)
    : GeneralDialog(typeid(KeySetExpireDateDialog).name(), parent),
      ui_(GpgFrontend::SecureCreateSharedObject<
          Ui_ModifiedExpirationDateTime>()),
      m_key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  init();
}

KeySetExpireDateDialog::KeySetExpireDateDialog(const KeyId& key_id,
                                               std::string subkey_fpr,
                                               QWidget* parent)
    : GeneralDialog(typeid(KeySetExpireDateDialog).name(), parent),
      ui_(GpgFrontend::SecureCreateSharedObject<
          Ui_ModifiedExpirationDateTime>()),
      m_key_(GpgKeyGetter::GetInstance().GetKey(key_id)),
      m_subkey_(std::move(subkey_fpr)) {
  init();
}

void KeySetExpireDateDialog::slot_confirm() {
  GF_UI_LOG_DEBUG("called: {} {}",
                  ui_->dateEdit->date().toString().toStdString(),
                  ui_->timeEdit->time().toString().toStdString());
  auto datetime = QDateTime(ui_->dateEdit->date(), ui_->timeEdit->time());
  std::unique_ptr<boost::posix_time::ptime> expires = nullptr;
  if (ui_->noExpirationCheckBox->checkState() == Qt::Unchecked) {
#ifdef GPGFRONTEND_GUI_QT6
    expires = std::make_unique<boost::posix_time::ptime>(
        boost::posix_time::from_time_t(
            datetime.toLocalTime().toSecsSinceEpoch()));
#else
    expires = std::make_unique<boost::posix_time::ptime>(
        boost::posix_time::from_time_t(datetime.toLocalTime().toTime_t()));
#endif
    GF_UI_LOG_DEBUG("keyid: {}", m_key_.GetId(), m_subkey_,
                    to_iso_string(*expires));
  } else {
    GF_UI_LOG_DEBUG("keyid: {}", m_key_.GetId(), m_subkey_, "Non Expired");
  }

  auto err = GpgKeyOpera::GetInstance().SetExpire(m_key_, m_subkey_, expires);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    auto* msg_box = new QMessageBox(qobject_cast<QWidget*>(this->parent()));
    msg_box->setAttribute(Qt::WA_DeleteOnClose);
    msg_box->setStandardButtons(QMessageBox::Ok);
    msg_box->setWindowTitle(_("Success"));
    msg_box->setText(_("The expire date of the key pair has been updated."));
    msg_box->setModal(true);
    msg_box->open();

    emit SignalKeyExpireDateUpdated();

    this->close();
  } else {
    QMessageBox::critical(
        this, _("Failure"),
        _("Failed to update the expire date of the key pair."));
  }
}

void KeySetExpireDateDialog::init() {
  ui_->setupUi(this);

  auto& settings = GlobalSettingStation::GetInstance().GetMainSettings();

  bool longer_expiration_date = false;
  try {
    longer_expiration_date = settings.lookup("general.longer_expiration_date");
    GF_UI_LOG_DEBUG("longer_expiration_date: {}", longer_expiration_date);

  } catch (...) {
    GF_UI_LOG_ERROR("setting operation error: longer_expiration_date");
  }

  auto max_date_time =
      longer_expiration_date
          ? QDateTime::currentDateTime().toLocalTime().addYears(30)
          : QDateTime::currentDateTime().toLocalTime().addYears(2);

  auto min_date_time = QDateTime::currentDateTime().addDays(7);

  ui_->dateEdit->setMaximumDateTime(max_date_time);
  ui_->dateEdit->setMinimumDateTime(min_date_time);

  // set default date time to expire date time
#ifdef GPGFRONTEND_GUI_QT6
  auto current_expire_time =
      QDateTime::fromSecsSinceEpoch(to_time_t(m_key_.GetExpireTime()));
#else
  auto current_expire_time =
      QDateTime::fromTime_t(to_time_t(m_key_.GetExpireTime()));
#endif
  ui_->dateEdit->setDateTime(current_expire_time);
  ui_->timeEdit->setDateTime(current_expire_time);

  connect(ui_->noExpirationCheckBox, &QCheckBox::stateChanged, this,
          &KeySetExpireDateDialog::slot_non_expired_checked);
  connect(ui_->button_box_, &QDialogButtonBox::accepted, this,
          &KeySetExpireDateDialog::slot_confirm);
  connect(this, &KeySetExpireDateDialog::SignalKeyExpireDateUpdated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);

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
