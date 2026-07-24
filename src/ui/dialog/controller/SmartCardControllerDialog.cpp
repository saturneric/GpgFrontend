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
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgSmartCardManager.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSubKey.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/key_generate/GenerateCardKeyDialog.h"

//
#include "ui_SmartCardControllerDialog.h"

namespace GpgFrontend::UI {

namespace {

/// how often the inserted-card list is polled, in milliseconds
constexpr int kCardPollIntervalMs = 3000;

/// settle time before asking gpg for the refreshed card status, in
/// milliseconds; SCD FETCH does not update the key stubs synchronously
constexpr int kFetchSettleMs = 1000;

/// columns of the on-card key table
enum CardKeyColumn {
  kColumnSlot = 0,
  kColumnUsage,
  kColumnType,
  kColumnAlgorithm,
  kColumnCreated,
  kColumnFingerprint,
  kColumnGrip,
  kCardKeyColumnCount,
};

const char* const kCardHowToUrl = "https://gnupg.org/howtos/card-howto/en/";

}  // namespace

SmartCardControllerDialog::SmartCardControllerDialog(QWidget* parent)
    : GeneralDialog("SmartCardControllerDialog", parent),
      ui_(QSharedPointer<Ui_SmartCardControllerDialog>::create()),
      channel_(kGpgFrontendDefaultChannel),
      scd_version_supported_(
          GpgSmartCardManager::GetInstance(channel_).IsSCDVersionSupported()) {
  ui_->setupUi(this);

  init_texts();
  init_actions();
  init_connections();

  ui_->cardSplitter->setStretchFactor(0, 2);
  ui_->cardSplitter->setStretchFactor(1, 1);

  for (const auto& key_db : GetGpgKeyDatabaseInfos()) {
    ui_->keyDBIndexComboBox->insertItem(
        key_db.channel, QString("%1: %2").arg(key_db.channel).arg(key_db.name));
  }

  // instant refresh
  slot_listen_smart_card_changes();

  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this,
          &SmartCardControllerDialog::slot_listen_smart_card_changes);

  if (scd_version_supported_) {
    timer_->start(kCardPollIntervalMs);
  }
}

void SmartCardControllerDialog::init_texts() {
  setWindowTitle(tr("Smart Card Controller"));

  ui_->smartCardLabel->setText(tr("Card"));
  ui_->keyStubLabel->setText(tr("Key Stubs in"));

  ui_->identityGroup->setTitle(tr("Identity"));
  ui_->readerKeyLabel->setText(tr("Reader"));
  ui_->serialKeyLabel->setText(tr("Serial Number"));
  ui_->manufacturerKeyLabel->setText(tr("Manufacturer"));
  ui_->cardKeyLabel->setText(tr("Card"));
  ui_->appKeyLabel->setText(tr("Application"));
  ui_->languageKeyLabel->setText(tr("Language"));
  ui_->sexKeyLabel->setText(tr("Sex"));

  ui_->accessGroup->setTitle(tr("Access & Status"));
  ui_->sigCounterKeyLabel->setText(tr("Signature Counter"));
  ui_->chv1KeyLabel->setText(tr("CHV1 Cached"));
  ui_->kdfKeyLabel->setText(tr("KDF Status"));

  ui_->cardKeysGroup->setTitle(tr("Keys on Card"));
  ui_->cardKeysTable->setColumnCount(kCardKeyColumnCount);
  ui_->cardKeysTable->setHorizontalHeaderLabels(
      {tr("Slot"), tr("Usage"), tr("Type"), tr("Algorithm"), tr("Created"),
       tr("Fingerprint"), tr("Grip")});
  ui_->cardKeysTable->verticalHeader()->hide();
  ui_->cardKeysTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  ui_->cardKeysTable->horizontalHeader()->setSectionResizeMode(
      kColumnFingerprint, QHeaderView::Stretch);

  ui_->capabilitiesGroup->setTitle(tr("Extended Capabilities"));
  ui_->kiKeyLabel->setText(tr("Key Info (ki)"));
  ui_->aacKeyLabel->setText(tr("Additional Auth (aac)"));
  ui_->btKeyLabel->setText(tr("Biometric Terminal (bt)"));
  ui_->kdfSupportedKeyLabel->setText(tr("KDF Supported"));
  ui_->statusIndicatorKeyLabel->setText(tr("Status Indicator"));

  ui_->additionalInfoGroup->setTitle(tr("Additional Info"));

  ui_->currentCardComboBox->setPlaceholderText(tr("No card detected"));

  ui_->cardholderButton->setText(tr("Cardholder"));
  ui_->accessCodesButton->setText(tr("Access Codes"));
  ui_->generateKeysButton->setText(tr("Generate Card Keys"));
  ui_->moveKeyToCardButton->setText(tr("Move Key to Card"));
  ui_->fetchButton->setText(tr("Fetch"));
  ui_->refreshButton->setText(tr("Refresh"));
  ui_->overflowButton->setToolTip(tr("More Actions"));

  auto title_font = ui_->cardHolderLabel->font();
  title_font.setBold(true);
  title_font.setPointSizeF(font().pointSizeF() + 2);
  ui_->cardHolderLabel->setFont(title_font);

  auto notice_font = ui_->noCardTitleLabel->font();
  notice_font.setBold(true);
  notice_font.setPointSizeF(font().pointSizeF() + 2);
  ui_->noCardTitleLabel->setFont(notice_font);
}

