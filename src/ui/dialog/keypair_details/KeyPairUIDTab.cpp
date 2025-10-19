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

#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyManager.h"
#include "core/function/gpg/GpgUIDOperator.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/RevocationOptionsDialog.h"
#include "ui/dialog/keypair_details/KeyNewUIDDialog.h"
#include "ui/dialog/keypair_details/KeyUIDSignDialog.h"
#include "ui/widgets/TOFUInfoPage.h"

namespace GpgFrontend::UI {

KeyPairUIDTab::KeyPairUIDTab(int channel, GpgKeyPtr key, QWidget* parent)
    : QWidget(parent),
      current_gpg_context_channel_(channel),
      m_key_(std::move(key)) {
  assert(m_key_ != nullptr);

  create_uid_list();
  create_sign_list();
  create_uid_popup_menu();
  create_sign_popup_menu();

  auto* uid_buttons_layout = new QHBoxLayout();

  auto* add_uid_button = new QPushButton(tr("New UID"));

  if (!m_key_->IsHasMasterKey()) {
    add_uid_button->setDisabled(true);
  }
  uid_buttons_layout->addWidget(add_uid_button);

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
  labels << tr("Index") << tr("Name") << tr("Email") << tr("Comment");
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

  for (auto& uid : m_key_->UIDs()) {
    this->buffered_uids_.push_back(std::move(uid));
  }

  const int rows = static_cast<int>(buffered_uids_.size());
  uid_list_->setRowCount(rows);

  for (const auto& uid : buffered_uids_) {
    auto* tmp0 = new QTableWidgetItem(uid.GetName());
    uid_list_->setItem(row, 1, tmp0);

    auto* tmp1 = new QTableWidgetItem(uid.GetEmail());
    uid_list_->setItem(row, 2, tmp1);

    auto* tmp2 = new QTableWidgetItem(uid.GetComment());
    uid_list_->setItem(row, 3, tmp2);

    auto* tmp3 = new QTableWidgetItem(QString::number(row + 1));
    tmp3->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tmp3->setTextAlignment(Qt::AlignCenter);
    uid_list_->setItem(row, 0, tmp3);

    if (row == 0) {
      for (auto i = 0; i < uid_list_->columnCount(); i++) {
        uid_list_->item(row, i)->setForeground(QColor(65, 105, 255));
        for (auto i = 0; i < uid_list_->columnCount(); i++) {
          auto font = uid_list_->item(row, i)->font();
          font.setBold(true);
          uid_list_->item(row, i)->setFont(font);
        }
      }
    }

    if (uid.GetRevoked() || uid.GetInvalid()) {
      for (auto i = 0; i < uid_list_->columnCount(); i++) {
        auto font = uid_list_->item(row, i)->font();
        font.setStrikeOut(true);
        uid_list_->item(row, i)->setFont(font);
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

    const int rows = static_cast<int>(buffered_signatures_.size());
    sig_list_->setRowCount(rows);

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
  const auto& target_uid = get_selected_uid();

  auto* key_sign_dialog = new KeyUIDSignDialog(
      current_gpg_context_channel_, m_key_, target_uid.GetUID(), this);
  key_sign_dialog->show();
}

void KeyPairUIDTab::slot_add_uid() {
  auto* key_new_uid_dialog =
      new KeyNewUIDDialog(current_gpg_context_channel_, m_key_, this);
  connect(key_new_uid_dialog, &KeyNewUIDDialog::finished, this,
          &KeyPairUIDTab::slot_add_uid_result);
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
  const auto& target_uid = get_selected_uid();

  auto keynames = QString();

  keynames.append(target_uid.GetUID().toHtmlEscaped());
  keynames.append("<br/>");

  int index = 1;
  for (const auto& uid : buffered_uids_) {
    if (uid.GetUID() == target_uid.GetUID()) {
      break;
    }
    index++;
  }

  if (index == 1) {
    QMessageBox::information(nullptr, tr("Invalid Operation"),
                             tr("Cannot delete the Primary UID."));
    return;
  }

  int ret = QMessageBox::warning(
      this, tr("Deleting UIDs"),
      "<b>" +
          QString(
              tr("Are you sure that you want to delete the following UID?")) +
          "</b><br/><br/>" + target_uid.GetUID().toHtmlEscaped() + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!GpgUIDOperator::GetInstance(current_gpg_context_channel_)
             .DeleteUID(m_key_, index)) {
      QMessageBox::critical(
          nullptr, tr("Operation Failed"),
          tr("An error occurred during the delete %1 operation.")
              .arg(target_uid.GetUID().toHtmlEscaped()));
    }
    emit SignalUpdateUIDInfo();
  }
}

void KeyPairUIDTab::slot_set_primary_uid() {
  const auto& target_uid = get_selected_uid();

  if (target_uid.GetUID() == buffered_uids_.front().GetUID()) {
    return;
  }

  QString keynames;

  keynames.append(target_uid.GetUID().toHtmlEscaped());
  keynames.append("<br/>");

  int ret = QMessageBox::warning(
      this, tr("Set Primary UID"),
      "<b>" + tr("Are you sure that you want to set the Primary UID to?") +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret != QMessageBox::Yes) return;

  if (!GpgUIDOperator::GetInstance(current_gpg_context_channel_)
           .SetPrimaryUID(m_key_, target_uid.GetUID())) {
    QMessageBox::critical(nullptr, tr("Operation Failed"),
                          tr("An error occurred during the operation."));
  } else {
    emit SignalUpdateUIDInfo();
  }
}

auto KeyPairUIDTab::get_sign_selected() -> SignIdArgsList {
  auto signatures = SignIdArgsList{};
  for (int i = 0; i < sig_list_->rowCount(); i++) {
    if (sig_list_->item(i, 0)->isSelected()) {
      auto& sign = buffered_signatures_[i];
      signatures.push_back({sign.GetKeyID(), sign.GetUID()});
    }
  }
  return signatures;
}

void KeyPairUIDTab::create_uid_popup_menu() {
  uid_popup_menu_ = new QMenu(this);

  set_primary_uid_act_ = new QAction(tr("Set As Primary"), this);
  connect(set_primary_uid_act_, &QAction::triggered, this,
          &KeyPairUIDTab::slot_set_primary_uid);
  sign_uid_act_ = new QAction(tr("Sign UID"), this);
  connect(sign_uid_act_, &QAction::triggered, this,
          &KeyPairUIDTab::slot_add_sign_single);
  rev_uid_act_ = new QAction(tr("Revoke UID"), this);
  connect(rev_uid_act_, &QAction::triggered, this,
          &KeyPairUIDTab::slot_rev_uid);
  del_uid_act_ = new QAction(tr("Delete UID"), this);
  connect(del_uid_act_, &QAction::triggered, this,
          &KeyPairUIDTab::slot_del_uid);

  if (m_key_->IsHasMasterKey()) {
    uid_popup_menu_->addAction(set_primary_uid_act_);
    uid_popup_menu_->addAction(rev_uid_act_);
    uid_popup_menu_->addAction(del_uid_act_);
  }

  uid_popup_menu_->addAction(sign_uid_act_);
}

void KeyPairUIDTab::contextMenuEvent(QContextMenuEvent* event) {
  if (uid_list_->selectedItems().length() > 0 &&
      sig_list_->selectedItems().isEmpty()) {
    const auto is_primary_uid =
        get_selected_uid().GetUID() == buffered_uids_.front().GetUID();
    set_primary_uid_act_->setDisabled(is_primary_uid);
    rev_uid_act_->setDisabled(is_primary_uid);
    del_uid_act_->setDisabled(is_primary_uid);

    uid_popup_menu_->exec(event->globalPos());
  }
}

void KeyPairUIDTab::slot_add_sign_single() {
  const auto& target_uid = get_selected_uid();

  auto* key_sign_dialog = new KeyUIDSignDialog(
      current_gpg_context_channel_, m_key_, target_uid.GetUID(), this);
  key_sign_dialog->show();
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
  if (selected_signs.empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one Key Signature before doing this operation."));
    return;
  }

  if (!GpgKeyGetter::GetInstance(current_gpg_context_channel_)
           .GetKey(selected_signs.front().first)
           .IsGood()) {
    QMessageBox::critical(
        nullptr, tr("Invalid Operation"),
        tr("To delete the signature, you need to have its corresponding public "
           "key in the local database."));
    return;
  }

  QString keynames;

  keynames.append(selected_signs.front().second);
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
  auto refreshed_key = GpgKeyGetter::GetInstance(current_gpg_context_channel_)
                           .GetKeyPtr(m_key_->ID());
  assert(refreshed_key != nullptr);

  std::swap(this->m_key_, refreshed_key);

  this->slot_refresh_uid_list();
  this->slot_refresh_tofu_info();
  this->slot_refresh_sig_list();
}

void KeyPairUIDTab::slot_rev_uid() {
  const auto& target_uid = get_selected_uid();

  if (target_uid.GetUID() == buffered_uids_.front().GetUID()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one UID before doing this operation."));
    return;
  }

  const auto uids = m_key_->UIDs();

  QString message = tr("<h3>Revoke UID Confirmation</h3><br />"
                       "<b>UID:</b> %1<br /><br />"
                       "Revoking a UID will make it permanently unusable. "
                       "This action is <b>irreversible</b>.<br />"
                       "Are you sure you want to revoke this UID?")
                        .arg(target_uid.GetUID().toHtmlEscaped());

  int ret = QMessageBox::warning(this, tr("Revoke UID"), message,
                                 QMessageBox::Cancel | QMessageBox::Yes,
                                 QMessageBox::Cancel);

  if (ret != QMessageBox::Yes) return;

  int index = 1;
  for (const auto& uid : buffered_uids_) {
    if (uid.GetUID() == target_uid.GetUID()) {
      break;
    }
    index++;
  }

  if (index == 1) {
    QMessageBox::information(nullptr, tr("Invalid Operation"),
                             tr("Cannot delete the Primary UID."));
    return;
  }

  QStringList codes;
  codes << tr("0 -> No Reason.") << tr("4 -> User ID is no longer valid.");
  auto* revocation_options_dialog = new RevocationOptionsDialog(codes, this);

  connect(revocation_options_dialog,
          &RevocationOptionsDialog::SignalRevokeOptionAccepted, this,
          [key = m_key_, index, channel = current_gpg_context_channel_, this](
              int code, const QString& text) {
            auto res = GpgUIDOperator::GetInstance(channel).RevokeUID(
                key, index, code == 1 ? 4 : 0, text);
            if (!res) {
              QMessageBox::critical(
                  nullptr, tr("Revocation Failed"),
                  tr("Failed to revoke the UID. Please try again."));
            } else {
              QMessageBox::information(
                  nullptr, tr("Revocation Successful"),
                  tr("The UID has been successfully revoked."));
              emit SignalUpdateUIDInfo();
            }
          });

  revocation_options_dialog->show();
}

auto KeyPairUIDTab::get_selected_uid() -> const GpgUID& {
  int row = 0;

  for (int i = 0; i < uid_list_->rowCount(); i++) {
    if (uid_list_->item(row, 0)->isSelected()) break;
    row++;
  }

  return buffered_uids_[row];
}
}  // namespace GpgFrontend::UI
