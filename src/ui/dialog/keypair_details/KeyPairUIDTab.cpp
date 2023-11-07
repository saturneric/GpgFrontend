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

#include "KeyPairUIDTab.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyManager.h"
#include "core/function/gpg/GpgUIDOperator.h"
#include "ui/SignalStation.h"
#include "ui/widgets/TOFUInfoPage.h"

namespace GpgFrontend::UI {

KeyPairUIDTab::KeyPairUIDTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), m_key_(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  create_uid_list();
  create_sign_list();
  create_manage_uid_menu();
  create_uid_popup_menu();
  create_sign_popup_menu();

  auto uidButtonsLayout = new QGridLayout();

  auto addUIDButton = new QPushButton(_("New UID"));
  auto manageUIDButton = new QPushButton(_("UID Management"));

  if (m_key_.IsHasMasterKey()) {
    manageUIDButton->setMenu(manage_selected_uid_menu_);
  } else {
    manageUIDButton->setDisabled(true);
  }

  uidButtonsLayout->addWidget(addUIDButton, 0, 1);
  uidButtonsLayout->addWidget(manageUIDButton, 0, 2);

  auto grid_layout = new QGridLayout();

  grid_layout->addWidget(uid_list_, 0, 0);
  grid_layout->addLayout(uidButtonsLayout, 1, 0);
  grid_layout->setContentsMargins(0, 10, 0, 0);

  auto uid_group_box = new QGroupBox();
  uid_group_box->setLayout(grid_layout);
  uid_group_box->setTitle(_("UIDs"));

  auto tofu_group_box = new QGroupBox();
  auto tofu_vbox_layout = new QVBoxLayout();
  tofu_group_box->setLayout(tofu_vbox_layout);
  tofu_group_box->setTitle(_("TOFU"));
#if !defined(RELEASE)
  tofu_tabs_ = new QTabWidget(this);
  tofu_vbox_layout->addWidget(tofu_tabs_);
#endif

  auto sign_grid_layout = new QGridLayout();
  sign_grid_layout->addWidget(sig_list_, 0, 0);
  sign_grid_layout->setContentsMargins(0, 10, 0, 0);

  auto sign_group_box = new QGroupBox();
  sign_group_box->setLayout(sign_grid_layout);
  sign_group_box->setTitle(_("Signature of Selected UID"));

  auto vboxLayout = new QVBoxLayout();
  vboxLayout->addWidget(uid_group_box);
#if !defined(RELEASE)
  // Function needed testing
  vboxLayout->addWidget(tofu_group_box);
#endif
  vboxLayout->addWidget(sign_group_box);

  vboxLayout->setContentsMargins(0, 0, 0, 0);

  connect(addUIDButton, &QPushButton::clicked, this,
          &KeyPairUIDTab::slot_add_uid);
  connect(uid_list_, &QTableWidget::itemSelectionChanged, this,
          &KeyPairUIDTab::slot_refresh_tofu_info);
  connect(uid_list_, &QTableWidget::itemSelectionChanged, this,
          &KeyPairUIDTab::slot_refresh_sig_list);

  // Key Database Refresh
  connect(SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyPairUIDTab::slot_refresh_key);

  connect(this, &KeyPairUIDTab::SignalUpdateUIDInfo,
          SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh);

  setLayout(vboxLayout);
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
  labels << _("Select") << _("Name") << _("Email") << _("Comment");
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
  labels << _("Key ID") << _("Name") << _("Email") << _("Create Date (UTC)")
         << _("Expired Date (UTC)");
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
    auto* tmp0 = new QTableWidgetItem(QString::fromStdString(uid.GetUID()));
    uid_list_->setItem(row, 1, tmp0);

    auto* tmp1 = new QTableWidgetItem(QString::fromStdString(uid.GetUID()));
    uid_list_->setItem(row, 2, tmp1);

    auto* tmp2 = new QTableWidgetItem(QString::fromStdString(uid.GetUID()));
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

  int uidRow = 0;
  tofu_tabs_->clear();
  for (const auto& uid : buffered_uids_) {
    // Only Show Selected UID Signatures
    if (!uid_list_->item(uidRow++, 0)->isSelected()) {
      continue;
    }
    auto tofu_infos = uid.GetTofuInfos();
    SPDLOG_DEBUG("tofu info size: {}", tofu_infos->size());
    if (tofu_infos->empty()) {
      tofu_tabs_->hide();
    } else {
      tofu_tabs_->show();
    }
    int index = 1;
    for (const auto& tofu_info : *tofu_infos) {
      tofu_tabs_->addTab(new TOFUInfoPage(tofu_info, this),
                         QString(_("TOFU %1")).arg(index++));
    }
  }
}

