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

#include "ui/keygen/KeygenDialog.h"

#include "gpg/function/GpgKeyOpera.h"
#include "ui/SignalStation.h"
#include "ui/WaitingDialog.h"
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

KeyGenDialog::KeyGenDialog(QWidget* parent) : QDialog(parent) {
  buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

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

  this->setWindowTitle(_("Generate Key"));
  this->setModal(true);

  connect(this, SIGNAL(KeyGenerated()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));

  generateKeyDialog();
}

void KeyGenDialog::generateKeyDialog() {
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

  this->setLayout(vbox2);

  set_signal_slot();

  refresh_widgets_state();
}

void KeyGenDialog::slotKeyGenAccept() {
  std::stringstream error_stream;

  /**
   * check for errors in keygen dialog input
   */
  if ((nameEdit->text()).size() < 5) {
    error_stream << "  " << _("Name must contain at least five characters.")
                 << std::endl;
  }
  if (emailEdit->text().isEmpty() || !check_email_address(emailEdit->text())) {
    error_stream << "  " << _("Please give a email address.") << std::endl;
  }

  /**
   * primary keys should have a reasonable expiration date (no more than 2 years
   * in the future)
   */
  if (dateEdit->dateTime() > max_date_time_) {
    error_stream << "  " << _("Expiration time too long.") << std::endl;
  }

  auto err_string = error_stream.str();

  if (err_string.empty()) {
    /**
     * create the string for key generation
     */
    gen_key_info_->setName(nameEdit->text().toStdString());
    gen_key_info_->setEmail(emailEdit->text().toStdString());
    gen_key_info_->setComment(commentEdit->text().toStdString());

    gen_key_info_->setKeySize(keySizeSpinBox->value());

    if (expireCheckBox->checkState()) {
      gen_key_info_->setNonExpired(true);
    } else {
      gen_key_info_->setExpired(
          boost::posix_time::from_time_t(dateEdit->dateTime().toTime_t()));
    }

    GpgGenKeyResult result;
    gpgme_error_t error = false;
    auto thread = QThread::create([&]() {
      error = GpgKeyOpera::GetInstance().GenerateKey(gen_key_info_, result);
    });
    thread->start();

    auto* dialog = new WaitingDialog(_("Generating"), this);
    dialog->show();

    while (thread->isRunning()) {
      QCoreApplication::processEvents();
    }

    dialog->close();

    if (gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
      auto* msg_box = new QMessageBox(nullptr);
      msg_box->setAttribute(Qt::WA_DeleteOnClose);
      msg_box->setStandardButtons(QMessageBox::Ok);
      msg_box->setWindowTitle(_("Success"));
      msg_box->setText(_("The new key pair has been generated."));
      msg_box->setModal(false);
      msg_box->open();

      emit KeyGenerated();
      this->close();
    } else {
      QMessageBox::critical(this, _("Failure"), _(gpgme_strerror(error)));
    }

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

void KeyGenDialog::slotExpireBoxChanged() {
  if (expireCheckBox->checkState()) {
    dateEdit->setEnabled(false);
  } else {
    dateEdit->setEnabled(true);
  }
}

QGroupBox* KeyGenDialog::create_key_usage_group_box() {
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

void KeyGenDialog::slotEncryptionBoxChanged(int state) {
  if (state == 0) {
    gen_key_info_->setAllowEncryption(false);
  } else {
    gen_key_info_->setAllowEncryption(true);
  }
}

void KeyGenDialog::slotSigningBoxChanged(int state) {
  if (state == 0) {
    gen_key_info_->setAllowSigning(false);
  } else {
    gen_key_info_->setAllowSigning(true);
  }
}

void KeyGenDialog::slotCertificationBoxChanged(int state) {
  if (state == 0) {
    gen_key_info_->setAllowCertification(false);
  } else {
    gen_key_info_->setAllowCertification(true);
  }
}

void KeyGenDialog::slotAuthenticationBoxChanged(int state) {
  if (state == 0) {
    gen_key_info_->setAllowAuthentication(false);
  } else {
    gen_key_info_->setAllowAuthentication(true);
  }
}

void KeyGenDialog::slotActivatedKeyType(int index) {
  qDebug() << "key type index changed " << index;

  gen_key_info_->setAlgo(this->keyTypeComboBox->itemText(index).toStdString());
  refresh_widgets_state();
}

void KeyGenDialog::refresh_widgets_state() {
  qDebug() << "refresh_widgets_state called";

  if (gen_key_info_->isAllowEncryption())
    key_usage_check_boxes_[0]->setCheckState(Qt::CheckState::Checked);
  else
    key_usage_check_boxes_[0]->setCheckState(Qt::CheckState::Unchecked);

  if (gen_key_info_->isAllowChangeEncryption())
    key_usage_check_boxes_[0]->setDisabled(false);
  else
    key_usage_check_boxes_[0]->setDisabled(true);

  if (gen_key_info_->isAllowSigning())
    key_usage_check_boxes_[1]->setCheckState(Qt::CheckState::Checked);
  else
    key_usage_check_boxes_[1]->setCheckState(Qt::CheckState::Unchecked);

  if (gen_key_info_->isAllowChangeSigning())
    key_usage_check_boxes_[1]->setDisabled(false);
  else
    key_usage_check_boxes_[1]->setDisabled(true);

  if (gen_key_info_->isAllowCertification())
    key_usage_check_boxes_[2]->setCheckState(Qt::CheckState::Checked);
  else
    key_usage_check_boxes_[2]->setCheckState(Qt::CheckState::Unchecked);

  if (gen_key_info_->isAllowChangeCertification())
    key_usage_check_boxes_[2]->setDisabled(false);
  else
    key_usage_check_boxes_[2]->setDisabled(true);

  if (gen_key_info_->isAllowAuthentication())
    key_usage_check_boxes_[3]->setCheckState(Qt::CheckState::Checked);
  else
    key_usage_check_boxes_[3]->setCheckState(Qt::CheckState::Unchecked);

  if (gen_key_info_->isAllowChangeAuthentication())
    key_usage_check_boxes_[3]->setDisabled(false);
  else
    key_usage_check_boxes_[3]->setDisabled(true);

  if (gen_key_info_->isAllowNoPassPhrase())
    noPassPhraseCheckBox->setDisabled(false);
  else
    noPassPhraseCheckBox->setDisabled(true);

  keySizeSpinBox->setRange(gen_key_info_->getSuggestMinKeySize(),
                           gen_key_info_->getSuggestMaxKeySize());
  keySizeSpinBox->setValue(gen_key_info_->getKeySize());
  keySizeSpinBox->setSingleStep(gen_key_info_->getSizeChangeStep());
}

void KeyGenDialog::set_signal_slot() {
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

  connect(noPassPhraseCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            if (state == 0) {
              gen_key_info_->setNonPassPhrase(false);
            } else {
              gen_key_info_->setNonPassPhrase(true);
            }
          });
}

bool KeyGenDialog::check_email_address(const QString& str) {
  return re_email.match(str).hasMatch();
}

QGroupBox* KeyGenDialog::create_basic_info_group_box() {
  errorLabel = new QLabel();
  nameEdit = new QLineEdit(this);
  emailEdit = new QLineEdit(this);
  commentEdit = new QLineEdit(this);
  keySizeSpinBox = new QSpinBox(this);
  keyTypeComboBox = new QComboBox(this);

  for (auto& algo : GenKeyInfo::getSupportedKeyAlgo()) {
    keyTypeComboBox->addItem(QString::fromStdString(algo));
  }
  if (!GenKeyInfo::getSupportedKeyAlgo().empty()) {
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

  noPassPhraseCheckBox = new QCheckBox(this);
  noPassPhraseCheckBox->setCheckState(Qt::Unchecked);

  auto* vbox1 = new QGridLayout;

  vbox1->addWidget(new QLabel(QString(_("Name")) + ": "), 0, 0);
  vbox1->addWidget(new QLabel(QString(_("Email Address")) + ": "), 1, 0);
  vbox1->addWidget(new QLabel(QString(_("Comment")) + ": "), 2, 0);
  vbox1->addWidget(new QLabel(QString(_("Expiration Date")) + ": "), 3, 0);
  vbox1->addWidget(new QLabel(QString(_("Never Expire")) + ": "), 3, 3);
  vbox1->addWidget(new QLabel(QString(_("KeySize (in Bit)")) + ": "), 4, 0);
  vbox1->addWidget(new QLabel(QString(_("Key Type")) + ": "), 5, 0);
  vbox1->addWidget(new QLabel(QString(_("Non Pass Phrase")) + ": "), 6, 0);

  vbox1->addWidget(nameEdit, 0, 1, 1, 3);
  vbox1->addWidget(emailEdit, 1, 1, 1, 3);
  vbox1->addWidget(commentEdit, 2, 1, 1, 3);
  vbox1->addWidget(dateEdit, 3, 1);
  vbox1->addWidget(expireCheckBox, 3, 2);
  vbox1->addWidget(keySizeSpinBox, 4, 1);
  vbox1->addWidget(keyTypeComboBox, 5, 1);
  vbox1->addWidget(noPassPhraseCheckBox, 6, 1);

  auto basicInfoGroupBox = new QGroupBox();
  basicInfoGroupBox->setLayout(vbox1);
  basicInfoGroupBox->setTitle(_("Basic Information"));

  return basicInfoGroupBox;
}

}  // namespace GpgFrontend::UI
