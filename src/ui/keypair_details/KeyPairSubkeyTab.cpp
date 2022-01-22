/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ui/keypair_details/KeyPairSubkeyTab.h"

#include "gpg/function/GpgKeyGetter.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {

KeyPairSubkeyTab::KeyPairSubkeyTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  LOG(INFO) << key_.GetEmail() << key_.IsPrivateKey() << key_.IsHasMasterKey()
            << key_.GetSubKeys()->front().IsPrivateKey();

  create_subkey_list();
  create_subkey_opera_menu();

  list_box_ = new QGroupBox(_("Subkey List"));
  detail_box_ = new QGroupBox(_("Detail of Selected Subkey"));

  auto uidButtonsLayout = new QGridLayout();

  auto addSubkeyButton = new QPushButton(_("Generate A New Subkey"));
  if (!key_.IsPrivateKey() || !key_.IsHasMasterKey()) {
    addSubkeyButton->setDisabled(true);
    setHidden(addSubkeyButton);
  }

  uidButtonsLayout->addWidget(addSubkeyButton, 0, 1);

  auto* baseLayout = new QVBoxLayout();

  auto subkeyListLayout = new QGridLayout();
  subkeyListLayout->addWidget(subkey_list_, 0, 0);
  subkeyListLayout->addLayout(uidButtonsLayout, 1, 0);
  subkeyListLayout->setContentsMargins(0, 10, 0, 0);

  auto* subkeyDetailLayout = new QGridLayout();

  subkeyDetailLayout->addWidget(new QLabel(QString(_("Key ID")) + ": "), 0, 0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Algorithm")) + ": "), 1,
                                0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Key Size")) + ": "), 2,
                                0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Usage")) + ": "), 3, 0);
  subkeyDetailLayout->addWidget(
      new QLabel(QString(_("Expires On (Local Time)")) + ": "), 4, 0);
  subkeyDetailLayout->addWidget(
      new QLabel(QString(_("Create Date (Local Time)")) + ": "), 5, 0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Existence")) + ": "), 6,
                                0);
  subkeyDetailLayout->addWidget(
      new QLabel(QString(_("Key in Smart Card")) + ": "), 7, 0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Fingerprint")) + ": "), 8,
                                0);

  key_id_var_label_ = new QLabel(this);
  key_size_var_label_ = new QLabel(this);
  expire_var_label_ = new QLabel(this);
  algorithm_var_label_ = new QLabel(this);
  created_var_label_ = new QLabel(this);
  usage_var_label_ = new QLabel(this);
  master_key_exist_var_label_ = new QLabel(this);
  fingerprint_var_label_ = new QLabel(this);
  card_key_label_ = new QLabel(this);

  subkeyDetailLayout->addWidget(key_id_var_label_, 0, 1, 1, 1);
  subkeyDetailLayout->addWidget(key_size_var_label_, 2, 1, 1, 2);
  subkeyDetailLayout->addWidget(expire_var_label_, 4, 1, 1, 2);
  subkeyDetailLayout->addWidget(algorithm_var_label_, 1, 1, 1, 2);
  subkeyDetailLayout->addWidget(created_var_label_, 5, 1, 1, 2);
  subkeyDetailLayout->addWidget(usage_var_label_, 3, 1, 1, 2);
  subkeyDetailLayout->addWidget(master_key_exist_var_label_, 6, 1, 1, 2);
  subkeyDetailLayout->addWidget(card_key_label_, 7, 1, 1, 2);
  subkeyDetailLayout->addWidget(fingerprint_var_label_, 8, 1, 1, 2);

  auto* copyKeyIdButton = new QPushButton(_("Copy"));
  copyKeyIdButton->setFlat(true);
  subkeyDetailLayout->addWidget(copyKeyIdButton, 0, 2);
  connect(copyKeyIdButton, &QPushButton::clicked, this, [=]() {
    QString fpr = key_id_var_label_->text().trimmed();
    QClipboard* cb = QApplication::clipboard();
    cb->setText(fpr);
  });

  list_box_->setLayout(subkeyListLayout);
  list_box_->setContentsMargins(0, 12, 0, 0);
  detail_box_->setLayout(subkeyDetailLayout);

  baseLayout->addWidget(list_box_);
  baseLayout->addWidget(detail_box_);
  baseLayout->addStretch();

  connect(addSubkeyButton, SIGNAL(clicked(bool)), this,
          SLOT(slot_add_subkey()));
  connect(subkey_list_, SIGNAL(itemSelectionChanged()), this,
          SLOT(slot_refresh_subkey_detail()));

  // key database refresh signal
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slot_refresh_key_info()));
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slot_refresh_subkey_list()));

  baseLayout->setContentsMargins(0, 0, 0, 0);

  setLayout(baseLayout);
  setAttribute(Qt::WA_DeleteOnClose, true);

  slot_refresh_subkey_list();
}