void KeyPairUIDTab::slot_refresh_sig_list() {
  int uidRow = 0, sigRow = 0;
  for (const auto& uid : buffered_uids_) {
    // Only Show Selected UID Signatures
    if (!uid_list_->item(uidRow++, 0)->isSelected()) {
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
      auto* tmp0 = new QTableWidgetItem(QString::fromStdString(sig.GetKeyID()));
      sig_list_->setItem(sigRow, 0, tmp0);

      if (gpgme_err_code(sig.GetStatus()) == GPG_ERR_NO_PUBKEY) {
        auto* tmp2 = new QTableWidgetItem("<Unknown>");
        sig_list_->setItem(sigRow, 1, tmp2);

        auto* tmp3 = new QTableWidgetItem("<Unknown>");
        sig_list_->setItem(sigRow, 2, tmp3);
      } else {
        auto* tmp2 =
            new QTableWidgetItem(QString::fromStdString(sig.GetName()));
        sig_list_->setItem(sigRow, 1, tmp2);

        auto* tmp3 =
            new QTableWidgetItem(QString::fromStdString(sig.GetEmail()));
        sig_list_->setItem(sigRow, 2, tmp3);
      }
#ifdef GPGFRONTEND_GUI_QT6
      auto* tmp4 = new QTableWidgetItem(QLocale::system().toString(
          QDateTime::fromSecsSinceEpoch(to_time_t(sig.GetCreateTime()))));
#else
      auto* tmp4 = new QTableWidgetItem(QLocale::system().toString(
          QDateTime::fromTime_t(to_time_t(sig.GetCreateTime()))));
#endif
      sig_list_->setItem(sigRow, 3, tmp4);

#ifdef GPGFRONTEND_GUI_QT6
      auto* tmp5 = new QTableWidgetItem(
          boost::posix_time::to_time_t(
              boost::posix_time::ptime(sig.GetExpireTime())) == 0
              ? _("Never Expires")
              : QLocale::system().toString(QDateTime::fromSecsSinceEpoch(
                    to_time_t(sig.GetExpireTime()))));
#else
      auto* tmp5 = new QTableWidgetItem(
          boost::posix_time::to_time_t(
              boost::posix_time::ptime(sig.GetExpireTime())) == 0
              ? _("Never Expires")
              : QLocale::system().toString(
                    QDateTime::fromTime_t(to_time_t(sig.GetExpireTime()))));
#endif
      tmp5->setTextAlignment(Qt::AlignCenter);
      sig_list_->setItem(sigRow, 4, tmp5);

      sigRow++;
    }

    break;
  }
}

void KeyPairUIDTab::slot_add_sign() {
  auto selected_uids = get_uid_checked();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, _("Invalid Operation"),
        _("Please select one or more UIDs before doing this operation."));
    return;
  }

  auto keySignDialog =
      new KeyUIDSignDialog(m_key_, std::move(selected_uids), this);
  keySignDialog->show();
}

UIDArgsListPtr KeyPairUIDTab::get_uid_checked() {
  auto selected_uids = std::make_unique<UIDArgsList>();
  for (int i = 0; i < uid_list_->rowCount(); i++) {
    if (uid_list_->item(i, 0)->checkState() == Qt::Checked)
      selected_uids->push_back(buffered_uids_[i].GetUID());
  }
  return selected_uids;
}

