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

#include "KeyPairSubkeyTab.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"

namespace GpgFrontend::UI {

KeyPairSubkeyTab::KeyPairSubkeyTab(int channel, const QString& key_id,
                                   QWidget* parent)
    : QWidget(parent),
      current_gpg_context_channel_(channel),
      key_(GpgKeyGetter::GetInstance(current_gpg_context_channel_)
               .GetKey(key_id)) {
  assert(key_.IsGood());

  create_subkey_list();
  create_subkey_opera_menu();

  list_box_ = new QGroupBox(tr("List of the primary key and subkey(s)"));
  detail_box_ = new QGroupBox(tr("Detail of Selected Primary Key/Subkey"));

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
  subkey_detail_layout->addWidget(new QLabel(tr("Key Type") + ": "), 1, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Algorithm") + ": "), 2, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Algorithm Detail") + ": "), 3,
                                  0);
  subkey_detail_layout->addWidget(new QLabel(tr("Key Size") + ": "), 4, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Usage") + ": "), 5, 0);
  subkey_detail_layout->addWidget(
      new QLabel(tr("Expires On (Local Time)") + ": "), 6, 0);
  subkey_detail_layout->addWidget(
      new QLabel(tr("Create Date (Local Time)") + ": "), 7, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Existence") + ": "), 8, 0);
  subkey_detail_layout->addWidget(new QLabel(tr("Key in Smart Card") + ": "), 9,
                                  0);
  subkey_detail_layout->addWidget(new QLabel(tr("Fingerprint") + ": "), 10, 0);

  key_type_var_label_ = new QLabel(this);
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

  // make keyid & fingerprint selectable for copy
  key_id_var_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  fingerprint_var_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  subkey_detail_layout->addWidget(key_id_var_label_, 0, 1, 1, 1);
  subkey_detail_layout->addWidget(key_type_var_label_, 1, 1, 1, 2);
  subkey_detail_layout->addWidget(algorithm_var_label_, 2, 1, 1, 2);
  subkey_detail_layout->addWidget(algorithm_detail_var_label_, 3, 1, 1, 2);
  subkey_detail_layout->addWidget(key_size_var_label_, 4, 1, 1, 2);
  subkey_detail_layout->addWidget(usage_var_label_, 5, 1, 1, 2);
  subkey_detail_layout->addWidget(expire_var_label_, 6, 1, 1, 2);
  subkey_detail_layout->addWidget(created_var_label_, 7, 1, 1, 2);
  subkey_detail_layout->addWidget(master_key_exist_var_label_, 8, 1, 1, 2);
  subkey_detail_layout->addWidget(card_key_label_, 9, 1, 1, 2);
  subkey_detail_layout->addWidget(fingerprint_var_label_, 10, 1, 1, 2);

  export_subkey_button_ = new QPushButton(tr("Export Subkey"));
  export_subkey_button_->setFlat(true);
  subkey_detail_layout->addWidget(export_subkey_button_, 0, 2);
  connect(export_subkey_button_, &QPushButton::clicked, this,
          &KeyPairSubkeyTab::slot_export_subkey);

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
  labels << tr("Key ID") << tr("Key Type") << tr("Key Size") << tr("Algorithm")
         << tr("Algorithm Detail") << tr("Create Date") << tr("Expire Date");

  subkey_list_->setHorizontalHeaderLabels(labels);
  subkey_list_->horizontalHeader()->setStretchLastSection(true);
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

  for (const auto& subkey : buffered_subkeys_) {
    auto* tmp0 = new QTableWidgetItem(subkey.GetID());
    tmp0->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(subkey.IsHasCertificationCapability()
                                          ? tr("Primary Key")
                                          : tr("Subkey"));
    tmp1->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(QString::number(subkey.GetKeyLength()));
    tmp2->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 2, tmp2);

    auto* tmp3 = new QTableWidgetItem(subkey.GetPubkeyAlgo());
    tmp3->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 3, tmp3);

    auto* tmp4 = new QTableWidgetItem(subkey.GetKeyAlgo());
    tmp4->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 4, tmp4);

    auto* tmp5 =
        new QTableWidgetItem(QLocale().toString(subkey.GetCreateTime()));
    tmp5->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 5, tmp5);

    auto* tmp6 =
        new QTableWidgetItem(subkey.GetExpireTime().toSecsSinceEpoch() == 0
                                 ? tr("Never Expire")
                                 : QLocale().toString(subkey.GetExpireTime()));
    tmp6->setTextAlignment(Qt::AlignCenter);
    subkey_list_->setItem(row, 6, tmp6);

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
  auto* dialog = new SubkeyGenerateDialog(current_gpg_context_channel_,
                                          key_.GetId(), this);
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
  fingerprint_var_label_->setWordWrap(true);  // for x448 and ed448

  export_subkey_button_->setText(subkey.IsHasCertificationCapability()
                                     ? tr("Export Primary Key")
                                     : tr("Export Subkey"));
  export_subkey_button_->setDisabled(!key_.IsPrivateKey() ||
                                     subkey.IsHasCertificationCapability() ||
                                     !subkey.IsSecretKey());

  key_type_var_label_->setText(
      subkey.IsHasCertificationCapability() ? tr("Primary Key") : tr("Subkey"));
}

