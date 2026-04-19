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

#include "core/function/gpg/GpgKeyOpera.h"
#include "core/model/CacheObject.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyGenerateHelper.h"

//
#include "ui_KeyGenDialog.h"

namespace {

const int kProfileNameMaxLen = 32;

auto MakeDefaultEasyModeConf() -> QJsonArray {
  auto make_entry = [](const QString& name, QString p_algo, QString s_algo = "",
                       const QString& ss_algo = "",
                       bool hidden = false) -> QJsonObject {
    QJsonObject obj;
    obj["name"] = name;
    obj["primary"] = QJsonObject{{"algo", p_algo}, {"validity", "2y"}};
    if (!s_algo.isEmpty()) {
      auto subkey_obj = QJsonObject{{"algo", s_algo}, {"validity", "2y"}};
      if (!ss_algo.isEmpty()) {
        subkey_obj["sub_algo"] = ss_algo;
      }
      obj["subkey"] = subkey_obj;
    }

    obj["hidden"] = hidden;
    return obj;
  };

  return QJsonArray{
      make_entry("Ed25519 / X25519 (Modern & Fast)", "ED25519", "CV25519"),
      make_entry("RSA-3072 (Balanced)", "RSA3072"),
      make_entry("NIST P-384 (Standard)", "NISTP384", "NISTP384"),
      make_entry("Brainpool P-256 (EU Standard)", "BRAINPOOLP256R1",
                 "BRAINPOOLP256R1"),
      make_entry("RSA-4096 (Higher Security)", "RSA4096"),
      make_entry("Ed448 (High Security)", "ED448", "X448"),
      make_entry("Ed448 + Kyber-1024 (PQC Hybrid)", "ED448", "KYBER1024",
                 "X448"),
      make_entry("RSA-2048 (Legacy)", "RSA2048", ""),
      make_entry("DSA-2048 + ELG-2048 (Deprecated)", "DSA2048", "ELG2048"),
  };
}

auto EasyModeConfFromJson(const QJsonObject& obj)
    -> std::optional<GpgFrontend::UI::KeyGenerateDialog::EasyModeConf> {
  if (obj.isEmpty() || !obj.contains("name") || !obj.contains("primary")) {
    return std::nullopt;
  }
  QJsonObject primary = obj.value("primary").toObject();
  QJsonObject subkey = obj.value("subkey").toObject();

  if (primary.isEmpty() || !primary.contains("algo") ||
      !primary.contains("validity")) {
    return std::nullopt;
  }

  GpgFrontend::UI::KeyGenerateDialog::EasyModeConf conf;
  conf.name = obj.value("name").toString().trimmed();

  auto raw_algo = primary.value("algo").toString().trimmed();
  auto [found, algo] =
      GpgFrontend::KeyGenerateInfo::SearchPrimaryKeyAlgo(raw_algo.toLower());
  if (found) {
    conf.key_algo = algo.Id();
  } else {
    LOG_W() << "invalid primary key algo from json: " << raw_algo
            << "ignoreing config...";
    return std::nullopt;
  }

  conf.key_validity = primary.value("validity").toString();
  conf.hidden = obj.value("hidden").toBool(false);

  if (!subkey.isEmpty() && subkey.contains("algo") &&
      subkey.contains("validity") &&
      !subkey.value("algo").toString().trimmed().isEmpty()) {
    conf.has_s_key = true;

    auto raw_s_algo = subkey.value("algo").toString().trimmed();
    auto [s_found, s_algo] =
        GpgFrontend::KeyGenerateInfo::SearchSubKeyAlgo(raw_s_algo.toLower());
    if (s_found) {
      conf.s_key_algo = s_algo.Id();
    } else {
      LOG_W() << "invalid subkey algo from json: " << raw_s_algo
              << "ignoreing config...";
      return std::nullopt;
    }

    conf.s_key_validity = subkey.value("validity").toString();

    if (subkey.contains("sub_algo") &&
        !subkey.value("sub_algo").toString().trimmed().isEmpty()) {
      auto raw_ss_algo = subkey.value("sub_algo").toString().trimmed();
      auto [ss_found, ss_algo] =
          GpgFrontend::KeyGenerateInfo::SearchSubKeyAlgo(raw_ss_algo.toLower());
      if (ss_found) {
        conf.s_key_sub_algo = ss_algo.Id();
      } else {
        LOG_W() << "invalid subkey's sub algo from json: " << raw_ss_algo
                << "ignoreing config...";
        return std::nullopt;
      }
    }
  } else {
    // if we don't set it explicitly, it may be defaulted to true or false by
    // compiler, which may cause unexpected behavior. So we set it explicitly to
    // false here.
    conf.has_s_key = false;
  }
  return conf;
}

auto EasyModeConfToJson(
    const GpgFrontend::UI::KeyGenerateDialog::EasyModeConf& conf)
    -> QJsonObject {
  QJsonObject obj;
  obj["name"] = conf.name;
  obj["hidden"] = conf.hidden;

  QJsonObject primary;
  primary["algo"] = conf.key_algo;
  primary["validity"] = conf.key_validity;
  obj["primary"] = primary;

  if (conf.has_s_key) {
    QJsonObject subkey;
    subkey["algo"] = conf.s_key_algo;
    subkey["validity"] = conf.s_key_validity;
    if (!conf.s_key_sub_algo.isEmpty()) {
      subkey["sub_algo"] = conf.s_key_sub_algo;
    }
    obj["subkey"] = subkey;
  }

  return obj;
}
}  // namespace

