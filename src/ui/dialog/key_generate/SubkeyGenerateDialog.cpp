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

#include "SubkeyGenerateDialog.h"

#include <cassert>

#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyGenerateHelper.h"

//
#include "ui_SubkeyGenDialog.h"

namespace GpgFrontend::UI {

SubkeyGenerateDialog::SubkeyGenerateDialog(int channel, GpgKeyPtr key,
                                           QWidget* parent)
    : GeneralDialog(typeid(SubkeyGenerateDialog).name(), parent),
      ui_(QSharedPointer<Ui_SubkeyGenDialog>::create()),
      current_gpg_context_channel_(channel),
      key_(std::move(key)),
      gen_subkey_info_(SecureCreateSharedObject<KeyGenerateInfo>(true)),
      supported_subkey_algos_(KeyGenerateInfo::GetSupportedSubkeyAlgo(
          current_gpg_context_channel_, *key_)) {
  ui_->setupUi(this);
  assert(key_ != nullptr);

  auto if_expire_options_supported = IsOpSupported<SetExpireOpTag>(channel);

  ui_->expireLabel->setHidden(!if_expire_options_supported);
  ui_->expireDateTimeEdit->setHidden(!if_expire_options_supported);
  ui_->nonExpiredCheckBox->setHidden(!if_expire_options_supported);

  ui_->algoLabel->setText(tr("Algorithm"));
  ui_->keyLengthLabel->setText(tr("Key Length"));
  ui_->expireLabel->setText(tr("Expire Date"));
  ui_->usageLabel->setText(tr("Usage"));
  ui_->encrCheckBox->setText(tr("Encrypt"));
  ui_->signCheckBox->setText(tr("Sign"));
  ui_->authCheckBox->setText(tr("Authentication"));
  ui_->nonExpiredCheckBox->setText(tr("Non Expired"));
  ui_->nonPassphraseCheckBox->setText(tr("No Passphrase"));
  ui_->scndAlgoLabel->setText(tr("Second Algorithm"));
  ui_->scndKeyLengthLabel->setText(tr("Second Key Length"));

  const auto min_date_time = QDateTime::currentDateTime().addDays(3);
  ui_->expireDateTimeEdit->setMinimumDateTime(min_date_time);
  ui_->expireDateTimeEdit->setDateTime(
      QDateTime::currentDateTime().addYears(2));

  // populate algo combo box with supported subkey algos
  PopulateAlgoComboBox(ui_->algoComboBox, supported_subkey_algos_);

  ui_->expireDateTimeEdit->setDateTime(gen_subkey_info_->GetExpireTime());
  ui_->expireDateTimeEdit->setDisabled(gen_subkey_info_->IsNonExpired());

  ui_->statusPlainTextEdit->appendPlainText(
      tr("Tipps: if the key pair has a passphrase, the subkey's "
         "passphrase must be equal to it."));

  this->setWindowTitle(tr("Generate New Subkey"));
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setModal(true);

  set_signal_slot_config();
  refresh_widgets_state();

  this->show();
  this->raise();
  this->activateWindow();
}

void SubkeyGenerateDialog::set_signal_slot_config() {
  connect(ui_->generateButton, &QPushButton::clicked, this,
          &SubkeyGenerateDialog::slot_key_gen_accept);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->nonExpiredCheckBox, &QCheckBox::checkStateChanged, this,
          [=](Qt::CheckState state) {
            gen_subkey_info_->SetNonExpired(state == Qt::Checked);
            refresh_widgets_state();
          });
#else
  connect(ui_->nonExpiredCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) {
            gen_subkey_info_->SetNonExpired(state == Qt::Checked);
            refresh_widgets_state();
          });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->encrCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_subkey_info_->SetAllowEncr(state == Qt::Checked);
          });
#else
  connect(ui_->encrCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
    gen_subkey_info_->SetAllowEncr(state == Qt::Checked);
  });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->signCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_subkey_info_->SetAllowSign(state == Qt::Checked);
          });
#else
  connect(ui_->signCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
    gen_subkey_info_->SetAllowSign(state == Qt::Checked);
  });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->authCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_subkey_info_->SetAllowAuth(state == Qt::Checked);
          });
#else
  connect(ui_->authCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
    gen_subkey_info_->SetAllowAuth(state == Qt::Checked);
  });
