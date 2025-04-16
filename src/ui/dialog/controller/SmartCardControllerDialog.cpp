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

#include "SmartCardControllerDialog.h"

#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/function/gpg/GpgAssuanHelper.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgSmartCardManager.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"

//
#include "ui_SmartCardControllerDialog.h"

namespace GpgFrontend::UI {
SmartCardControllerDialog::SmartCardControllerDialog(QWidget* parent)
    : GeneralDialog("SmartCardControllerDialog", parent),
      ui_(QSharedPointer<Ui_SmartCardControllerDialog>::create()),
      channel_(kGpgFrontendDefaultChannel) {
  ui_->setupUi(this);

  ui_->smartCardLabel->setText(tr("Smart Card(s):"));
  ui_->keyStubLabel->setText(tr("Key Stub(s) in Key Database(s):"));

  ui_->cNameButton->setText(tr("Change Name"));
  ui_->cLangButton->setText(tr("Change Language"));
  ui_->cGenderButton->setText(tr("Change Gender"));
  ui_->cLoginDataButton->setText(tr("Change Login Data"));
  ui_->cPubKeyURLButton->setText(tr("Change Public Key URL"));
  ui_->cPINButton->setText(tr("Change PIN"));
  ui_->cAdminPINButton->setText(tr("Change Admin PIN"));
  ui_->cResetCodeButton->setText(tr("Change Reset Code"));
  ui_->fetchButton->setText(tr("Fetch"));
  ui_->restartGpgAgentButton->setText(tr("Restart All Gpg-Agents"));
  ui_->refreshButton->setText(tr("Refresh"));

  ui_->operationGroupBox->setTitle(tr("Operations"));

  for (const auto& key_db : GetGpgKeyDatabaseInfos()) {
    ui_->keyDBIndexComboBox->insertItem(
        key_db.channel, QString("%1: %2").arg(key_db.channel).arg(key_db.name));
  }

  connect(ui_->keyDBIndexComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          [=](int index) { refresh_key_tree_view(index); });

  connect(ui_->keyDBIndexComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString& serial_number) {
            select_smart_card_by_serial_number(serial_number);
          });

  connect(ui_->refreshButton, &QPushButton::clicked, this,
          [=](bool) { slot_refresh(); });

  connect(ui_->fetchButton, &QPushButton::clicked, this,
          [=](bool) { slot_fetch_smart_card_keys(); });

  connect(ui_->cNameButton, &QPushButton::clicked, this,
          [=](bool) { modify_key_attribute("DISP-NAME"); });

  connect(ui_->cLangButton, &QPushButton::clicked, this,
          [=](bool) { modify_key_attribute("DISP-LANG"); });

  connect(ui_->cGenderButton, &QPushButton::clicked, this,
          [=](bool) { modify_key_attribute("DISP-SEX"); });

  connect(ui_->cPubKeyURLButton, &QPushButton::clicked, this,
          [=](bool) { modify_key_attribute("PUBKEY-URL"); });

  connect(ui_->cLoginDataButton, &QPushButton::clicked, this,
          [=](bool) { modify_key_attribute("LOGIN-DATA"); });

  connect(ui_->cPINButton, &QPushButton::clicked, this,
          [=](bool) { modify_key_pin("OPENPGP.1"); });

  connect(ui_->cAdminPINButton, &QPushButton::clicked, this,
          [=](bool) { modify_key_pin("OPENPGP.3"); });

  connect(ui_->cResetCodeButton, &QPushButton::clicked, this,
          [=](bool) { modify_key_pin("OPENPGP.2"); });

  connect(ui_->restartGpgAgentButton, &QPushButton::clicked, this, [=](bool) {
    GpgFrontend::GpgAdvancedOperator::RestartGpgComponents(
        [=](int err, DataObjectPtr) {
          if (err >= 0) {
            QMessageBox::information(
                this, tr("Successful Operation"),
                tr("Restart all the GnuPG's components successfully"));
          } else {
            QMessageBox::critical(
                this, tr("Failed Operation"),
                tr("Failed to restart all or one of the GnuPG's component(s)"));
          }
        });
  });

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this, [=]() {
            refresh_key_tree_view(ui_->keyDBIndexComboBox->currentIndex());
          });

  // instant refresh
  slot_listen_smart_card_changes();

  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this,
          &SmartCardControllerDialog::slot_listen_smart_card_changes);
  timer_->start(3000);

  setWindowTitle(tr("Smart Card Controller"));
}

