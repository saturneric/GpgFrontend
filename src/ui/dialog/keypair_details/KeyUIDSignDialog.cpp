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

#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

KeyUIDSignDialog::KeyUIDSignDialog(int channel, const GpgKeyPtr& key,
                                   const QString& uid, QWidget* parent)
    : GeneralDialog(typeid(KeyUIDSignDialog).name(), parent),
      current_gpg_context_channel_(channel),
      m_uid_(uid),
      m_key_(key) {
  // A reload of the key databases replaces the channel this works on; find
  // the database again, or say it is gone.
  KeepBoundToKeyDatabase(
      this, [this]() { return current_gpg_context_channel_; },
      [this](int channel) { current_gpg_context_channel_ = channel; });

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

  sign_key_button_ = new QPushButton(tr("Sign"));

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

  non_expire_check_ = new QCheckBox(tr("Non Expired"));
  non_expire_check_->setTristate(false);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(non_expire_check_, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) -> void {
            expires_edit_->setEnabled(state == Qt::Unchecked);
          });
#else
  connect(non_expire_check_, &QCheckBox::stateChanged, this,
          [this](int state) -> void { expires_edit_->setEnabled(state == 0); });
#endif

  // Say what is being certified: the dialog otherwise only lists signers.
  auto* target_label = new QLabel(
      m_uid_.isEmpty() ? tr("Certify all user IDs of: %1").arg(m_key_->UID())
                       : tr("Certify user ID: %1").arg(m_uid_));
  target_label->setTextFormat(Qt::PlainText);
  target_label->setWordWrap(true);

  auto* layout = new QGridLayout();
  auto* time_layout = new QGridLayout();

  layout->addWidget(target_label, 0, 0);
  layout->addWidget(m_key_list_, 1, 0);
  layout->addWidget(sign_key_button_, 3, 0, Qt::AlignRight);
  time_layout->addWidget(new QLabel(tr("Expire Date")), 0, 0);
  time_layout->addWidget(expires_edit_, 0, 1);
  time_layout->addWidget(non_expire_check_, 0, 2);
  layout->addLayout(time_layout, 2, 0);

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
  // The signers are the ticked rows, not the highlighted ones.
  auto keys = m_key_list_->GetCheckedKeys();
  if (keys.isEmpty()) {
    QMessageBox::information(this, tr("No Signer Selected"),
                             tr("Please check at least one signer key."));
    return;
  }

  std::optional<QDateTime> expires = std::nullopt;
  if (!non_expire_check_->isChecked()) expires = expires_edit_->dateTime();

  auto f = [this, keys, expires](const OperaWaitingHd& hd) {
    KeyManagementOperation::GetInstance(current_gpg_context_channel_)
        .SignKey(
            m_key_, keys, m_uid_, expires,
            [this, self = QPointer<KeyUIDSignDialog>(this), hd](
                GpgError err, const DataObjectPtr&) {
              hd();
              // A reload of the key databases may have closed the dialog.
              if (self == nullptr) return;

              if (CheckGpgError(err) == GPG_ERR_CANCELED) {
                // The user dismissed the passphrase prompt: nothing failed,
                // and the dialog stays open for another try.
                return;
              }

              if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                QMessageBox::critical(
                    this, tr("Unsuccessful Operation"),
                    tr("Signature operation failed for UID %1")
                            .arg(m_uid_.isEmpty() ? m_key_->UID() : m_uid_) +
                        "\n\n" + DescribeGpgErrCode(err).second);
                return;
              }

              auto* msg_box =
                  new QMessageBox(qobject_cast<QWidget*>(this->parent()));
              msg_box->setAttribute(Qt::WA_DeleteOnClose);
              msg_box->setStandardButtons(QMessageBox::Ok);
              msg_box->setWindowTitle(tr("Operation Complete"));
              msg_box->setText(
                  tr("The signature operation of the UID is complete"));
              msg_box->setModal(true);
              msg_box->open();

              emit SignalKeyUIDSignUpdate();
              this->close();
            });
  };
  GpgOperaHelper::WaitForOpera(this, tr("Signing UID"), f);
}

}  // namespace GpgFrontend::UI
