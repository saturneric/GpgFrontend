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

namespace {

auto ComboCurrentNameType(QComboBox* combo) -> QPair<QString, QString> {
  if (combo == nullptr) return {};

  const auto data = combo->currentData().toMap();
  return {data.value("name").toString(), data.value("type").toString()};
}

auto SetComboCurrentAlgo(QComboBox* combo, const GpgFrontend::KeyAlgo& algo)
    -> bool {
  if (combo == nullptr) return false;

  for (int i = 0; i < combo->count(); ++i) {
    const auto data = combo->itemData(i).toMap();

    if (data.value("id").toString() == algo.Id() &&
        data.value("name").toString() == algo.Name() &&
        data.value("type").toString() == algo.Type()) {
      combo->setCurrentIndex(i);
      return true;
    }
  }

  return false;
}

auto IsAlgoInList(const GpgFrontend::KeyAlgo& algo,
                  const QContainer<GpgFrontend::KeyAlgo>& algos) -> bool {
  return std::any_of(
      algos.cbegin(), algos.cend(),
      [&](const GpgFrontend::KeyAlgo& item) { return item == algo; });
}

auto ComboCurrentAlgo(QComboBox* combo,
                      const QContainer<GpgFrontend::KeyAlgo>& algos)
    -> GpgFrontend::KeyAlgo {
  if (combo == nullptr || combo->currentIndex() < 0) {
    return GpgFrontend::KeyGenerateInfo::kNoneAlgo;
  }

  const auto data = combo->currentData().toMap();
  const auto id = data.value("id").toString();
  const auto type = data.value("type").toString();

  auto it = std::find_if(algos.cbegin(), algos.cend(),
                         [&](const GpgFrontend::KeyAlgo& algo) {
                           return algo.Id() == id && algo.Type() == type;
                         });

  return it != algos.cend() ? *it : GpgFrontend::KeyGenerateInfo::kNoneAlgo;
}

}  // namespace

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

  const auto engine =
      OpenPGPContext::GetInstance(current_gpg_context_channel_).Engine();

  // The "subkey passphrase must equal the primary key's" constraint is a GnuPG
  // behaviour; rPGP does not impose it, so only show this tip for GnuPG.
  if (engine == OpenPGPEngine::kGNUPG) {
    ui_->statusPlainTextEdit->appendPlainText(
        tr("Tipps: if the key pair has a passphrase, the subkey's "
           "passphrase must be equal to it."));
  }

  // Post-quantum (PQC) subkey algorithms require a v6 primary key; for a v4
  // primary key they are filtered out of the algorithm list above. Tell the
  // user why instead of leaving them silently absent. KeyVersion() reports 0
  // when the engine does not expose the key format (e.g. GnuPG), so only show
  // this when the key is known to be v4.
  if (key_->KeyVersion() == 4) {
    ui_->statusPlainTextEdit->appendPlainText(
        tr("Note: post-quantum (PQC) subkey algorithms are unavailable here "
           "because the primary key uses the v4 key format. Generate a v6 key "
           "to use PQC algorithms."));
  }

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

  auto get_exact_algo = [](QComboBox* algo_combo, QComboBox* len_combo,
                           const QContainer<KeyAlgo>& algos) {
    if (algo_combo == nullptr || len_combo == nullptr ||
        algo_combo->currentIndex() < 0) {
      return std::make_tuple(false, KeyAlgo{});
    }

    const auto data = algo_combo->currentData().toMap();
    const auto name = data.value("name").toString();
    const auto type = data.value("type").toString();
    const auto key_len = len_combo->currentText().toInt();

    return GetAlgoByNameTypeAndKeyLength(name, type, key_len, algos);
  };

  auto validate_generate_button = [this, get_exact_algo]() -> void {
    const auto [primary_found, primary_algo] = get_exact_algo(
        ui_->algoComboBox, ui_->keyLengthComboBox, supported_subkey_algos_);

    bool secondary_found = true;
    const auto sub_algos =
        gen_subkey_info_->GetAlgo().SubAlgos(current_gpg_context_channel_);

    if (!sub_algos.isEmpty()) {
      secondary_found =
          gen_subkey_info_->SubAlgo() != KeyGenerateInfo::kNoneAlgo;
    }

    ui_->generateButton->setDisabled(!(primary_found && secondary_found));
  };

  auto on_primary_algo_changed = [this, validate_generate_button](int index) {
    if (index < 0) {
      gen_subkey_info_->SetAlgo(KeyGenerateInfo::kNoneAlgo);
      gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
      refresh_widgets_state();
      validate_generate_button();
      return;
    }

    auto algo = ComboCurrentAlgo(ui_->algoComboBox, supported_subkey_algos_);
    gen_subkey_info_->SetAlgo(algo);
    gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);

    refresh_widgets_state();
    validate_generate_button();
  };

  auto on_primary_length_changed = [this, get_exact_algo,
                                    validate_generate_button](const QString&) {
    const auto [found, exact_algo] = get_exact_algo(
        ui_->algoComboBox, ui_->keyLengthComboBox, supported_subkey_algos_);

    if (!found) {
      validate_generate_button();
      return;
    }

    const auto old_sub_algo = gen_subkey_info_->SubAlgo();

    gen_subkey_info_->SetAlgo(exact_algo);

    const auto sub_algos =
        gen_subkey_info_->GetAlgo().SubAlgos(current_gpg_context_channel_);

    if (old_sub_algo != KeyGenerateInfo::kNoneAlgo &&
        IsAlgoInList(old_sub_algo, sub_algos)) {
      gen_subkey_info_->SetSubAlgo(old_sub_algo);
    } else {
      gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
    }

    refresh_widgets_state();
    validate_generate_button();
  };

  auto on_secondary_algo_changed = [this, validate_generate_button](int index) {
    const auto sub_algos =
        gen_subkey_info_->GetAlgo().SubAlgos(current_gpg_context_channel_);

    if (index < 0 || sub_algos.isEmpty()) {
      gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
      refresh_hybrid_algo_widgets_state();
      validate_generate_button();
      return;
    }

    auto algo = ComboCurrentAlgo(ui_->scndAlgoComboBox, sub_algos);
    gen_subkey_info_->SetSubAlgo(algo);

    refresh_hybrid_algo_widgets_state();
    validate_generate_button();
  };

  auto on_secondary_length_changed =
      [this, get_exact_algo, validate_generate_button](const QString&) {
        const auto sub_algos =
            gen_subkey_info_->GetAlgo().SubAlgos(current_gpg_context_channel_);

        const auto [found, exact_algo] = get_exact_algo(
            ui_->scndAlgoComboBox, ui_->scndKeyLengthComboBox, sub_algos);

        if (found) {
          gen_subkey_info_->SetSubAlgo(exact_algo);
        }

        validate_generate_button();
      };

  connect(ui_->algoComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          on_primary_algo_changed);

  connect(ui_->keyLengthComboBox, &QComboBox::currentTextChanged, this,
          on_primary_length_changed);

  connect(ui_->scndAlgoComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          on_secondary_algo_changed);

  connect(ui_->scndKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          on_secondary_length_changed);

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
  SetComboCurrentAlgo(ui_->algoComboBox, gen_subkey_info_->GetAlgo());
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
  const auto sub_algos =
      gen_subkey_info_->GetAlgo().SubAlgos(current_gpg_context_channel_);

  if (!sub_algos.empty()) {
    ui_->scndAlgoComboBox->setEnabled(true);
    ui_->scndAlgoComboBox->setHidden(false);
    ui_->scndAlgoLabel->setHidden(false);
    ui_->scndKeyLengthLabel->setHidden(false);

    ui_->scndAlgoComboBox->blockSignals(true);
    ui_->scndAlgoComboBox->clear();
    PopulateAlgoComboBox(ui_->scndAlgoComboBox, sub_algos);

    const auto current_sub_algo = gen_subkey_info_->SubAlgo();
    const bool current_sub_algo_supported =
        current_sub_algo != KeyGenerateInfo::kNoneAlgo &&
        IsAlgoInList(current_sub_algo, sub_algos);

    if (current_sub_algo_supported) {
      SetComboCurrentAlgo(ui_->scndAlgoComboBox, current_sub_algo);
    } else if (ui_->scndAlgoComboBox->count() > 0) {
      ui_->scndAlgoComboBox->setCurrentIndex(0);

      const auto default_sub_algo =
          ComboCurrentAlgo(ui_->scndAlgoComboBox, sub_algos);
      gen_subkey_info_->SetSubAlgo(default_sub_algo);
    } else {
      gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
    }

    ui_->scndAlgoComboBox->blockSignals(false);
  } else {
    gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);

    ui_->scndAlgoComboBox->blockSignals(true);
    ui_->scndAlgoComboBox->clear();
    ui_->scndAlgoComboBox->setEnabled(false);
    ui_->scndAlgoComboBox->setHidden(true);
    ui_->scndAlgoComboBox->blockSignals(false);

    ui_->scndAlgoLabel->setHidden(true);
    ui_->scndKeyLengthLabel->setHidden(true);

    ui_->scndKeyLengthComboBox->blockSignals(true);
    ui_->scndKeyLengthComboBox->clear();
    ui_->scndKeyLengthComboBox->setEnabled(false);
    ui_->scndKeyLengthComboBox->setHidden(true);
    ui_->scndKeyLengthComboBox->blockSignals(false);
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

  const auto sub_algos =
      gen_subkey_info_->GetAlgo().SubAlgos(current_gpg_context_channel_);

  if (!sub_algos.isEmpty()) {
    if (gen_subkey_info_->SubAlgo() == KeyGenerateInfo::kNoneAlgo ||
        !IsAlgoInList(gen_subkey_info_->SubAlgo(), sub_algos)) {
      err_stream << " -> " << tr("Please give a valid second algorithm.")
                 << Qt::endl;
    }
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

  // WaitForOpera() runs a nested event loop and returns only after the
  // completion callback below has run, so the flag reliably reflects the
  // outcome by the time it returns.
  auto succeeded = std::make_shared<bool>(false);

  GpgOperaHelper::WaitForOpera(
      this, tr("Generating"),
      [this, succeeded, key = this->key_,
       gen_key_info =
           this->gen_subkey_info_](const OperaWaitingHd& hd) -> void {
        KeyGenerationOperation::GetInstance(current_gpg_context_channel_)
            .GenerateSubkey(key, gen_key_info,
                            [this, hd, succeeded](
                                GpgError err, const DataObjectPtr&) -> void {
                              // stop showing waiting dialog
                              hd();

                              if (CheckGpgError(err) == GPG_ERR_USER_1) {
                                QMessageBox::critical(
                                    this, tr("Error"),
                                    tr("Unknown error occurred"));
                                return;
                              }

                              if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                                // Report the failure on the still-open dialog
                                // so the user can adjust and retry; success is
                                // handled by the caller.
                                CommonUtils::RaiseMessageBox(this, err);
                                return;
                              }

                              *succeeded = true;
                              emit UISignalStation::GetInstance()
                                  -> SignalKeyDatabaseRefresh();
                            });
      });

  // Keep the dialog open on failure so the user can adjust and retry without
  // losing the entered configuration. Only close on success.
  if (!*succeeded) return;

  // Close the dialog first, then confirm to the user anchored to the parent
  // window, so the success notice doesn't appear layered over a dialog that is
  // about to disappear.
  auto* notify_parent = this->parentWidget();
  this->accept();

  QMessageBox::information(notify_parent, tr("Success"),
                           tr("Subkey generation completed successfully."));
}

