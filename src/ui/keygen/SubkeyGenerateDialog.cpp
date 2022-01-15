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

#include "ui/keygen/SubkeyGenerateDialog.h"

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyOpera.h"
#include "ui/SignalStation.h"
#include "ui/WaitingDialog.h"
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

SubkeyGenerateDialog::SubkeyGenerateDialog(const KeyId& key_id, QWidget* parent)
    : QDialog(parent), key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  // max expire date time
  bool longer_expiration_date = false;
  try {
    longer_expiration_date = settings.lookup("general.longer_expiration_date");
    LOG(INFO) << "longer_expiration_date" << longer_expiration_date;

  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("longer_expiration_date");
  }

  max_date_time_ = longer_expiration_date
                       ? QDateTime::currentDateTime().toLocalTime().addYears(30)
                       : QDateTime::currentDateTime().toLocalTime().addYears(2);

  buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  keyUsageGroupBox = create_key_usage_group_box();

  auto* groupGrid = new QGridLayout(this);
  groupGrid->addWidget(create_basic_info_group_box(), 0, 0);
  groupGrid->addWidget(keyUsageGroupBox, 1, 0);

  auto* nameList = new QWidget(this);
  nameList->setLayout(groupGrid);

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(nameList);
  vbox2->addWidget(errorLabel);
  vbox2->addWidget(buttonBox);

  this->setWindowTitle(_("Generate New Subkey"));
  this->setLayout(vbox2);
  this->setModal(true);

  connect(this, SIGNAL(SubKeyGenerated()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));

  set_signal_slot();
  refresh_widgets_state();
}

QGroupBox* SubkeyGenerateDialog::create_key_usage_group_box() {
  auto* groupBox = new QGroupBox(this);
  auto* grid = new QGridLayout(this);

  groupBox->setTitle(_("Key Usage"));

  auto* encrypt = new QCheckBox(_("Encryption"), groupBox);
  encrypt->setTristate(false);

  auto* sign = new QCheckBox(_("Signing"), groupBox);
  sign->setTristate(false);

  auto* cert = new QCheckBox(_("Certification"), groupBox);
  cert->setTristate(false);

  auto* auth = new QCheckBox(_("Authentication"), groupBox);
  auth->setTristate(false);

  key_usage_check_boxes_.push_back(encrypt);
  key_usage_check_boxes_.push_back(sign);
  key_usage_check_boxes_.push_back(cert);
  key_usage_check_boxes_.push_back(auth);

  grid->addWidget(encrypt, 0, 0);
  grid->addWidget(sign, 0, 1);
  grid->addWidget(cert, 1, 0);
  grid->addWidget(auth, 1, 1);

  groupBox->setLayout(grid);

  return groupBox;
}

QGroupBox* SubkeyGenerateDialog::create_basic_info_group_box() {
  errorLabel = new QLabel();
  keySizeSpinBox = new QSpinBox(this);
  keyTypeComboBox = new QComboBox(this);

  for (auto& algo : GenKeyInfo::GetSupportedKeyAlgo()) {
    keyTypeComboBox->addItem(QString::fromStdString(algo));
  }
  if (!GenKeyInfo::GetSupportedKeyAlgo().empty()) {
    keyTypeComboBox->setCurrentIndex(0);
  }

  dateEdit = new QDateTimeEdit(QDateTime::currentDateTime().addYears(2), this);
  dateEdit->setMinimumDateTime(QDateTime::currentDateTime());
  dateEdit->setMaximumDateTime(max_date_time_);
  dateEdit->setDisplayFormat("dd/MM/yyyy hh:mm:ss");
  dateEdit->setCalendarPopup(true);
  dateEdit->setEnabled(true);

  expireCheckBox = new QCheckBox(this);
  expireCheckBox->setCheckState(Qt::Unchecked);

  auto* vbox1 = new QGridLayout;

  vbox1->addWidget(new QLabel(QString(_("Expiration Date")) + ": "), 2, 0);
  vbox1->addWidget(new QLabel(QString(_("Never Expire")) + ": "), 2, 3);
  vbox1->addWidget(new QLabel(QString(_("KeySize (in Bit)")) + ": "), 1, 0);
  vbox1->addWidget(new QLabel(QString(_("Key Type")) + ": "), 0, 0);

  vbox1->addWidget(dateEdit, 2, 1);
  vbox1->addWidget(expireCheckBox, 2, 2);
  vbox1->addWidget(keySizeSpinBox, 1, 1);
  vbox1->addWidget(keyTypeComboBox, 0, 1);

  auto basicInfoGroupBox = new QGroupBox();
  basicInfoGroupBox->setLayout(vbox1);
  basicInfoGroupBox->setTitle(_("Basic Information"));

  return basicInfoGroupBox;
}

void SubkeyGenerateDialog::set_signal_slot() {
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotKeyGenAccept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  connect(expireCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(slotExpireBoxChanged()));

  connect(key_usage_check_boxes_[0], SIGNAL(stateChanged(int)), this,
          SLOT(slotEncryptionBoxChanged(int)));
  connect(key_usage_check_boxes_[1], SIGNAL(stateChanged(int)), this,
          SLOT(slotSigningBoxChanged(int)));
  connect(key_usage_check_boxes_[2], SIGNAL(stateChanged(int)), this,
          SLOT(slotCertificationBoxChanged(int)));
  connect(key_usage_check_boxes_[3], SIGNAL(stateChanged(int)), this,
          SLOT(slotAuthenticationBoxChanged(int)));

  connect(keyTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(slotActivatedKeyType(int)));
}