#endif

  auto get_exact_algo = [this](QComboBox* algoCombo, QComboBox* lenCombo) {
    if (algoCombo->currentIndex() < 0) return std::make_tuple(false, KeyAlgo{});

    auto data = algoCombo->currentData().toMap();
    QString name = data["name"].toString();
    QString type = data["type"].toString();
    int key_len = lenCombo->currentText().toInt();

    return GetAlgoByNameTypeAndKeyLength(name, type, key_len,
                                         supported_subkey_algos_);
  };

  auto validate_generate_button = [this, get_exact_algo]() -> void {
    auto [primaryFound, primaryAlgo] =
        get_exact_algo(ui_->algoComboBox, ui_->keyLengthComboBox);

    bool secondary_found = true;
    if (ui_->scndAlgoComboBox->isEnabled() &&
        !ui_->scndAlgoComboBox->isHidden()) {
      auto [subFound, subAlgo] =
          get_exact_algo(ui_->scndAlgoComboBox, ui_->scndKeyLengthComboBox);
      secondary_found = subFound;
    }

    ui_->generateButton->setDisabled(!(primaryFound && secondary_found));
  };

  auto on_algo_changed = [this, validate_generate_button](QComboBox* algoCombo,
                                                          bool is_sub_algo) {
    auto data = algoCombo->currentData().toMap();
    QString name = data["name"].toString();
    QString type = data["type"].toString();

    auto [found, base_algo] =
        GetAlgoByNameType(name, type, supported_subkey_algos_);
    if (found) {
      if (is_sub_algo) {
        gen_subkey_info_->SetSubAlgo(base_algo);
      } else {
        gen_subkey_info_->SetAlgo(base_algo);
      }
    } else {
      if (is_sub_algo) gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
    }

    if (is_sub_algo) {
      refresh_hybrid_algo_widgets_state();
    } else {
      refresh_widgets_state();
    }

    validate_generate_button();
  };

  auto on_length_changed = [this, get_exact_algo, validate_generate_button](
                               QComboBox* algoCombo, QComboBox* lenCombo,
                               bool is_sub_algo) {
    auto [found, exact_algo] = get_exact_algo(algoCombo, lenCombo);
    if (found) {
      if (is_sub_algo) {
        gen_subkey_info_->SetSubAlgo(exact_algo);
      } else {
        gen_subkey_info_->SetAlgo(exact_algo);
      }
    }
    validate_generate_button();
  };

  connect(ui_->algoComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=](int) { on_algo_changed(ui_->algoComboBox, false); });

  connect(ui_->scndAlgoComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=](int) { on_algo_changed(ui_->scndAlgoComboBox, true); });

  connect(ui_->keyLengthComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString&) {
            on_length_changed(ui_->algoComboBox, ui_->keyLengthComboBox, false);
          });

  connect(ui_->scndKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString&) {
            on_length_changed(ui_->scndAlgoComboBox, ui_->scndKeyLengthComboBox,
                              true);
          });

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->nonPassphraseCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) -> void {
            gen_subkey_info_->SetNonPassPhrase(state == Qt::Checked);
          });
#else
  connect(ui_->nonPassphraseCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            gen_subkey_info_->SetNonPassPhrase(state == Qt::Checked);
          });
#endif
}

