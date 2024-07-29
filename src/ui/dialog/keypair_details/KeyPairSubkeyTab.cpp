/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "KeyPairSubkeyTab.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/CommonUtils.h"
#include "ui/UISignalStation.h"

namespace GpgFrontend::UI {

KeyPairSubkeyTab::KeyPairSubkeyTab(const QString& key_id, QWidget* parent)
    : QWidget(parent), key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  create_subkey_list();
  create_subkey_opera_menu();

  list_box_ = new QGroupBox(tr("Subkey List"));
  detail_box_ = new QGroupBox(tr("Detail of Selected Subkey"));

  auto* uid_buttons_layout = new QGridLayout();

  auto* add_subkey_button = new QPushButton(tr("Generate A New Subkey"));
  if (!key_.IsPrivateKey() || !key_.IsHasMasterKey()) {
    add_subkey_button->setDisabled(true);
    setHidden(add_subkey_button);
  }

  uid_buttons_layout->addWidget(add_subkey_button, 0, 1);

  auto* base_layout = new QVBoxLayout();

  auto* subkey_list_layout = new QGridLayout();
  subkey_list_layout->addWidget(subkey_list_, 0, 0);
  subkey_list_layout->addLayout(uid_buttons_layout, 1, 0);
  subkey_list_layout->setContentsMargins(0, 10, 0, 0);

  auto* subkey_detail_layout = new QGridLayout();

  subkey_detail_layout->addWidget(new QLabel(tr("Key ID") + ": "), 0, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Algorithm") + ": "), 1, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Algorithm Detail") + ": "), 2,
                                  0);
  subkey_detail_layout->addWidget(new QLabel(tr("Key Size") + ": "), 3, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Usage") + ": "), 4, 0);
  subkey_detail_layout->addWidget(
      new QLabel(tr("Expires On (Local Time)") + ": "), 5, 0);
  subkey_detail_layout->addWidget(
      new QLabel(tr("Create Date (Local Time)") + ": "), 6, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Existence") + ": "), 7, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Key in Smart Card") + ": "), 8,
                                  0);
  subkey_detail_layout->addWidget(new QLabel(tr("Fingerprint") + ": "), 9, 0);

  key_id_var_label_ = new QLabel(this);
  key_size_var_label_ = new QLabel(this);
  expire_var_label_ = new QLabel(this);
  algorithm_var_label_ = new QLabel(this);
  algorithm_detail_var_label_ = new QLabel(this);
  created_var_label_ = new QLabel(this);
  usage_var_label_ = new QLabel(this);
  master_key_exist_var_label_ = new QLabel(this);
  fingerprint_var_label_ = new QLabel(this);
  card_key_label_ = new QLabel(this);

  subkey_detail_layout->addWidget(key_id_var_label_, 0, 1, 1, 1);
  subkey_detail_layout->addWidget(algorithm_var_label_, 1, 1, 1, 2);
  subkey_detail_layout->addWidget(algorithm_detail_var_label_, 2, 1, 1, 2);
  subkey_detail_layout->addWidget(key_size_var_label_, 3, 1, 1, 2);
  subkey_detail_layout->addWidget(usage_var_label_, 4, 1, 1, 2);
  subkey_detail_layout->addWidget(expire_var_label_, 5, 1, 1, 2);
  subkey_detail_layout->addWidget(created_var_label_, 6, 1, 1, 2);
  subkey_detail_layout->addWidget(master_key_exist_var_label_, 7, 1, 1, 2);
  subkey_detail_layout->addWidget(card_key_label_, 8, 1, 1, 2);
  subkey_detail_layout->addWidget(fingerprint_var_label_, 9, 1, 1, 2);

  auto* copy_key_id_button = new QPushButton(tr("Copy"));
  copy_key_id_button->setFlat(true);
  subkey_detail_layout->addWidget(copy_key_id_button, 0, 2);
  connect(copy_key_id_button, &QPushButton::clicked, this, [=]() {
    QString fpr = key_id_var_label_->text().trimmed();
    QClipboard* cb = QApplication::clipboard();
    cb->setText(fpr);
  });

  list_box_->setLayout(subkey_list_layout);
  list_box_->setContentsMargins(0, 12, 0, 0);
  detail_box_->setLayout(subkey_detail_layout);

  base_layout->addWidget(list_box_);
  base_layout->addWidget(detail_box_);
  base_layout->addStretch();

  connect(add_subkey_button, &QPushButton::clicked, this,
          &KeyPairSubkeyTab::slot_add_subkey);
  connect(subkey_list_, &QTableWidget::itemSelectionChanged, this,
          &KeyPairSubkeyTab::slot_refresh_subkey_detail);

  // key database refresh signal
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyPairSubkeyTab::slot_refresh_key_info);
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyPairSubkeyTab::slot_refresh_subkey_list);

  base_layout->setContentsMargins(0, 0, 0, 0);

  setLayout(base_layout);
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
  labels << tr("Subkey ID") << tr("Key Size") << tr("Algo") << tr("Create Date")
         << tr("Expire Date");

  subkey_list_->setHorizontalHeaderLabels(labels);
  subkey_list_->horizontalHeader()->setStretchLastSection(false);
}