void SubkeyGenerateDialog::refresh_hybrid_algo_widgets_state() {
  const auto sub_algos =
      gen_subkey_info_->GetAlgo().SubAlgos(current_gpg_context_channel_);

  if (sub_algos.isEmpty() || ui_->scndAlgoComboBox->isHidden()) {
    ui_->scndKeyLengthComboBox->blockSignals(true);
    ui_->scndKeyLengthComboBox->clear();
    ui_->scndKeyLengthComboBox->setEnabled(false);
    ui_->scndKeyLengthComboBox->setHidden(true);
    ui_->scndKeyLengthComboBox->blockSignals(false);

    ui_->scndKeyLengthLabel->setHidden(true);
    return;
  }

  ui_->scndKeyLengthLabel->setHidden(false);
  ui_->scndKeyLengthComboBox->setEnabled(true);
  ui_->scndKeyLengthComboBox->setHidden(false);

  ui_->scndKeyLengthComboBox->blockSignals(true);

  const auto [name, type] = ComboCurrentNameType(ui_->scndAlgoComboBox);

  SetKeyLengthComboxBoxByAlgo(ui_->scndKeyLengthComboBox,
                              SearchAlgoByNameType(name, type, sub_algos));

  if (gen_subkey_info_->SubAlgo() != KeyGenerateInfo::kNoneAlgo) {
    ui_->scndKeyLengthComboBox->setCurrentText(
        QString::number(gen_subkey_info_->SubAlgo().KeyLength()));
  }

  ui_->scndKeyLengthComboBox->blockSignals(false);
}

}  // namespace GpgFrontend::UI
