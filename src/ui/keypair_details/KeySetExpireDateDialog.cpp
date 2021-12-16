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
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
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
      ui(std::make_shared<Ui_ModifiedExpirationDateTime>()),
      mKey(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  init();
}

KeySetExpireDateDialog::KeySetExpireDateDialog(const KeyId& key_id,
                                               std::string subkey_fpr,
                                               QWidget* parent)
    : QDialog(parent),
      ui(std::make_shared<Ui_ModifiedExpirationDateTime>()),
      mKey(GpgKeyGetter::GetInstance().GetKey(key_id)),
      mSubkey(std::move(subkey_fpr)) {
  init();
}

void KeySetExpireDateDialog::slotConfirm() {
  LOG(INFO) << "Called" << ui->dateEdit->date().toString().toStdString()
            << ui->timeEdit->time().toString().toStdString();
  auto datetime = QDateTime(ui->dateEdit->date(), ui->timeEdit->time());
  std::unique_ptr<boost::posix_time::ptime> expires = nullptr;
  if (ui->noExpirationCheckBox->checkState() == Qt::Unchecked) {
    expires = std::make_unique<boost::posix_time::ptime>(
        boost::posix_time::from_time_t(datetime.toLocalTime().toTime_t()));
    LOG(INFO) << "keyid" << mKey.id() << mSubkey << *expires;
  } else {
    LOG(INFO) << "keyid" << mKey.id() << mSubkey << "Non Expired";
  }

  auto err = GpgKeyOpera::GetInstance().SetExpire(mKey, mSubkey, expires);

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR) {
    auto* msg_box = new QMessageBox(nullptr);
    msg_box->setAttribute(Qt::WA_DeleteOnClose);
    msg_box->setStandardButtons(QMessageBox::Ok);
    msg_box->setWindowTitle(_("Success"));
    msg_box->setText(_("The expire date of the key pair has been updated."));
    msg_box->setModal(false);
    msg_box->open();

    emit signalKeyExpireDateUpdated();

  } else {
    QMessageBox::critical(this, _("Failure"), _(gpgme_strerror(err)));
  }

  this->close();
}

void KeySetExpireDateDialog::init() {
  ui->setupUi(this);

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

  ui->dateEdit->setMaximumDateTime(max_date_time);
  ui->dateEdit->setMinimumDateTime(min_date_time);

  ui->dateEdit->setDateTime(max_date_time);
  ui->timeEdit->setDateTime(max_date_time);

  connect(ui->noExpirationCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(slotNonExpiredChecked(int)));
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &KeySetExpireDateDialog::slotConfirm);
  connect(this, SIGNAL(signalKeyExpireDateUpdated()),
          SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()));

  ui->titleLabel->setText(_("Modified Expiration Date (Local Time)"));
  ui->label->setText(
      _("Tips: For the sake of security, the key is valid for up to two years. "
        "If you are an expert user, please unlock it for a longer time in the "
        "settings."));
  ui->noExpirationCheckBox->setText(_("No Expiration"));
  this->setWindowTitle(_("Modified Expiration Date"));
}

void KeySetExpireDateDialog::slotNonExpiredChecked(int state) {
  ui->dateEdit->setDisabled(state == Qt::Checked);
  ui->timeEdit->setDisabled(state == Qt::Checked);
}

}  // namespace GpgFrontend::UI