void KeyPairSubkeyTab::create_subkey_list() {
  subkey_list_ = new QTableWidget(this);

  subkey_list_->setColumnCount(5);
  subkey_list_->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  subkey_list_->verticalHeader()->hide();
  subkey_list_->setShowGrid(false);
  subkey_list_->setSelectionBehavior(QAbstractItemView::SelectRows);

  // tableitems not editable
  subkey_list_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  subkey_list_->setFocusPolicy(Qt::NoFocus);
  subkey_list_->setAlternatingRowColors(true);

  QStringList labels;
  labels << _("Subkey ID") << _("Key Size") << _("Algo")
         << _("Create Date (UTC)") << _("Expire Date (UTC)");

  subkey_list_->setHorizontalHeaderLabels(labels);
  subkey_list_->horizontalHeader()->setStretchLastSection(false);
}

void KeyPairSubkeyTab::slot_refresh_subkey_list() {
  LOG(INFO) << "Called";
  int row = 0;

  subkey_list_->setSelectionMode(QAbstractItemView::SingleSelection);

  this->buffered_subkeys_.clear();
  auto sub_keys = key_.GetSubKeys();
  for (auto& sub_key : *sub_keys) {
    if (sub_key.IsDisabled() || sub_key.IsRevoked()) continue;
    this->buffered_subkeys_.push_back(std::move(sub_key));
  }

  subkey_list_->setRowCount(buffered_subkeys_.size());

  for (const auto& subkeys : buffered_subkeys_) {
    auto* tmp0 = new QTableWidgetItem(QString::fromStdString(subkeys.GetID()));
    tmp0->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(QString::number(subkeys.GetKeyLength()));
    tmp1->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 1, tmp1);

    auto* tmp2 =
        new QTableWidgetItem(QString::fromStdString(subkeys.GetPubkeyAlgo()));
    tmp2->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 2, tmp2);

    auto* tmp3 = new QTableWidgetItem(
        QString::fromStdString(to_iso_string(subkeys.GetCreateTime())));
    tmp3->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 3, tmp3);

    auto* tmp4 = new QTableWidgetItem(
        boost::posix_time::to_time_t(
            boost::posix_time::ptime(subkeys.GetExpireTime())) == 0
            ? _("Never Expire")
            : QString::fromStdString(to_iso_string(subkeys.GetExpireTime())));
    tmp4->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 4, tmp4);

    if (!row) {
      for (auto i = 0; i < subkey_list_->columnCount(); i++) {
        subkey_list_->item(row, i)->setForeground(QColor(65, 105, 255));
      }
    }

    row++;
  }

  if (subkey_list_->rowCount() > 0) {
    subkey_list_->selectRow(0);
  }
}

void KeyPairSubkeyTab::slot_add_subkey() {
  auto dialog = new SubkeyGenerateDialog(key_.GetId(), this);
  dialog->show();
}