void KeyPairUIDTab::create_manage_uid_menu() {
  manage_selected_uid_menu_ = new QMenu(this);

  auto* signUIDAct = new QAction(_("Sign Selected UID(s)"), this);
  connect(signUIDAct, &QAction::triggered, this, &KeyPairUIDTab::slot_add_sign);
  auto* delUIDAct = new QAction(_("Delete Selected UID(s)"), this);
  connect(delUIDAct, &QAction::triggered, this, &KeyPairUIDTab::slot_del_uid);

  if (m_key_.IsHasMasterKey()) {
    manage_selected_uid_menu_->addAction(signUIDAct);
    manage_selected_uid_menu_->addAction(delUIDAct);
  }
}

void KeyPairUIDTab::slot_add_uid() {
  auto keyNewUIDDialog = new KeyNewUIDDialog(m_key_.GetId(), this);
  connect(keyNewUIDDialog, &KeyNewUIDDialog::finished, this,
          &KeyPairUIDTab::slot_add_uid_result);
  connect(keyNewUIDDialog, &KeyNewUIDDialog::finished, keyNewUIDDialog,
          &KeyPairUIDTab::deleteLater);
  keyNewUIDDialog->show();
}

void KeyPairUIDTab::slot_add_uid_result(int result) {
  if (result == 1) {
    QMessageBox::information(nullptr, _("Successful Operation"),
                             _("Successfully added a new UID."));
  } else if (result == -1) {
    QMessageBox::critical(nullptr, _("Operation Failed"),
                          _("An error occurred during the operation."));
  }
}

void KeyPairUIDTab::slot_del_uid() {
  auto selected_uids = get_uid_checked();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, _("Invalid Operation"),
        _("Please select one or more UIDs before doing this operation."));
    return;
  }

  QString keynames;
  for (auto& uid : *selected_uids) {
    keynames.append(QString::fromStdString(uid));
    keynames.append("<br/>");
  }

  int ret = QMessageBox::warning(
      this, _("Deleting UIDs"),
      "<b>" +
          QString(
              _("Are you sure that you want to delete the following UIDs?")) +
          "</b><br/><br/>" + keynames + +"<br/>" +
          _("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    for (const auto& uid : *selected_uids) {
      SPDLOG_DEBUG("uid: {}", uid);
      if (!GpgUIDOperator::GetInstance().RevUID(m_key_, uid)) {
        QMessageBox::critical(
            nullptr, _("Operation Failed"),
            QString(_("An error occurred during the delete %1 operation."))
                .arg(uid.c_str()));
      }
    }
    emit SignalUpdateUIDInfo();
  }
}

void KeyPairUIDTab::slot_set_primary_uid() {
  auto selected_uids = get_uid_selected();

  if (selected_uids->empty()) {
    auto emptyUIDMsg = new QMessageBox();
    emptyUIDMsg->setText("Please select one UID before doing this operation.");
    emptyUIDMsg->exec();
    return;
  }

  QString keynames;

  keynames.append(QString::fromStdString(selected_uids->front()));
  keynames.append("<br/>");

  int ret = QMessageBox::warning(
      this, _("Set Primary UID"),
      "<b>" +
          QString(_("Are you sure that you want to set the Primary UID to?")) +
          "</b><br/><br/>" + keynames + +"<br/>" +
          _("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!GpgUIDOperator::GetInstance().SetPrimaryUID(m_key_,
                                                     selected_uids->front())) {
      QMessageBox::critical(nullptr, _("Operation Failed"),
                            _("An error occurred during the operation."));
    } else {
      emit SignalUpdateUIDInfo();
    }
  }
}

UIDArgsListPtr KeyPairUIDTab::get_uid_selected() {
  auto uids = std::make_unique<UIDArgsList>();
  for (int i = 0; i < uid_list_->rowCount(); i++) {
    if (uid_list_->item(i, 0)->isSelected()) {
      uids->push_back(buffered_uids_[i].GetUID());
    }
  }
  return uids;
}

