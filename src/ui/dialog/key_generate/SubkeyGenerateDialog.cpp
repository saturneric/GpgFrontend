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
#include <cstddef>

#include "core/function/gpg/GpgKeyOpera.h"
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
      gen_subkey_info_(QSharedPointer<KeyGenerateInfo>::create(true)),
      supported_subkey_algos_(KeyGenerateInfo::GetSupportedSubkeyAlgo()) {
  ui_->setupUi(this);
  assert(key_ != nullptr);

  ui_->algoLabel->setText(tr("Algorithm"));
  ui_->keyLengthLabel->setText(tr("Key Length"));
  ui_->expireLabel->setText(tr("Expire Date"));
  ui_->usageLabel->setText(tr("Usage"));
  ui_->encrCheckBox->setText(tr("Encrypt"));
  ui_->signCheckBox->setText(tr("Sign"));
  ui_->authCheckBox->setText(tr("Authentication"));
  ui_->nonExpiredCheckBox->setText(tr("Non Expired"));
  ui_->nonPassphraseCheckBox->setText(tr("No Passphrase"));

  const auto min_date_time = QDateTime::currentDateTime().addDays(3);
  ui_->expireDateTimeEdit->setMinimumDateTime(min_date_time);

  QSet<QString> algo_set;
  for (const auto& algo : supported_subkey_algos_) {
    algo_set.insert(algo.Name());
  }
  ui_->algoComboBox->addItems(QStringList(algo_set.cbegin(), algo_set.cend()));

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
}

void SubkeyGenerateDialog::set_signal_slot_config() {
  connect(ui_->generateButton, &QPushButton::clicked, this,
          &SubkeyGenerateDialog::slot_key_gen_accept);

  connect(ui_->nonExpiredCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) {
            gen_subkey_info_->SetNonExpired(state == Qt::Checked);
            refresh_widgets_state();
          });

  connect(ui_->encrCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
    gen_subkey_info_->SetAllowEncr(state == Qt::Checked);
  });
  connect(ui_->signCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
    gen_subkey_info_->SetAllowSign(state == Qt::Checked);
  });
  connect(ui_->authCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
    gen_subkey_info_->SetAllowAuth(state == Qt::Checked);
  });

  connect(ui_->algoComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString& text) {
            auto [found, algo] = GetAlgoByName(text, supported_subkey_algos_);
            ui_->generateButton->setDisabled(!found);
            if (found) gen_subkey_info_->SetAlgo(algo);

            refresh_widgets_state();
          });

  connect(ui_->nonPassphraseCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            gen_subkey_info_->SetNonPassPhrase(state == Qt::Checked);
          });

  connect(ui_->keyLengthComboBox, &QComboBox::currentTextChanged, this,
          [this](const QString& text) -> void {
            auto [found, algo] = GetAlgoByNameAndKeyLength(
                ui_->algoComboBox->currentText(), text.toInt(),
                supported_subkey_algos_);

            if (found) gen_subkey_info_->SetAlgo(algo);
          });
}

void SubkeyGenerateDialog::refresh_widgets_state() {
  ui_->algoComboBox->blockSignals(true);
  ui_->algoComboBox->setCurrentText(gen_subkey_info_->GetAlgo().Name());
  ui_->algoComboBox->blockSignals(false);

  ui_->keyLengthComboBox->blockSignals(true);
  SetKeyLengthComboxBoxByAlgo(ui_->keyLengthComboBox,
                              SearchAlgoByName(ui_->algoComboBox->currentText(),
                                               supported_subkey_algos_));
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
      [this, key = this->key_,
       gen_key_info = this->gen_subkey_info_](const OperaWaitingHd& hd) {
        GpgKeyOpera::GetInstance(current_gpg_context_channel_)
            .GenerateSubkey(key, gen_key_info,
                            [this, hd](GpgError err, const DataObjectPtr&) {
                              // stop showing waiting dialog
                              hd();

                              if (CheckGpgError(err) == GPG_ERR_USER_1) {
                                QMessageBox::critical(
                                    this, tr("Error"),
                                    tr("Unknown error occurred"));
                                return;
                              }

                              CommonUtils::RaiseMessageBox(this, err);
                              if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                                emit UISignalStation::GetInstance()
                                    -> SignalKeyDatabaseRefresh();
                              }
                            });
      });
  this->done(0);
}

}  // namespace GpgFrontend::UI
