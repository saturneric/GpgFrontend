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

#include "KeyPairUIDTab.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyManager.h"
#include "core/function/gpg/GpgUIDOperator.h"
#include "ui/UISignalStation.h"
#include "ui/widgets/TOFUInfoPage.h"

namespace GpgFrontend::UI {

KeyPairUIDTab::KeyPairUIDTab(int channel, const QString& key_id,
                             QWidget* parent)
    : QWidget(parent),
      current_gpg_context_channel_(channel),
      m_key_(GpgKeyGetter::GetInstance(current_gpg_context_channel_)
                 .GetKey(key_id)) {
  assert(m_key_.IsGood());

  create_uid_list();
  create_sign_list();
  create_manage_uid_menu();
  create_uid_popup_menu();
  create_sign_popup_menu();

  auto* uid_buttons_layout = new QGridLayout();

  auto* add_uid_button = new QPushButton(tr("New UID"));
  auto* manage_uid_button = new QPushButton(tr("UID Management"));

  if (m_key_.IsHasMasterKey()) {
    manage_uid_button->setMenu(manage_selected_uid_menu_);
  } else {
    add_uid_button->setDisabled(true);
    manage_uid_button->setDisabled(true);
  }

  uid_buttons_layout->addWidget(add_uid_button, 0, 1);
  uid_buttons_layout->addWidget(manage_uid_button, 0, 2);

  auto* grid_layout = new QGridLayout();

  grid_layout->addWidget(uid_list_, 0, 0);
  grid_layout->addLayout(uid_buttons_layout, 1, 0);
  grid_layout->setContentsMargins(0, 10, 0, 0);

  auto* uid_group_box = new QGroupBox();
  uid_group_box->setLayout(grid_layout);
  uid_group_box->setTitle(tr("UIDs"));

  auto* tofu_group_box = new QGroupBox();
  auto* tofu_vbox_layout = new QVBoxLayout();
  tofu_group_box->setLayout(tofu_vbox_layout);
  tofu_group_box->setTitle(tr("TOFU"));
#if !defined(RELEASE)
  tofu_tabs_ = new QTabWidget(this);
  tofu_vbox_layout->addWidget(tofu_tabs_);
#endif

  auto* sign_grid_layout = new QGridLayout();
  sign_grid_layout->addWidget(sig_list_, 0, 0);
  sign_grid_layout->setContentsMargins(0, 10, 0, 0);

  auto* sign_group_box = new QGroupBox();
  sign_group_box->setLayout(sign_grid_layout);
  sign_group_box->setTitle(tr("Signature of Selected UID"));

  auto* vbox_layout = new QVBoxLayout();
  vbox_layout->addWidget(uid_group_box);
#if !defined(RELEASE)
  // Function needed testing
  vbox_layout->addWidget(tofu_group_box);
#endif
  vbox_layout->addWidget(sign_group_box);

  vbox_layout->setContentsMargins(0, 0, 0, 0);

  connect(add_uid_button, &QPushButton::clicked, this,
          &KeyPairUIDTab::slot_add_uid);
  connect(uid_list_, &QTableWidget::itemSelectionChanged, this,
          &KeyPairUIDTab::slot_refresh_tofu_info);
  connect(uid_list_, &QTableWidget::itemSelectionChanged, this,
          &KeyPairUIDTab::slot_refresh_sig_list);

  // Key Database Refresh
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyPairUIDTab::slot_refresh_key);

  connect(this, &KeyPairUIDTab::SignalUpdateUIDInfo,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);

  setLayout(vbox_layout);
  setAttribute(Qt::WA_DeleteOnClose, true);

  slot_refresh_uid_list();
}

void KeyPairUIDTab::create_uid_list() {
  uid_list_ = new QTableWidget(this);
  uid_list_->setColumnCount(4);
  uid_list_->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  uid_list_->verticalHeader()->hide();
  uid_list_->setShowGrid(false);
  uid_list_->setSelectionBehavior(QAbstractItemView::SelectRows);
  uid_list_->setSelectionMode(QAbstractItemView::SingleSelection);

  // tableitems not editable
  uid_list_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  uid_list_->setFocusPolicy(Qt::NoFocus);
  uid_list_->setAlternatingRowColors(true);

  QStringList labels;
  labels << tr("Select") << tr("Name") << tr("Email") << tr("Comment");
  uid_list_->setHorizontalHeaderLabels(labels);
  uid_list_->horizontalHeader()->setStretchLastSection(true);
}

