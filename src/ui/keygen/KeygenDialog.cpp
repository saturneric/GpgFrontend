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

#include "ui/keygen/KeygenDialog.h"

#include "gpg/function/GpgKeyOpera.h"
#include "ui/SignalStation.h"
#include "ui/WaitingDialog.h"

namespace GpgFrontend::UI {

KeyGenDialog::KeyGenDialog(QWidget* parent) : QDialog(parent) {
  buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  this->setWindowTitle(tr("Generate Key"));
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
  QString errorString = "";

  /**
   * check for errors in keygen dialog input
   */
  if ((nameEdit->text()).size() < 5) {
    errorString.append(tr("  Name must contain at least five characters.  \n"));
  }
  if (emailEdit->text().isEmpty() || !check_email_address(emailEdit->text())) {
    errorString.append(tr("  Please give a email address.   \n"));
  }

  /**
   * primary keys should have a reasonable expiration date (no more than 2 years
   * in the future)
   */
  if (dateEdit->dateTime() > QDateTime::currentDateTime().addYears(2)) {
    errorString.append(tr("  Expiration time no more than 2 years.  \n"));
  }

  if (errorString.isEmpty()) {
    /**
     * create the string for key generation
     */

    genKeyInfo->setUserid(
        QString("%1(%3)<%2>")
            .arg(nameEdit->text(), emailEdit->text(), commentEdit->text())
            .toStdString());

    genKeyInfo->setKeySize(keySizeSpinBox->value());

    if (expireCheckBox->checkState()) {
      genKeyInfo->setNonExpired(true);
    } else {
      genKeyInfo->setExpired(
          boost::posix_time::from_time_t(dateEdit->dateTime().toTime_t())
              .date());
    }

    gpgme_error_t error = false;
    auto thread = QThread::create(
        [&]() { error = GpgKeyOpera::GetInstance().GenerateKey(genKeyInfo); });
    thread->start();

    auto* dialog = new WaitingDialog("Generating", this);
    dialog->show();

    while (thread->isRunning()) {
      QCoreApplication::processEvents();
    }

    dialog->close();

    if (gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
      auto* msg_box = new QMessageBox(nullptr);
      msg_box->setAttribute(Qt::WA_DeleteOnClose);
      msg_box->setStandardButtons(QMessageBox::Ok);
      msg_box->setWindowTitle(tr("Success"));
      msg_box->setText(tr("The new key pair has been generated."));
      msg_box->setModal(false);
      msg_box->open();

      emit KeyGenerated();
      this->close();
    } else {
      QMessageBox::critical(this, tr("Failure"), tr(gpgme_strerror(error)));
    }

  } else {
    /**
     * create error message
     */
    errorLabel->setAutoFillBackground(true);
    QPalette error = errorLabel->palette();
    error.setColor(QPalette::Window, "#ff8080");
    errorLabel->setPalette(error);
    errorLabel->setText(errorString);

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

  groupBox->setTitle(tr("Key Usage"));

  auto* encrypt = new QCheckBox(tr("Encryption"), groupBox);
  encrypt->setTristate(false);

  auto* sign = new QCheckBox(tr("Signing"), groupBox);
  sign->setTristate(false);

  auto* cert = new QCheckBox(tr("Certification"), groupBox);
  cert->setTristate(false);

  auto* auth = new QCheckBox(tr("Authentication"), groupBox);
  auth->setTristate(false);

  keyUsageCheckBoxes.push_back(encrypt);
  keyUsageCheckBoxes.push_back(sign);
  keyUsageCheckBoxes.push_back(cert);
  keyUsageCheckBoxes.push_back(auth);

  grid->addWidget(encrypt, 0, 0);
  grid->addWidget(sign, 0, 1);
  grid->addWidget(cert, 1, 0);
  grid->addWidget(auth, 1, 1);

  groupBox->setLayout(grid);

  return groupBox;
}

void KeyGenDialog::slotEncryptionBoxChanged(int state) {
  if (state == 0) {
    genKeyInfo->setAllowEncryption(false);
  } else {
    genKeyInfo->setAllowEncryption(true);
  }
}

void KeyGenDialog::slotSigningBoxChanged(int state) {
  if (state == 0) {
    genKeyInfo->setAllowSigning(false);
  } else {
    genKeyInfo->setAllowSigning(true);
  }
}

void KeyGenDialog::slotCertificationBoxChanged(int state) {
  if (state == 0) {
    genKeyInfo->setAllowCertification(false);
  } else {
    genKeyInfo->setAllowCertification(true);
  }
}

void KeyGenDialog::slotAuthenticationBoxChanged(int state) {
  if (state == 0) {
    genKeyInfo->setAllowAuthentication(false);
  } else {
    genKeyInfo->setAllowAuthentication(true);
  }
}

void KeyGenDialog::slotActivatedKeyType(int index) {
  qDebug() << "key type index changed " << index;

  genKeyInfo->setAlgo(this->keyTypeComboBox->itemText(index).toStdString());
  refresh_widgets_state();
}

void KeyGenDialog::refresh_widgets_state() {
  qDebug() << "refresh_widgets_state called";

  if (genKeyInfo->isAllowEncryption())
    keyUsageCheckBoxes[0]->setCheckState(Qt::CheckState::Checked);
  else
    keyUsageCheckBoxes[0]->setCheckState(Qt::CheckState::Unchecked);

  if (genKeyInfo->isAllowChangeEncryption())
    keyUsageCheckBoxes[0]->setDisabled(false);
  else
    keyUsageCheckBoxes[0]->setDisabled(true);

  if (genKeyInfo->isAllowSigning())
    keyUsageCheckBoxes[1]->setCheckState(Qt::CheckState::Checked);
  else
    keyUsageCheckBoxes[1]->setCheckState(Qt::CheckState::Unchecked);

  if (genKeyInfo->isAllowChangeSigning())
    keyUsageCheckBoxes[1]->setDisabled(false);
  else
    keyUsageCheckBoxes[1]->setDisabled(true);

  if (genKeyInfo->isAllowCertification())
    keyUsageCheckBoxes[2]->setCheckState(Qt::CheckState::Checked);
  else
    keyUsageCheckBoxes[2]->setCheckState(Qt::CheckState::Unchecked);

  if (genKeyInfo->isAllowChangeCertification())
    keyUsageCheckBoxes[2]->setDisabled(false);
  else
    keyUsageCheckBoxes[2]->setDisabled(true);

  if (genKeyInfo->isAllowAuthentication())
    keyUsageCheckBoxes[3]->setCheckState(Qt::CheckState::Checked);
  else
    keyUsageCheckBoxes[3]->setCheckState(Qt::CheckState::Unchecked);

  if (genKeyInfo->isAllowChangeAuthentication())
    keyUsageCheckBoxes[3]->setDisabled(false);
  else
    keyUsageCheckBoxes[3]->setDisabled(true);

  if (genKeyInfo->isAllowNoPassPhrase())
    noPassPhraseCheckBox->setDisabled(false);
  else
    noPassPhraseCheckBox->setDisabled(true);

  keySizeSpinBox->setRange(genKeyInfo->getSuggestMinKeySize(),
                           genKeyInfo->getSuggestMaxKeySize());
  keySizeSpinBox->setValue(genKeyInfo->getKeySize());
  keySizeSpinBox->setSingleStep(genKeyInfo->getSizeChangeStep());
}

void KeyGenDialog::set_signal_slot() {
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotKeyGenAccept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  connect(expireCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(slotExpireBoxChanged()));

  connect(keyUsageCheckBoxes[0], SIGNAL(stateChanged(int)), this,
          SLOT(slotEncryptionBoxChanged(int)));
  connect(keyUsageCheckBoxes[1], SIGNAL(stateChanged(int)), this,
          SLOT(slotSigningBoxChanged(int)));
  connect(keyUsageCheckBoxes[2], SIGNAL(stateChanged(int)), this,
          SLOT(slotCertificationBoxChanged(int)));
  connect(keyUsageCheckBoxes[3], SIGNAL(stateChanged(int)), this,
          SLOT(slotAuthenticationBoxChanged(int)));

  connect(keyTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(slotActivatedKeyType(int)));

  connect(noPassPhraseCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            if (state == 0) {
              genKeyInfo->setNonPassPhrase(false);
            } else {
              genKeyInfo->setNonPassPhrase(true);
            }
          });
}

bool KeyGenDialog::check_email_address(const QString& str) {
  return re_email.match(str).hasMatch();
}

QGroupBox* KeyGenDialog::create_basic_info_group_box() {
  errorLabel = new QLabel(tr(""));
  nameEdit = new QLineEdit(this);
  emailEdit = new QLineEdit(this);
  commentEdit = new QLineEdit(this);
  keySizeSpinBox = new QSpinBox(this);
  keyTypeComboBox = new QComboBox(this);

  for (auto& algo : GenKeyInfo::SupportedKeyAlgo) {
    keyTypeComboBox->addItem(QString::fromStdString(algo));
  }
  if (!GenKeyInfo::SupportedKeyAlgo.empty()) {
    keyTypeComboBox->setCurrentIndex(0);
  }

  QDateTime maxDateTime = QDateTime::currentDateTime().addYears(2);

  dateEdit = new QDateTimeEdit(maxDateTime, this);
  dateEdit->setMinimumDateTime(QDateTime::currentDateTime());
  dateEdit->setMaximumDateTime(maxDateTime);
  dateEdit->setDisplayFormat("dd/MM/yyyy hh:mm:ss");
  dateEdit->setCalendarPopup(true);
  dateEdit->setEnabled(true);

  expireCheckBox = new QCheckBox(this);
  expireCheckBox->setCheckState(Qt::Unchecked);

  noPassPhraseCheckBox = new QCheckBox(this);
  noPassPhraseCheckBox->setCheckState(Qt::Unchecked);

  auto* vbox1 = new QGridLayout;

  vbox1->addWidget(new QLabel(tr("Name:")), 0, 0);
  vbox1->addWidget(new QLabel(tr("Email Address:")), 1, 0);
  vbox1->addWidget(new QLabel(tr("Comment:")), 2, 0);
  vbox1->addWidget(new QLabel(tr("Expiration Date:")), 3, 0);
  vbox1->addWidget(new QLabel(tr("Never Expire")), 3, 3);
  vbox1->addWidget(new QLabel(tr("KeySize (in Bit):")), 4, 0);
  vbox1->addWidget(new QLabel(tr("Key Type:")), 5, 0);
  vbox1->addWidget(new QLabel(tr("Non Pass Phrase")), 6, 0);

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
  basicInfoGroupBox->setTitle(tr("Basic Information"));

  return basicInfoGroupBox;
}

}  // namespace GpgFrontend::UI
