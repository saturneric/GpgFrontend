/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ui/key_generate/KeygenDialog.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "dialog/WaitingDialog.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {

KeyGenDialog::KeyGenDialog(QWidget* parent) : QDialog(parent) {
  button_box_ =
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

  connect(this, &KeyGenDialog::SignalKeyGenerated, SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh);

  generate_key_dialog();
}

void KeyGenDialog::generate_key_dialog() {
  key_usage_group_box_ = create_key_usage_group_box();

  auto* groupGrid = new QGridLayout(this);
  groupGrid->addWidget(create_basic_info_group_box(), 0, 0);
  groupGrid->addWidget(key_usage_group_box_, 1, 0);

  auto* nameList = new QWidget(this);
  nameList->setLayout(groupGrid);

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(nameList);
  vbox2->addWidget(error_label_);
  vbox2->addWidget(button_box_);

  this->setLayout(vbox2);

  set_signal_slot();

  refresh_widgets_state();
}

void KeyGenDialog::slot_key_gen_accept() {
  std::stringstream error_stream;

  /**
   * check for errors in keygen dialog input
   */
  if ((name_edit_->text()).size() < 5) {
    error_stream << "  " << _("Name must contain at least five characters.")
                 << std::endl;
  }
  if (email_edit_->text().isEmpty() ||
      !check_email_address(email_edit_->text())) {
    error_stream << "  " << _("Please give a email address.") << std::endl;
  }

  /**
   * primary keys should have a reasonable expiration date (no more than 2 years
   * in the future)
   */
  if (date_edit_->dateTime() > max_date_time_) {
    error_stream << "  " << _("Expiration time too long.") << std::endl;
  }

  auto err_string = error_stream.str();

  if (err_string.empty()) {
    /**
     * create the string for key generation
     */
    gen_key_info_->SetName(name_edit_->text().toStdString());
    gen_key_info_->SetEmail(email_edit_->text().toStdString());
    gen_key_info_->SetComment(comment_edit_->text().toStdString());

    gen_key_info_->SetKeyLength(key_size_spin_box_->value());

    if (expire_check_box_->checkState()) {
      gen_key_info_->SetNonExpired(true);
    } else {
      gen_key_info_->SetExpireTime(
          boost::posix_time::from_time_t(date_edit_->dateTime().toTime_t()));
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

    LOG(INFO) << "generate done";

    if (gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
      auto* msg_box = new QMessageBox((QWidget*)this->parent());
      msg_box->setAttribute(Qt::WA_DeleteOnClose);
      msg_box->setStandardButtons(QMessageBox::Ok);
      msg_box->setWindowTitle(_("Success"));
      msg_box->setText(_("The new key pair has been generated."));
      msg_box->setModal(true);
      msg_box->open();

      LOG(INFO) << "generate success";

      emit SignalKeyGenerated();
      this->close();
    } else {
      QMessageBox::critical(this, _("Failure"), _(gpgme_strerror(error)));
    }

  } else {
    /**
     * create error message
     */
    error_label_->setAutoFillBackground(true);
    QPalette error = error_label_->palette();
    error.setColor(QPalette::Window, "#ff8080");
    error_label_->setPalette(error);
    error_label_->setText(err_string.c_str());

    this->show();
  }
}

void KeyGenDialog::slot_expire_box_changed() {
  if (expire_check_box_->checkState()) {
    date_edit_->setEnabled(false);
  } else {
    date_edit_->setEnabled(true);
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

void KeyGenDialog::slot_encryption_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowEncryption(false);
  } else {
    gen_key_info_->SetAllowEncryption(true);
  }
}

void KeyGenDialog::slot_signing_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowSigning(false);
  } else {
    gen_key_info_->SetAllowSigning(true);
  }
}

void KeyGenDialog::slot_certification_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowCertification(false);
  } else {
    gen_key_info_->SetAllowCertification(true);
  }
}

void KeyGenDialog::slot_authentication_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowAuthentication(false);
  } else {
    gen_key_info_->SetAllowAuthentication(true);
  }
}

void KeyGenDialog::slot_activated_key_type(int index) {
  qDebug() << "key type index changed " << index;

  gen_key_info_->SetAlgo(
      this->key_type_combo_box_->itemText(index).toStdString());
  refresh_widgets_state();
}

void KeyGenDialog::refresh_widgets_state() {
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

  if (gen_key_info_->IsAllowNoPassPhrase())
    no_pass_phrase_check_box_->setDisabled(false);
  else
    no_pass_phrase_check_box_->setDisabled(true);

  key_size_spin_box_->setRange(gen_key_info_->GetSuggestMinKeySize(),
                               gen_key_info_->GetSuggestMaxKeySize());
  key_size_spin_box_->setValue(gen_key_info_->GetKeyLength());
  key_size_spin_box_->setSingleStep(gen_key_info_->GetSizeChangeStep());
}