void SmartCardControllerDialog::init_actions() {
  auto* cardholder_menu = new QMenu(ui_->cardholderButton);
  cardholder_menu->addAction(tr("Change Name"), this,
                             [=]() { modify_key_attribute("DISP-NAME"); });
  cardholder_menu->addAction(tr("Change Language"), this,
                             [=]() { modify_key_attribute("DISP-LANG"); });
  cardholder_menu->addAction(tr("Change Sex"), this,
                             [=]() { modify_key_attribute("DISP-SEX"); });
  cardholder_menu->addAction(tr("Change Login Data"), this,
                             [=]() { modify_key_attribute("LOGIN-DATA"); });
  cardholder_menu->addAction(tr("Change Public Key URL"), this,
                             [=]() { modify_key_attribute("PUBKEY-URL"); });
  ui_->cardholderButton->setMenu(cardholder_menu);

  auto* access_menu = new QMenu(ui_->accessCodesButton);
  access_menu->addAction(tr("Change PIN"), this,
                         [=]() { modify_key_pin("OPENPGP.1"); });
  access_menu->addAction(tr("Change Admin PIN"), this,
                         [=]() { modify_key_pin("OPENPGP.3"); });
  access_menu->addAction(tr("Change Reset Code"), this,
                         [=]() { modify_key_pin("OPENPGP.2"); });
  ui_->accessCodesButton->setMenu(access_menu);

  auto* overflow_menu = new QMenu(ui_->overflowButton);
  overflow_menu->addAction(tr("Restart All Gpg-Agents"), this, [=]() {
    bool ret = true;
    for (const auto& channel : OpenPGPContext::GetAllChannelId()) {
      ret = GpgAdvancedOperator::GetInstance(channel).RestartGpgComponents();
      if (!ret) break;
    }

    if (ret) {
      QMessageBox::information(
          this, tr("Successful Operation"),
          tr("Restart all the GnuPG's components successfully"));
    } else {
      QMessageBox::critical(
          this, tr("Failed Operation"),
          tr("Failed to restart all or one of the GnuPG's component(s)"));
    }
  });
  overflow_menu->addSeparator();
  overflow_menu->addAction(tr("Open GnuPG Smart Card HOWTO"), this, [=]() {
    QDesktopServices::openUrl(QUrl(QLatin1String(kCardHowToUrl)));
  });
  ui_->overflowButton->setMenu(overflow_menu);
}

void SmartCardControllerDialog::init_connections() {
  connect(ui_->keyDBIndexComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          [=](int index) { refresh_key_tree_view(index); });

  connect(ui_->currentCardComboBox, &QComboBox::currentTextChanged, this,
          [=](const QString& serial_number) {
            select_smart_card_by_serial_number(serial_number);
          });

  connect(ui_->refreshButton, &QPushButton::clicked, this,
          [=](bool) { slot_refresh(); });

  connect(ui_->fetchButton, &QPushButton::clicked, this,
          [=](bool) { slot_fetch_smart_card_keys(); });

  connect(ui_->generateKeysButton, &QPushButton::clicked, this, [=](bool) {
    auto serial_number = ui_->currentCardComboBox->currentText();
    auto* d = new GenerateCardKeyDialog(channel_, serial_number, this);
    connect(d, &GenerateCardKeyDialog::finished, this, [=](int ret) {
      if (ret == 1) {
        fetch_smart_card_info(serial_number);
      } else if (ret == -1) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Generate card key failed."));
      }
    });
  });

  connect(ui_->moveKeyToCardButton, &QPushButton::clicked, this,
          [=](bool) { slot_move_key_to_card(); });

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this, [=]() {
            refresh_key_tree_view(ui_->keyDBIndexComboBox->currentIndex());
          });
}

