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

#include "KeyGenerateDialog.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/CacheUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyGenerateHelper.h"

//
#include "ui_KeyGenDialog.h"

namespace GpgFrontend::UI {

KeyGenerateDialog::KeyGenerateDialog(int channel, QWidget* parent)
    : GeneralDialog(typeid(KeyGenerateDialog).name(), parent),
      ui_(QSharedPointer<Ui_KeyGenDialog>::create()),
      gen_key_info_(QSharedPointer<KeyGenerateInfo>::create()),
      gen_subkey_info_(nullptr),
      supported_primary_key_algos_(
          KeyGenerateInfo::GetSupportedKeyAlgo(channel)),
      supported_subkey_algos_(KeyGenerateInfo::GetSupportedSubkeyAlgo(channel)),
      channel_(channel) {
  ui_->setupUi(this);

  for (const auto& key_db : GetGpgKeyDatabaseInfos()) {
    ui_->keyDBIndexComboBox->insertItem(
        key_db.channel, QString("%1: %2").arg(key_db.channel).arg(key_db.name));
  }

  ui_->easyAlgoComboBox->addItems({
      tr("Custom"),
      "RSA",
      "DSA",
      "ECC (25519)",
  });

  ui_->easyValidityPeriodComboBox->addItems({
      tr("Custom"),
      tr("3 Months"),
      tr("6 Months"),
      tr("1 Year"),
      tr("2 Years"),
      tr("5 Years"),
      tr("10 Years"),
      tr("Non Expired"),
  });

  ui_->easyCombinationComboBox->addItems({
      tr("Primary Key Only"),
      tr("Primary Key With Subkey"),
  });

  ui_->nameLabel->setText(tr("Name"));
  ui_->emailLabel->setText(tr("Email"));
  ui_->commentLabel->setText(tr("Comment"));
  ui_->keyDBLabel->setText(tr("Key Database"));
  ui_->easyAlgoLabel->setText(tr("Algorithm"));
  ui_->combinationLabel->setText(tr("Combination"));
  ui_->easyValidPeriodLabel->setText(tr("Validity Period"));

  ui_->pAlgoLabel->setText(tr("Algorithm"));
  ui_->pExpireDateLabel->setText(tr("Validity Period"));
  ui_->pKeyLengthLabel->setText(tr("Key Length"));
  ui_->pUsageLabel->setText(tr("Usage"));
  ui_->pEncrCheckBox->setText(tr("Encrypt"));
  ui_->pSignCheckBox->setText(tr("Sign"));
  ui_->pAuthCheckBox->setText(tr("Authentication"));
  ui_->noPassphraseCheckBox->setText(tr("No Passphrase"));
  ui_->pExpireCheckBox->setText(tr("Non Expired"));

  ui_->sAlgoLabel->setText(tr("Algorithm"));
  ui_->sExpireDateLabel->setText(tr("Expire Date"));
  ui_->sKeyLengthLabel->setText(tr("Key Length"));
  ui_->sUsageLabel->setText(tr("Usage"));
  ui_->sEncrCheckBox->setText(tr("Encrypt"));
  ui_->sSignCheckBox->setText(tr("Sign"));
  ui_->sAuthCheckBox->setText(tr("Authentication"));
  ui_->sExpireCheckBox->setText(tr("Non Expired"));

  assert(ui_->tabWidget->count() == 3);
  ui_->tabWidget->setTabText(0, tr("Easy Mode"));
  ui_->tabWidget->setTabText(1, tr("Primary Key"));
  ui_->tabWidget->setTabText(2, tr("Subkey"));
  ui_->tabWidget->setCurrentIndex(0);

  ui_->generateButton->setText(tr("Generate"));

  const auto min_date_time = QDateTime::currentDateTime().addDays(3);
  ui_->pExpireDateTimeEdit->setMinimumDateTime(min_date_time);
  ui_->sExpireDateTimeEdit->setMinimumDateTime(min_date_time);

  QSet<QString> p_algo_set;
  for (const auto& algo : supported_primary_key_algos_) {
    p_algo_set.insert(algo.Name());
  }
  ui_->pAlgoComboBox->addItems(
      QStringList(p_algo_set.cbegin(), p_algo_set.cend()));

  QSet<QString> s_algo_set;
  for (const auto& algo : supported_subkey_algos_) {
    s_algo_set.insert(algo.Name());
  }
  ui_->sAlgoComboBox->addItem(tr("None"));
  ui_->sAlgoComboBox->addItems(
      QStringList(s_algo_set.cbegin(), s_algo_set.cend()));

  ui_->easyAlgoComboBox->setCurrentText("RSA");
  ui_->easyValidityPeriodComboBox->setCurrentText(tr("2 Years"));

  set_signal_slot_config();

  slot_easy_mode_changed("RSA");
  slot_easy_valid_date_changed(tr("2 Years"));

  this->setWindowTitle(tr("Generate Key"));
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setModal(true);

  this->show();
  this->raise();
  this->activateWindow();
}

void KeyGenerateDialog::slot_key_gen_accept() {
  QString buffer;
  QTextStream err_stream(&buffer);

  if (ui_->nameEdit->text().size() < 5) {
    err_stream << " -> " << tr("Name must contain at least five characters.")
               << Qt::endl;
  }
  if (ui_->emailEdit->text().isEmpty() ||
      !IsEmailAddress(ui_->emailEdit->text())) {
    err_stream << " -> " << tr("Please give a valid email address.")
               << Qt::endl;
  }

  if (gen_key_info_->GetAlgo() == KeyGenerateInfo::kNoneAlgo) {
    err_stream << " -> " << tr("Please give a valid primary key algorithm.")
               << Qt::endl;
  }

  const auto min_expire_date = QDateTime::currentDateTime().addSecs(120);

  if (!gen_key_info_->IsNonExpired() &&
      gen_key_info_->GetExpireTime() < min_expire_date) {
    err_stream << " -> "
               << tr("Time to primary key expiration must not be less than 120 "
                     "seconds.")
               << Qt::endl;
  }

  if (gen_subkey_info_ != nullptr) {
    if (gen_subkey_info_->GetAlgo() == KeyGenerateInfo::kNoneAlgo) {
      err_stream << " -> " << tr("Please give a valid subkey algorithm.")
                 << Qt::endl;
    }

    if (!gen_subkey_info_->IsNonExpired() &&
        gen_subkey_info_->GetExpireTime() < min_expire_date) {
      err_stream
          << " -> "
          << tr("Time to subkey expiration must not be less than 120 seconds.")
          << Qt::endl;
    }
  }

  const auto err_string = err_stream.readAll();
  if (!err_string.isEmpty()) {
    ui_->statusPlainTextEdit->clear();
    ui_->statusPlainTextEdit->appendPlainText(err_string);
    return;
  }

  gen_key_info_->SetName(ui_->nameEdit->text());
  gen_key_info_->SetEmail(ui_->emailEdit->text());
  gen_key_info_->SetComment(ui_->commentEdit->text());

  LOG_D() << "try to generate key at gpg context channel: " << channel_;

  do_generate();
  this->done(0);
}

void KeyGenerateDialog::refresh_widgets_state() {
  ui_->pAlgoComboBox->blockSignals(true);
  ui_->pAlgoComboBox->setCurrentText(gen_key_info_->GetAlgo().Name());
  ui_->pAlgoComboBox->blockSignals(false);

  ui_->pKeyLengthComboBox->blockSignals(true);
  SetKeyLengthComboxBoxByAlgo(
      ui_->pKeyLengthComboBox,
      SearchAlgoByName(ui_->pAlgoComboBox->currentText(),
                       supported_primary_key_algos_));
  ui_->pKeyLengthComboBox->setCurrentText(
      QString::number(gen_key_info_->GetKeyLength()));
  ui_->pKeyLengthComboBox->blockSignals(false);

  ui_->pEncrCheckBox->blockSignals(true);
  ui_->pEncrCheckBox->setCheckState(
      gen_key_info_->IsAllowEncr() ? Qt::Checked : Qt::Unchecked);
  ui_->pEncrCheckBox->setEnabled(gen_key_info_->IsAllowModifyEncr());
  ui_->pEncrCheckBox->blockSignals(false);

  ui_->pSignCheckBox->blockSignals(true);
  ui_->pSignCheckBox->setCheckState(
      gen_key_info_->IsAllowSign() ? Qt::Checked : Qt::Unchecked);
  ui_->pSignCheckBox->setEnabled(gen_key_info_->IsAllowModifySign());
  ui_->pSignCheckBox->blockSignals(false);

  ui_->pAuthCheckBox->blockSignals(true);
  ui_->pAuthCheckBox->setCheckState(
      gen_key_info_->IsAllowAuth() ? Qt::Checked : Qt::Unchecked);
  ui_->pAuthCheckBox->setEnabled(gen_key_info_->IsAllowModifyAuth());
  ui_->pAuthCheckBox->blockSignals(false);

  ui_->noPassphraseCheckBox->setEnabled(gen_key_info_->IsAllowNoPassPhrase());

  ui_->pExpireDateTimeEdit->blockSignals(true);
  ui_->pExpireDateTimeEdit->setDateTime(gen_key_info_->GetExpireTime());
  ui_->pExpireDateTimeEdit->setDisabled(gen_key_info_->IsNonExpired());
  ui_->pExpireDateTimeEdit->blockSignals(false);

  ui_->pExpireCheckBox->blockSignals(true);
  ui_->pExpireCheckBox->setChecked(gen_key_info_->IsNonExpired());
  ui_->pExpireCheckBox->blockSignals(false);

  ui_->generateButton->setDisabled(false);

  if (gen_subkey_info_ == nullptr) {
    ui_->sTab->setDisabled(true);

    ui_->sAlgoComboBox->blockSignals(true);
    ui_->sAlgoComboBox->setCurrentText(tr("None"));
    ui_->sAlgoComboBox->blockSignals(false);

    ui_->sKeyLengthComboBox->blockSignals(true);
    ui_->sKeyLengthComboBox->clear();
    ui_->sKeyLengthComboBox->blockSignals(false);

    ui_->sEncrCheckBox->blockSignals(true);
    ui_->sEncrCheckBox->setCheckState(Qt::Unchecked);
    ui_->sEncrCheckBox->blockSignals(false);

    ui_->sSignCheckBox->blockSignals(true);
    ui_->sSignCheckBox->setCheckState(Qt::Unchecked);
    ui_->sSignCheckBox->blockSignals(false);

    ui_->sAuthCheckBox->blockSignals(true);
    ui_->sAuthCheckBox->setCheckState(Qt::Unchecked);
    ui_->sAuthCheckBox->blockSignals(false);

    ui_->sExpireDateTimeEdit->blockSignals(true);
    ui_->sExpireDateTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui_->sExpireDateTimeEdit->setDisabled(true);
    ui_->sExpireDateTimeEdit->blockSignals(false);

    ui_->sExpireCheckBox->blockSignals(true);
    ui_->sExpireCheckBox->setCheckState(Qt::Unchecked);
    ui_->sExpireCheckBox->blockSignals(false);

    ui_->easyCombinationComboBox->blockSignals(true);
    ui_->easyCombinationComboBox->setCurrentText(tr("Primary Key Only"));
    ui_->easyCombinationComboBox->blockSignals(false);
    return;
  }

  ui_->sTab->setDisabled(false);

  ui_->sAlgoComboBox->blockSignals(true);
  ui_->sAlgoComboBox->setCurrentText(gen_subkey_info_->GetAlgo().Name());
  ui_->sAlgoComboBox->blockSignals(false);

  ui_->sKeyLengthComboBox->blockSignals(true);
  SetKeyLengthComboxBoxByAlgo(
      ui_->sKeyLengthComboBox,
      SearchAlgoByName(ui_->sAlgoComboBox->currentText(),
                       supported_subkey_algos_));
  ui_->sKeyLengthComboBox->setCurrentText(
      QString::number(gen_subkey_info_->GetKeyLength()));
  ui_->sKeyLengthComboBox->blockSignals(false);

  ui_->sEncrCheckBox->blockSignals(true);
  ui_->sEncrCheckBox->setCheckState(
      gen_subkey_info_->IsAllowEncr() ? Qt::Checked : Qt::Unchecked);
  ui_->sEncrCheckBox->setEnabled(gen_subkey_info_->IsAllowModifyEncr());
  ui_->sEncrCheckBox->blockSignals(false);

  ui_->sSignCheckBox->blockSignals(true);
  ui_->sSignCheckBox->setCheckState(
      gen_subkey_info_->IsAllowSign() ? Qt::Checked : Qt::Unchecked);
  ui_->sSignCheckBox->setEnabled(gen_subkey_info_->IsAllowModifySign());
  ui_->sSignCheckBox->blockSignals(false);

  ui_->sAuthCheckBox->blockSignals(true);
  ui_->sAuthCheckBox->setCheckState(
      gen_subkey_info_->IsAllowAuth() ? Qt::Checked : Qt::Unchecked);
  ui_->sAuthCheckBox->setEnabled(gen_subkey_info_->IsAllowModifyAuth());
  ui_->sAuthCheckBox->blockSignals(false);

  ui_->sExpireDateTimeEdit->blockSignals(true);
  ui_->sExpireDateTimeEdit->setDateTime(gen_subkey_info_->GetExpireTime());
  ui_->sExpireDateTimeEdit->setDisabled(gen_subkey_info_->IsNonExpired());
  ui_->sExpireDateTimeEdit->blockSignals(false);

  ui_->sExpireCheckBox->blockSignals(true);
  ui_->sExpireCheckBox->setChecked(gen_subkey_info_->IsNonExpired());
  ui_->sExpireCheckBox->blockSignals(false);

  ui_->easyCombinationComboBox->blockSignals(true);
  ui_->easyCombinationComboBox->setCurrentText(tr("Primary Key With Subkey"));
  ui_->easyCombinationComboBox->blockSignals(false);
}

void KeyGenerateDialog::set_signal_slot_config() {
  connect(ui_->generateButton, &QPushButton::clicked, this,
          &KeyGenerateDialog::slot_key_gen_accept);

  connect(
      ui_->pExpireCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
        gen_key_info_->SetNonExpired(state == Qt::Checked);
        gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addYears(2));
        slot_set_easy_valid_date_2_custom();
        refresh_widgets_state();
      });
  connect(ui_->sExpireCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            gen_subkey_info_->SetNonExpired(state == Qt::Checked);
            gen_subkey_info_->SetExpireTime(
                QDateTime::currentDateTime().addYears(2));
            slot_set_easy_valid_date_2_custom();
            refresh_widgets_state();
          });

  connect(
      ui_->pEncrCheckBox, &QCheckBox::stateChanged, this,
      [this](int state) { gen_key_info_->SetAllowEncr(state == Qt::Checked); });
  connect(
      ui_->pSignCheckBox, &QCheckBox::stateChanged, this,
      [this](int state) { gen_key_info_->SetAllowSign(state == Qt::Checked); });
  connect(
      ui_->pAuthCheckBox, &QCheckBox::stateChanged, this,
      [this](int state) { gen_key_info_->SetAllowAuth(state == Qt::Checked); });

  connect(ui_->sEncrCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            gen_subkey_info_->SetAllowEncr(state == Qt::Checked);
          });
  connect(ui_->sSignCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            gen_subkey_info_->SetAllowSign(state == Qt::Checked);
          });
  connect(ui_->sAuthCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            gen_subkey_info_->SetAllowAuth(state == Qt::Checked);
          });

  connect(ui_->noPassphraseCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            gen_key_info_->SetNonPassPhrase(state != 0);
            if (gen_subkey_info_ != nullptr) {
              gen_subkey_info_->SetNonPassPhrase(state != 0);
            }
          });

  connect(ui_->pAlgoComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString&) {
            sync_gen_key_algo_info();
            slot_set_easy_key_algo_2_custom();
            refresh_widgets_state();
          });

  connect(ui_->sAlgoComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString&) {
            sync_gen_subkey_algo_info();
            slot_set_easy_key_algo_2_custom();
            refresh_widgets_state();
          });

  connect(ui_->easyAlgoComboBox, &QComboBox::currentTextChanged, this,
          &KeyGenerateDialog::slot_easy_mode_changed);

  connect(ui_->easyValidityPeriodComboBox, &QComboBox::currentTextChanged, this,
          &KeyGenerateDialog::slot_easy_valid_date_changed);

  connect(ui_->pExpireDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this,
          [=](const QDateTime& dt) {
            gen_key_info_->SetExpireTime(dt);

            slot_set_easy_valid_date_2_custom();
          });

  connect(ui_->sExpireDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this,
          [=](const QDateTime& dt) {
            gen_subkey_info_->SetExpireTime(dt);

            slot_set_easy_valid_date_2_custom();
          });

  connect(ui_->keyDBIndexComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          [=](int index) { channel_ = index; });

  connect(ui_->easyCombinationComboBox, &QComboBox::currentTextChanged, this,
          &KeyGenerateDialog::slot_easy_combination_changed);

  connect(ui_->pKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          [this](const QString& text) -> void {
            auto [found, algo] = GetAlgoByNameAndKeyLength(
                ui_->pAlgoComboBox->currentText(), text.toInt(),
                supported_primary_key_algos_);

            if (found) gen_key_info_->SetAlgo(algo);
          });

  connect(ui_->sKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          [this](const QString& text) -> void {
            auto [found, algo] = GetAlgoByNameAndKeyLength(
                ui_->sAlgoComboBox->currentText(), text.toInt(),
                supported_subkey_algos_);

            if (found) gen_subkey_info_->SetAlgo(algo);
          });

  connect(this, &KeyGenerateDialog::SignalKeyGenerated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
}

