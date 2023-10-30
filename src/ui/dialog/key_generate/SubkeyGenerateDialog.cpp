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

#include "SubkeyGenerateDialog.h"

#include <cassert>
#include <cstddef>

#include "core/common/CoreCommonUtil.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "ui/SignalStation.h"
#include "ui/dialog/WaitingDialog.h"

namespace GpgFrontend::UI {

SubkeyGenerateDialog::SubkeyGenerateDialog(const KeyId& key_id, QWidget* parent)
    : GeneralDialog(typeid(SubkeyGenerateDialog).name(), parent),
      key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  bool longer_expiration_date =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.longer_expiration_date", false);

  bool use_pinentry_as_password_input_dialog =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.use_pinentry_as_password_input_dialog", false);

  use_pinentry_ = use_pinentry_as_password_input_dialog;

  max_date_time_ = longer_expiration_date
                       ? QDateTime::currentDateTime().toLocalTime().addYears(30)
                       : QDateTime::currentDateTime().toLocalTime().addYears(2);

  button_box_ =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  key_usage_group_box_ = create_key_usage_group_box();

  auto* group_grid = new QGridLayout(this);
  group_grid->addWidget(create_basic_info_group_box(), 0, 0);
  group_grid->addWidget(key_usage_group_box_, 1, 0);

  auto* tipps_label = new QLabel(
      QString(_("Tipps: if the key pair has a passphrase, the subkey's "
                "passphrase must be equal to it.")));
  group_grid->addWidget(tipps_label);

  auto* name_list = new QWidget(this);
  name_list->setLayout(group_grid);

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(name_list);
  vbox2->addWidget(error_label_);
  vbox2->addWidget(button_box_);

  this->setWindowTitle(_("Generate New Subkey"));
  this->setLayout(vbox2);
  this->setModal(true);

  connect(this, &SubkeyGenerateDialog::SignalSubKeyGenerated,
          SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh);

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
  error_label_ = new QLabel();
  key_size_spin_box_ = new QSpinBox(this);
  key_type_combo_box_ = new QComboBox(this);
  no_pass_phrase_check_box_ = new QCheckBox(this);
  passphrase_edit_ = new QLineEdit(this);

  for (auto& algo : GenKeyInfo::GetSupportedSubkeyAlgo()) {
    key_type_combo_box_->addItem(QString::fromStdString(algo.first));
  }
  if (!GenKeyInfo::GetSupportedSubkeyAlgo().empty()) {
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

  passphrase_edit_->setEchoMode(QLineEdit::Password);
  passphrase_edit_->setHidden(use_pinentry_);

  auto* vbox1 = new QGridLayout;

  vbox1->addWidget(new QLabel(QString(_("Key Type")) + ": "), 0, 0);
  vbox1->addWidget(new QLabel(QString(_("KeySize (in Bit)")) + ": "), 1, 0);
  vbox1->addWidget(new QLabel(QString(_("Expiration Date")) + ": "), 2, 0);
  vbox1->addWidget(new QLabel(QString(_("Never Expire"))), 2, 3);
  if (!use_pinentry_)
    vbox1->addWidget(new QLabel(QString(_("Password")) + ": "), 3, 0);
  vbox1->addWidget(new QLabel(QString(_("Non Pass Phrase"))), 3, 3);

  vbox1->addWidget(key_type_combo_box_, 0, 1);
  vbox1->addWidget(key_size_spin_box_, 1, 1);
  vbox1->addWidget(date_edit_, 2, 1);
  vbox1->addWidget(expire_check_box_, 2, 2);
  if (!use_pinentry_) vbox1->addWidget(passphrase_edit_, 3, 1);
  vbox1->addWidget(no_pass_phrase_check_box_, 3, 2);

  auto basicInfoGroupBox = new QGroupBox();
  basicInfoGroupBox->setLayout(vbox1);
  basicInfoGroupBox->setTitle(_("Basic Information"));

  return basicInfoGroupBox;
}

void SubkeyGenerateDialog::set_signal_slot() {
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &SubkeyGenerateDialog::slot_key_gen_accept);
  connect(button_box_, &QDialogButtonBox::rejected, this,
          &SubkeyGenerateDialog::reject);

  connect(expire_check_box_, &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_expire_box_changed);

  connect(key_usage_check_boxes_[0], &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_encryption_box_changed);
  connect(key_usage_check_boxes_[1], &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_signing_box_changed);
  connect(key_usage_check_boxes_[2], &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_certification_box_changed);
  connect(key_usage_check_boxes_[3], &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_authentication_box_changed);

  connect(key_type_combo_box_, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &SubkeyGenerateDialog::slot_activated_key_type);