void KeyPairUIDTab::create_sign_list() {
  sig_list_ = new QTableWidget(this);
  sig_list_->setColumnCount(5);
  sig_list_->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  sig_list_->verticalHeader()->hide();
  sig_list_->setShowGrid(false);
  sig_list_->setSelectionBehavior(QAbstractItemView::SelectRows);

  // table items not editable
  sig_list_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around table items)
  // may be it should focus on whole row
  sig_list_->setFocusPolicy(Qt::NoFocus);
  sig_list_->setAlternatingRowColors(true);

  QStringList labels;
  labels << tr("Key ID") << tr("Name") << tr("Email") << tr("Create Date")
         << tr("Expired Date");
  sig_list_->setHorizontalHeaderLabels(labels);
  sig_list_->horizontalHeader()->setStretchLastSection(false);
}

void KeyPairUIDTab::slot_refresh_uid_list() {
  int row = 0;

  uid_list_->setSelectionMode(QAbstractItemView::SingleSelection);

  this->buffered_uids_.clear();

  auto uids = m_key_.GetUIDs();
  for (auto& uid : *uids) {
    if (uid.GetInvalid() || uid.GetRevoked()) {
      continue;
    }
    this->buffered_uids_.push_back(std::move(uid));
  }

  uid_list_->setRowCount(buffered_uids_.size());

  for (const auto& uid : buffered_uids_) {
    auto* tmp0 = new QTableWidgetItem(uid.GetName());
    uid_list_->setItem(row, 1, tmp0);

    auto* tmp1 = new QTableWidgetItem(uid.GetEmail());
    uid_list_->setItem(row, 2, tmp1);

    auto* tmp2 = new QTableWidgetItem(uid.GetComment());
    uid_list_->setItem(row, 3, tmp2);

    auto* tmp3 = new QTableWidgetItem(QString::number(row));
    tmp3->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    tmp3->setTextAlignment(Qt::AlignCenter);
    tmp3->setCheckState(Qt::Unchecked);
    uid_list_->setItem(row, 0, tmp3);

    if (!row) {
      for (auto i = 0; i < uid_list_->columnCount(); i++) {
        uid_list_->item(row, i)->setForeground(QColor(65, 105, 255));
      }
    }

    row++;
  }

  if (uid_list_->rowCount() > 0) {
    uid_list_->selectRow(0);
  }

  slot_refresh_sig_list();
  slot_refresh_tofu_info();
}

void KeyPairUIDTab::slot_refresh_tofu_info() {
  if (this->tofu_tabs_ == nullptr) return;

  int uid_row = 0;
  tofu_tabs_->clear();
  for (const auto& uid : buffered_uids_) {
    // Only Show Selected UID Signatures
    if (!uid_list_->item(uid_row++, 0)->isSelected()) {
      continue;
    }
    auto tofu_infos = uid.GetTofuInfos();
    if (tofu_infos->empty()) {
      tofu_tabs_->hide();
    } else {
      tofu_tabs_->show();
    }
    int index = 1;
    for (const auto& tofu_info : *tofu_infos) {
      tofu_tabs_->addTab(new TOFUInfoPage(tofu_info, this),
                         tr("TOFU %1").arg(index++));
    }
  }
}

void KeyPairUIDTab::slot_refresh_sig_list() {
  int uid_row = 0;
  int sig_row = 0;
  for (const auto& uid : buffered_uids_) {
    // Only Show Selected UID Signatures
    if (!uid_list_->item(uid_row++, 0)->isSelected()) {
      continue;
    }

    buffered_signatures_.clear();
    auto signatures = uid.GetSignatures();
    for (auto& sig : *signatures) {
      if (sig.IsInvalid() || sig.IsRevoked()) {
        continue;
      }
      buffered_signatures_.push_back(std::move(sig));
    }

    sig_list_->setRowCount(buffered_signatures_.size());

    for (const auto& sig : buffered_signatures_) {
      auto* tmp0 = new QTableWidgetItem(sig.GetKeyID());
      sig_list_->setItem(sig_row, 0, tmp0);

      if (gpgme_err_code(sig.GetStatus()) == GPG_ERR_NO_PUBKEY) {
        auto* tmp2 = new QTableWidgetItem("<Unknown>");
        sig_list_->setItem(sig_row, 1, tmp2);

        auto* tmp3 = new QTableWidgetItem("<Unknown>");
        sig_list_->setItem(sig_row, 2, tmp3);
      } else {
        auto* tmp2 = new QTableWidgetItem(sig.GetName());
        sig_list_->setItem(sig_row, 1, tmp2);

        auto* tmp3 = new QTableWidgetItem(sig.GetEmail());
        sig_list_->setItem(sig_row, 2, tmp3);
      }
      auto* tmp4 =
          new QTableWidgetItem(QLocale().toString(sig.GetCreateTime()));
      sig_list_->setItem(sig_row, 3, tmp4);

      auto* tmp5 =
          new QTableWidgetItem(sig.GetExpireTime().toSecsSinceEpoch() == 0
                                   ? tr("Never Expires")
                                   : QLocale().toString(sig.GetExpireTime()));
      tmp5->setTextAlignment(Qt::AlignCenter);
      sig_list_->setItem(sig_row, 4, tmp5);

      sig_row++;
    }

    break;
  }
}