void SmartCardControllerDialog::select_smart_card_by_serial_number(
    const QString& serial_number) {
  if (serial_number.isEmpty()) {
    reset_status();
    return;
  }

  auto [ret, err] =
      GpgSmartCardManager::GetInstance(channel_).SelectCardBySerialNumber(
          serial_number);
  if (!ret) {
    LOG_E() << "select card by serial number failed: " << err;
    reset_status();
    return;
  }

  LOG_D() << "selected smart card by serial number: " << serial_number;

  has_card_ = true;
  fetch_smart_card_info(serial_number);
}

void SmartCardControllerDialog::fetch_smart_card_info(
    const QString& serial_number) {
  if (!has_card_) return;

  auto card_info =
      GpgSmartCardManager::GetInstance(channel_).FetchCardInfoBySerialNumber(
          serial_number);
  if (card_info == nullptr) {
    reset_status();
    return;
  }

  card_info_ = *card_info;
  has_card_ = true;

  print_smart_card_info();
  slot_disable_controllers(!has_card_);
  refresh_key_tree_view(ui_->keyDBIndexComboBox->currentIndex());
}

void SmartCardControllerDialog::print_smart_card_info() {
  if (!has_card_) return;

  QString html;
  QTextStream out(&html);
  const auto& card = card_info_;

  out << "<h2>" << tr("OpenPGP Card Information") << "</h2>";

  out << "<h3>" << tr("Basic Information") << "</h3><ul>";
  out << "<li><b>" << tr("Reader:") << "</b> " << card.reader << "</li>";
  out << "<li><b>" << tr("Serial Number:") << "</b> " << card.serial_number
      << "</li>";
  out << "<li><b>" << tr("Card Type:") << "</b> " << card.card_type << "</li>";
  out << "<li><b>" << tr("Card Version:") << "</b> " << card.card_version
      << "</li>";
  out << "<li><b>" << tr("App Type:") << "</b> " << card.app_type << "</li>";
  out << "<li><b>" << tr("App Version:") << "</b> " << card.app_version
      << "</li>";
  out << "<li><b>" << tr("Manufacturer ID:") << "</b> " << card.manufacturer_id
      << "</li>";
  out << "<li><b>" << tr("Manufacturer:") << "</b> " << card.manufacturer
      << "</li>";
  out << "<li><b>" << tr("Card Holder:") << "</b> " << card.card_holder
      << "</li>";
  out << "<li><b>" << tr("Language:") << "</b> " << card.display_language
      << "</li>";
  out << "<li><b>" << tr("Sex:") << "</b> " << card.display_sex << "</li>";
  out << "</ul>";

  out << "<h3>" << tr("Status") << "</h3><ul>";
  out << "<li><b>" << tr("Signature Counter:") << "</b> " << card.sig_counter
      << "</li>";
  out << "<li><b>" << tr("CHV1 Cached:") << "</b> " << card.chv1_cached
      << "</li>";
  out << "<li><b>" << tr("CHV Max Length:") << "</b> "
      << QString("%1, %2, %3")
             .arg(card.chv_max_len[0])
             .arg(card.chv_max_len[1])
             .arg(card.chv_max_len[2])
      << "</li>";
  out << "<li><b>" << tr("CHV Retry Left:") << "</b> "
      << QString("%1, %2, %3")
             .arg(card.chv_retry[0])
             .arg(card.chv_retry[1])
             .arg(card.chv_retry[2])
      << "</li>";
  out << "<li><b>" << tr("KDF Status:") << "</b> ";
  switch (card.kdf_do_enabled) {
    case 0:
      out << tr("Not enabled");
      break;
    case 1:
      out << tr("Enabled (no protection)");
      break;
    case 2:
      out << tr("Enabled with salt protection");
      break;
    default:
      out << tr("Unknown");
      break;
  }
  out << "</li>";
  out << "<li><b>" << tr("UIF:") << "</b><ul>";
  out << "<li>"
      << tr("Sign: %1").arg(card.uif.sign ? tr("✔ Enabled") : tr("❌ Disabled"))
      << "</li>";
  out << "<li>"
      << tr("Encrypt: %1")
             .arg(card.uif.encrypt ? tr("✔ Enabled") : tr("❌ Disabled"))
      << "</li>";
  out << "<li>"
      << tr("Authenticate: %1")
             .arg(card.uif.auth ? tr("✔ Enabled") : tr("❌ Disabled"))
      << "</li>";
  out << "</ul></li>";
  out << "</ul>";

  out << "<h3>" << tr("Key Information") << "</h3>";
  out << "<br />";

  if (card.card_keys_info.isEmpty()) {
    out << "<i>" << tr("No key information available.") << "</i>";
  } else {
    out << "<table border='1' cellspacing='0' cellpadding='4'>";
    out << "<tr><th>" << tr("No.") << "</th><th>" << tr("Fingerprint")
        << "</th><th>" << tr("Created") << "</th><th>" << tr("Grip")
        << "</th><th>" << tr("Type") << "</th><th>" << tr("Algorithm")
        << "</th><th>" << tr("Usage") << "</th><th>" << tr("Curve")
        << "</th></tr>";

    for (auto it = card.card_keys_info.begin(); it != card.card_keys_info.end();
         ++it) {
      const auto& info = it.value();
      out << "<tr><td>" << it.key() << "</td><td>" << info.fingerprint
          << "</td><td>" << info.created.toString(Qt::ISODate) << "</td><td>"
          << info.grip << "</td><td>" << info.key_type << "</td><td>"
          << info.algo << "</td><td>" << info.usage << "</td><td>" << info.algo
          << "</td></tr>";
    }

    out << "</table>";
  }

  out << "<br />";

  out << "<h3>" << tr("Extended Capabilities") << "</h3><ul>";
  out << "<li>"
      << tr("Key Info (ki): %1").arg(card.ext_cap.ki ? tr("Yes") : tr("No"))
      << "</li>";
  out << "<li>"
      << tr("Additional Auth (aac): %1")
             .arg(card.ext_cap.aac ? tr("Yes") : tr("No"))
      << "</li>";
  out << "<li>"
      << tr("Biometric Terminal (bt): %1")
             .arg(card.ext_cap.bt ? tr("Yes") : tr("No"))
      << "</li>";
  out << "<li>"
      << tr("KDF Supported: %1").arg(card.ext_cap.kdf ? tr("Yes") : tr("No"))
      << "</li>";
  out << "<li>" << tr("Status Indicator: %1").arg(card.ext_cap.status_indicator)
      << "</li>";
  out << "</ul>";

  if (!card.additional_card_infos.isEmpty()) {
    out << "<h3>" << tr("Additional Info") << "</h3><ul>";
    for (auto it = card.additional_card_infos.begin();
         it != card.additional_card_infos.end(); ++it) {
      out << "<li><b>" << tr("%1:").arg(it.key()) << "</b> " << it.value()
          << "</li>";
    }
    out << "</ul>";
  }

  ui_->cardInfoEdit->setText(html);
}