SignIdArgsListPtr KeyPairUIDTab::get_sign_selected() {
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

  auto* serPrimaryUIDAct = new QAction(_("Set As Primary"), this);
  connect(serPrimaryUIDAct, &QAction::triggered, this,
          &KeyPairUIDTab::slot_set_primary_uid);
  auto* signUIDAct = new QAction(_("Sign UID"), this);
  connect(signUIDAct, &QAction::triggered, this,
          &KeyPairUIDTab::slot_add_sign_single);
  auto* delUIDAct = new QAction(_("Delete UID"), this);
  connect(delUIDAct, &QAction::triggered, this,
          &KeyPairUIDTab::slot_del_uid_single);

  if (m_key_.IsHasMasterKey()) {
    uid_popup_menu_->addAction(serPrimaryUIDAct);
    uid_popup_menu_->addAction(signUIDAct);
    uid_popup_menu_->addAction(delUIDAct);
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
        nullptr, _("Invalid Operation"),
        _("Please select one UID before doing this operation."));
    return;
  }

  auto keySignDialog =
      new KeyUIDSignDialog(m_key_, std::move(selected_uids), this);
  keySignDialog->show();
}

void KeyPairUIDTab::slot_del_uid_single() {
  auto selected_uids = get_uid_selected();
  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, _("Invalid Operation"),
        _("Please select one UID before doing this operation."));
    return;
  }

  QString keynames;

  keynames.append(QString::fromStdString(selected_uids->front()));
  keynames.append("<br/>");

  int ret = QMessageBox::warning(
      this, _("Deleting UID"),
      "<b>" +
          QString(
              _("Are you sure that you want to delete the following uid?")) +
          "</b><br/><br/>" + keynames + +"<br/>" +
          _("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!GpgUIDOperator::GetInstance().RevUID(m_key_, selected_uids->front())) {
      QMessageBox::critical(nullptr, _("Operation Failed"),
                            _("An error occurred during the operation."));
    } else {
      emit SignalUpdateUIDInfo();
    }
  }
}

void KeyPairUIDTab::create_sign_popup_menu() {
  sign_popup_menu_ = new QMenu(this);

  auto* delSignAct = new QAction(_("Delete(Revoke) Key Signature"), this);
  connect(delSignAct, &QAction::triggered, this, &KeyPairUIDTab::slot_del_sign);

  sign_popup_menu_->addAction(delSignAct);
}

void KeyPairUIDTab::slot_del_sign() {
  auto selected_signs = get_sign_selected();
  if (selected_signs->empty()) {
    QMessageBox::information(
        nullptr, _("Invalid Operation"),
        _("Please select one Key Signature before doing this operation."));
    return;
  }

  if (!GpgKeyGetter::GetInstance()
           .GetKey(selected_signs->front().first)
           .IsGood()) {
    QMessageBox::critical(
        nullptr, _("Invalid Operation"),
        _("To delete the signature, you need to have its corresponding public "
          "key in the local database."));
    return;
  }

  QString keynames;

  keynames.append(QString::fromStdString(selected_signs->front().second));
  keynames.append("<br/>");

  int ret =
      QMessageBox::warning(this, _("Deleting Key Signature"),
                           "<b>" +
                               QString(_("Are you sure that you want to delete "
                                         "the following signature?")) +
                               "</b><br/><br/>" + keynames + +"<br/>" +
                               _("The action can not be undone."),
                           QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!GpgKeyManager::GetInstance().RevSign(m_key_, selected_signs)) {
      QMessageBox::critical(nullptr, _("Operation Failed"),
                            _("An error occurred during the operation."));
    }
  }
}
void KeyPairUIDTab::slot_refresh_key() {
  // refresh the key
  GpgKey refreshed_key = GpgKeyGetter::GetInstance().GetKey(m_key_.GetId());
  std::swap(this->m_key_, refreshed_key);

  this->slot_refresh_uid_list();
  this->slot_refresh_tofu_info();
  this->slot_refresh_sig_list();
}

}  // namespace GpgFrontend::UI
