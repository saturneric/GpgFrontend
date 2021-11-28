/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/keypair_details/KeySetExpireDateDialog.h"

#include <utility>

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyOpera.h"

namespace GpgFrontend::UI {

KeySetExpireDateDialog::KeySetExpireDateDialog(const KeyId& key_id,
                                               QWidget* parent)
    : QDialog(parent), mKey(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  init();
}

KeySetExpireDateDialog::KeySetExpireDateDialog(const KeyId& key_id,
                                               std::string subkey_id,
                                               QWidget* parent)
    : QDialog(parent),
      mKey(GpgKeyGetter::GetInstance().GetKey(key_id)),
      mSubkey(std::move(subkey_id)) {
  init();
}

void KeySetExpireDateDialog::slotConfirm() {
  std::unique_ptr<boost::gregorian::date> expires = nullptr;
  if (this->nonExpiredCheck->checkState() == Qt::Unchecked) {
    expires = std::make_unique<boost::gregorian::date>(
        boost::posix_time::from_time_t(
            this->dateTimeEdit->dateTime().toTime_t())
            .date());
  }
  GpgKeyOpera::GetInstance().SetExpire(mKey, mSubkey, expires);
  this->close();
}

void KeySetExpireDateDialog::init() {
  QDateTime maxDateTime = QDateTime::currentDateTime().addYears(2);
  dateTimeEdit = new QDateTimeEdit(maxDateTime);
  dateTimeEdit->setMinimumDateTime(QDateTime::currentDateTime().addSecs(1));
  dateTimeEdit->setMaximumDateTime(maxDateTime);
  nonExpiredCheck = new QCheckBox();
  nonExpiredCheck->setTristate(false);
  confirmButton = new QPushButton(tr("Confirm"));

  auto* gridLayout = new QGridLayout();
  gridLayout->addWidget(dateTimeEdit, 0, 0, 1, 2);
  gridLayout->addWidget(nonExpiredCheck, 0, 2, 1, 1, Qt::AlignRight);
  gridLayout->addWidget(new QLabel(tr("Never Expire")), 0, 3);
  gridLayout->addWidget(confirmButton, 1, 3);

  connect(nonExpiredCheck, SIGNAL(stateChanged(int)), this,
          SLOT(slotNonExpiredChecked(int)));
  connect(confirmButton, SIGNAL(clicked(bool)), this, SLOT(slotConfirm()));

  this->setLayout(gridLayout);
  this->setWindowTitle("Edit Expire Datetime");
  this->setModal(true);
  this->setAttribute(Qt::WA_DeleteOnClose, true);
}

void KeySetExpireDateDialog::slotNonExpiredChecked(int state) {
  if (state == 0) {
    this->dateTimeEdit->setDisabled(false);
  } else {
    this->dateTimeEdit->setDisabled(true);
  }
}

}  // namespace GpgFrontend::UI