void KeyPairUIDTab::slot_add_sign() {
  auto selected_uids = get_uid_checked();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one or more UIDs before doing this operation."));
    return;
  }

  auto* key_sign_dialog = new KeyUIDSignDialog(
      current_gpg_context_channel_, m_key_, std::move(selected_uids), this);
  key_sign_dialog->show();
}

auto KeyPairUIDTab::get_uid_checked() -> UIDArgsListPtr {
  auto selected_uids = std::make_unique<UIDArgsList>();
  for (int i = 0; i < uid_list_->rowCount(); i++) {
    if (uid_list_->item(i, 0)->checkState() == Qt::Checked) {
      selected_uids->push_back(buffered_uids_[i].GetUID());
    }
  }
  return selected_uids;
}

void KeyPairUIDTab::create_manage_uid_menu() {
  manage_selected_uid_menu_ = new QMenu(this);

  auto* sign_uid_act = new QAction(tr("Sign Selected UID(s)"), this);
  connect(sign_uid_act, &QAction::triggered, this,
          &KeyPairUIDTab::slot_add_sign);
  auto* del_uid_act = new QAction(tr("Delete Selected UID(s)"), this);
  connect(del_uid_act, &QAction::triggered, this, &KeyPairUIDTab::slot_del_uid);

  if (m_key_.IsHasMasterKey()) {
    manage_selected_uid_menu_->addAction(sign_uid_act);
    manage_selected_uid_menu_->addAction(del_uid_act);
  }
}

void KeyPairUIDTab::slot_add_uid() {
  auto* key_new_uid_dialog =
      new KeyNewUIDDialog(current_gpg_context_channel_, m_key_.GetId(), this);
  connect(key_new_uid_dialog, &KeyNewUIDDialog::finished, this,
          &KeyPairUIDTab::slot_add_uid_result);
  connect(key_new_uid_dialog, &KeyNewUIDDialog::finished, key_new_uid_dialog,
          &KeyPairUIDTab::deleteLater);
  key_new_uid_dialog->show();
}

void KeyPairUIDTab::slot_add_uid_result(int result) {
  if (result == 1) {
    QMessageBox::information(nullptr, tr("Successful Operation"),
                             tr("Successfully added a new UID."));
  } else if (result == -1) {
    QMessageBox::critical(nullptr, tr("Operation Failed"),
                          tr("An error occurred during the operation."));
  }
}

void KeyPairUIDTab::slot_del_uid() {
  auto selected_uids = get_uid_checked();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one or more UIDs before doing this operation."));
    return;
  }

  QString keynames;
  for (auto& uid : *selected_uids) {
    keynames.append(uid);
    keynames.append("<br/>");
  }

  int ret = QMessageBox::warning(
      this, tr("Deleting UIDs"),
      "<b>" +
          QString(
              tr("Are you sure that you want to delete the following UIDs?")) +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    for (const auto& uid : *selected_uids) {
      if (!GpgUIDOperator::GetInstance(current_gpg_context_channel_)
               .RevUID(m_key_, uid)) {
        QMessageBox::critical(
            nullptr, tr("Operation Failed"),
            tr("An error occurred during the delete %1 operation.").arg(uid));
      }
    }
    emit SignalUpdateUIDInfo();
  }
}

void KeyPairUIDTab::slot_set_primary_uid() {
  auto selected_uids = get_uid_selected();

  if (selected_uids->empty()) {
    auto* empty_uid_msg = new QMessageBox();
    empty_uid_msg->setText(
        "Please select one UID before doing this operation.");
    empty_uid_msg->exec();
    return;
  }

  QString keynames;

  keynames.append(selected_uids->front());
  keynames.append("<br/>");

  int ret = QMessageBox::warning(
      this, tr("Set Primary UID"),
      "<b>" + tr("Are you sure that you want to set the Primary UID to?") +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!GpgUIDOperator::GetInstance(current_gpg_context_channel_)
             .SetPrimaryUID(m_key_, selected_uids->front())) {
      QMessageBox::critical(nullptr, tr("Operation Failed"),
                            tr("An error occurred during the operation."));
    } else {
      emit SignalUpdateUIDInfo();
    }
  }
}

auto KeyPairUIDTab::get_uid_selected() -> UIDArgsListPtr {
  auto uids = std::make_unique<UIDArgsList>();
  for (int i = 0; i < uid_list_->rowCount(); i++) {
    if (uid_list_->item(i, 0)->isSelected()) {
      uids->push_back(buffered_uids_[i].GetUID());
    }
  }
  return uids;
}