namespace GpgFrontend::UI {

KeyGenerateDialog::KeyGenerateDialog(int channel, QWidget* parent)
    : GeneralDialog(typeid(KeyGenerateDialog).name(), parent),
      channel_(channel),
      ui_(QSharedPointer<Ui_KeyGenDialog>::create()),
      gen_key_info_(SecureCreateSharedObject<KeyGenerateInfo>()),
      gen_subkey_info_(nullptr),
      supported_primary_key_algos_(
          KeyGenerateInfo::GetSupportedKeyAlgo(channel)),
      supported_subkey_algos_(
          KeyGenerateInfo::GetSupportedSubkeyAlgo(channel)) {
  ui_->setupUi(this);

  for (const auto& key_db : GetGpgKeyDatabaseInfos()) {
    auto bnd_type = ConvertPGPBackendType2String(
        GpgContext::GetInstance(key_db.channel).Engine());
    ui_->keyDBIndexComboBox->insertItem(key_db.channel, QString("[%2]: %3 (%1)")
                                                            .arg(bnd_type)
                                                            .arg(key_db.channel)
                                                            .arg(key_db.name));
  }
  ui_->keyDBIndexComboBox->setCurrentIndex(channel);

  for (const auto& option : k_expire_options_list_) {
    ui_->easyValidityPeriodComboBox->addItem(option.display);
  }

  ui_->easyCombinationComboBox->addItems({
      tr("Primary Key Only"),
      tr("Primary Key With Subkey"),
  });

  ui_->profileGroupBox->setTitle(tr("Profile"));
  ui_->basicGroupBox->setTitle(tr("Basic"));

  ui_->nameLabel->setText(tr("Name"));
  ui_->emailLabel->setText(tr("Email"));
  ui_->commentLabel->setText(tr("Comment"));
  ui_->keyDBLabel->setText(tr("Key Database"));
  ui_->easyProfileLabel->setText(tr("Name"));
  ui_->combinationLabel->setText(tr("Combination"));
  ui_->easyValidPeriodLabel->setText(tr("Validity Period"));

  ui_->savePushButton->setText(tr("Save"));
  ui_->savePushButton->setToolTip(
      tr("Save current configuration as a new profile"));
  ui_->deletePushButton->setText(tr("Delete"));
  ui_->deletePushButton->setToolTip(tr("Delete current selected profile"));
  ui_->reset2DefaultPushButton->setText(tr("Reset To Default"));
  ui_->reset2DefaultPushButton->setToolTip(
      tr("Reset profile list to default configuration"));

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

  set_signal_slot_config();

  load_easy_profile_config();

  QString info_text;
  info_text += (tr("GnuPG Version: %1") + "\n\n").arg(GnuPGVersion());

  info_text +=
      tr("If subkey is specified, it will be generated together with the "
         "primary key. Therefore, you may need to enter the passphrase "
         "additionally for the subkey generation.") +
      "\n\n";

  info_text += "\n";
  info_text += tr("Supported Primary Key Algorithms: ") + "\n";
  for (const auto& algo : supported_primary_key_algos_) {
    if (algo.Id() == "none") continue;
    info_text += QString("  - %1 (%2, %3 bits)\n")
                     .arg(algo.Name())
                     .arg(algo.Type())
                     .arg(algo.KeyLength());
  }

  info_text += "\n";
  info_text += tr("Supported Subkey Algorithms: ") + "\n";
  for (const auto& algo : supported_subkey_algos_) {
    if (algo.Id() == "none") continue;
    info_text += QString("  - %1 (%2, %3 bits)\n")
                     .arg(algo.Name())
                     .arg(algo.Type())
                     .arg(algo.KeyLength());
  }

  info_text += "\n";
  info_text += tr(
      "Please select a key algorithm and configure the parameters as needed.");

  ui_->statusPlainTextEdit->clear();
  ui_->statusPlainTextEdit->setPlainText(info_text);

  // flush easy profile cache on dialog close
  connect(this, &QDialog::finished, this,
          [this](int) { flush_easy_profile_config_cache(); });

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

  // handle sub algo related widgets
  QContainer<KeyAlgo> sub_algos = gen_subkey_info_->GetAlgo().SubAlgos();
  if (!sub_algos.empty()) {
    ui_->scndAlgoComboBox->setEnabled(true);
    ui_->scndAlgoComboBox->setHidden(false);
    ui_->scndAlgoLabel->setHidden(false);
    ui_->scndKeyLengthLabel->setHidden(false);

    ui_->scndAlgoComboBox->blockSignals(true);
    ui_->scndAlgoComboBox->clear();
    QSet<QString> sub_algo_names;
    for (const auto& algo : sub_algos) {
      sub_algo_names.insert(algo.Name());
    }
    for (const auto& algo_name : sub_algo_names) {
      ui_->scndAlgoComboBox->addItem(algo_name);
    }
    ui_->scndAlgoComboBox->blockSignals(false);

    if (!gen_subkey_info_->SubAlgo().Id().isEmpty()) {
      ui_->scndAlgoComboBox->blockSignals(true);
      ui_->scndAlgoComboBox->setCurrentText(gen_subkey_info_->SubAlgo().Name());
      ui_->scndAlgoComboBox->blockSignals(false);
    }

  } else {
    ui_->scndAlgoComboBox->setEnabled(false);
    ui_->scndAlgoComboBox->clear();
    ui_->scndAlgoComboBox->setHidden(true);
    ui_->scndAlgoLabel->setHidden(true);
    ui_->scndKeyLengthLabel->setHidden(true);
  }

  refresh_hybrid_algo_widgets_state();
}

void KeyGenerateDialog::set_signal_slot_config() {
  connect(ui_->generateButton, &QPushButton::clicked, this,
          &KeyGenerateDialog::slot_key_gen_accept);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(
      ui_->pExpireCheckBox, &QCheckBox::checkStateChanged, this,
      [this](Qt::CheckState state) {
        gen_key_info_->SetNonExpired(state == Qt::Checked);
        gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addYears(2));
        slot_set_easy_valid_date_2_custom();
        refresh_widgets_state();
      });
#else
  connect(
      ui_->pExpireCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
        gen_key_info_->SetNonExpired(state == Qt::Checked);
        gen_key_info_->SetExpireTime(QDateTime::currentDateTime().addYears(2));
        slot_set_easy_valid_date_2_custom();
        refresh_widgets_state();
      });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->sExpireCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_subkey_info_->SetNonExpired(state == Qt::Checked);
            gen_subkey_info_->SetExpireTime(
                QDateTime::currentDateTime().addYears(2));
            slot_set_easy_valid_date_2_custom();
            refresh_widgets_state();
          });
