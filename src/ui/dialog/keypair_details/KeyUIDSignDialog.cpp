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

#include "KeyUIDSignDialog.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyManager.h"
#include "ui/UISignalStation.h"

namespace GpgFrontend::UI {

KeyUIDSignDialog::KeyUIDSignDialog(const GpgKey& key, UIDArgsListPtr uid,
                                   QWidget* parent)
    : GeneralDialog(typeid(KeyUIDSignDialog).name(), parent),
      m_uids_(std::move(uid)),
      m_key_(key) {
  const auto key_id = m_key_.GetId();
  m_key_list_ = new KeyList(KeyMenuAbility::NONE, this);
  m_key_list_->AddListGroupTab(
      _("Signers"), "signers", KeyListRow::ONLY_SECRET_KEY,
      KeyListColumn::NAME | KeyListColumn::EmailAddress,
      [key_id](const GpgKey& key, const KeyTable&) -> bool {
        return !(key.IsDisabled() || !key.IsHasCertificationCapability() ||
                 !key.IsHasMasterKey() || key.IsExpired() || key.IsRevoked() ||
                 key_id == key.GetId());
      });
  m_key_list_->SlotRefresh();

  sign_key_button_ = new QPushButton("Sign");

  /**
   * A DateTime after 5 Years is recommend.
   */
  expires_edit_ = new QDateTimeEdit(QDateTime::currentDateTime().addYears(5));
  expires_edit_->setMinimumDateTime(QDateTime::currentDateTime());

  /**
   * Note further that the OpenPGP protocol uses 32 bit values for timestamps
   * and thus can only encode dates up to the year 2106.
   */
  expires_edit_->setMaximumDate(QDate(2106, 1, 1));

  non_expire_check_ = new QCheckBox("Non Expired");
  non_expire_check_->setTristate(false);

  connect(non_expire_check_, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            if (state == 0)
              expires_edit_->setDisabled(false);
            else
              expires_edit_->setDisabled(true);
          });

  auto layout = new QGridLayout();

  auto timeLayout = new QGridLayout();

  layout->addWidget(m_key_list_, 0, 0);
  layout->addWidget(sign_key_button_, 2, 0, Qt::AlignRight);
  timeLayout->addWidget(new QLabel(_("Expire Date")), 0, 0);
  timeLayout->addWidget(expires_edit_, 0, 1);
  timeLayout->addWidget(non_expire_check_, 0, 2);
  layout->addLayout(timeLayout, 1, 0);

  connect(sign_key_button_, &QPushButton::clicked, this,
          &KeyUIDSignDialog::slot_sign_key);

  this->setLayout(layout);
  this->setModal(true);
  this->setWindowTitle(_("Sign For Key's UID(s)"));
  this->adjustSize();

  setAttribute(Qt::WA_DeleteOnClose, true);

  connect(this, &KeyUIDSignDialog::SignalKeyUIDSignUpdate,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
}

void KeyUIDSignDialog::slot_sign_key(bool clicked) {
  // Set Signers
  auto key_ids = m_key_list_->GetChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  SPDLOG_DEBUG("key info got");
#ifdef GPGFRONTEND_GUI_QT6
  auto expires =
      std::make_unique<boost::posix_time::ptime>(boost::posix_time::from_time_t(
          expires_edit_->dateTime().toSecsSinceEpoch()));
#else
  auto expires = std::make_unique<boost::posix_time::ptime>(
      boost::posix_time::from_time_t(expires_edit_->dateTime().toTime_t()));
#endif

  SPDLOG_DEBUG("sign start");
  for (const auto& uid : *m_uids_) {
    SPDLOG_DEBUG("sign uid: {}", uid);
    // Sign For mKey
    if (!GpgKeyManager::GetInstance().SignKey(m_key_, *keys, uid, expires)) {
      QMessageBox::critical(
          nullptr, _("Unsuccessful Operation"),
          QString(_("Signature operation failed for UID %1")).arg(uid.c_str()));
    }
  }

  QMessageBox::information(nullptr, _("Operation Complete"),
                           _("The signature operation of the UID is complete"));
  this->close();
  emit SignalKeyUIDSignUpdate();
}

}  // namespace GpgFrontend::UI