void KeyGenerateDialog::sync_gen_key_algo_info() {
  auto [found, algo] = GetAlgoByName(ui_->pAlgoComboBox->currentText(),

                                     supported_primary_key_algos_);

  if (found) gen_key_info_->SetAlgo(found ? algo : KeyGenerateInfo::kNoneAlgo);
}

void KeyGenerateDialog::sync_gen_subkey_algo_info() {
  if (gen_subkey_info_ != nullptr) {
    auto [s_found, algo] = GetAlgoByName(ui_->sAlgoComboBox->currentText(),
                                         supported_subkey_algos_);

    if (s_found) {
      gen_subkey_info_->SetAlgo(s_found ? algo : KeyGenerateInfo::kNoneAlgo);
    }
  }
}

void KeyGenerateDialog::slot_easy_mode_changed(const QString& mode) {
  if (mode == "RSA") {
    auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("rsa2048");
    if (found) gen_key_info_->SetAlgo(algo);

    gen_subkey_info_ = nullptr;
  }

  else if (mode == "DSA") {
    auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("dsa2048");
    if (found) gen_key_info_->SetAlgo(algo);

    if (gen_subkey_info_ == nullptr) {
      create_sync_gen_subkey_info();
    }

    auto [s_found, s_algo] = KeyGenerateInfo::SearchSubKeyAlgo("elg2048");
    if (s_found) gen_subkey_info_->SetAlgo(s_algo);
  }

  else if (mode == "ECC (25519)") {
    auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
    if (found) gen_key_info_->SetAlgo(algo);

    if (gen_subkey_info_ == nullptr) {
      create_sync_gen_subkey_info();
    }

    auto [s_found, s_algo] = KeyGenerateInfo::SearchSubKeyAlgo("cv25519");
    if (s_found) gen_subkey_info_->SetAlgo(s_algo);
  }

  else {
    auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("rsa2048");
    if (found) gen_key_info_->SetAlgo(algo);

    if (gen_subkey_info_ == nullptr) {
      create_sync_gen_subkey_info();
    }

    auto [s_found, s_algo] = KeyGenerateInfo::SearchSubKeyAlgo("rsa2048");
    if (s_found) gen_subkey_info_->SetAlgo(s_algo);
  }

  refresh_widgets_state();
}