#else
  connect(ui_->sExpireCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            gen_subkey_info_->SetNonExpired(state == Qt::Checked);
            gen_subkey_info_->SetExpireTime(
                QDateTime::currentDateTime().addYears(2));
            slot_set_easy_valid_date_2_custom();
            refresh_widgets_state();
          });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->pEncrCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_key_info_->SetAllowEncr(state == Qt::Checked);
          });
#else
  connect(
      ui_->pEncrCheckBox, &QCheckBox::stateChanged, this,
      [this](int state) { gen_key_info_->SetAllowEncr(state == Qt::Checked); });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->pSignCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_key_info_->SetAllowSign(state == Qt::Checked);
          });
#else
  connect(
      ui_->pSignCheckBox, &QCheckBox::stateChanged, this,
      [this](int state) { gen_key_info_->SetAllowSign(state == Qt::Checked); });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->pAuthCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_key_info_->SetAllowAuth(state == Qt::Checked);
          });
#else
  connect(
      ui_->pAuthCheckBox, &QCheckBox::stateChanged, this,
      [this](int state) { gen_key_info_->SetAllowAuth(state == Qt::Checked); });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->sEncrCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_subkey_info_->SetAllowEncr(state == Qt::Checked);
          });
#else
  connect(ui_->sEncrCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            gen_subkey_info_->SetAllowEncr(state == Qt::Checked);
          });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->sSignCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_subkey_info_->SetAllowSign(state == Qt::Checked);
          });
