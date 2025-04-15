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

#include "KeyUIDSignDialog.h"

#include "core/function/gpg/GpgKeyManager.h"
#include "ui/UISignalStation.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

KeyUIDSignDialog::KeyUIDSignDialog(int channel, const GpgKeyPtr& key,
                                   const QString& uid, QWidget* parent)
    : GeneralDialog(typeid(KeyUIDSignDialog).name(), parent),
      current_gpg_context_channel_(channel),
      m_uid_(uid),
      m_key_(key) {
  assert(m_key_ != nullptr);
  const auto key_id = m_key_->ID();

  m_key_list_ = new KeyList(
      channel, KeyMenuAbility::kCOLUMN_FILTER | KeyMenuAbility::kSEARCH_BAR,
      GpgKeyTableColumn::kNAME | GpgKeyTableColumn::kEMAIL_ADDRESS |
          GpgKeyTableColumn::kKEY_ID,
      this);
  m_key_list_->AddListGroupTab(
      tr("Signers"), "signers", GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [key_id](const GpgAbstractKey* key) -> bool {
        if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) return false;
        return !(key->IsDisabled() || !key->IsHasCertCap() ||
                 !dynamic_cast<const GpgKey*>(key)->IsHasMasterKey() ||
                 key->IsExpired() || key->IsRevoked() || key_id == key->ID());
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
          [this](int state) -> void { expires_edit_->setEnabled(state == 0); });

  auto* layout = new QGridLayout();
  auto* time_layout = new QGridLayout();

  layout->addWidget(m_key_list_, 0, 0);
  layout->addWidget(sign_key_button_, 2, 0, Qt::AlignRight);
  time_layout->addWidget(new QLabel(tr("Expire Date")), 0, 0);
  time_layout->addWidget(expires_edit_, 0, 1);
  time_layout->addWidget(non_expire_check_, 0, 2);
  layout->addLayout(time_layout, 1, 0);

  connect(sign_key_button_, &QPushButton::clicked, this,
          &KeyUIDSignDialog::slot_sign_key);

  this->setLayout(layout);
  this->setModal(true);
  this->setWindowTitle(tr("Sign For Key's UID(s)"));
  this->adjustSize();

  setAttribute(Qt::WA_DeleteOnClose, true);

  connect(this, &KeyUIDSignDialog::SignalKeyUIDSignUpdate,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
}

void KeyUIDSignDialog::slot_sign_key(bool) {
  // Set Signers
  auto keys = m_key_list_->GetSelectedKeys();
  assert(std::all_of(keys.begin(), keys.end(),
                     [](const auto& key) { return key->IsGood(); }));

  auto expires = std::make_unique<QDateTime>(expires_edit_->dateTime());

  // Sign For mKey
  if (!GpgKeyManager::GetInstance(current_gpg_context_channel_)
           .SignKey(m_key_, keys, m_uid_, expires)) {
    QMessageBox::critical(
        nullptr, tr("Unsuccessful Operation"),
        tr("Signature operation failed for UID %1").arg(m_uid_));
  }

  QMessageBox::information(
      nullptr, tr("Operation Complete"),
      tr("The signature operation of the UID is complete"));
  this->close();
  emit SignalKeyUIDSignUpdate();
}

}  // namespace GpgFrontend::UI