void KeyGenerateDialog::slot_easy_valid_date_changed(const QString& mode) {
  if (mode == tr("3 Months")) {
    gen_key_info_->SetNonExpired(false);
    gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addMonths(3));
  }

  else if (mode == tr("6 Months")) {
    gen_key_info_->SetNonExpired(false);
    gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addMonths(6));
  }

  else if (mode == tr("1 Year")) {
    gen_key_info_->SetNonExpired(false);
    gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addYears(1));
  }

  else if (mode == tr("2 Years")) {
    gen_key_info_->SetNonExpired(false);
    gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addYears(2));
  }

  else if (mode == tr("5 Years")) {
    gen_key_info_->SetNonExpired(false);
    gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addYears(5));
  }

  else if (mode == tr("10 Years")) {
    gen_key_info_->SetNonExpired(false);
    gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addYears(10));

  }

  else if (mode == tr("Non Expired")) {
    gen_key_info_->SetNonExpired(true);
    gen_key_info_->SetExpireTime(QDateTime::currentDateTime());
  }

  else {
    gen_key_info_->SetNonExpired(false);
    gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addYears(2));
  }

  if (gen_subkey_info_ != nullptr) {
    gen_subkey_info_->SetNonExpired(gen_key_info_->IsNonExpired());
    gen_subkey_info_->SetExpireTime(gen_key_info_->GetExpireTime());
  }

  refresh_widgets_state();
}