#else
  connect(ui_->sSignCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            gen_subkey_info_->SetAllowSign(state == Qt::Checked);
          });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->sAuthCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            gen_subkey_info_->SetAllowAuth(state == Qt::Checked);
          });
#else
  connect(ui_->sAuthCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) {
            gen_subkey_info_->SetAllowAuth(state == Qt::Checked);
          });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->noPassphraseCheckBox, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) -> void {
            gen_key_info_->SetNonPassPhrase(state != Qt::Unchecked);
            if (gen_subkey_info_ != nullptr) {
              gen_subkey_info_->SetNonPassPhrase(state != Qt::Unchecked);
            }
          });
#else
  connect(ui_->noPassphraseCheckBox, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            gen_key_info_->SetNonPassPhrase(state != 0);
            if (gen_subkey_info_ != nullptr) {
              gen_subkey_info_->SetNonPassPhrase(state != 0);
            }
          });
#endif

  connect(ui_->pAlgoComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString&) -> void {
            sync_gen_key_algo_info();
            slot_set_easy_key_algo_2_custom();
            refresh_widgets_state();
          });

  connect(ui_->sAlgoComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString&) -> void {
            sync_gen_subkey_algo_info();
            slot_set_easy_key_algo_2_custom();
            refresh_widgets_state();

            if (ui_->scndAlgoComboBox->isEnabled()) {
              auto [found, algo] =
                  GetAlgoByName(ui_->scndAlgoComboBox->currentText(),
                                supported_subkey_algos_);
              if (found) {
                gen_subkey_info_->SetSubAlgo(algo);
              }
            } else {
              gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
            }
          });

  connect(ui_->easyProfileComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          &KeyGenerateDialog::slot_easy_profile_changed);

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
          [=](int index) -> void { channel_ = index; });

  connect(ui_->easyCombinationComboBox, &QComboBox::currentTextChanged, this,
          &KeyGenerateDialog::slot_easy_combination_changed);

  connect(ui_->pKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          [this](const QString& text) -> void {
            auto [found, algo] = GetAlgoByNameAndKeyLength(
                ui_->pAlgoComboBox->currentText(), text.toInt(),
                supported_primary_key_algos_);

            if (found) {
              gen_key_info_->SetAlgo(algo);
              slot_set_easy_key_algo_2_custom();
            }
          });

  connect(ui_->sKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          [this](const QString& text) -> void {
            auto [found, algo] = GetAlgoByNameAndKeyLength(
                ui_->sAlgoComboBox->currentText(), text.toInt(),
                supported_subkey_algos_);

            if (found) {
              gen_subkey_info_->SetAlgo(algo);
              slot_set_easy_key_algo_2_custom();
            }
          });

  connect(this, &KeyGenerateDialog::SignalKeyGenerated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);

  connect(ui_->savePushButton, &QPushButton::clicked, this,
          &KeyGenerateDialog::slot_save_as_easy_profile_config);

  connect(ui_->deletePushButton, &QPushButton::clicked, this,
          &KeyGenerateDialog::slot_delete_easy_profile_config);

  connect(ui_->reset2DefaultPushButton, &QPushButton::clicked, this,
          &KeyGenerateDialog::slot_reset_easy_profile_config_to_default);

  connect(ui_->scndAlgoComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString& text) -> void {
            auto [found, algo] = GetAlgoByName(text, supported_subkey_algos_);
            if (found) {
              gen_subkey_info_->SetSubAlgo(algo);
              slot_set_easy_key_algo_2_custom();
            }
            refresh_hybrid_algo_widgets_state();
          });

  connect(ui_->scndKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          [this](const QString& text) -> void {
            auto [found, algo] = GetAlgoByNameAndKeyLength(
                ui_->scndAlgoComboBox->currentText(), text.toInt(),
                supported_subkey_algos_);

            if (found) {
              gen_subkey_info_->SetSubAlgo(algo);
              slot_set_easy_key_algo_2_custom();
            }
          });
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

    // if algo not found, we don't set sub algo to none algo, because some sub
    // algos
    gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
  }
}

