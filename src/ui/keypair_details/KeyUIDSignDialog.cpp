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

#include "ui/keypair_details/KeyUIDSignDialog.h"

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyManager.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {

KeyUIDSignDialog::KeyUIDSignDialog(const GpgKey& key, UIDArgsListPtr uid,
                                   QWidget* parent)
    : QDialog(parent), mUids(std::move(uid)), mKey(key) {
  const auto key_id = mKey.id();
  mKeyList = new KeyList(false, this);
  mKeyList->addListGroupTab(_("Signers"), KeyListRow::ONLY_SECRET_KEY,
                            KeyListColumn::NAME | KeyListColumn::EmailAddress,
                            [key_id](const GpgKey& key) -> bool {
                              if (key.disabled() || !key.can_certify() ||
                                  !key.has_master_key() || key.expired() ||
                                  key.revoked() || key_id == key.id())
                                return false;
                              else
                                return true;
                            });
  mKeyList->slotRefresh();

  signKeyButton = new QPushButton("Sign");

  /**
   * A DateTime after 5 Years is recommend.
   */
  expiresEdit = new QDateTimeEdit(QDateTime::currentDateTime().addYears(5));
  expiresEdit->setMinimumDateTime(QDateTime::currentDateTime());

  /**
   * Note further that the OpenPGP protocol uses 32 bit values for timestamps
   * and thus can only encode dates up to the year 2106.
   */
  expiresEdit->setMaximumDate(QDate(2106, 1, 1));

  nonExpireCheck = new QCheckBox("Non Expired");
  nonExpireCheck->setTristate(false);

  connect(nonExpireCheck, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            if (state == 0)
              expiresEdit->setDisabled(false);
            else
              expiresEdit->setDisabled(true);
          });

  auto layout = new QGridLayout();

  auto timeLayout = new QGridLayout();

  layout->addWidget(mKeyList, 0, 0);
  layout->addWidget(signKeyButton, 2, 0, Qt::AlignRight);
  timeLayout->addWidget(new QLabel(_("Expire Date")), 0, 0);
  timeLayout->addWidget(expiresEdit, 0, 1);
  timeLayout->addWidget(nonExpireCheck, 0, 2);
  layout->addLayout(timeLayout, 1, 0);

  connect(signKeyButton, SIGNAL(clicked(bool)), this, SLOT(slotSignKey(bool)));

  this->setLayout(layout);
  this->setModal(true);
  this->setWindowTitle(_("Sign For Key's UID(s)"));
  this->adjustSize();

  setAttribute(Qt::WA_DeleteOnClose, true);

  connect(this, SIGNAL(signalKeyUIDSignUpdate()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));
}

void KeyUIDSignDialog::slotSignKey(bool clicked) {
  LOG(INFO) << "Called";

  // Set Signers
  auto key_ids = mKeyList->getChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  LOG(INFO) << "Key Info Got";
  auto expires = std::make_unique<boost::posix_time::ptime>(
      boost::posix_time::from_time_t(expiresEdit->dateTime().toTime_t()));

  LOG(INFO) << "Sign Start";
  for (const auto& uid : *mUids) {
    LOG(INFO) << "Sign UID" << uid;
    // Sign For mKey
    if (!GpgKeyManager::GetInstance().signKey(mKey, *keys, uid, expires)) {
      QMessageBox::critical(
          nullptr, _("Unsuccessful Operation"),
          QString(_("Signature operation failed for UID %1")).arg(uid.c_str()));
    }
  }

  QMessageBox::information(nullptr, _("Operation Complete"),
                           _("The signature operation of the UID is complete"));
  this->close();
  emit signalKeyUIDSignUpdate();
}

}  // namespace GpgFrontend::UI