void KeyPairSubkeyTab::create_subkey_opera_menu() {
  subkey_opera_menu_ = new QMenu(this);
  edit_subkey_act_ = new QAction(tr("Edit Expire Date"));
  connect(edit_subkey_act_, &QAction::triggered, this,
          &KeyPairSubkeyTab::slot_edit_subkey);

  export_subkey_act_ = new QAction(tr("Export"));
  connect(export_subkey_act_, &QAction::triggered, this,
          &KeyPairSubkeyTab::slot_export_subkey);

  subkey_opera_menu_->addAction(export_subkey_act_);
  subkey_opera_menu_->addAction(edit_subkey_act_);
}

void KeyPairSubkeyTab::slot_edit_subkey() {
  auto* dialog =
      new KeySetExpireDateDialog(current_gpg_context_channel_, key_.GetId(),
                                 get_selected_subkey().GetFingerprint(), this);
  dialog->show();
}

void KeyPairSubkeyTab::slot_revoke_subkey() {}

void KeyPairSubkeyTab::contextMenuEvent(QContextMenuEvent* event) {
  if (key_.IsPrivateKey() && !subkey_list_->selectedItems().isEmpty()) {
    const auto& subkey = get_selected_subkey();

    if (subkey.IsHasCertificationCapability()) return;

    export_subkey_act_->setDisabled(!subkey.IsSecretKey());
    edit_subkey_act_->setDisabled(!subkey.IsSecretKey());

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
  key_ = GpgKeyGetter::GetInstance(current_gpg_context_channel_)
             .GetKey(key_.GetId());
  assert(key_.IsGood());
}

void KeyPairSubkeyTab::slot_export_subkey() {
  int ret = QMessageBox::question(
      this, tr("Exporting Subkey"),
      "<h3>" + tr("You are about to export a private subkey.") + "</h3>\n" +
          tr("While subkeys are less critical than the primary key, "
             "they should still be handled with care.") +
          "<br /><br />" +
          tr("Do you want to proceed with exporting this subkey?"),
      QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Cancel);

  if (ret != QMessageBox::Yes) {
    QMessageBox::information(this, tr("Export Cancelled"),
                             tr("The subkey export has been cancelled."));
    return;
  }

  const auto& subkey = get_selected_subkey();

  auto [err, gf_buffer] =
      GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
          .ExportSubkey(subkey.GetFingerprint(), true);

  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseMessageBox(this, err);
    return;
  }

  // generate a file name
#if defined(_WIN32) || defined(WIN32)
  auto file_string = key_.GetName() + "[" + key_.GetEmail() + "](" +
                     subkey.GetID() + ")_subkey.asc";
#else
  auto file_string = key_.GetName() + "<" + key_.GetEmail() + ">(" +
                     subkey.GetID() + ")_subkey.asc";
#endif
  std::replace(file_string.begin(), file_string.end(), ' ', '_');

  auto file_name = QFileDialog::getSaveFileName(
      this, tr("Export Key To File"), file_string,
      tr("Key Files") + " (*.asc *.txt);;All Files (*)");

  if (file_name.isEmpty()) return;

  if (!WriteFileGFBuffer(file_name, gf_buffer)) {
    QMessageBox::critical(this, tr("Export Error"),
                          tr("Couldn't open %1 for writing").arg(file_name));
    return;
  }
}

}  // namespace GpgFrontend::UI