namespace {
auto ParseValidityString(const QString& v) -> QDateTime {
  QDateTime now = QDateTime::currentDateTime();
  if (v == "forever" || v == "none") return now.addYears(100);
  if (v.endsWith("y")) return now.addYears(v.left(v.length() - 1).toInt());
  if (v.endsWith("m")) return now.addMonths(v.left(v.length() - 1).toInt());
  if (v.endsWith("d")) return now.addDays(v.left(v.length() - 1).toInt());
  if (v.endsWith("t")) return now.addSecs(v.left(v.length() - 1).toInt());
  return now.addYears(2);
}
}  // namespace

void KeyGenerateDialog::slot_easy_profile_changed(int index) {
  if (!easy_profile_conf_index_.contains(index)) {
    ui_->deletePushButton->setDisabled(true);
    return;
  }

  ui_->deletePushButton->setDisabled(false);

  auto c = easy_profile_conf_index_.value(index);
  if ((c.has_s_key && c.key_validity == c.s_key_validity) || !c.has_s_key) {
    const auto expire_option =
        k_expire_options_.value(c.key_validity, k_custom_expire_option_);
    ui_->easyValidityPeriodComboBox->setCurrentText(expire_option.display);
  } else {
    slot_set_easy_valid_date_2_custom();
  }

  auto [found, algo] =
      KeyGenerateInfo::SearchPrimaryKeyAlgo(c.key_algo.toLower());
  if (found) gen_key_info_->SetAlgo(algo);

  auto dt = ParseValidityString(c.key_validity);
  gen_key_info_->SetNonExpired(c.key_validity == "forever" ||
                               c.key_validity == "none");

  gen_key_info_->SetExpireTime(dt);

  if (c.has_s_key) {
    if (gen_subkey_info_ == nullptr) {
      create_sync_gen_subkey_info();
    }

    auto [s_found, s_algo] =
        KeyGenerateInfo::SearchSubKeyAlgo(c.s_key_algo.toLower());
    if (s_found) gen_subkey_info_->SetAlgo(s_algo);

    gen_subkey_info_->SetNonExpired(c.s_key_validity == "forever" ||
                                    c.s_key_validity == "none");

    auto dt = ParseValidityString(c.s_key_validity);
    gen_subkey_info_->SetExpireTime(dt);

    if (c.s_key_sub_algo.isEmpty()) {
      gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
    } else {
      auto [sub_found, sub_algo] =
          KeyGenerateInfo::SearchSubKeyAlgo(c.s_key_sub_algo.toLower());
      if (sub_found) {
        gen_subkey_info_->SetSubAlgo(sub_algo);
      } else {
        gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
      }
    }

  } else {
    gen_subkey_info_ = nullptr;
  }

  refresh_widgets_state();
}

void KeyGenerateDialog::slot_easy_valid_date_changed(const QString& mode) {
  auto it = std::find_if(
      k_expire_options_list_.begin(), k_expire_options_list_.end(),
      [&](const ExpireOption& o) -> bool { return o.display == mode; });

  auto expire_option =
      it != k_expire_options_list_.end() ? *it : k_default_expire_option_;

  if (expire_option.key != "custom") {
    gen_key_info_->SetNonExpired(expire_option.non_expired);
    gen_key_info_->SetExpireTime(expire_option.calc_expire_time());

    if (gen_subkey_info_ != nullptr) {
      gen_subkey_info_->SetNonExpired(gen_key_info_->IsNonExpired());
      gen_subkey_info_->SetExpireTime(gen_key_info_->GetExpireTime());
    }
  }

  refresh_widgets_state();
}

void KeyGenerateDialog::slot_set_easy_valid_date_2_custom() {
  ui_->easyValidityPeriodComboBox->blockSignals(true);
  ui_->easyValidityPeriodComboBox->setCurrentText(tr("Custom"));
  ui_->easyValidityPeriodComboBox->blockSignals(false);
}