  connect(no_pass_phrase_check_box_, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            gen_key_info_->SetNonPassPhrase(state != 0);
            passphrase_edit_->setDisabled(state != 0);
          });
}

void SubkeyGenerateDialog::slot_expire_box_changed() {
  if (expire_check_box_->checkState()) {
    date_edit_->setEnabled(false);
  } else {
    date_edit_->setEnabled(true);
  }
}

void SubkeyGenerateDialog::refresh_widgets_state() {
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

  key_size_spin_box_->setRange(gen_key_info_->GetSuggestMinKeySize(),
                               gen_key_info_->GetSuggestMaxKeySize());
  key_size_spin_box_->setValue(gen_key_info_->GetKeyLength());
  key_size_spin_box_->setSingleStep(gen_key_info_->GetSizeChangeStep());
}

void SubkeyGenerateDialog::slot_key_gen_accept() {
  std::stringstream err_stream;

  /**
   * primary keys should have a reasonable expiration date (no more than 2 years
   * in the future)
   */
  if (date_edit_->dateTime() > QDateTime::currentDateTime().addYears(2)) {
    err_stream << "  " << _("Expiration time no more than 2 years.") << "  ";
  }

  if (!use_pinentry_ && passphrase_edit_->isEnabled() &&
      passphrase_edit_->text().size() == 0) {
    err_stream << "  " << _("Password is empty.") << std::endl;
  }

  auto err_string = err_stream.str();

  if (err_string.empty()) {
    gen_key_info_->SetKeyLength(key_size_spin_box_->value());

    if (expire_check_box_->checkState()) {
      gen_key_info_->SetNonExpired(true);
    } else {
#ifdef GPGFRONTEND_GUI_QT6
      gen_key_info_->SetExpireTime(boost::posix_time::from_time_t(
          date_edit_->dateTime().toSecsSinceEpoch()));
#else
      gen_key_info_->SetExpireTime(
          boost::posix_time::from_time_t(date_edit_->dateTime().toTime_t()));
#endif
    }

    if (!use_pinentry_ && !gen_key_info_->IsNoPassPhrase()) {
      CoreCommonUtil::GetInstance()->SetTempCacheValue(
          "__key_passphrase", this->passphrase_edit_->text().toStdString());
    }

    GpgError error;
    // TODO: remove plain qt thread usage
    auto thread = QThread::create([&]() {
      SPDLOG_DEBUG("thread started");
      error = GpgKeyOpera::GetInstance().GenerateSubkey(key_, gen_key_info_);
    });
    thread->start();

    auto* waiting_dialog = new WaitingDialog(_("Generating"), this);
    waiting_dialog->show();

    while (thread->isRunning()) {
      QCoreApplication::processEvents();
    }
    waiting_dialog->close();

    if (!use_pinentry_ && !gen_key_info_->IsNoPassPhrase()) {
      CoreCommonUtil::GetInstance()->ResetTempCacheValue("__key_passphrase");
    }

    if (CheckGpgError(error) == GPG_ERR_NO_ERROR) {
      auto* msg_box = new QMessageBox(qobject_cast<QWidget*>(this->parent()));
      msg_box->setAttribute(Qt::WA_DeleteOnClose);
      msg_box->setStandardButtons(QMessageBox::Ok);
      msg_box->setWindowTitle(_("Success"));
      msg_box->setText(_("The new subkey has been generated."));
      msg_box->setModal(true);
      msg_box->open();

      emit SignalSubKeyGenerated();
      this->close();
    } else {
      QMessageBox::critical(this, _("Failure"), _("Failed to generate key."));
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

void SubkeyGenerateDialog::slot_encryption_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowEncryption(false);
  } else {
    gen_key_info_->SetAllowEncryption(true);
  }
}

void SubkeyGenerateDialog::slot_signing_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowSigning(false);
  } else {
    gen_key_info_->SetAllowSigning(true);
  }
}

void SubkeyGenerateDialog::slot_certification_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowCertification(false);
  } else {
    gen_key_info_->SetAllowCertification(true);
  }
}

void SubkeyGenerateDialog::slot_authentication_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowAuthentication(false);
  } else {
    gen_key_info_->SetAllowAuthentication(true);
  }
}

void SubkeyGenerateDialog::slot_activated_key_type(int index) {
  SPDLOG_DEBUG("key type index changed: {}", index);

  // check
  assert(gen_key_info_->GetSupportedSubkeyAlgo().size() >
         static_cast<size_t>(index));
  gen_key_info_->SetAlgo(gen_key_info_->GetSupportedSubkeyAlgo()[index]);
  refresh_widgets_state();
}

}  // namespace GpgFrontend::UI