void KeyGenerateDialog::slot_set_easy_valid_date_2_custom() {
  ui_->easyValidityPeriodComboBox->blockSignals(true);
  ui_->easyValidityPeriodComboBox->setCurrentText(tr("Custom"));
  ui_->easyValidityPeriodComboBox->blockSignals(false);
}

void KeyGenerateDialog::slot_set_easy_key_algo_2_custom() {
  ui_->easyAlgoComboBox->blockSignals(true);
  ui_->easyAlgoComboBox->setCurrentText(tr("Custom"));
  ui_->easyAlgoComboBox->blockSignals(false);
}

void KeyGenerateDialog::slot_easy_combination_changed(const QString& mode) {
  if (mode == tr("Primary Key Only")) {
    gen_subkey_info_ = nullptr;
  } else {
    create_sync_gen_subkey_info();
  }

  slot_set_easy_key_algo_2_custom();
  refresh_widgets_state();
}

void KeyGenerateDialog::do_generate() {
  if (!GetSettings()
           .value("gnupg/use_pinentry_as_password_input_dialog",
                  !IsFlatpakENV())
           .toBool() &&
      !ui_->noPassphraseCheckBox->isChecked()) {
    SetCacheValue("PinentryContext", "NEW_PASSPHRASE");
  }

  auto f = [this,
            gen_key_info = this->gen_key_info_](const OperaWaitingHd& hd) {
    GpgKeyOpera::GetInstance(channel_).GenerateKeyWithSubkey(
        gen_key_info, gen_subkey_info_,
        [this, hd](GpgError err, const DataObjectPtr&) {
          // stop showing waiting dialog
          hd();

          if (CheckGpgError(err) == GPG_ERR_USER_1) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Unknown error occurred"));
            return;
          }

          CommonUtils::RaiseMessageBox(
              this->parentWidget() != nullptr ? this->parentWidget() : this,
              err);
          if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
            emit SignalKeyGenerated();
          }
        });
  };
  GpgOperaHelper::WaitForOpera(this, tr("Generating"), f);
}

void KeyGenerateDialog::create_sync_gen_subkey_info() {
  if (gen_subkey_info_ == nullptr) {
    gen_subkey_info_ = QSharedPointer<KeyGenerateInfo>::create(true);
  }

  sync_gen_subkey_algo_info();
  slot_easy_valid_date_changed(ui_->easyValidityPeriodComboBox->currentText());
}
}  // namespace GpgFrontend::UI