void KeyPairSubkeyTab::slot_refresh_subkey_list() {
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
    auto* tmp0 = new QTableWidgetItem(subkeys.GetID());
    tmp0->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(QString::number(subkeys.GetKeyLength()));
    tmp1->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(subkeys.GetPubkeyAlgo());
    tmp2->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 2, tmp2);

    auto* tmp3 =
        new QTableWidgetItem(QLocale().toString(subkeys.GetCreateTime()));
    tmp3->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 3, tmp3);

    auto* tmp4 =
        new QTableWidgetItem(subkeys.GetExpireTime().toSecsSinceEpoch() == 0
                                 ? tr("Never Expire")
                                 : QLocale().toString(subkeys.GetExpireTime()));
    tmp4->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 4, tmp4);

    if (!row) {
      for (auto i = 0; i < subkey_list_->columnCount(); i++) {
        subkey_list_->item(row, i)->setForeground(QColor(65, 105, 255));
      }
    }

    row++;
  }

  qCDebug(ui, "subkey_list_ refreshed");

  if (subkey_list_->rowCount() > 0) {
    subkey_list_->selectRow(0);
  }
}

void KeyPairSubkeyTab::slot_add_subkey() {
  auto* dialog = new SubkeyGenerateDialog(key_.GetId(), this);
  dialog->show();
}

void KeyPairSubkeyTab::slot_refresh_subkey_detail() {
  const auto& subkey = get_selected_subkey();

  key_id_var_label_->setText(subkey.GetID());
  key_size_var_label_->setText(QString::number(subkey.GetKeyLength()));

  time_t subkey_time_t = subkey.GetExpireTime().toSecsSinceEpoch();

  expire_var_label_->setText(subkey_time_t == 0
                                 ? tr("Never Expires")
                                 : QLocale().toString(subkey.GetExpireTime()));

  if (subkey_time_t != 0 &&
      subkey.GetExpireTime() < QDateTime::currentDateTime()) {
    auto palette_expired = expire_var_label_->palette();
    palette_expired.setColor(expire_var_label_->foregroundRole(), Qt::red);
    expire_var_label_->setPalette(palette_expired);
  } else {
    auto palette_valid = expire_var_label_->palette();
    palette_valid.setColor(expire_var_label_->foregroundRole(), Qt::darkGreen);
    expire_var_label_->setPalette(palette_valid);
  }

  algorithm_var_label_->setText(subkey.GetPubkeyAlgo());
  algorithm_detail_var_label_->setText(subkey.GetKeyAlgo());
  created_var_label_->setText(QLocale().toString(subkey.GetCreateTime()));

  QString buffer;
  QTextStream usage_steam(&buffer);

  if (subkey.IsHasCertificationCapability()) {
    usage_steam << tr("Certificate") << " ";
  }
  if (subkey.IsHasEncryptionCapability()) usage_steam << tr("Encrypt") << " ";
  if (subkey.IsHasSigningCapability()) usage_steam << tr("Sign") << " ";
  if (subkey.IsHasAuthenticationCapability()) usage_steam << tr("Auth") << " ";

  usage_var_label_->setText(usage_steam.readAll());

  // Show the situation that secret key not exists.
  master_key_exist_var_label_->setText(subkey.IsSecretKey() ? tr("Exists")
                                                            : tr("Not Exists"));

  // Show the situation if key in a smart card.
  card_key_label_->setText(subkey.IsCardKey() ? tr("Yes") : tr("No"));

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

  fingerprint_var_label_->setText(BeautifyFingerprint(subkey.GetFingerprint()));
}

void KeyPairSubkeyTab::create_subkey_opera_menu() {
  subkey_opera_menu_ = new QMenu(this);
  auto* edit_subkey_act = new QAction(tr("Edit Expire Date"));
  connect(edit_subkey_act, &QAction::triggered, this,
          &KeyPairSubkeyTab::slot_edit_subkey);

  subkey_opera_menu_->addAction(edit_subkey_act);
}

void KeyPairSubkeyTab::slot_edit_subkey() {
  auto* dialog = new KeySetExpireDateDialog(
      key_.GetId(), get_selected_subkey().GetFingerprint(), this);
  dialog->show();
}

void KeyPairSubkeyTab::slot_revoke_subkey() {}

void KeyPairSubkeyTab::contextMenuEvent(QContextMenuEvent* event) {
  if (key_.IsPrivateKey() && !subkey_list_->selectedItems().isEmpty()) {
    subkey_opera_menu_->exec(event->globalPos());
  }
}

auto KeyPairSubkeyTab::get_selected_subkey() -> const GpgSubKey& {
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