void SmartCardControllerDialog::slot_refresh() {
  fetch_smart_card_info(ui_->currentCardComboBox->currentText());
}

void SmartCardControllerDialog::refresh_key_tree_view(int channel) {
  if (!has_card_) return;

  QStringList card_fprs;
  for (const auto& key_info : card_info_.card_keys_info.values()) {
    card_fprs.append(key_info.fingerprint);
  }

  if (card_fprs.isEmpty()) {
    ui_->cardKeysTreeView->SetKeyFilter([](auto) { return false; });
    return;
  }

  ui_->cardKeysTreeView->SetChannel(channel);
  ui_->cardKeysTreeView->SetKeyFilter([=](const GpgAbstractKey* k) {
    return card_fprs.contains(k->Fingerprint());
  });
  ui_->cardKeysTreeView->expandAll();
}

void SmartCardControllerDialog::reset_status() {
  has_card_ = false;
  ui_->cardInfoEdit->clear();
  slot_disable_controllers(true);
  card_info_ = GpgOpenPGPCard();

  QString html;
  QTextStream out(&html);

  out << "<h2>" << tr("No OpenPGP Smart Card Found") << "</h2>";
  out << "<p>" << tr("No OpenPGP-compatible smart card has been detected.")
      << "</p>";

  out << "<p>"
      << tr("An OpenPGP Smart Card is a physical device that securely "
            "stores your private cryptographic keys and can be used for "
            "digital signing, encryption, and authentication. Popular "
            "examples include YubiKey, Nitrokey, and other "
            "GnuPG-compatible tokens.")
      << "</p>";

  out << "<p>"
      << tr("Make sure your card is inserted and properly recognized by "
            "the system. You can also try reconnecting the card or "
            "restarting the application.")
      << "</p>";

  out << "<p>" << tr("Read the GnuPG Smart Card HOWTO: ")
      << "https://gnupg.org/howtos/card-howto/en/" << "</p>";

  ui_->cardInfoEdit->setText(html);
}

void SmartCardControllerDialog::slot_listen_smart_card_changes() {
  auto serial_numbers =
      GpgSmartCardManager::GetInstance(channel_).GetSerialNumbers();

  const auto hash = QCryptographicHash::hash(serial_numbers.join(' ').toUtf8(),
                                             QCryptographicHash::Sha1)
                        .toHex();
  // check and skip
  if (cached_status_hash_ == hash) return;

  ui_->currentCardComboBox->clear();
  if (serial_numbers.isEmpty()) {
    LOG_D() << "no inserted and supported smart card found.";
    reset_status();
    return;
  }

  int index = 0;
  for (const auto& serial_number : serial_numbers) {
    ui_->currentCardComboBox->insertItem(index++, serial_number);
  }
  cached_status_hash_ = hash;
  ui_->currentCardComboBox->setCurrentIndex(0);
  select_smart_card_by_serial_number(ui_->currentCardComboBox->currentText());
}