void KeyGenDialog::set_signal_slot() {
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &KeyGenDialog::slot_key_gen_accept);
  connect(button_box_, &QDialogButtonBox::rejected, this,
          &KeyGenDialog::reject);

  connect(expire_check_box_, &QCheckBox::stateChanged, this,
          &KeyGenDialog::slot_expire_box_changed);

  connect(key_usage_check_boxes_[0], &QCheckBox::stateChanged, this,
          &KeyGenDialog::slot_encryption_box_changed);
  connect(key_usage_check_boxes_[1], &QCheckBox::stateChanged, this,
          &KeyGenDialog::slot_signing_box_changed);
  connect(key_usage_check_boxes_[2], &QCheckBox::stateChanged, this,
          &KeyGenDialog::slot_certification_box_changed);
  connect(key_usage_check_boxes_[3], &QCheckBox::stateChanged, this,
          &KeyGenDialog::slot_authentication_box_changed);

  connect(key_type_combo_box_, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &KeyGenDialog::slot_activated_key_type);

  connect(no_pass_phrase_check_box_, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            if (state == 0) {
              gen_key_info_->SetNonPassPhrase(false);
            } else {
              gen_key_info_->SetNonPassPhrase(true);
            }
          });
}

bool KeyGenDialog::check_email_address(const QString& str) {
  return re_email_.match(str).hasMatch();
}

QGroupBox* KeyGenDialog::create_basic_info_group_box() {
  error_label_ = new QLabel();
  name_edit_ = new QLineEdit(this);
  email_edit_ = new QLineEdit(this);
  comment_edit_ = new QLineEdit(this);
  key_size_spin_box_ = new QSpinBox(this);
  key_type_combo_box_ = new QComboBox(this);

  for (auto& algo : GenKeyInfo::GetSupportedKeyAlgo()) {
    key_type_combo_box_->addItem(QString::fromStdString(algo));
  }
  if (!GenKeyInfo::GetSupportedKeyAlgo().empty()) {
    key_type_combo_box_->setCurrentIndex(0);
  }

  date_edit_ =
      new QDateTimeEdit(QDateTime::currentDateTime().addYears(2), this);
  date_edit_->setMinimumDateTime(QDateTime::currentDateTime());
  date_edit_->setMaximumDateTime(max_date_time_);
  date_edit_->setDisplayFormat("dd/MM/yyyy hh:mm:ss");
  date_edit_->setCalendarPopup(true);
  date_edit_->setEnabled(true);

  expire_check_box_ = new QCheckBox(this);
  expire_check_box_->setCheckState(Qt::Unchecked);

  no_pass_phrase_check_box_ = new QCheckBox(this);
  no_pass_phrase_check_box_->setCheckState(Qt::Unchecked);

  auto* vbox1 = new QGridLayout;

  vbox1->addWidget(new QLabel(QString(_("Name")) + ": "), 0, 0);
  vbox1->addWidget(new QLabel(QString(_("Email Address")) + ": "), 1, 0);
  vbox1->addWidget(new QLabel(QString(_("Comment")) + ": "), 2, 0);
  vbox1->addWidget(new QLabel(QString(_("Expiration Date")) + ": "), 3, 0);
  vbox1->addWidget(new QLabel(QString(_("Never Expire")) + ": "), 3, 3);
  vbox1->addWidget(new QLabel(QString(_("KeySize (in Bit)")) + ": "), 4, 0);
  vbox1->addWidget(new QLabel(QString(_("Key Type")) + ": "), 5, 0);
  vbox1->addWidget(new QLabel(QString(_("Non Pass Phrase")) + ": "), 6, 0);

  vbox1->addWidget(name_edit_, 0, 1, 1, 3);
  vbox1->addWidget(email_edit_, 1, 1, 1, 3);
  vbox1->addWidget(comment_edit_, 2, 1, 1, 3);
  vbox1->addWidget(date_edit_, 3, 1);
  vbox1->addWidget(expire_check_box_, 3, 2);
  vbox1->addWidget(key_size_spin_box_, 4, 1);
  vbox1->addWidget(key_type_combo_box_, 5, 1);
  vbox1->addWidget(no_pass_phrase_check_box_, 6, 1);

  auto basicInfoGroupBox = new QGroupBox();
  basicInfoGroupBox->setLayout(vbox1);
  basicInfoGroupBox->setTitle(_("Basic Information"));

  return basicInfoGroupBox;
}

}  // namespace GpgFrontend::UI