void KeyGenerateDialog::slot_set_easy_key_algo_2_custom() {
  ui_->easyProfileComboBox->blockSignals(true);
  ui_->easyProfileComboBox->setCurrentText(tr("Custom"));
  ui_->easyProfileComboBox->blockSignals(false);
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
  auto f = [this, gen_key_info =
                      this->gen_key_info_](const OperaWaitingHd& hd) -> void {
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
    gen_subkey_info_ = SecureCreateSharedObject<KeyGenerateInfo>(true);
  }

  sync_gen_subkey_algo_info();
  slot_easy_valid_date_changed(ui_->easyValidityPeriodComboBox->currentText());
}

void KeyGenerateDialog::load_easy_profile_config() {
  QContainer<EasyModeConf> easy_mode_conf = easy_mode_conf_;

  if (easy_mode_conf.empty()) {
    QJsonArray conf;

    CacheObject cache("key_gen_easy_mode_config");

    if (!cache.isEmpty() && cache.isArray()) conf = cache.array();
    if (conf.empty()) {
      LOG_D() << "no easy mode config found in cache, "
                 "loading default config.";
      // use a default config by default
      conf = MakeDefaultEasyModeConf();
      cache.setArray(conf);
    }

    LOG_D() << "loading easy mode config: " << conf;

    for (const auto& item : conf) {
      auto obj = item.toObject();
      auto conf_opt = EasyModeConfFromJson(obj);
      if (!conf_opt) {
        LOG_W() << "invalid easy mode config item: " << obj.toVariantMap();
        continue;
      }
      easy_mode_conf.push_back(*conf_opt);
    }
  }

  ui_->easyProfileComboBox->blockSignals(true);

  easy_profile_conf_index_.clear();
  ui_->easyProfileComboBox->clear();
  ui_->easyProfileComboBox->addItem(tr("Custom"));

  int index = 1;
  for (const auto& item : easy_mode_conf) {
    if (item.hidden || item.name.isEmpty()) continue;
    easy_profile_conf_index_.insert(index++, item);
    ui_->easyProfileComboBox->addItem(item.name);
  }

  easy_mode_conf_ = easy_mode_conf;

  ui_->easyProfileComboBox->blockSignals(false);

  ui_->easyProfileComboBox->setCurrentIndex(
      easy_profile_conf_index_.isEmpty() ? 0
                                         : easy_profile_conf_index_.firstKey());

  refresh_widgets_state();
}

void KeyGenerateDialog::slot_save_as_easy_profile_config() {
  EasyModeConf conf;

  auto validity = ui_->easyValidityPeriodComboBox->currentText();
  auto it = std::find_if(
      k_expire_options_.begin(), k_expire_options_.end(),
      [&](const ExpireOption& o) -> bool { return o.display == validity; });

  auto expire_option =
      it != k_expire_options_.end() ? *it : k_default_expire_option_;

  conf.key_algo = gen_key_info_->GetAlgo().Id();
  conf.key_validity =
      gen_key_info_->IsNonExpired() ? "forever" : expire_option.key;

  if (conf.key_validity == "custom") {
    conf.key_validity =
        QString::number(gen_key_info_->GetExpireTime().toSecsSinceEpoch() -
                        QDateTime::currentDateTime().toSecsSinceEpoch()) +
        "t";
  }

  conf.has_s_key = gen_subkey_info_ != nullptr;
  if (conf.has_s_key) {
    conf.s_key_algo = gen_subkey_info_->GetAlgo().Id();
    conf.s_key_validity =
        gen_subkey_info_->IsNonExpired() ? "forever" : expire_option.key;
  }

  if (conf.s_key_validity == "custom") {
    conf.s_key_validity =
        QString::number(gen_subkey_info_->GetExpireTime().toSecsSinceEpoch() -
                        QDateTime::currentDateTime().toSecsSinceEpoch()) +
        "t";
  }

  if (gen_subkey_info_->SubAlgo().Id() != KeyGenerateInfo::kNoneAlgo.Id()) {
    conf.s_key_sub_algo = gen_subkey_info_->SubAlgo().Id();
  } else {
    conf.s_key_sub_algo = "";
  }

  LOG_D() << "try to save easy mode config, ss_algo: " << conf.s_key_sub_algo;

  bool ok;
  auto profile_name = QInputDialog::getText(this, tr("Save Profile"),
                                            tr("Please enter profile name:"),
                                            QLineEdit::Normal, "", &ok);

  if (!ok) {
    return;
  }

  if (profile_name.trimmed().isEmpty()) {
    QMessageBox::warning(this, tr("Notice"),
                         tr("Profile was not saved: Name cannot be empty."));
    return;
  }

  if (profile_name.trimmed().compare(tr("Custom"), Qt::CaseInsensitive) == 0) {
    QMessageBox::warning(this, tr("Notice"),
                         tr("The profile name 'Custom' is reserved. Please "
                            "choose another name."));
    return;
  }

  if (profile_name.trimmed().length() > kProfileNameMaxLen) {
    QMessageBox::warning(
        this, tr("Notice"),
        tr("Profile was not saved: Name cannot be longer than %1 characters.")
            .arg(kProfileNameMaxLen));
    return;
  }

  bool exists = std::any_of(easy_mode_conf_.begin(), easy_mode_conf_.end(),
                            [&](const EasyModeConf& existing) -> bool {
                              return existing.name.trimmed().compare(
                                         profile_name.trimmed(),
                                         Qt::CaseInsensitive) == 0;
                            });

  if (exists) {
    QMessageBox::warning(this, tr("Notice"),
                         tr("Profile was not saved: Name already exists."));
    return;
  }

  conf.name = profile_name.trimmed();
  conf.hidden = false;
  easy_mode_conf_.push_front(conf);

  load_easy_profile_config();
}