void SmartCardControllerDialog::slot_disable_controllers(bool disable) {
  ui_->operationGroupBox->setDisabled(disable);
  ui_->keyDBIndexComboBox->setDisabled(disable);
  ui_->cardKeysTreeView->setDisabled(disable);
}

void SmartCardControllerDialog::slot_fetch_smart_card_keys() {
  GpgSmartCardManager::GetInstance().Fetch(
      ui_->currentCardComboBox->currentText());

  QTimer::singleShot(1000, [=]() {
    GpgCommandExecutor::GetInstance(channel_).GpgExecuteSync(
        {{},
         {"--card-status"},
         [=](int exit_code, const QString&, const QString&) {
           LOG_D() << "gpg --card--status exit code: " << exit_code;
           if (exit_code != 0) return;

           emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();
         }});
  });
}

auto AskIsoDisplayName(QWidget* parent, bool* ok) -> QString {
  QString surname = QInputDialog::getText(
      parent, QObject::tr("Cardholder's Surname"),
      QObject::tr("Please enter your surname (e.g., Lee):"), QLineEdit::Normal,
      "", ok);
  if (!*ok || surname.trimmed().isEmpty()) return {};

  QString given_name = QInputDialog::getText(
      parent, QObject::tr("Cardholder's Given Name"),
      QObject::tr("Please enter your given name (e.g., Chris):"),
      QLineEdit::Normal, "", ok);
  if (!*ok || given_name.trimmed().isEmpty()) return {};

  QString iso_name = surname.trimmed() + "<<" + given_name.trimmed();
  iso_name.replace(" ", "<");

  if (iso_name.length() > 39) {
    QMessageBox::warning(
        parent, QObject::tr("Too Long"),
        QObject::tr("Combined name too long (max 39 characters)."));
    *ok = false;
    return {};
  }

  return iso_name;
}

void SmartCardControllerDialog::modify_key_attribute(const QString& attr) {
  QString value;
  bool ok = false;

  if (attr == "DISP-SEX") {
    QStringList options;
    options << tr("1 - Male") << tr("2 - Female");

    const QString selected = QInputDialog::getItem(
        this, tr("Modify Card Attribute"),
        tr("Select sex to store in '%1':").arg(attr), options, 0, false, &ok);

    if (!ok || selected.isEmpty()) return;

    value = selected.left(1);
  } else if (attr == "DISP-NAME") {
    value = AskIsoDisplayName(this, &ok);
    if (!ok || value.trimmed().isEmpty()) {
      LOG_D() << "user canceled or empty input.";
      return;
    }
  } else {
    value = QInputDialog::getText(
        this, tr("Modify Card Attribute"),
        tr("Enter new value for attribute '%1':").arg(attr), QLineEdit::Normal,
        "", &ok);

    if (!ok || value.isEmpty()) {
      LOG_D() << "user canceled or empty input.";
      return;
    }
  }

  auto [r, err] =
      GpgSmartCardManager::GetInstance(channel_).ModifyAttr(attr, value);

  if (!r) {
    LOG_D() << "SCD SETATTR command failed for attr" << attr;
    QMessageBox::critical(
        this, tr("Failed"),
        tr("Failed to set attribute '%1'. Reason: %2. ").arg(attr).arg(err));
    return;
  }
  QMessageBox::information(this, tr("Success"),
                           tr("Attribute operation completed successfully."));
  fetch_smart_card_info(ui_->currentCardComboBox->currentText());
}

void SmartCardControllerDialog::modify_key_pin(const QString& pinref) {
  auto [success, err] =
      GpgSmartCardManager::GetInstance(channel_).ModifyPin(pinref);

  if (!success) {
    QString message;
    if (pinref == "OPENPGP.3") {
      message = tr("Failed to change Admin PIN.");
    } else if (pinref == "OPENPGP.2") {
      message = tr("Failed to set the Reset Code.");
    } else {
      message = tr("Failed to change PIN.");
    }

    message += tr("Reason: ") + err;

    QMessageBox::critical(this, tr("Error"), message);
    return;
  }

  QMessageBox::information(this, tr("Success"),
                           tr("PIN operation completed successfully."));
  fetch_smart_card_info(ui_->currentCardComboBox->currentText());
}

}  // namespace GpgFrontend::UI
