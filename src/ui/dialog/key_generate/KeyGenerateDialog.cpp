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

#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/model/CacheObject.h"
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

auto ComboCurrentNameType(QComboBox* combo) -> QPair<QString, QString> {
  if (combo == nullptr) return {};

  const auto data = combo->currentData().toMap();
  return {data.value("name").toString(), data.value("type").toString()};
}

auto ComboCurrentIdType(QComboBox* combo) -> QPair<QString, QString> {
  if (combo == nullptr) return {};

  const auto data = combo->currentData().toMap();
  return {data.value("id").toString(), data.value("type").toString()};
}

auto SetComboCurrentAlgo(QComboBox* combo, const GpgFrontend::KeyAlgo& algo)
    -> bool {
  if (combo == nullptr) {
    return false;
  }

  for (int i = 0; i < combo->count(); ++i) {
    const auto data = combo->itemData(i).toMap();

    if (data.value("name").toString() == algo.Name() &&
        data.value("type").toString() == algo.Type()) {
      combo->setCurrentIndex(i);
      return true;
    }
  }

  LOG_W() << "failed to set combo current algo:"
          << "id=" << algo.Id() << "name=" << algo.Name()
          << "type=" << algo.Type() << "length=" << algo.KeyLength();

  return false;
}

auto MakeDefaultEasyModeConf() -> QJsonArray {
  auto make_primary = [](const QString& algo, const QString& type,
                         const QString& sub_algo = "",
                         const QString& sub_algo_type = "") -> QJsonObject {
    QJsonObject primary{
        {"algo", algo},
        {"type", type},
        {"validity", "2y"},
    };

    if (!sub_algo.isEmpty()) {
      primary["sub_algo"] = sub_algo;
      primary["sub_algo_type"] = sub_algo_type;
    }

    return primary;
  };

  auto make_subkey = [](const QString& algo, const QString& type,
                        const QString& sub_algo = "",
                        const QString& sub_algo_type = "") -> QJsonObject {
    QJsonObject subkey{
        {"algo", algo},
        {"type", type},
        {"validity", "2y"},
    };

    if (!sub_algo.isEmpty()) {
      subkey["sub_algo"] = sub_algo;
      subkey["sub_algo_type"] = sub_algo_type;
    }

    return subkey;
  };

  auto make_entry = [](const QString& name, const QJsonObject& primary,
                       const QJsonObject& subkey = {},
                       bool hidden = false) -> QJsonObject {
    QJsonObject obj;
    obj["name"] = name;
    obj["primary"] = primary;
    obj["hidden"] = hidden;

    if (!subkey.isEmpty()) {
      obj["subkey"] = subkey;
    }

    return obj;
  };

  return QJsonArray{
      // Recommended modern default.
      make_entry("Ed25519 / X25519 (Modern & Fast)",
                 make_primary("ed25519", "EdDSA"),
                 make_subkey("cv25519", "ECDH")),

      // Higher classical security level.
      make_entry("Ed448 / X448 (High Security)", make_primary("ed448", "EdDSA"),
                 make_subkey("x448", "ECDH")),

      // Conservative compatibility profiles.
      make_entry("RSA-3072 (Balanced Compatibility)",
                 make_primary("rsa3072", "RSA")),

      make_entry("RSA-4096 (High Compatibility)",
                 make_primary("rsa4096", "RSA")),

      // Standards-oriented ECC profiles.
      make_entry("NIST P-384 (Standard ECC)", make_primary("nistp384", "ECDSA"),
                 make_subkey("nistp384", "ECDH")),

      make_entry("Brainpool P-256 (EU Standard ECC)",
                 make_primary("brainpoolp256r1", "ECDSA"),
                 make_subkey("brainpoolp256r1", "ECDH")),

      // PQC hybrid encryption with classical signing primary.
      // This is useful for GnuPG 2.5.x style hybrid encryption profiles.
      make_entry("Ed25519 / ML-KEM-768 + X25519 (PQC Encryption Hybrid)",
                 make_primary("ed25519", "EdDSA"),
                 make_subkey("ky768", "HYBRID-KEM", "cv25519", "ECDH")),

      make_entry("Ed448 / ML-KEM-1024 + X448 (PQC Encryption Hybrid)",
                 make_primary("ed448", "EdDSA"),
                 make_subkey("kyber1024", "HYBRID-KEM", "x448", "ECDH")),

      // Full PQC hybrid signing primary + PQC hybrid encryption subkey.
      // These will only be shown when the selected engine supports them.
      make_entry("ML-DSA-65 + Ed25519 / ML-KEM-768 + X25519 (Full PQC Hybrid)",
                 make_primary("mldsa65", "HYBRID-SIGN", "ed25519", "EdDSA"),
                 make_subkey("ky768", "HYBRID-KEM", "cv25519", "ECDH")),

      make_entry("ML-DSA-87 + Ed448 / ML-KEM-1024 + X448 (Full PQC Hybrid)",
                 make_primary("mldsa87", "HYBRID-SIGN", "ed448", "EdDSA"),
                 make_subkey("kyber1024", "HYBRID-KEM", "x448", "ECDH")),

      // Legacy profiles. Keep visible only if you still want users to find
      // them.
      make_entry("RSA-2048 (Legacy Compatibility)",
                 make_primary("rsa2048", "RSA")),

      // Deprecated profile. Keep in default config but hidden from normal
      // users.
      make_entry("DSA-2048 + ELG-2048 (Deprecated)",
                 make_primary("dsa2048", "DSA"),
                 make_subkey("elg2048", "ELG-E"), true),
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
  if (raw_algo.isEmpty()) {
    LOG_W() << "invalid primary key algo from json: " << raw_algo
            << "ignoring config...";
    return std::nullopt;
  }

  conf.key_algo = raw_algo.toLower();
  conf.key_algo_type = primary.value("type").toString().trimmed();

  if (primary.contains("sub_algo") &&
      !primary.value("sub_algo").toString().trimmed().isEmpty()) {
    auto raw_key_sub_algo = primary.value("sub_algo").toString().trimmed();
    conf.key_sub_algo = raw_key_sub_algo.toLower();
    conf.key_sub_algo_type =
        primary.value("sub_algo_type").toString().trimmed();

    if (conf.key_sub_algo_type.isEmpty()) {
      auto [ss_found, ss_algo] =
          GpgFrontend::KeyGenerateInfo::SearchSubKeyAlgo(conf.key_sub_algo);
      if (ss_found) {
        conf.key_sub_algo_type = ss_algo.Type();
      }
    }
  }

  conf.key_validity = primary.value("validity").toString();
  conf.hidden = obj.value("hidden").toBool(false);

  if (!subkey.isEmpty() && subkey.contains("algo") &&
      subkey.contains("validity") &&
      !subkey.value("algo").toString().trimmed().isEmpty()) {
    conf.has_s_key = true;

    auto raw_s_algo = subkey.value("algo").toString().trimmed();
    if (raw_s_algo.isEmpty()) {
      LOG_W() << "invalid subkey algo from json: " << raw_s_algo
              << "ignoring config...";
      return std::nullopt;
    }

    conf.s_key_algo = raw_s_algo.toLower();
    conf.s_key_algo_type = subkey.value("type").toString().trimmed();

    if (conf.s_key_algo_type.isEmpty()) {
      auto [s_found, s_algo] =
          GpgFrontend::KeyGenerateInfo::SearchSubKeyAlgo(conf.s_key_algo);
      if (s_found) {
        conf.s_key_algo_type = s_algo.Type();
      }
    }

    conf.s_key_validity = subkey.value("validity").toString();

    if (subkey.contains("sub_algo") &&
        !subkey.value("sub_algo").toString().trimmed().isEmpty()) {
      auto raw_ss_algo = subkey.value("sub_algo").toString().trimmed();
      if (!raw_ss_algo.isEmpty()) {
        conf.s_key_sub_algo = raw_ss_algo.toLower();
        conf.s_key_sub_algo_type =
            subkey.value("sub_algo_type").toString().trimmed();

        if (conf.s_key_sub_algo_type.isEmpty()) {
          auto [ss_found, ss_algo] =
              GpgFrontend::KeyGenerateInfo::SearchSubKeyAlgo(
                  conf.s_key_sub_algo);
          if (ss_found) {
            conf.s_key_sub_algo_type = ss_algo.Type();
          }
        }
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
  primary["type"] = conf.key_algo_type;
  primary["validity"] = conf.key_validity;

  if (!conf.key_sub_algo.isEmpty()) {
    primary["sub_algo"] = conf.key_sub_algo;
    primary["sub_algo_type"] = conf.key_sub_algo_type;
  }

  obj["primary"] = primary;

  if (conf.has_s_key) {
    QJsonObject subkey;
    subkey["algo"] = conf.s_key_algo;
    subkey["type"] = conf.s_key_algo_type;
    subkey["validity"] = conf.s_key_validity;

    if (!conf.s_key_sub_algo.isEmpty()) {
      subkey["sub_algo"] = conf.s_key_sub_algo;
      subkey["sub_algo_type"] = conf.s_key_sub_algo_type;
    }

    obj["subkey"] = subkey;
  }

  return obj;
}

void CompactLayout(QLayout* layout, int margin, int spacing) {
  if (layout == nullptr) return;

  layout->setContentsMargins(margin, margin, margin, margin);
  layout->setSpacing(spacing);

  for (int i = 0; i < layout->count(); ++i) {
    auto* item = layout->itemAt(i);
    if (item == nullptr) continue;

    if (auto* child_layout = item->layout(); child_layout != nullptr) {
      CompactLayout(child_layout, margin, spacing);
    }

    if (auto* widget = item->widget(); widget != nullptr) {
      if (auto* widget_layout = widget->layout(); widget_layout != nullptr) {
        CompactLayout(widget_layout, margin, spacing);
      }
    }
  }
}

#ifdef Q_OS_MACOS

void SetFormLayoutCompact(QFormLayout* layout) {
  if (layout == nullptr) return;

  layout->setContentsMargins(0, 0, 0, 0);
  layout->setHorizontalSpacing(8);
  layout->setVerticalSpacing(5);
  layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
}

void SetGridLayoutCompact(QGridLayout* layout) {
  if (layout == nullptr) return;

  layout->setContentsMargins(0, 0, 0, 0);
  layout->setHorizontalSpacing(8);
  layout->setVerticalSpacing(5);
}

#endif

auto ComboCurrentAlgo(
    QComboBox* combo,
    const GpgFrontend::QContainer<GpgFrontend::KeyAlgo>& algos)
    -> GpgFrontend::KeyAlgo {
  if (combo == nullptr) return GpgFrontend::KeyGenerateInfo::kNoneAlgo;

  const auto data = combo->currentData().toMap();
  const auto id = data.value("id").toString();
  const auto type = data.value("type").toString();
  const auto key_length = data.value("length").toInt();

  auto it = std::find_if(algos.cbegin(), algos.cend(),
                         [&](const GpgFrontend::KeyAlgo& algo) {
                           if (algo.Id() != id || algo.Type() != type)
                             return false;

                           // Some old itemData may not contain length.
                           if (key_length <= 0) return true;

                           return algo.KeyLength() == key_length;
                         });

  return it != algos.cend() ? *it : GpgFrontend::KeyGenerateInfo::kNoneAlgo;
}

struct RandomIdentity {
  QString name;
  QString email;
};

// Build an anonymous, privacy-preserving identity so users don't have to
// hand-type a name and email for throwaway or test keys. Deliberately avoids
// any real-looking human name: the identity is an opaque random token that
// carries no personal information and cannot impersonate a real person. Both
// the local-part and the domain label are randomized, but the domain always
// ends in an RFC 2606 / RFC 6761 reserved TLD (.test/.example/.invalid) so the
// address is guaranteed never to route to a real domain. Generation stays
// fully local.
auto GenerateRandomIdentity() -> RandomIdentity {
  // Reserved, non-routable TLDs: a random domain label under any of these can
  // never collide with a domain someone actually owns.
  static const QStringList kReservedTlds = {"test", "example", "invalid"};

  auto* rng = QRandomGenerator::global();

  // Random base-36 tokens (lowercase letters + digits). 64 bits of entropy is
  // far more than enough to keep them unique and unguessable.
  const auto random_token = [rng]() -> QString {
    return QString::number(rng->generate64(), 36).rightJustified(8, '0');
  };

  const auto user_token = random_token();
  const auto domain_label = random_token();
  const auto tld = kReservedTlds.at(rng->bounded(kReservedTlds.size()));

  RandomIdentity identity;
  identity.name = QString("anon-%1").arg(user_token);
  identity.email = QString("anon-%1@%2.%3").arg(user_token, domain_label, tld);
  return identity;
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
  InitUi();

  for (const auto& key_db : GetGpgKeyDatabaseInfos()) {
    auto bnd_type = ConvertOpenPGPEngine2String(
        OpenPGPContext::GetInstance(key_db.channel).Engine());
    ui_->keyDBIndexComboBox->addItem(QString("[%2]: %3 (%1)")
                                         .arg(bnd_type)
                                         .arg(key_db.channel)
                                         .arg(key_db.name),
                                     key_db.channel);
  }
  const int key_db_combo_index = ui_->keyDBIndexComboBox->findData(channel);
  ui_->keyDBIndexComboBox->setCurrentIndex(
      key_db_combo_index >= 0 ? key_db_combo_index : 0);
  ui_->keyDBIndexComboBox->setEnabled(false);

  for (const auto& option : k_expire_options_list_) {
    ui_->easyValidityPeriodComboBox->addItem(option.display);
  }

  auto if_expire_options_supported = IsOpSupported<SetExpireOpTag>(channel);

  // the rPGP backend has very limited support for key generation, so we disable
  // some options when rPGP is used as the backend engine.
  ui_->easyValidPeriodLabel->setHidden(!if_expire_options_supported);
  ui_->easyValidityPeriodComboBox->setHidden(!if_expire_options_supported);
  ui_->pExpireCheckBox->setHidden(!if_expire_options_supported);
  ui_->sExpireCheckBox->setHidden(!if_expire_options_supported);
  ui_->pExpireDateTimeEdit->setHidden(!if_expire_options_supported);
  ui_->sExpireDateTimeEdit->setHidden(!if_expire_options_supported);
  ui_->pExpireDateLabel->setHidden(!if_expire_options_supported);
  ui_->sExpireDateLabel->setHidden(!if_expire_options_supported);

  ui_->easyCombinationComboBox->addItems({
      tr("Primary Key Only"),
      tr("Primary Key With Subkey"),
  });

  ui_->profileGroupBox->setTitle(tr("Profile"));
  ui_->basicGroupBox->setTitle(tr("Basic"));

  ui_->nameLabel->setText(tr("Name"));
  ui_->emailLabel->setText(tr("Email"));
  ui_->commentLabel->setText(tr("Comment"));

  ui_->randomIdentityButton->setIcon(QIcon(":/icons/refresh.png"));
  ui_->randomIdentityButton->setToolTip(
      tr("Fill in a random anonymous identity (for throwaway or test keys)"));
  ui_->randomIdentityButton->setAutoRaise(true);
  ui_->keyDBLabel->setText(tr("Key Database"));
  ui_->easyProfileLabel->setText(tr("Name"));
  ui_->combinationLabel->setText(tr("Combination"));
  ui_->easyValidPeriodLabel->setText(tr("Validity Period"));

  ui_->savePushButton->setText(tr("Save"));
  ui_->savePushButton->setToolTip(
      tr("Save current configuration as a new profile"));
  ui_->deletePushButton->setText(tr("Delete"));
  ui_->deletePushButton->setToolTip(tr("Delete current selected profile"));
  ui_->reset2DefaultPushButton->setText(tr("Reset"));
  ui_->reset2DefaultPushButton->setToolTip(
      tr("Reset profile list to default configuration"));

  ui_->pAlgoLabel->setText(tr("Algorithm"));
  ui_->pExpireDateLabel->setText(tr("Validity Period"));
  ui_->pKeyLengthLabel->setText(tr("Key Length"));
  ui_->pScndAlgoLabel->setText(tr("Second Algorithm"));
  ui_->pScndKeyLengthLabel->setText(tr("Second Key Length"));
  ui_->pKeyFormatLabel->setText(tr("Key Format"));
  ui_->pUsageLabel->setText(tr("Usage"));

  // v4 stays the interoperable default; v6 (RFC 9580) is opt-in because much of
  // the ecosystem still has weak v6 support.
  ui_->pKeyFormatComboBox->addItem(tr("v4 (Compatible)"), 4);
  ui_->pKeyFormatComboBox->addItem(tr("v6 (Modern)"), 6);
  ui_->pEncrCheckBox->setText(tr("Encrypt"));
  ui_->pSignCheckBox->setText(tr("Sign"));
  ui_->pAuthCheckBox->setText(tr("Authentication"));
  ui_->noPassphraseCheckBox->setText(tr("No Passphrase"));
  ui_->pExpireCheckBox->setText(tr("Non Expired"));

  ui_->sAlgoLabel->setText(tr("Algorithm"));
  ui_->sExpireDateLabel->setText(tr("Expire Date"));
  ui_->sKeyLengthLabel->setText(tr("Key Length"));
  ui_->sUsageLabel->setText(tr("Usage"));
  ui_->scndAlgoLabel->setText(tr("Second Algorithm"));
  ui_->scndKeyLengthLabel->setText(tr("Second Key Length"));
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

  PopulateAlgoComboBox(ui_->pAlgoComboBox, supported_primary_key_algos_);
  PopulateAlgoComboBox(ui_->sAlgoComboBox, supported_subkey_algos_);

  set_signal_slot_config();

  load_easy_profile_config();

  QString info_text;

  auto& ctx = OpenPGPContext::GetInstance(channel_);

  info_text += (tr("%1 Engine Version: %2") + "\n\n")
                   .arg(ConvertOpenPGPEngine2String(ctx.Engine()))
                   .arg(ctx.EngineVersion());

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

void KeyGenerateDialog::InitUi() {
  setObjectName(QStringLiteral("KeyGenerateDialog"));

#ifdef Q_OS_MACOS
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
  CompactLayout(layout(), 4, 5);

  ui_->profileGroupBox->setFlat(true);
  ui_->basicGroupBox->setFlat(true);

  SetFormLayoutCompact(ui_->formLayout_2);
  SetFormLayoutCompact(ui_->formLayout);
  SetFormLayoutCompact(ui_->formLayout_3);

  SetGridLayoutCompact(ui_->gridLayout_2);
  SetGridLayoutCompact(ui_->gridLayout_4);

  ui_->verticalLayout_2->setContentsMargins(6, 6, 6, 6);
  ui_->verticalLayout_2->setSpacing(5);

  ui_->verticalLayout->setContentsMargins(0, 0, 0, 0);
  ui_->verticalLayout->setSpacing(4);

  ui_->verticalLayout_3->setContentsMargins(6, 6, 6, 6);
  ui_->verticalLayout_4->setContentsMargins(6, 6, 6, 6);
  ui_->verticalLayout_7->setContentsMargins(6, 6, 6, 6);

  ui_->profileGroupBox->layout()->setContentsMargins(8, 8, 8, 8);
  ui_->basicGroupBox->layout()->setContentsMargins(8, 8, 8, 8);
#endif

  ui_->tabWidget->setDocumentMode(true);
  ui_->tabWidget->setUsesScrollButtons(true);
  ui_->tabWidget->setElideMode(Qt::ElideRight);

  ui_->statusPlainTextEdit->setReadOnly(true);
  ui_->statusPlainTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);

  ui_->statusPlainTextEdit->setFocusPolicy(Qt::ClickFocus);

  QFont status_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#ifndef Q_OS_MACOS
  status_font.setPointSize(std::max(10, status_font.pointSize()));
#endif
  status_font.setStyleHint(QFont::Monospace);
  status_font.setFixedPitch(true);
  ui_->statusPlainTextEdit->setFont(status_font);

  const auto setup_combo = [](QComboBox* combo) {
    combo->setMinimumContentsLength(14);
    combo->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon);
  };

  setup_combo(ui_->keyDBIndexComboBox);
  setup_combo(ui_->easyProfileComboBox);
  setup_combo(ui_->easyCombinationComboBox);
  setup_combo(ui_->easyValidityPeriodComboBox);
  setup_combo(ui_->pAlgoComboBox);
  setup_combo(ui_->pKeyLengthComboBox);
  setup_combo(ui_->pScndAlgoComboBox);
  setup_combo(ui_->pScndKeyLengthComboBox);
  setup_combo(ui_->pKeyFormatComboBox);
  setup_combo(ui_->sAlgoComboBox);
  setup_combo(ui_->sKeyLengthComboBox);
  setup_combo(ui_->scndAlgoComboBox);
  setup_combo(ui_->scndKeyLengthComboBox);

  const auto setup_button = [](QPushButton* button) {
    button->setAutoDefault(false);
  };

  setup_button(ui_->savePushButton);
  setup_button(ui_->deletePushButton);
  setup_button(ui_->reset2DefaultPushButton);
  setup_button(ui_->generateButton);

  ui_->generateButton->setDefault(true);

#ifndef Q_OS_MACOS
  ui_->generateButton->setMinimumHeight(32);
#endif

  ui_->pExpireDateTimeEdit->setCalendarPopup(true);
  ui_->sExpireDateTimeEdit->setCalendarPopup(true);

  setStyleSheet(R"(
QDialog#KeyGenerateDialog QPlainTextEdit {
  selection-background-color: palette(highlight);
  selection-color: palette(highlighted-text);
}

QDialog#KeyGenerateDialog QPushButton#generateButton {
  font-weight: 600;
}
)");
}

void KeyGenerateDialog::slot_key_gen_accept() {
  QString buffer;
  QTextStream err_stream(&buffer);

  if (ui_->nameEdit->text().trimmed().size() < 5) {
    err_stream << " -> " << tr("Name must contain at least five characters.")
               << Qt::endl;
  }
  // The name and comment become part of an RFC 2822 mail name-addr; reject the
  // structural delimiters '(', ')', '<', '>' and control characters.
  if (!IsValidUserIdComponent(ui_->nameEdit->text()) ||
      !IsValidUserIdComponent(ui_->commentEdit->text())) {
    err_stream << " -> "
               << tr("Name and comment must not contain the characters '(', "
                     "')', '<', '>' or control characters.")
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

  // Keep the dialog open on failure so the user can adjust and retry without
  // losing the entered configuration. Only close on success.
  if (!do_generate()) return;

  // Close the dialog first, then confirm to the user anchored to the parent
  // window, so the success notice doesn't appear layered over a dialog that is
  // about to disappear.
  auto* notify_parent = this->parentWidget();
  this->accept();

  QMessageBox::information(notify_parent, tr("Success"),
                           tr("Key generation completed successfully."));
}

void KeyGenerateDialog::refresh_widgets_state() {
  ui_->pAlgoComboBox->blockSignals(true);
  SetComboCurrentAlgo(ui_->pAlgoComboBox, gen_key_info_->GetAlgo());
  ui_->pAlgoComboBox->blockSignals(false);

  ui_->pKeyLengthComboBox->blockSignals(true);
  const auto [p_name, p_type] = ComboCurrentNameType(ui_->pAlgoComboBox);
  SetKeyLengthComboxBoxByAlgo(
      ui_->pKeyLengthComboBox,
      SearchAlgoByNameType(p_name, p_type, supported_primary_key_algos_));
  ui_->pKeyLengthComboBox->setCurrentText(
      QString::number(gen_key_info_->GetKeyLength()));
  ui_->pKeyLengthComboBox->blockSignals(false);

  refresh_primary_hybrid_algo_widgets_state();
  refresh_key_version_widgets_state();

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
    SetComboCurrentAlgo(ui_->sAlgoComboBox, KeyGenerateInfo::kNoneAlgo);
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
    return;
  }

  ui_->sTab->setDisabled(false);

  ui_->sAlgoComboBox->blockSignals(true);
  SetComboCurrentAlgo(ui_->sAlgoComboBox, gen_subkey_info_->GetAlgo());
  ui_->sAlgoComboBox->blockSignals(false);

  ui_->sKeyLengthComboBox->blockSignals(true);
  const auto [s_name, s_type] = ComboCurrentNameType(ui_->sAlgoComboBox);
  SetKeyLengthComboxBoxByAlgo(
      ui_->sKeyLengthComboBox,
      SearchAlgoByNameType(s_name, s_type, supported_subkey_algos_));
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
  QContainer<KeyAlgo> sub_algos =
      gen_subkey_info_->GetAlgo().SubAlgos(channel_);
  if (!sub_algos.empty()) {
    ui_->scndAlgoComboBox->setEnabled(true);
    ui_->scndAlgoComboBox->setHidden(false);
    ui_->scndAlgoLabel->setHidden(false);
    ui_->scndKeyLengthLabel->setHidden(false);

    ui_->scndAlgoComboBox->blockSignals(true);
    PopulateAlgoComboBox(ui_->scndAlgoComboBox, sub_algos);
    ui_->scndAlgoComboBox->blockSignals(false);

    if (!gen_subkey_info_->SubAlgo().Id().isEmpty() &&
        gen_subkey_info_->SubAlgo() != KeyGenerateInfo::kNoneAlgo) {
      ui_->scndAlgoComboBox->blockSignals(true);
      SetComboCurrentAlgo(ui_->scndAlgoComboBox, gen_subkey_info_->SubAlgo());
      ui_->scndAlgoComboBox->blockSignals(false);
    } else if (const auto first_selectable =
                   FirstSelectableComboIndex(ui_->scndAlgoComboBox);
               first_selectable >= 0) {
      ui_->scndAlgoComboBox->blockSignals(true);
      // Skip any leading section header (e.g. "ECC") so the default lands on a
      // real algorithm rather than a non-selectable family label.
      ui_->scndAlgoComboBox->setCurrentIndex(first_selectable);
      const auto [id, type] = ComboCurrentIdType(ui_->scndAlgoComboBox);
      auto [found, algo] = GetAlgoByIdType(id, type, sub_algos);
      if (found) gen_subkey_info_->SetSubAlgo(algo);
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

  connect(ui_->randomIdentityButton, &QToolButton::clicked, this,
          &KeyGenerateDialog::slot_fill_random_identity);

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

  connect(ui_->pKeyFormatComboBox, &QComboBox::currentIndexChanged, this,
          [this](int) -> void {
            const auto data = ui_->pKeyFormatComboBox->currentData();
            if (data.isValid()) gen_key_info_->SetKeyVersion(data.toInt());
          });

  connect(
      ui_->sAlgoComboBox, &QComboBox::currentTextChanged, this,
      [this](const QString&) -> void {
        sync_gen_subkey_algo_info();
        slot_set_easy_key_algo_2_custom();
        refresh_widgets_state();

        if (gen_subkey_info_ == nullptr) return;

        if (!ui_->scndAlgoComboBox->isEnabled() ||
            ui_->scndAlgoComboBox->count() <= 0) {
          gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
          return;
        }

        const auto sub_algos = gen_subkey_info_->GetAlgo().SubAlgos(channel_);
        const auto [name, type] = ComboCurrentNameType(ui_->scndAlgoComboBox);

        auto [found, algo] = GetAlgoByNameType(name, type, sub_algos);
        if (found) {
          gen_subkey_info_->SetSubAlgo(algo);
        } else {
          gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
        }

        refresh_hybrid_algo_widgets_state();
      });

  connect(
      ui_->pScndAlgoComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
      this, [this](int index) -> void {
        if (index < 0) {
          gen_key_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
          return;
        }

        const auto sub_algos = gen_key_info_->GetAlgo().SubAlgos(channel_);
        const auto algo = ComboCurrentAlgo(ui_->pScndAlgoComboBox, sub_algos);

        gen_key_info_->SetSubAlgo(algo);
        slot_set_easy_key_algo_2_custom();
        refresh_primary_hybrid_algo_widgets_state();
      });

  connect(ui_->easyProfileComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          &KeyGenerateDialog::slot_easy_profile_changed);

  connect(ui_->easyValidityPeriodComboBox, &QComboBox::currentTextChanged, this,
          &KeyGenerateDialog::slot_easy_valid_date_changed);

  connect(ui_->pExpireDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this,
          [=](const QDateTime& dt) {
            if (gen_key_info_ == nullptr) return;
            gen_key_info_->SetExpireTime(dt);

            slot_set_easy_valid_date_2_custom();
          });

  connect(ui_->sExpireDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this,
          [=](const QDateTime& dt) {
            if (gen_subkey_info_ == nullptr) return;
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
            const auto [name, type] = ComboCurrentNameType(ui_->pAlgoComboBox);
            auto [found, algo] = GetAlgoByNameTypeAndKeyLength(
                name, type, text.toInt(), supported_primary_key_algos_);

            if (!found) return;

            const auto old_sub_algo = gen_key_info_->SubAlgo();

            gen_key_info_->SetAlgo(algo);

            const auto sub_algos = gen_key_info_->GetAlgo().SubAlgos(channel_);
            if (old_sub_algo != KeyGenerateInfo::kNoneAlgo &&
                std::any_of(sub_algos.cbegin(), sub_algos.cend(),
                            [&](const KeyAlgo& sub_algo) {
                              return sub_algo == old_sub_algo;
                            })) {
              gen_key_info_->SetSubAlgo(old_sub_algo);
            } else {
              gen_key_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
            }

            slot_set_easy_key_algo_2_custom();
            refresh_primary_hybrid_algo_widgets_state();
          });

  connect(ui_->sKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          [this](const QString& text) -> void {
            if (gen_subkey_info_ == nullptr) return;

            const auto [name, type] = ComboCurrentNameType(ui_->sAlgoComboBox);

            auto [found, algo] = GetAlgoByNameTypeAndKeyLength(
                name, type, text.toInt(), supported_subkey_algos_);

            if (!found) return;

            const auto old_sub_algo = gen_subkey_info_->SubAlgo();

            gen_subkey_info_->SetAlgo(algo);

            const auto sub_algos =
                gen_subkey_info_->GetAlgo().SubAlgos(channel_);
            if (old_sub_algo != KeyGenerateInfo::kNoneAlgo &&
                std::any_of(sub_algos.cbegin(), sub_algos.cend(),
                            [&](const KeyAlgo& sub_algo) {
                              return sub_algo == old_sub_algo;
                            })) {
              gen_subkey_info_->SetSubAlgo(old_sub_algo);
            } else {
              gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
            }

            slot_set_easy_key_algo_2_custom();
            refresh_widgets_state();
          });

  connect(ui_->pScndKeyLengthComboBox, &QComboBox::currentTextChanged, this,
          [this](const QString& text) -> void {
            const auto sub_algos = gen_key_info_->GetAlgo().SubAlgos(channel_);
            const auto [name, type] =
                ComboCurrentNameType(ui_->pScndAlgoComboBox);

            auto [found, algo] = GetAlgoByNameTypeAndKeyLength(
                name, type, text.toInt(), sub_algos);

            if (!found) return;

            gen_key_info_->SetSubAlgo(algo);
            slot_set_easy_key_algo_2_custom();

            // Keep combo display consistent, especially when multiple variants
            // share the same visible name.
            refresh_primary_hybrid_algo_widgets_state();
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

  connect(
      ui_->scndAlgoComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
      this, [this](int index) -> void {
        if (gen_subkey_info_ == nullptr) return;

        if (index < 0) {
          gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
          return;
        }

        const auto sub_algos = gen_subkey_info_->GetAlgo().SubAlgos(channel_);
        const auto algo = ComboCurrentAlgo(ui_->scndAlgoComboBox, sub_algos);

        gen_subkey_info_->SetSubAlgo(algo);
        slot_set_easy_key_algo_2_custom();
        refresh_hybrid_algo_widgets_state();
      });

  connect(
      ui_->scndKeyLengthComboBox, &QComboBox::currentTextChanged, this,
      [this](const QString& text) -> void {
        if (gen_subkey_info_ == nullptr) return;

        const auto sub_algos = gen_subkey_info_->GetAlgo().SubAlgos(channel_);
        const auto [name, type] = ComboCurrentNameType(ui_->scndAlgoComboBox);

        auto [found, algo] =
            GetAlgoByNameTypeAndKeyLength(name, type, text.toInt(), sub_algos);

        if (found) {
          gen_subkey_info_->SetSubAlgo(algo);
          slot_set_easy_key_algo_2_custom();
        }
      });
}

void KeyGenerateDialog::sync_gen_key_algo_info() {
  const auto [name, type] = ComboCurrentNameType(ui_->pAlgoComboBox);

  auto [found, algo] =
      GetAlgoByNameType(name, type, supported_primary_key_algos_);

  if (found) {
    gen_key_info_->SetAlgo(algo);
  } else {
    gen_key_info_->SetAlgo(KeyGenerateInfo::kNoneAlgo);
  }

  gen_key_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
}

void KeyGenerateDialog::sync_gen_subkey_algo_info() {
  if (gen_subkey_info_ == nullptr) return;

  const auto [name, type] = ComboCurrentNameType(ui_->sAlgoComboBox);

  auto [found, algo] = GetAlgoByNameType(name, type, supported_subkey_algos_);

  if (found) {
    gen_subkey_info_->SetAlgo(algo);
  } else {
    gen_subkey_info_->SetAlgo(KeyGenerateInfo::kNoneAlgo);
  }

  gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
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
    ui_->easyValidityPeriodComboBox->blockSignals(true);

    if ((c.has_s_key && c.key_validity == c.s_key_validity) || !c.has_s_key) {
      const auto expire_option =
          k_expire_options_.value(c.key_validity, k_custom_expire_option_);
      ui_->easyValidityPeriodComboBox->setCurrentText(expire_option.display);
    } else {
      ui_->easyValidityPeriodComboBox->setCurrentText(tr("Custom"));
    }

    ui_->easyValidityPeriodComboBox->blockSignals(false);
  } else {
    slot_set_easy_valid_date_2_custom();
  }

  auto [found, algo] = GetAlgoByIdType(c.key_algo, c.key_algo_type,
                                       supported_primary_key_algos_);
  if (found) {
    gen_key_info_->SetAlgo(algo);

    if (c.key_sub_algo.isEmpty()) {
      gen_key_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
    } else {
      const auto sub_algos = algo.SubAlgos(channel_);

      auto [sub_found, sub_algo] =
          GetAlgoByIdType(c.key_sub_algo, c.key_sub_algo_type, sub_algos);

      if (sub_found) {
        gen_key_info_->SetSubAlgo(sub_algo);
      } else {
        gen_key_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
      }
    }
  }

  auto dt = ParseValidityString(c.key_validity);
  gen_key_info_->SetNonExpired(c.key_validity == "forever" ||
                               c.key_validity == "none");

  gen_key_info_->SetExpireTime(dt);

  if (c.has_s_key) {
    if (gen_subkey_info_ == nullptr) {
      create_sync_gen_subkey_info(false);
    }

    auto [s_found, s_algo] = GetAlgoByIdType(c.s_key_algo, c.s_key_algo_type,
                                             supported_subkey_algos_);
    if (s_found) gen_subkey_info_->SetAlgo(s_algo);

    gen_subkey_info_->SetNonExpired(c.s_key_validity == "forever" ||
                                    c.s_key_validity == "none");

    auto dt = ParseValidityString(c.s_key_validity);
    gen_subkey_info_->SetExpireTime(dt);

    if (c.s_key_sub_algo.isEmpty()) {
      gen_subkey_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
    } else {
      const auto sub_algos = gen_subkey_info_->GetAlgo().SubAlgos(channel_);

      auto [sub_found, sub_algo] =
          GetAlgoByIdType(c.s_key_sub_algo, c.s_key_sub_algo_type, sub_algos);

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

auto KeyGenerateDialog::do_generate() -> bool {
  // WaitForOpera() runs a nested event loop and returns only after the
  // completion callback below has run, so the flag reliably reflects the
  // outcome by the time this function returns.
  auto succeeded = std::make_shared<bool>(false);

  auto f = [this, succeeded, gen_key_info = this->gen_key_info_](
               const OperaWaitingHd& hd) -> void {
    KeyGenerationOperation::GetInstance(channel_).GenerateKeyWithSubkey(
        gen_key_info, gen_subkey_info_,
        [this, hd, succeeded](GpgError err, const DataObjectPtr&) {
          // stop showing waiting dialog
          hd();

          if (CheckGpgError(err) == GPG_ERR_USER_1) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Unknown error occurred"));
            return;
          }

          if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
            // Report the failure on the still-open dialog so the user can fix
            // the input and retry; success is handled by the caller.
            CommonUtils::RaiseMessageBox(this, err);
            return;
          }

          *succeeded = true;
          emit SignalKeyGenerated();
        });
  };
  GpgOperaHelper::WaitForOpera(this, tr("Generating"), f);

  return *succeeded;
}

void KeyGenerateDialog::create_sync_gen_subkey_info(bool sync_from_ui) {
  if (gen_subkey_info_ == nullptr) {
    gen_subkey_info_ = SecureCreateSharedObject<KeyGenerateInfo>(true);
  }

  if (sync_from_ui) {
    sync_gen_subkey_algo_info();
    slot_easy_valid_date_changed(
        ui_->easyValidityPeriodComboBox->currentText());
  }
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

  // Collect supported, visible profiles and split them into family tiers so the
  // elliptic-curve and quantum-resistant profiles each get their own header.
  // A profile's family is taken from its primary key algorithm.
  QContainer<EasyModeConf> classical_profiles;
  QContainer<EasyModeConf> ecc_profiles;
  QContainer<EasyModeConf> pqc_profiles;

  for (const auto& item : easy_mode_conf) {
    if (item.hidden || item.name.isEmpty()) continue;

    auto [found, algo] = GetAlgoByIdType(item.key_algo, item.key_algo_type,
                                         supported_primary_key_algos_);
    if (!found) continue;  // skip config with unsupported algo

    if (!item.key_sub_algo.isEmpty()) {
      const auto sub_algos = algo.SubAlgos(channel_);

      auto [p_sub_found, p_sub_algo] =
          GetAlgoByIdType(item.key_sub_algo, item.key_sub_algo_type, sub_algos);

      if (!p_sub_found) continue;
    }

    bool is_pqc = algo.IsPostQuantum();
    const bool is_ecc = algo.IsEcc();

    if (item.has_s_key) {
      auto [s_found, s_algo] = GetAlgoByIdType(
          item.s_key_algo, item.s_key_algo_type, supported_subkey_algos_);

      if (!s_found) continue;  // skip config with unsupported subkey algo

      if (!item.s_key_sub_algo.isEmpty()) {
        const auto sub_algos = s_algo.SubAlgos(channel_);

        auto [ss_found, ss_algo] = GetAlgoByIdType(
            item.s_key_sub_algo, item.s_key_sub_algo_type, sub_algos);

        if (!ss_found) continue;
      }

      if (s_algo.IsPostQuantum()) is_pqc = true;
    }

    auto& bucket =
        is_pqc ? pqc_profiles : (is_ecc ? ecc_profiles : classical_profiles);
    bucket.push_back(item);
  }

  // The map key must match each profile's final combo index (section headers
  // occupy an index but are not selectable profiles).
  //
  // Track the Ed25519 (Curve25519) profile so it can be the default selection:
  // it is the recommended modern, fast default. The first match wins, and since
  // ECC profiles are added before PQC ones this resolves to the pure
  // Ed25519/X25519 profile rather than a PQC hybrid that also uses an Ed25519
  // primary.
  int preferred_default_index = -1;
  auto add_profiles = [this, &preferred_default_index](
                          const QContainer<EasyModeConf>& profiles) {
    for (const auto& item : profiles) {
      const int index = ui_->easyProfileComboBox->count();
      easy_profile_conf_index_.insert(index, item);
      ui_->easyProfileComboBox->addItem(item.name);

      if (preferred_default_index < 0 && item.key_algo == "ed25519" &&
          item.key_algo_type.compare("EdDSA", Qt::CaseInsensitive) == 0) {
        preferred_default_index = index;
      }
    }
  };

  add_profiles(classical_profiles);

  if (!ecc_profiles.isEmpty()) {
    AddComboSectionHeader(ui_->easyProfileComboBox, tr("ECC"));
    add_profiles(ecc_profiles);
  }

  if (!pqc_profiles.isEmpty()) {
    AddComboSectionHeader(ui_->easyProfileComboBox, tr("Post-Quantum"));
    add_profiles(pqc_profiles);
  }

  easy_mode_conf_ = easy_mode_conf;

  ui_->easyProfileComboBox->blockSignals(false);

  ui_->easyProfileComboBox->setCurrentIndex(
      preferred_default_index >= 0 ? preferred_default_index
      : easy_profile_conf_index_.isEmpty()
          ? 0
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
  conf.key_algo_type = gen_key_info_->GetAlgo().Type();
  conf.key_validity =
      gen_key_info_->IsNonExpired() ? "forever" : expire_option.key;

  if (gen_key_info_->SubAlgo().Id() != KeyGenerateInfo::kNoneAlgo.Id()) {
    conf.key_sub_algo = gen_key_info_->SubAlgo().Id();
    conf.key_sub_algo_type = gen_key_info_->SubAlgo().Type();
  } else {
    conf.key_sub_algo.clear();
    conf.key_sub_algo_type.clear();
  }

  if (conf.key_validity == "custom") {
    conf.key_validity =
        QString::number(gen_key_info_->GetExpireTime().toSecsSinceEpoch() -
                        QDateTime::currentDateTime().toSecsSinceEpoch()) +
        "t";
  }

  conf.has_s_key = gen_subkey_info_ != nullptr;
  if (conf.has_s_key) {
    conf.s_key_algo = gen_subkey_info_->GetAlgo().Id();
    conf.s_key_algo_type = gen_subkey_info_->GetAlgo().Type();
    conf.s_key_validity =
        gen_subkey_info_->IsNonExpired() ? "forever" : expire_option.key;
  }
  if (conf.s_key_validity == "custom") {
    conf.s_key_validity =
        QString::number(gen_subkey_info_->GetExpireTime().toSecsSinceEpoch() -
                        QDateTime::currentDateTime().toSecsSinceEpoch()) +
        "t";
  }

  if (gen_subkey_info_ != nullptr &&
      gen_subkey_info_->SubAlgo().Id() != KeyGenerateInfo::kNoneAlgo.Id()) {
    conf.s_key_sub_algo = gen_subkey_info_->SubAlgo().Id();
    conf.s_key_sub_algo_type = gen_subkey_info_->SubAlgo().Type();
  } else {
    conf.s_key_sub_algo.clear();
    conf.s_key_sub_algo_type.clear();
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

void KeyGenerateDialog::slot_fill_random_identity() {
  const auto identity = GenerateRandomIdentity();
  ui_->nameEdit->setText(identity.name);
  ui_->emailEdit->setText(identity.email);
}

void KeyGenerateDialog::refresh_hybrid_algo_widgets_state() {
  if (gen_subkey_info_ == nullptr || ui_->scndAlgoComboBox->isHidden()) {
    ui_->scndKeyLengthComboBox->setEnabled(false);
    ui_->scndKeyLengthComboBox->clear();
    ui_->scndKeyLengthComboBox->setHidden(true);
    ui_->scndKeyLengthLabel->setHidden(true);
    return;
  }

  const auto sub_algos = gen_subkey_info_->GetAlgo().SubAlgos(channel_);
  if (sub_algos.isEmpty()) {
    ui_->scndKeyLengthComboBox->setEnabled(false);
    ui_->scndKeyLengthComboBox->clear();
    ui_->scndKeyLengthComboBox->setHidden(true);
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

  ui_->scndKeyLengthComboBox->setCurrentText(
      QString::number(gen_subkey_info_->SubAlgo().KeyLength()));

  ui_->scndKeyLengthComboBox->blockSignals(false);
}

void KeyGenerateDialog::refresh_primary_hybrid_algo_widgets_state() {
  const auto sub_algos = gen_key_info_->GetAlgo().SubAlgos(channel_);

  if (sub_algos.isEmpty()) {
    ui_->pScndAlgoComboBox->blockSignals(true);
    ui_->pScndAlgoComboBox->clear();
    ui_->pScndAlgoComboBox->setEnabled(false);
    ui_->pScndAlgoComboBox->setHidden(true);
    ui_->pScndAlgoComboBox->blockSignals(false);

    ui_->pScndKeyLengthComboBox->blockSignals(true);
    ui_->pScndKeyLengthComboBox->clear();
    ui_->pScndKeyLengthComboBox->setEnabled(false);
    ui_->pScndKeyLengthComboBox->setHidden(true);
    ui_->pScndKeyLengthComboBox->blockSignals(false);

    ui_->pScndAlgoLabel->setHidden(true);
    ui_->pScndKeyLengthLabel->setHidden(true);

    gen_key_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
    return;
  }

  ui_->pScndAlgoLabel->setHidden(false);
  ui_->pScndAlgoComboBox->setEnabled(true);
  ui_->pScndAlgoComboBox->setHidden(false);

  ui_->pScndAlgoComboBox->blockSignals(true);
  PopulateAlgoComboBox(ui_->pScndAlgoComboBox, sub_algos);
  ui_->pScndAlgoComboBox->blockSignals(false);

  const auto current_sub_algo = gen_key_info_->SubAlgo();

  const bool current_sub_algo_supported =
      current_sub_algo != KeyGenerateInfo::kNoneAlgo &&
      std::any_of(
          sub_algos.cbegin(), sub_algos.cend(),
          [&](const KeyAlgo& algo) { return algo == current_sub_algo; });

  ui_->pScndAlgoComboBox->blockSignals(true);

  const auto first_selectable =
      FirstSelectableComboIndex(ui_->pScndAlgoComboBox);
  if (current_sub_algo_supported) {
    SetComboCurrentAlgo(ui_->pScndAlgoComboBox, current_sub_algo);
  } else if (first_selectable >= 0) {
    // Skip any leading section header (e.g. "ECC") so the default lands on a
    // real algorithm rather than a non-selectable family label.
    ui_->pScndAlgoComboBox->setCurrentIndex(first_selectable);

    const auto algo = ComboCurrentAlgo(ui_->pScndAlgoComboBox, sub_algos);
    gen_key_info_->SetSubAlgo(algo);
  } else {
    gen_key_info_->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
  }

  ui_->pScndAlgoComboBox->blockSignals(false);
  ui_->pScndKeyLengthLabel->setHidden(false);
  ui_->pScndKeyLengthComboBox->setEnabled(true);
  ui_->pScndKeyLengthComboBox->setHidden(false);

  ui_->pScndKeyLengthComboBox->blockSignals(true);

  const auto [name, type] = ComboCurrentNameType(ui_->pScndAlgoComboBox);

  SetKeyLengthComboxBoxByAlgo(ui_->pScndKeyLengthComboBox,
                              SearchAlgoByNameType(name, type, sub_algos));

  ui_->pScndKeyLengthComboBox->setCurrentText(
      QString::number(gen_key_info_->SubAlgo().KeyLength()));

  ui_->pScndKeyLengthComboBox->blockSignals(false);
}

void KeyGenerateDialog::refresh_key_version_widgets_state() {
  // Only engines that can emit more than one key format expose this choice.
  // GnuPG generates v4 keys and ignores the requested version entirely.
  const bool engine_supports_choice =
      OpenPGPContext::GetInstance(channel_).Engine() == OpenPGPEngine::kRPGP;

  ui_->pKeyFormatLabel->setHidden(!engine_supports_choice);
  ui_->pKeyFormatComboBox->setHidden(!engine_supports_choice);
  if (!engine_supports_choice) return;

  // Post-quantum algorithms are only defined for the v6 key format, so when any
  // selected algorithm is post-quantum we pin the format to v6 and lock it.
  const auto is_pqc = [](const KeyAlgo& a) { return a.IsPostQuantum(); };
  bool force_v6 =
      is_pqc(gen_key_info_->GetAlgo()) || is_pqc(gen_key_info_->SubAlgo());
  if (gen_subkey_info_ != nullptr) {
    force_v6 = force_v6 || is_pqc(gen_subkey_info_->GetAlgo()) ||
               is_pqc(gen_subkey_info_->SubAlgo());
  }

  if (force_v6) {
    gen_key_info_->SetKeyVersion(6);
  } else if (gen_key_info_->GetKeyVersion() == 0) {
    // Default to the interoperable v4 format until the user opts into v6.
    gen_key_info_->SetKeyVersion(4);
  }

  ui_->pKeyFormatComboBox->blockSignals(true);
  const int index =
      ui_->pKeyFormatComboBox->findData(gen_key_info_->GetKeyVersion());
  ui_->pKeyFormatComboBox->setCurrentIndex(index >= 0 ? index : 0);
  ui_->pKeyFormatComboBox->blockSignals(false);

  ui_->pKeyFormatComboBox->setDisabled(force_v6);
  ui_->pKeyFormatComboBox->setToolTip(
      force_v6 ? tr("Post-quantum algorithms require the v6 key format.")
               : QString());
}

}  // namespace GpgFrontend::UI