auto KeyPairUIDTab::get_sign_selected() -> SignIdArgsListPtr {
  auto signatures = std::make_unique<SignIdArgsList>();
  for (int i = 0; i < sig_list_->rowCount(); i++) {
    if (sig_list_->item(i, 0)->isSelected()) {
      auto& sign = buffered_signatures_[i];
      signatures->push_back({sign.GetKeyID(), sign.GetUID()});
    }
  }
  return signatures;
}

void KeyPairUIDTab::create_uid_popup_menu() {
  uid_popup_menu_ = new QMenu(this);

  auto* ser_primary_uid_act = new QAction(tr("Set As Primary"), this);
  connect(ser_primary_uid_act, &QAction::triggered, this,
          &KeyPairUIDTab::slot_set_primary_uid);
  auto* sign_uid_act = new QAction(tr("Sign UID"), this);
  connect(sign_uid_act, &QAction::triggered, this,
          &KeyPairUIDTab::slot_add_sign_single);
  auto* del_uid_act = new QAction(tr("Delete UID"), this);
  connect(del_uid_act, &QAction::triggered, this,
          &KeyPairUIDTab::slot_del_uid_single);

  if (m_key_.IsHasMasterKey()) {
    uid_popup_menu_->addAction(ser_primary_uid_act);
    uid_popup_menu_->addAction(sign_uid_act);
    uid_popup_menu_->addAction(del_uid_act);
  }
}

void KeyPairUIDTab::contextMenuEvent(QContextMenuEvent* event) {
  if (uid_list_->selectedItems().length() > 0 &&
      sig_list_->selectedItems().isEmpty()) {
    uid_popup_menu_->exec(event->globalPos());
  }
}

void KeyPairUIDTab::slot_add_sign_single() {
  auto selected_uids = get_uid_selected();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one UID before doing this operation."));
    return;
  }

  auto* key_sign_dialog = new KeyUIDSignDialog(
      current_gpg_context_channel_, m_key_, std::move(selected_uids), this);
  key_sign_dialog->show();
}

void KeyPairUIDTab::slot_del_uid_single() {
  auto selected_uids = get_uid_selected();
  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one UID before doing this operation."));
    return;
  }

  QString keynames;

  keynames.append(selected_uids->front());
  keynames.append("<br/>");

  int ret = QMessageBox::warning(
      this, tr("Deleting UID"),
      "<b>" +
          QString(
              tr("Are you sure that you want to delete the following uid?")) +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!GpgUIDOperator::GetInstance(current_gpg_context_channel_)
             .RevUID(m_key_, selected_uids->front())) {
      QMessageBox::critical(nullptr, tr("Operation Failed"),
                            tr("An error occurred during the operation."));
    } else {
      emit SignalUpdateUIDInfo();
    }
  }
}

void KeyPairUIDTab::create_sign_popup_menu() {
  sign_popup_menu_ = new QMenu(this);

  auto* del_sign_act = new QAction(tr("Delete(Revoke) Key Signature"), this);
  connect(del_sign_act, &QAction::triggered, this,
          &KeyPairUIDTab::slot_del_sign);

  sign_popup_menu_->addAction(del_sign_act);
}

void KeyPairUIDTab::slot_del_sign() {
  auto selected_signs = get_sign_selected();
  if (selected_signs->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one Key Signature before doing this operation."));
    return;
  }

  if (!GpgKeyGetter::GetInstance(current_gpg_context_channel_)
           .GetKey(selected_signs->front().first)
           .IsGood()) {
    QMessageBox::critical(
        nullptr, tr("Invalid Operation"),
        tr("To delete the signature, you need to have its corresponding public "
           "key in the local database."));
    return;
  }

  QString keynames;

  keynames.append(selected_signs->front().second);
  keynames.append("<br/>");

  int ret = QMessageBox::warning(this, tr("Deleting Key Signature"),
                                 "<b>" +
                                     tr("Are you sure that you want to delete "
                                        "the following signature?") +
                                     "</b><br/><br/>" + keynames + +"<br/>" +
                                     tr("The action can not be undone."),
                                 QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!GpgKeyManager::GetInstance(current_gpg_context_channel_)
             .RevSign(m_key_, selected_signs)) {
      QMessageBox::critical(nullptr, tr("Operation Failed"),
                            tr("An error occurred during the operation."));
    }
  }
}
void KeyPairUIDTab::slot_refresh_key() {
  // refresh the key
  GpgKey refreshed_key = GpgKeyGetter::GetInstance(current_gpg_context_channel_)
                             .GetKey(m_key_.GetId());
  assert(refreshed_key.IsGood());

  std::swap(this->m_key_, refreshed_key);

  this->slot_refresh_uid_list();
  this->slot_refresh_tofu_info();
  this->slot_refresh_sig_list();
}

}  // namespace GpgFrontend::UI
