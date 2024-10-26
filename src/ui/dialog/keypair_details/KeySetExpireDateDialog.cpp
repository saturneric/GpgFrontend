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

#include "KeySetExpireDateDialog.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui_ModifiedExpirationDateTime.h"

namespace GpgFrontend::UI {

KeySetExpireDateDialog::KeySetExpireDateDialog(int channel, const KeyId& key_id,
                                               QWidget* parent)
    : GeneralDialog(typeid(KeySetExpireDateDialog).name(), parent),
      ui_(GpgFrontend::SecureCreateSharedObject<
          Ui_ModifiedExpirationDateTime>()),
      current_gpg_context_channel_(channel),
      m_key_(GpgKeyGetter::GetInstance(current_gpg_context_channel_)
                 .GetKey(key_id)) {
  assert(m_key_.IsGood());
  init();
}

KeySetExpireDateDialog::KeySetExpireDateDialog(int channel, const KeyId& key_id,
                                               QString subkey_fpr,
                                               QWidget* parent)
    : GeneralDialog(typeid(KeySetExpireDateDialog).name(), parent),
      ui_(GpgFrontend::SecureCreateSharedObject<
          Ui_ModifiedExpirationDateTime>()),
      current_gpg_context_channel_(channel),
      m_key_(GpgKeyGetter::GetInstance(current_gpg_context_channel_)
                 .GetKey(key_id)),
      m_subkey_(std::move(subkey_fpr)) {
  assert(m_key_.IsGood());
  init();
}

void KeySetExpireDateDialog::slot_confirm() {
  auto datetime = QDateTime(ui_->dateEdit->date(), ui_->timeEdit->time());
  std::unique_ptr<QDateTime> expires = nullptr;
  if (ui_->noExpirationCheckBox->checkState() == Qt::Unchecked) {
    expires = std::make_unique<QDateTime>(datetime.toLocalTime());
  }

  auto err = GpgKeyOpera::GetInstance(current_gpg_context_channel_)
                 .SetExpire(m_key_, m_subkey_, expires);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    auto* msg_box = new QMessageBox(qobject_cast<QWidget*>(this->parent()));
    msg_box->setAttribute(Qt::WA_DeleteOnClose);
    msg_box->setStandardButtons(QMessageBox::Ok);
    msg_box->setWindowTitle(tr("Success"));
    msg_box->setText(tr("The expire date of the key pair has been updated."));
    msg_box->setModal(true);
    msg_box->open();

    emit SignalKeyExpireDateUpdated();

    this->close();
  } else {
    QMessageBox::critical(
        this, tr("Failure"),
        tr("Failed to update the expire date of the key pair."));
  }
}

void KeySetExpireDateDialog::init() {
  ui_->setupUi(this);

  auto settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetSettings();

  bool longer_expiration_date =
      settings.value("basic/longer_expiration_date").toBool();

  auto max_date_time =
      longer_expiration_date
          ? QDateTime::currentDateTime().toLocalTime().addYears(30)
          : QDateTime::currentDateTime().toLocalTime().addYears(2);

  auto min_date_time = QDateTime::currentDateTime().addDays(7);

  ui_->dateEdit->setMaximumDateTime(max_date_time);
  ui_->dateEdit->setMinimumDateTime(min_date_time);

  // set default date time to expire date time
  auto current_expire_time = m_key_.GetExpireTime();

  ui_->dateEdit->setDateTime(current_expire_time);
  ui_->timeEdit->setDateTime(current_expire_time);

  connect(ui_->noExpirationCheckBox, &QCheckBox::stateChanged, this,
          &KeySetExpireDateDialog::slot_non_expired_checked);
  connect(ui_->button_box_, &QDialogButtonBox::accepted, this,
          &KeySetExpireDateDialog::slot_confirm);
  connect(this, &KeySetExpireDateDialog::SignalKeyExpireDateUpdated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);

  if (m_key_.GetExpireTime().toSecsSinceEpoch() == 0) {
    ui_->noExpirationCheckBox->setCheckState(Qt::Checked);
  } else {
    ui_->dateEdit->setDateTime(m_key_.GetExpireTime());
    ui_->timeEdit->setDateTime(m_key_.GetExpireTime());
  }

  ui_->titleLabel->setText(tr("Modified Expiration Date (Local Time)"));
  ui_->label->setText(tr(
      "Tips: For the sake of security, the key is valid for up to two years. "
      "If you are an expert user, please unlock it for a longer time in the "
      "settings."));
  ui_->noExpirationCheckBox->setText(tr("No Expiration"));
  this->setWindowTitle(tr("Modified Expiration Date"));
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setModal(true);
}

void KeySetExpireDateDialog::slot_non_expired_checked(int state) {
  ui_->dateEdit->setDisabled(state == Qt::Checked);
  ui_->timeEdit->setDisabled(state == Qt::Checked);
}

}  // namespace GpgFrontend::UI
