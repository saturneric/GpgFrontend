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

#include "core/function/gpg/GpgAssuanHelper.h"
#include "core/utils/GpgUtils.h"

//
#include "ui_SmartCardControllerDialog.h"

namespace GpgFrontend::UI {
SmartCardControllerDialog::SmartCardControllerDialog(QWidget* parent)
    : GeneralDialog("SmartCardControllerDialog", parent),
      ui_(QSharedPointer<Ui_SmartCardControllerDialog>::create()),
      channel_(kGpgFrontendDefaultChannel) {
  ui_->setupUi(this);

  for (const auto& key_db : GetGpgKeyDatabaseInfos()) {
    ui_->keyDBIndexComboBox->insertItem(
        key_db.channel, QString("%1: %2").arg(key_db.channel).arg(key_db.name));
  }

  connect(ui_->keyDBIndexComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          [=](int index) {
            channel_ = index;
            ui_->cardKeysTreeView->SetChannel(channel_);
            ui_->cardKeysTreeView->expandAll();
          });

  slot_refresh();
}

void SmartCardControllerDialog::get_smart_card_serial_number() {
  auto [ret, status] = GpgAssuanHelper::GetInstance().SendStatusCommand(
      GpgComponentType::kGPG_AGENT, "SCD SERIALNO --all openpgp");
  if (!ret || status.isEmpty()) return;

  auto line = status.front();
  auto token = line.split(' ');

  if (token.size() != 2) {
    LOG_E() << "invalid response of command SERIALNO: " << line;
    return;
  }

  serial_number_ = token.back().trimmed();
  LOG_D() << "get smart card serial number: " << serial_number_;
}

void SmartCardControllerDialog::fetch_smart_card_info() {
  if (serial_number_.isEmpty()) return;

  auto [ret, status] = GpgAssuanHelper::GetInstance().SendStatusCommand(
      GpgComponentType::kGPG_AGENT, "SCD LEARN --force " + serial_number_);
  if (!ret || status.isEmpty()) return;

  LOG_D() << "fetch card raw info: " << status;
  card_info_ = GpgOpenPGPCard(status);

  if (!card_info_.good) {
    LOG_E() << "parse card raw info failed";
    return;
  }
}

void SmartCardControllerDialog::print_smart_card_info() {
  if (!card_info_.good) return;

  QString html;
  QTextStream out(&html);
  auto card = card_info_;

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
  out << "<li><b>" << tr("Manufacturer:") << "</b> " << card.manufacturer
      << "</li>";
  out << "<li><b>" << tr("Card Holder:") << "</b> " << card.card_holder
      << "</li>";
  out << "<li><b>" << tr("Language:") << "</b> " << card.display_language
      << "</li>";
  out << "<li><b>" << tr("Sex:") << "</b> " << card.display_sex << "</li>";
  out << "</ul>";

  out << "<h3>" << tr("Status") << "</h3><ul>";
  out << "<li><b>" << tr("CHV Status:") << "</b> " << card.chv_status
      << "</li>";
  out << "<li><b>" << tr("Signature Counter:") << "</b> " << card.sig_counter
      << "</li>";
  out << "<li><b>" << tr("KDF:") << "</b> " << card.kdf << "</li>";
  out << "<li><b>" << tr("UIF1:") << "</b> " << card.uif1 << "</li>";
  out << "<li><b>" << tr("UIF2:") << "</b> " << card.uif2 << "</li>";
  out << "<li><b>" << tr("UIF3:") << "</b> " << card.uif3 << "</li>";
  out << "</ul>";

  out << "<h3>" << tr("Fingerprints") << "</h3><ul>";
  for (auto it = card.fprs.begin(); it != card.fprs.end(); ++it) {
    out << "<li><b>" << tr("Key %1:").arg(it.key()) << "</b> " << it.value()
        << "</li>";
  }
  out << "</ul>";

  if (!card.card_infos.isEmpty()) {
    out << "<h3>" << tr("Additional Info") << "</h3><ul>";
    for (auto it = card.card_infos.begin(); it != card.card_infos.end(); ++it) {
      out << "<li><b>" << tr("%1:").arg(it.key()) << "</b> " << it.value()
          << "</li>";
    }
    out << "</ul>";
  }

  ui_->cardInfoEdit->setText(html);
}

void SmartCardControllerDialog::slot_refresh() {
  ui_->cardInfoEdit->clear();

  get_smart_card_serial_number();
  fetch_smart_card_info();

  print_smart_card_info();
  refresh_key_tree_view();
}

void SmartCardControllerDialog::refresh_key_tree_view() {
  if (card_info_.fprs.isEmpty()) {
    ui_->cardKeysTreeView->SetKeyFilter([](auto) { return false; });
    return;
  }

  QStringList card_fprs(card_info_.fprs.begin(), card_info_.fprs.end());
  ui_->cardKeysTreeView->SetKeyFilter([=](const GpgAbstractKey* k) {
    return card_fprs.contains(k->Fingerprint());
  });
}

}  // namespace GpgFrontend::UI