void SmartCardControllerDialog::select_smart_card_by_serial_number(
    const QString& serial_number) {
  if (serial_number.isEmpty()) {
    reset_status();
    return;
  }

  auto [err, status] =
      GpgSmartCardManager::GetInstance(channel_).SelectCardBySerialNumber(
          serial_number);
  if (err != GPG_ERR_NO_ERROR) {
    LOG_E() << "select card by serial number failed, err:" << CheckGpgError(err)
            << "status:" << status;
    CommonUtils::RaiseFailureMessageBox(this, err, status);
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

  reset_status();

  auto card_info =
      GpgSmartCardManager::GetInstance(channel_).FetchCardInfoBySerialNumber(
          serial_number);
  if (card_info == nullptr) {
    LOG_E() << "card info is nullptr, serial number:" << serial_number;
    reset_status();
    return;
  }

  card_info_ = *card_info;
  has_card_ = true;

  render_card_info();
  slot_disable_controllers(!has_card_);
  refresh_key_tree_view(ui_->keyDBIndexComboBox->currentIndex());
}

void SmartCardControllerDialog::render_card_info() {
  if (!has_card_) return;

  ui_->detailStackedWidget->setCurrentWidget(ui_->detailPage);
  ui_->keyStubContainer->show();

  render_identity();
  render_status();
  render_card_keys();
  render_capabilities();
}

void SmartCardControllerDialog::render_identity() {
  const auto& card = card_info_;

  ui_->cardHolderLabel->setText(card.card_holder.isEmpty() ? tr("Unnamed Card")
                                                           : card.card_holder);

  ui_->readerValueLabel->setText(card.reader);
  ui_->serialValueLabel->setText(card.serial_number);

  auto manufacturer =
      card.manufacturer.isEmpty() ? tr("Unknown") : card.manufacturer;
  ui_->manufacturerValueLabel->setText(
      QString("%1 (0x%2)")
          .arg(manufacturer,
               QString("%1").arg(card.manufacturer_id, 4, 16, QChar('0'))));

  ui_->cardValueLabel->setText(
      tr("%1, version %2").arg(card.card_type).arg(card.card_version));
  ui_->appValueLabel->setText(
      tr("%1, version %2").arg(card.app_type).arg(card.app_version));

  ui_->languageValueLabel->setText(
      card.display_language.isEmpty() ? tr("Not set") : card.display_language);
  ui_->sexValueLabel->setText(card.display_sex.isEmpty() ? tr("Not set")
                                                         : card.display_sex);

  SetChip(ui_->statusChipLabel, tr("● Ready"), AccentColor(palette(), true));
  SetChip(ui_->manufacturerChipLabel, manufacturer,
          palette().color(QPalette::Link));
}

void SmartCardControllerDialog::render_status() {
  const auto& card = card_info_;

  // the retry counters are the numbers a user actually needs at a glance, an
  // exhausted counter means the card is one step from being bricked
  const std::array<QLabel*, 3> chips = {
      ui_->pinChipLabel, ui_->resetCodeChipLabel, ui_->adminPinChipLabel};
  const std::array<QString, 3> names = {tr("PIN"), tr("Reset Code"),
                                        tr("Admin PIN")};

  for (auto i = 0U; i < chips.size(); ++i) {
    const auto retry = card.chv_retry.at(i);
    const auto max_len = card.chv_max_len.at(i);
    if (retry < 0) {
      SetChip(chips.at(i), tr("%1 n/a").arg(names.at(i)),
              AccentColor(palette(), false));
      continue;
    }
    SetChip(chips.at(i), tr("%1 %2 left").arg(names.at(i)).arg(retry),
            AccentColor(palette(), retry > 0));
    chips.at(i)->setToolTip(
        tr("%1 retries left, maximum length %2").arg(retry).arg(max_len));
  }

  const auto enabled = tr("Enabled");
  const auto disabled = tr("Disabled");
  ui_->uifChipLabel->setText(
      tr("User Interaction Flag — Sign: %1 · Encrypt: %2 · Authenticate: %3")
          .arg(card.uif.sign ? enabled : disabled,
               card.uif.encrypt ? enabled : disabled,
               card.uif.auth ? enabled : disabled));

  ui_->sigCounterValueLabel->setText(QString::number(card.sig_counter));
  ui_->chv1ValueLabel->setText(card.chv1_cached > 0 ? tr("Yes") : tr("No"));

  QString kdf;
  switch (card.kdf_do_enabled) {
    case 0:
      kdf = tr("Not enabled");
      break;
    case 1:
      kdf = tr("Enabled (no protection)");
      break;
    case 2:
      kdf = tr("Enabled with salt protection");
      break;
    default:
      kdf = tr("Unknown");
      break;
  }
  ui_->kdfValueLabel->setText(kdf);
}

void SmartCardControllerDialog::render_card_keys() {
  const auto& keys = card_info_.card_keys_info;

  ui_->cardKeysTable->setVisible(!keys.isEmpty());
  ui_->noCardKeysLabel->setVisible(keys.isEmpty());
  ui_->noCardKeysLabel->setText(tr("No key information available."));

  ui_->cardKeysTable->clearContents();
  ui_->cardKeysTable->setRowCount(static_cast<int>(keys.size()));

  auto row = 0;
  for (auto it = keys.begin(); it != keys.end(); ++it, ++row) {
    const auto& info = it.value();

    const std::array<QString, kCardKeyColumnCount> values = {
        QString::number(it.key()),
        info.usage,
        info.key_type,
        info.algo,
        info.created.isValid() ? info.created.toString(Qt::ISODate) : tr("N/A"),
        info.fingerprint,
        info.grip};

    for (auto column = 0; column < kCardKeyColumnCount; ++column) {
      auto* item = new QTableWidgetItem(values.at(column));
      item->setToolTip(values.at(column));
      ui_->cardKeysTable->setItem(row, column, item);
    }
  }
}

void SmartCardControllerDialog::render_capabilities() {
  const auto& card = card_info_;

  const auto yes = tr("Yes");
  const auto no = tr("No");
  ui_->kiValueLabel->setText(card.ext_cap.ki ? yes : no);
  ui_->aacValueLabel->setText(card.ext_cap.aac ? yes : no);
  ui_->btValueLabel->setText(card.ext_cap.bt ? yes : no);
  ui_->kdfSupportedValueLabel->setText(card.ext_cap.kdf ? yes : no);
  ui_->statusIndicatorValueLabel->setText(
      QString::number(card.ext_cap.status_indicator));

  // the additional infos are card specific, so the rows are rebuilt every time
  auto* form = ui_->additionalInfoFormLayout;
  while (form->rowCount() > 0) form->removeRow(0);

  const auto& infos = card.additional_card_infos;
  ui_->additionalInfoGroup->setVisible(!infos.isEmpty());

  for (auto it = infos.begin(); it != infos.end(); ++it) {
    auto* value = new QLabel(it.value(), ui_->additionalInfoGroup);
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(new QLabel(it.key(), ui_->additionalInfoGroup), value);
  }
}

void SmartCardControllerDialog::slot_refresh() {
  scd_version_supported_ =
      GpgSmartCardManager::GetInstance(channel_).IsSCDVersionSupported();
  if (scd_version_supported_ && !timer_->isActive()) {
    timer_->start(kCardPollIntervalMs);
  }
  fetch_smart_card_info(ui_->currentCardComboBox->currentText());
}

void SmartCardControllerDialog::refresh_key_tree_view(int channel) {
  // without a card there is nothing to match against, and leaving the filter
  // alone would list the whole key database as if it were on the card
  if (!has_card_) {
    ui_->cardKeysTreeView->SetKeyFilter([](auto) { return false; });
    return;
  }

  ui_->cardKeysTreeView->SetChannel(channel);

  QStringList card_fprs;
  for (const auto& key_info : card_info_.card_keys_info.values()) {
    card_fprs.append(key_info.fingerprint);
  }

  LOG_D() << "card key fingerprints:" << card_fprs;
  if (card_fprs.isEmpty()) {
    ui_->cardKeysTreeView->SetKeyFilter([](auto) { return false; });
    return;
  }

  ui_->cardKeysTreeView->SetKeyFilter([=](const GpgAbstractKey* k) {
    return card_fprs.contains(k->Fingerprint());
  });
  ui_->cardKeysTreeView->expandAll();
}

void SmartCardControllerDialog::reset_status() {
  has_card_ = false;
  card_info_ = GpgOpenPGPCard();

  slot_disable_controllers(true);
  ui_->detailStackedWidget->setCurrentWidget(ui_->noCardPage);

  // the stub pane has nothing to say without a card, hiding it lets the notice
  // use the full width instead of sitting next to an empty disabled tree
  ui_->keyStubContainer->hide();

  ui_->statusChipLabel->clear();
  ui_->manufacturerChipLabel->clear();

  ui_->noCardTitleLabel->setText(tr("No OpenPGP Smart Card Found"));

  // the label holds a link, so Qt renders the whole text as rich text and the
  // paragraphs have to be markup rather than newlines
  const QStringList paragraphs = {
      tr("No OpenPGP-compatible smart card has been detected.").toHtmlEscaped(),
      tr("An OpenPGP Smart Card is a physical device that securely "
         "stores your private cryptographic keys and can be used for "
         "digital signing, encryption, and authentication. Popular "
         "examples include YubiKey, Nitrokey, and other "
         "GnuPG-compatible tokens.")
          .toHtmlEscaped(),
      tr("Make sure your card is inserted and properly recognized by "
         "the system. You can also try reconnecting the card or "
         "restarting the application.")
          .toHtmlEscaped(),
      QString(R"(<a href="%1">%2</a>)")
          .arg(QLatin1String(kCardHowToUrl),
               tr("Read the GnuPG Smart Card HOWTO").toHtmlEscaped()),
  };

  ui_->noCardBodyLabel->setText("<p>" + paragraphs.join("</p><p>") + "</p>");

  // only relevant when the installed scdaemon is actually too old, showing it
  // unconditionally sends users chasing a version they already have
  ui_->noCardNoticeLabel->setVisible(!scd_version_supported_);
  ui_->noCardNoticeLabel->setText(
      tr("Note: Smart card support of GpgFrontend requires GnuPG version "
         "2.3.0 or later."));
}

void SmartCardControllerDialog::slot_listen_smart_card_changes() {
  if (!scd_version_supported_) {
    LOG_D() << "scd version is not suppored";
    reset_status();
    return;
  }

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
  ui_->cardholderButton->setDisabled(disable);
  ui_->accessCodesButton->setDisabled(disable);
  ui_->generateKeysButton->setDisabled(disable);
  ui_->moveKeyToCardButton->setDisabled(disable);
  ui_->fetchButton->setDisabled(disable);
  ui_->keyDBIndexComboBox->setDisabled(disable);
  ui_->cardKeysTreeView->setDisabled(disable);

  // refresh and the overflow menu stay reachable, the user has to be able to
  // rescan and restart the agents precisely when no card is detected
}

void SmartCardControllerDialog::slot_fetch_smart_card_keys() {
  ui_->fetchButton->setDisabled(true);

  auto err = GpgSmartCardManager::GetInstance().Fetch(
      ui_->currentCardComboBox->currentText());

  if (err != GPG_ERR_NO_ERROR) {
    // re-enable before bailing out, otherwise one failed fetch kills the
    // button for the rest of the dialog's life
    ui_->fetchButton->setDisabled(false);
    CommonUtils::RaiseFailureMessageBox(this, err);
    return;
  }

  QTimer::singleShot(kFetchSettleMs, this, [=]() {
    GpgCommandExecutor::GetInstance(channel_).GpgExecuteSync(
        {{},
         {"--card-status"},
         [=](int exit_code, const QString&, const QString&) {
           ui_->fetchButton->setDisabled(false);
           LOG_D() << "gpg --card--status exit code: " << exit_code;
           if (exit_code != 0) return;
           emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();
         }});
  });
}

void SmartCardControllerDialog::slot_move_key_to_card() {
  const auto serial = ui_->currentCardComboBox->currentText();
  if (serial.isEmpty()) {
    QMessageBox::information(this, tr("No Card"),
                             tr("No smart card is currently selected."));
    return;
  }

  // the key must be moved through the gpg-agent of the ring it lives in
  const int ring_channel = ui_->keyDBIndexComboBox->currentIndex();

  auto all_keys = GpgKeyRepository::GetInstance(ring_channel).Fetch();
  QList<GpgKeyPtr> secret_keys;
  QStringList key_labels;
  for (const auto& k : all_keys) {
    if (k == nullptr || !k->IsPrivateKey()) continue;
    secret_keys << k;
    key_labels << QString("%1 <%2> (%3)").arg(k->Name(), k->Email(), k->ID());
  }
  if (secret_keys.isEmpty()) {
    QMessageBox::information(
        this, tr("No Secret Keys"),
        tr("The selected key ring has no secret keys to move to a card."));
    return;
  }

  bool ok = false;
  const auto chosen_key = QInputDialog::getItem(
      this, tr("Select Key"), tr("Which key do you want to move to the card?"),
      key_labels, 0, false, &ok);
  if (!ok) return;
  const auto key_index = key_labels.indexOf(chosen_key);
  if (key_index < 0) return;
  const auto& key = secret_keys.at(key_index);

  // collect the (sub)keys that can actually be moved onto a card
  const auto subkeys = key->SubKeys();
  QList<int> movable_indexes;
  QStringList sub_labels;
  for (int i = 0; i < static_cast<int>(subkeys.size()); i++) {
    const auto& sk = subkeys[i];
    if (!sk.IsSecretKey() || sk.IsADSK() || sk.IsCardKey() || sk.IsRevoked() ||
        sk.IsExpired()) {
      continue;
    }
    if (GpgSmartCardManager::CandidateSlots(sk).isEmpty()) continue;
    movable_indexes << i;
    sub_labels << QString("%1 — %2 (%3)")
                      .arg(i == 0 ? tr("Primary Key") : tr("Subkey"))
                      .arg(sk.Algo())
                      .arg(sk.ID());
  }
  if (movable_indexes.isEmpty()) {
    QMessageBox::information(
        this, tr("No Movable Keys"),
        tr("This key has no part that can be moved to a smart card."));
    return;
  }

  int subkey_index = movable_indexes.front();
  if (movable_indexes.size() > 1) {
    bool sub_ok = false;
    const auto chosen_sub =
        QInputDialog::getItem(this, tr("Select Subkey"),
                              tr("Which part of the key do you want to move?"),
                              sub_labels, 0, false, &sub_ok);
    if (!sub_ok) return;
    const auto sub_index = sub_labels.indexOf(chosen_sub);
    if (sub_index < 0) return;
    subkey_index = movable_indexes.at(sub_index);
  }

  if (MoveKeyToCardInteractive(this, ring_channel, key, subkey_index, serial)) {
    fetch_smart_card_info(serial);
  }
}

auto AskIsoDisplayName(QWidget* parent, bool* ok) -> QString {
  QString surname = QInputDialog::getText(
      parent, SmartCardControllerDialog::tr("Cardholder's Surname"),
      SmartCardControllerDialog::tr("Please enter your surname (e.g., Lee):"),
      QLineEdit::Normal, "", ok);
  if (!*ok || surname.trimmed().isEmpty()) return {};

  QString given_name = QInputDialog::getText(
      parent, SmartCardControllerDialog::tr("Cardholder's Given Name"),
      SmartCardControllerDialog::tr(
          "Please enter your given name (e.g., Chris):"),
      QLineEdit::Normal, "", ok);
  if (!*ok || given_name.trimmed().isEmpty()) return {};

  QString iso_name = surname.trimmed() + "<<" + given_name.trimmed();
  iso_name.replace(" ", "<");

  if (iso_name.length() > 39) {
    QMessageBox::warning(parent, SmartCardControllerDialog::tr("Too Long"),
                         SmartCardControllerDialog::tr(
                             "Combined name too long (max 39 characters)."));
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
    options << "1 - " + tr("Male") << "2 - " + tr("Female");

    const QString selected = QInputDialog::getItem(
        this, tr("Modify Card Attribute"),
        tr("Select sex to store in '%1'").arg(attr) + ": ", options, 0, false,
        &ok);

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
        tr("Enter new value for attribute '%1'").arg(attr) + ": ",
        QLineEdit::Normal, "", &ok);

    if (!ok || value.isEmpty()) {
      LOG_D() << "user canceled or empty input.";
      return;
    }
  }

  auto [err, status] =
      GpgSmartCardManager::GetInstance(channel_).ModifyAttr(attr, value);

  if (err != GPG_ERR_NO_ERROR) {
    LOG_D() << "SCD SETATTR command failed for attr:" << attr
            << ", err:" << CheckGpgError(err);
    CommonUtils::RaiseFailureMessageBox(this, err, status);
    return;
  }
  QMessageBox::information(this, tr("Success"),
                           tr("Attribute operation completed successfully."));
  fetch_smart_card_info(ui_->currentCardComboBox->currentText());
}

void SmartCardControllerDialog::modify_key_pin(const QString& pinref) {
  auto [err, status] =
      GpgSmartCardManager::GetInstance(channel_).ModifyPin(pinref);

  if (err != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseFailureMessageBox(this, err, status);
    return;
  }

  QMessageBox::information(this, tr("Success"),
                           tr("PIN operation completed successfully."));
  fetch_smart_card_info(ui_->currentCardComboBox->currentText());
}

}  // namespace GpgFrontend::UI