void KeyPairSubkeyTab::slot_refresh_subkey_detail() {
  auto& subkey = get_selected_subkey();

  key_id_var_label_->setText(QString::fromStdString(subkey.GetID()));
  key_size_var_label_->setText(QString::number(subkey.GetKeyLength()));

  time_t subkey_time_t = boost::posix_time::to_time_t(
      boost::posix_time::ptime(subkey.GetExpireTime()));

  expire_var_label_->setText(
      subkey_time_t == 0 ? _("Never Expires")
                         : QLocale::system().toString(QDateTime::fromTime_t(
                               to_time_t(subkey.GetExpireTime()))));
  if (subkey_time_t != 0 &&
      subkey.GetExpireTime() < boost::posix_time::second_clock::local_time()) {
    auto paletteExpired = expire_var_label_->palette();
    paletteExpired.setColor(expire_var_label_->foregroundRole(), Qt::red);
    expire_var_label_->setPalette(paletteExpired);
  } else {
    auto paletteValid = expire_var_label_->palette();
    paletteValid.setColor(expire_var_label_->foregroundRole(), Qt::darkGreen);
    expire_var_label_->setPalette(paletteValid);
  }

  algorithm_var_label_->setText(QString::fromStdString(subkey.GetPubkeyAlgo()));
  created_var_label_->setText(QLocale::system().toString(
      QDateTime::fromTime_t(to_time_t(subkey.GetCreateTime()))));

  std::stringstream usage_steam;

  if (subkey.IsHasCertificationCapability())
    usage_steam << _("Certificate") << " ";
  if (subkey.IsHasEncryptionCapability()) usage_steam << _("Encrypt") << " ";
  if (subkey.IsHasSigningCapability()) usage_steam << _("Sign") << " ";
  if (subkey.IsHasAuthenticationCapability()) usage_steam << _("Auth") << " ";

  usage_var_label_->setText(usage_steam.str().c_str());

  // Show the situation that secret key not exists.
  master_key_exist_var_label_->setText(subkey.IsSecretKey() ? _("Exists")
                                                            : _("Not Exists"));

  // Show the situation if key in a smart card.
  card_key_label_->setText(subkey.IsCardKey() ? _("Yes") : _("No"));

  if (!subkey.IsSecretKey()) {
    auto palette_expired = master_key_exist_var_label_->palette();
    palette_expired.setColor(master_key_exist_var_label_->foregroundRole(),
                             Qt::red);
    master_key_exist_var_label_->setPalette(palette_expired);
  } else {
    auto palette_valid = master_key_exist_var_label_->palette();
    palette_valid.setColor(master_key_exist_var_label_->foregroundRole(),
                           Qt::darkGreen);
    master_key_exist_var_label_->setPalette(palette_valid);
  }

  if (!subkey.IsCardKey()) {
    auto palette_expired = card_key_label_->palette();
    palette_expired.setColor(card_key_label_->foregroundRole(), Qt::red);
    card_key_label_->setPalette(palette_expired);
  } else {
    auto palette_valid = card_key_label_->palette();
    palette_valid.setColor(card_key_label_->foregroundRole(), Qt::darkGreen);
    card_key_label_->setPalette(palette_valid);
  }

  fingerprint_var_label_->setText(
      QString::fromStdString(subkey.GetFingerprint()));
}

void KeyPairSubkeyTab::create_subkey_opera_menu() {
  subkey_opera_menu_ = new QMenu(this);
  // auto *revokeSubkeyAct = new QAction(_("Revoke Subkey"));
  auto* editSubkeyAct = new QAction(_("Edit Expire Date"));
  connect(editSubkeyAct, SIGNAL(triggered(bool)), this,
          SLOT(slot_edit_subkey()));

  // subkeyOperaMenu->addAction(revokeSubkeyAct);
  subkey_opera_menu_->addAction(editSubkeyAct);
}

void KeyPairSubkeyTab::slot_edit_subkey() {
  LOG(INFO) << "Fpr" << get_selected_subkey().GetFingerprint();

  auto dialog = new KeySetExpireDateDialog(
      key_.GetId(), get_selected_subkey().GetFingerprint(), this);
  dialog->show();
}

void KeyPairSubkeyTab::slot_revoke_subkey() {}

void KeyPairSubkeyTab::contextMenuEvent(QContextMenuEvent* event) {
  if (!subkey_list_->selectedItems().isEmpty()) {
    subkey_opera_menu_->exec(event->globalPos());
  }
}

const GpgSubKey& KeyPairSubkeyTab::get_selected_subkey() {
  int row = 0;

  for (int i = 0; i < subkey_list_->rowCount(); i++) {
    if (subkey_list_->item(row, 0)->isSelected()) break;
    row++;
  }

  return buffered_subkeys_[row];
}
void KeyPairSubkeyTab::slot_refresh_key_info() {
  key_ = GpgKeyGetter::GetInstance().GetKey(key_.GetId());
}

}  // namespace GpgFrontend::UI