void SubkeyGenerateDialog::refresh_widgets_state() {
  ui_->algoComboBox->blockSignals(true);
  ui_->algoComboBox->setCurrentText(gen_subkey_info_->GetAlgo().Name());
  ui_->algoComboBox->blockSignals(false);

  ui_->keyLengthComboBox->blockSignals(true);

  auto data = ui_->algoComboBox->currentData().toMap();
  auto name = data["name"].toString();
  auto type = data["type"].toString();
  SetKeyLengthComboxBoxByAlgo(
      ui_->keyLengthComboBox,
      SearchAlgoByNameType(name, type, supported_subkey_algos_));

  ui_->keyLengthComboBox->setCurrentText(
      QString::number(gen_subkey_info_->GetKeyLength()));
  ui_->keyLengthComboBox->blockSignals(false);

  ui_->encrCheckBox->blockSignals(true);
  ui_->encrCheckBox->setChecked(gen_subkey_info_->IsAllowEncr());
  ui_->encrCheckBox->setEnabled(gen_subkey_info_->IsAllowModifyEncr());
  ui_->encrCheckBox->blockSignals(false);

  ui_->signCheckBox->blockSignals(true);
  ui_->signCheckBox->setChecked(gen_subkey_info_->IsAllowSign());
  ui_->signCheckBox->setEnabled(gen_subkey_info_->IsAllowModifySign());
  ui_->signCheckBox->blockSignals(false);

  ui_->authCheckBox->blockSignals(true);
  ui_->authCheckBox->setChecked(gen_subkey_info_->IsAllowAuth());
  ui_->authCheckBox->setEnabled(gen_subkey_info_->IsAllowModifyAuth());
  ui_->authCheckBox->blockSignals(false);

  ui_->nonPassphraseCheckBox->setEnabled(
      gen_subkey_info_->IsAllowNoPassPhrase());

  ui_->expireDateTimeEdit->blockSignals(true);
  ui_->expireDateTimeEdit->setDateTime(gen_subkey_info_->GetExpireTime());
  ui_->expireDateTimeEdit->setDisabled(gen_subkey_info_->IsNonExpired());
  ui_->expireDateTimeEdit->blockSignals(false);

  ui_->nonExpiredCheckBox->blockSignals(true);
  ui_->nonExpiredCheckBox->setChecked(gen_subkey_info_->IsNonExpired());
  ui_->nonExpiredCheckBox->blockSignals(false);

  // Enable or disable second algo combo box according to whether current algo
  // has sub algos
  QContainer<KeyAlgo> sub_algos =
      gen_subkey_info_->GetAlgo().SubAlgos(current_gpg_context_channel_);
  if (!sub_algos.empty()) {
    ui_->scndAlgoComboBox->setEnabled(true);
    ui_->scndAlgoComboBox->setHidden(false);
    ui_->scndAlgoLabel->setHidden(false);
    ui_->scndKeyLengthLabel->setHidden(false);

    ui_->scndAlgoComboBox->blockSignals(true);
    ui_->scndAlgoComboBox->clear();

    PopulateAlgoComboBox(ui_->scndAlgoComboBox, sub_algos);

    ui_->scndAlgoComboBox->blockSignals(false);
  } else {
    ui_->scndAlgoComboBox->setEnabled(false);
    ui_->scndAlgoComboBox->clear();
    ui_->scndAlgoComboBox->setHidden(true);
    ui_->scndAlgoLabel->setHidden(true);
    ui_->scndKeyLengthLabel->setHidden(true);
  }

  refresh_hybrid_algo_widgets_state();
}

void SubkeyGenerateDialog::slot_key_gen_accept() {
  QString buffer;
  QTextStream err_stream(&buffer);

  if (gen_subkey_info_->GetAlgo() == KeyGenerateInfo::kNoneAlgo) {
    err_stream << " -> " << tr("Please give a valid subkey algorithm.")
               << Qt::endl;
  }

  if (!gen_subkey_info_->IsNonExpired() &&
      gen_subkey_info_->GetExpireTime() <
          QDateTime::currentDateTime().addSecs(120)) {
    err_stream
        << " -> "
        << tr("Time to subkey expiration must not be less than 120 seconds.")
        << Qt::endl;
  }

  const auto err_string = err_stream.readAll();
  if (!err_string.isEmpty()) {
    ui_->statusPlainTextEdit->clear();
    ui_->statusPlainTextEdit->appendPlainText(err_string);
    return;
  }

  GpgOperaHelper::WaitForOpera(
      this, tr("Generating"),
      [this, key = this->key_, gen_key_info = this->gen_subkey_info_](
          const OperaWaitingHd& hd) -> void {
        KeyGenerationOperation::GetInstance(current_gpg_context_channel_)
            .GenerateSubkey(
                key, gen_key_info,
                [this, hd](GpgError err, const DataObjectPtr&) -> void {
                  // stop showing waiting dialog
                  hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  CommonUtils::RaiseMessageBox(this, err);
                  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                    emit UISignalStation::GetInstance()
                        ->SignalKeyDatabaseRefresh();
                  }
                });
      });
  this->done(0);
}

void SubkeyGenerateDialog::refresh_hybrid_algo_widgets_state() {
  if (ui_->scndAlgoComboBox->isHidden()) {
    ui_->scndKeyLengthComboBox->setEnabled(false);
    ui_->scndKeyLengthComboBox->clear();
    ui_->scndKeyLengthComboBox->setHidden(true);
    return;
  }

  ui_->scndKeyLengthComboBox->setEnabled(true);
  ui_->scndKeyLengthComboBox->setHidden(false);
  ui_->scndKeyLengthComboBox->blockSignals(true);

  auto data = ui_->algoComboBox->currentData().toMap();
  auto name = data["name"].toString();
  auto type = data["type"].toString();
  SetKeyLengthComboxBoxByAlgo(
      ui_->scndKeyLengthComboBox,
      SearchAlgoByNameType(name, type, supported_subkey_algos_));

  ui_->scndKeyLengthComboBox->setCurrentText(
      QString::number(gen_subkey_info_->SubAlgo().KeyLength()));
  ui_->scndKeyLengthComboBox->blockSignals(false);
}
}  // namespace GpgFrontend::UI