void SubkeyGenerateDialog::slotExpireBoxChanged() {
  if (expireCheckBox->checkState()) {
    dateEdit->setEnabled(false);
  } else {
    dateEdit->setEnabled(true);
  }
}

void SubkeyGenerateDialog::refresh_widgets_state() {
  qDebug() << "refresh_widgets_state called";

  if (gen_key_info_->IsAllowEncryption())
    key_usage_check_boxes_[0]->setCheckState(Qt::CheckState::Checked);
  else
    key_usage_check_boxes_[0]->setCheckState(Qt::CheckState::Unchecked);

  if (gen_key_info_->IsAllowChangeEncryption())
    key_usage_check_boxes_[0]->setDisabled(false);
  else
    key_usage_check_boxes_[0]->setDisabled(true);

  if (gen_key_info_->IsAllowSigning())
    key_usage_check_boxes_[1]->setCheckState(Qt::CheckState::Checked);
  else
    key_usage_check_boxes_[1]->setCheckState(Qt::CheckState::Unchecked);

  if (gen_key_info_->IsAllowChangeSigning())
    key_usage_check_boxes_[1]->setDisabled(false);
  else
    key_usage_check_boxes_[1]->setDisabled(true);

  if (gen_key_info_->IsAllowCertification())
    key_usage_check_boxes_[2]->setCheckState(Qt::CheckState::Checked);
  else
    key_usage_check_boxes_[2]->setCheckState(Qt::CheckState::Unchecked);

  if (gen_key_info_->IsAllowChangeCertification())
    key_usage_check_boxes_[2]->setDisabled(false);
  else
    key_usage_check_boxes_[2]->setDisabled(true);

  if (gen_key_info_->IsAllowAuthentication())
    key_usage_check_boxes_[3]->setCheckState(Qt::CheckState::Checked);
  else
    key_usage_check_boxes_[3]->setCheckState(Qt::CheckState::Unchecked);

  if (gen_key_info_->IsAllowChangeAuthentication())
    key_usage_check_boxes_[3]->setDisabled(false);
  else
    key_usage_check_boxes_[3]->setDisabled(true);

  keySizeSpinBox->setRange(gen_key_info_->GetSuggestMinKeySize(),
                           gen_key_info_->GetSuggestMaxKeySize());
  keySizeSpinBox->setValue(gen_key_info_->GetKeyLength());
  keySizeSpinBox->setSingleStep(gen_key_info_->GetSizeChangeStep());
}

void SubkeyGenerateDialog::slotKeyGenAccept() {
  std::stringstream err_stream;

  /**
   * primary keys should have a reasonable expiration date (no more than 2 years
   * in the future)
   */
  if (dateEdit->dateTime() > QDateTime::currentDateTime().addYears(2)) {
    err_stream << "  " << _("Expiration time no more than 2 years.") << "  ";
  }

  auto err_string = err_stream.str();

  if (err_string.empty()) {
    gen_key_info_->SetKeyLength(keySizeSpinBox->value());

    if (expireCheckBox->checkState()) {
      gen_key_info_->SetNonExpired(true);
    } else {
      gen_key_info_->SetExpireTime(
          boost::posix_time::from_time_t(dateEdit->dateTime().toTime_t()));
    }

    GpgError error;
    auto thread = QThread::create([&]() {
      LOG(INFO) << "Thread Started";
      error = GpgKeyOpera::GetInstance().GenerateSubkey(key_, gen_key_info_);
    });
    thread->start();

    auto* dialog = new WaitingDialog(_("Generating"), this);
    dialog->show();

    while (thread->isRunning()) {
      QCoreApplication::processEvents();
    }
    dialog->close();

    if (check_gpg_error_2_err_code(error) == GPG_ERR_NO_ERROR) {
      auto* msg_box = new QMessageBox(nullptr);
      msg_box->setAttribute(Qt::WA_DeleteOnClose);
      msg_box->setStandardButtons(QMessageBox::Ok);
      msg_box->setWindowTitle(_("Success"));
      msg_box->setText(_("The new subkey has been generated."));
      msg_box->setModal(false);
      msg_box->open();

      emit SubKeyGenerated();
      this->close();
    } else
      QMessageBox::critical(this, _("Failure"), _(gpgme_strerror(error)));

  } else {
    /**
     * create error message
     */
    errorLabel->setAutoFillBackground(true);
    QPalette error = errorLabel->palette();
    error.setColor(QPalette::Window, "#ff8080");
    errorLabel->setPalette(error);
    errorLabel->setText(err_string.c_str());

    this->show();
  }
}

void SubkeyGenerateDialog::slotEncryptionBoxChanged(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowEncryption(false);
  } else {
    gen_key_info_->SetAllowEncryption(true);
  }
}

void SubkeyGenerateDialog::slotSigningBoxChanged(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowSigning(false);
  } else {
    gen_key_info_->SetAllowSigning(true);
  }
}

void SubkeyGenerateDialog::slotCertificationBoxChanged(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowCertification(false);
  } else {
    gen_key_info_->SetAllowCertification(true);
  }
}

void SubkeyGenerateDialog::slotAuthenticationBoxChanged(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowAuthentication(false);
  } else {
    gen_key_info_->SetAllowAuthentication(true);
  }
}

void SubkeyGenerateDialog::slotActivatedKeyType(int index) {
  qDebug() << "key type index changed " << index;
  gen_key_info_->SetAlgo(this->keyTypeComboBox->itemText(index).toStdString());
  refresh_widgets_state();
}

}  // namespace GpgFrontend::UI