void KeyGenerateDialog::slot_delete_easy_profile_config() {
  QString profile_name = ui_->easyProfileComboBox->currentText().trimmed();

  if (profile_name.compare(tr("Custom"), Qt::CaseInsensitive) == 0) {
    QMessageBox::information(this, tr("Notice"),
                             tr("The 'Custom' profile cannot be deleted."));
    return;
  }

  auto it = std::find_if(easy_mode_conf_.begin(), easy_mode_conf_.end(),
                         [&](const EasyModeConf& c) -> bool {
                           return c.name.trimmed().compare(
                                      profile_name, Qt::CaseInsensitive) == 0;
                         });

  if (it == easy_mode_conf_.end()) {
    QMessageBox::warning(this, tr("Notice"),
                         tr("Selected profile does not exist."));
    return;
  }

  auto reply = QMessageBox::question(
      this, tr("Delete Profile"),
      tr("Are you sure you want to delete the profile '%1'?").arg(profile_name),
      QMessageBox::Yes | QMessageBox::No);

  if (reply != QMessageBox::Yes) {
    return;
  }

  easy_mode_conf_.erase(it);
  flush_easy_profile_config_cache();
  load_easy_profile_config();
}

void KeyGenerateDialog::flush_easy_profile_config_cache() {
  CacheObject cache("key_gen_easy_mode_config");

  QJsonArray array;
  for (const auto& conf : easy_mode_conf_) {
    auto object = EasyModeConfToJson(conf);
    if (!object.isEmpty()) array.append(object);
  }

  cache.setArray(array);
}

void KeyGenerateDialog::slot_reset_easy_profile_config_to_default() {
  auto reply = QMessageBox::question(
      this, tr("Reset To Default"),
      tr("Are you sure you want to reset the easy profile configuration to "
         "default? This action cannot be undone."),
      QMessageBox::Yes | QMessageBox::No);

  if (reply != QMessageBox::Yes) {
    return;
  }

  {
    CacheObject cache("key_gen_easy_mode_config");
    cache.setArray(MakeDefaultEasyModeConf());
  }

  easy_mode_conf_ = {};
  load_easy_profile_config();
}

void KeyGenerateDialog::refresh_hybrid_algo_widgets_state() {
  if (ui_->scndAlgoComboBox->isHidden()) {
    ui_->scndKeyLengthComboBox->setEnabled(false);
    ui_->scndKeyLengthComboBox->clear();
    ui_->scndKeyLengthComboBox->setHidden(true);
    return;
  }

  ui_->scndKeyLengthComboBox->setEnabled(true);
  ui_->scndKeyLengthComboBox->setHidden(false);
  ui_->scndKeyLengthComboBox->blockSignals(true);
  SetKeyLengthComboxBoxByAlgo(
      ui_->scndKeyLengthComboBox,
      SearchAlgoByName(ui_->scndAlgoComboBox->currentText(),
                       supported_subkey_algos_));
  ui_->scndKeyLengthComboBox->setCurrentText(
      QString::number(gen_subkey_info_->SubAlgo().KeyLength()));
  ui_->scndKeyLengthComboBox->blockSignals(false);
}
}  // namespace GpgFrontend::UI
