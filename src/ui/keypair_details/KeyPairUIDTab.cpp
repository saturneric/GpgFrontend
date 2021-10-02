/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/keypair_details/KeyPairUIDTab.h"
#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyManager.h"
#include "gpg/function/UidOperator.h"

namespace GpgFrontend::UI {

KeyPairUIDTab::KeyPairUIDTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent),
      mKey(std::move(GpgKeyGetter::GetInstance().GetKey(key_id))) {
  createUIDList();
  createSignList();
  createManageUIDMenu();
  createUIDPopupMenu();
  createSignPopupMenu();

  auto uidButtonsLayout = new QGridLayout();

  auto addUIDButton = new QPushButton(tr("New UID"));
  auto manageUIDButton = new QPushButton(tr("UID Management"));

  if (mKey.has_master_key()) {
    manageUIDButton->setMenu(manageSelectedUIDMenu);
  } else {
    manageUIDButton->setDisabled(true);
  }

  uidButtonsLayout->addWidget(addUIDButton, 0, 1);
  uidButtonsLayout->addWidget(manageUIDButton, 0, 2);

  auto gridLayout = new QGridLayout();

  gridLayout->addWidget(uidList, 0, 0);
  gridLayout->addLayout(uidButtonsLayout, 1, 0);
  gridLayout->setContentsMargins(0, 10, 0, 0);

  auto uidGroupBox = new QGroupBox();
  uidGroupBox->setLayout(gridLayout);
  uidGroupBox->setTitle(tr("UIDs"));

  auto signGridLayout = new QGridLayout();
  signGridLayout->addWidget(sigList, 0, 0);
  signGridLayout->setContentsMargins(0, 10, 0, 0);

  auto signGroupBox = new QGroupBox();
  signGroupBox->setLayout(signGridLayout);
  signGroupBox->setTitle(tr("Signature of Selected UID"));

  auto vboxLayout = new QVBoxLayout();
  vboxLayout->addWidget(uidGroupBox);
  vboxLayout->addWidget(signGroupBox);
  vboxLayout->setContentsMargins(0, 0, 0, 0);

  connect(addUIDButton, SIGNAL(clicked(bool)), this, SLOT(slotAddUID()));
  connect(uidList, SIGNAL(itemSelectionChanged()), this,
          SLOT(slotRefreshSigList()));

  setLayout(vboxLayout);
  setAttribute(Qt::WA_DeleteOnClose, true);

  slotRefreshUIDList();
}

void KeyPairUIDTab::createUIDList() {
  uidList = new QTableWidget(this);
  uidList->setColumnCount(4);
  uidList->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  uidList->verticalHeader()->hide();
  uidList->setShowGrid(false);
  uidList->setSelectionBehavior(QAbstractItemView::SelectRows);
  uidList->setSelectionMode(QAbstractItemView::SingleSelection);

  // tableitems not editable
  uidList->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  uidList->setFocusPolicy(Qt::NoFocus);
  uidList->setAlternatingRowColors(true);

  QStringList labels;
  labels << tr("Select") << tr("Name") << tr("Email") << tr("Comment");
  uidList->setHorizontalHeaderLabels(labels);
  uidList->horizontalHeader()->setStretchLastSection(true);
}

void KeyPairUIDTab::createSignList() {
  sigList = new QTableWidget(this);
  sigList->setColumnCount(5);
  sigList->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  sigList->verticalHeader()->hide();
  sigList->setShowGrid(false);
  sigList->setSelectionBehavior(QAbstractItemView::SelectRows);

  // table items not editable
  sigList->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around table items)
  // may be it should focus on whole row
  sigList->setFocusPolicy(Qt::NoFocus);
  sigList->setAlternatingRowColors(true);

  QStringList labels;
  labels << tr("Key ID") << tr("Name") << tr("Email") << tr("Create Date")
         << tr("Expired Date");
  sigList->setHorizontalHeaderLabels(labels);
  sigList->horizontalHeader()->setStretchLastSection(false);
}

void KeyPairUIDTab::slotRefreshUIDList() {
  int row = 0;

  uidList->setSelectionMode(QAbstractItemView::SingleSelection);

  this->buffered_uids.clear();

  auto uids = mKey.uids();
  for (auto& uid : *uids) {
    if (uid.invalid() || uid.revoked()) {
      continue;
    }
    this->buffered_uids.push_back(std::move(uid));
  }

  uidList->setRowCount(buffered_uids.size());

  for (const auto& uid : buffered_uids) {
    auto* tmp0 = new QTableWidgetItem(QString::fromStdString(uid.name()));
    uidList->setItem(row, 1, tmp0);

    auto* tmp1 = new QTableWidgetItem(QString::fromStdString(uid.email()));
    uidList->setItem(row, 2, tmp1);

    auto* tmp2 = new QTableWidgetItem(QString::fromStdString(uid.comment()));
    uidList->setItem(row, 3, tmp2);

    auto* tmp3 = new QTableWidgetItem(QString::number(row));
    tmp3->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    tmp3->setTextAlignment(Qt::AlignCenter);
    tmp3->setCheckState(Qt::Unchecked);
    uidList->setItem(row, 0, tmp3);

    row++;
  }

  if (uidList->rowCount() > 0) {
    uidList->selectRow(0);
  }

  slotRefreshSigList();
}

void KeyPairUIDTab::slotRefreshSigList() {
  int uidRow = 0, sigRow = 0;
  for (const auto& uid : buffered_uids) {
    // Only Show Selected UID Signatures
    if (!uidList->item(uidRow++, 0)->isSelected()) {
      continue;
    }

    buffered_signatures.clear();
    auto signatures = uid.signatures();
    for (auto& sig : *signatures) {
      if (sig.invalid() || sig.revoked()) {
        continue;
      }
      buffered_signatures.push_back(std::move(sig));
    }

    sigList->setRowCount(buffered_signatures.size());

    for (const auto& sig : buffered_signatures) {
      auto* tmp0 = new QTableWidgetItem(QString::fromStdString(sig.keyid()));
      sigList->setItem(sigRow, 0, tmp0);

      if (gpgme_err_code(sig.status()) == GPG_ERR_NO_PUBKEY) {
        auto* tmp2 = new QTableWidgetItem("<Unknown>");
        sigList->setItem(sigRow, 1, tmp2);

        auto* tmp3 = new QTableWidgetItem("<Unknown>");
        sigList->setItem(sigRow, 2, tmp3);
      } else {
        auto* tmp2 = new QTableWidgetItem(QString::fromStdString(sig.name()));
        sigList->setItem(sigRow, 1, tmp2);

        auto* tmp3 = new QTableWidgetItem(QString::fromStdString(sig.email()));
        sigList->setItem(sigRow, 2, tmp3);
      }

      auto* tmp4 = new QTableWidgetItem(QString::fromStdString(
          boost::gregorian::to_iso_string(sig.create_time())));
      sigList->setItem(sigRow, 3, tmp4);

      auto* tmp5 = new QTableWidgetItem(
          boost::posix_time::to_time_t(
              boost::posix_time::ptime(sig.expire_time())) == 0
              ? tr("Never Expires")
              : QString::fromStdString(
                    boost::gregorian::to_iso_string(sig.expire_time())));
      tmp5->setTextAlignment(Qt::AlignCenter);
      sigList->setItem(sigRow, 4, tmp5);

      sigRow++;
    }

    break;
  }
}

void KeyPairUIDTab::slotAddSign() {
  auto selected_uids = getUIDChecked();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one or more UIDs before doing this operation."));
    return;
  }

  auto keySignDialog = new KeyUIDSignDialog(mKey, selected_uids, this);
  keySignDialog->show();
}

UIDArgsListPtr KeyPairUIDTab::getUIDChecked() {
  auto selected_uids = std::make_unique<UIDArgsList>();
  for (int i = 0; i < uidList->rowCount(); i++) {
    if (uidList->item(i, 0)->checkState() == Qt::Checked)
      selected_uids->push_back(buffered_uids[i].uid());
  }
  return selected_uids;
}

void KeyPairUIDTab::createManageUIDMenu() {
  manageSelectedUIDMenu = new QMenu(this);

  auto* signUIDAct = new QAction(tr("Sign Selected UID(s)"), this);
  connect(signUIDAct, SIGNAL(triggered()), this, SLOT(slotAddSign()));
  auto* delUIDAct = new QAction(tr("Delete Selected UID(s)"), this);
  connect(delUIDAct, SIGNAL(triggered()), this, SLOT(slotDelUID()));

  if (mKey.has_master_key()) {
    manageSelectedUIDMenu->addAction(signUIDAct);
    manageSelectedUIDMenu->addAction(delUIDAct);
  }
}

void KeyPairUIDTab::slotAddUID() {
  auto keyNewUIDDialog = new KeyNewUIDDialog(mKey.id(), this);
  connect(keyNewUIDDialog, SIGNAL(finished(int)), this,
          SLOT(slotAddUIDResult(int)));
  connect(keyNewUIDDialog, SIGNAL(finished(int)), keyNewUIDDialog,
          SLOT(deleteLater()));
  keyNewUIDDialog->show();
}

void KeyPairUIDTab::slotAddUIDResult(int result) {
  if (result == 1) {
    QMessageBox::information(nullptr, tr("Successful Operation"),
                             tr("Successfully added a new UID."));
  } else if (result == -1) {
    QMessageBox::critical(nullptr, tr("Operation Failed"),
                          tr("An error occurred during the operation."));
  }
}

void KeyPairUIDTab::slotDelUID() {
  auto selected_uids = getUIDChecked();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one or more UIDs before doing this operation."));
    return;
  }

  QString keynames;
  for (auto& uid : *selected_uids) {
    keynames.append(QString::fromStdString(uid));
    keynames.append("<br/>");
  }

  int ret = QMessageBox::warning(
      this, tr("Deleting UIDs"),
      "<b>" + tr("Are you sure that you want to delete the following uids?") +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  bool if_success = true;

  if (ret == QMessageBox::Yes) {
    for (const auto& uid : *selected_uids) {
      if (UidOperator::GetInstance().revUID(mKey, uid)) {
        if_success = false;
      }
    }

    if (!if_success) {
      QMessageBox::critical(nullptr, tr("Operation Failed"),
                            tr("An error occurred during the operation."));
    }
  }
}

void KeyPairUIDTab::slotSetPrimaryUID() {
  auto selected_uids = getUIDSelected();

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
      this, tr("Set Primary UID"),
      "<b>" + tr("Are you sure that you want to set the Primary UID to?") +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!UidOperator::GetInstance().setPrimaryUID(mKey,
                                                  selected_uids->front())) {
      QMessageBox::critical(nullptr, tr("Operation Failed"),
                            tr("An error occurred during the operation."));
    }
  }
}

UIDArgsListPtr KeyPairUIDTab::getUIDSelected() {
  auto uids = std::make_unique<UIDArgsList>();
  for (int i = 0; i < uidList->rowCount(); i++) {
    if (uidList->item(i, 0)->isSelected()) {
      uids->push_back(buffered_uids[i].uid());
    }
  }
  return std::move(uids);
}

SignIdArgsListPtr KeyPairUIDTab::getSignSelected() {
  auto signatures = std::make_unique<SignIdArgsList>();
  for (int i = 0; i < sigList->rowCount(); i++) {
    if (sigList->item(i, 0)->isSelected()) {
      auto& sign = buffered_signatures[i];
      signatures->push_back({sign.keyid(), sign.uid()});
    }
  }
  return signatures;
}

void KeyPairUIDTab::createUIDPopupMenu() {
  uidPopupMenu = new QMenu(this);

  auto* serPrimaryUIDAct = new QAction(tr("Set As Primary"), this);
  connect(serPrimaryUIDAct, SIGNAL(triggered()), this,
          SLOT(slotSetPrimaryUID()));
  auto* signUIDAct = new QAction(tr("Sign UID"), this);
  connect(signUIDAct, SIGNAL(triggered()), this, SLOT(slotAddSignSingle()));
  auto* delUIDAct = new QAction(tr("Delete UID"), this);
  connect(delUIDAct, SIGNAL(triggered()), this, SLOT(slotDelUIDSingle()));

  if (mKey.has_master_key()) {
    uidPopupMenu->addAction(serPrimaryUIDAct);
    uidPopupMenu->addAction(signUIDAct);
    uidPopupMenu->addAction(delUIDAct);
  }
}

void KeyPairUIDTab::contextMenuEvent(QContextMenuEvent* event) {
  if (uidList->selectedItems().length() > 0 &&
      sigList->selectedItems().isEmpty()) {
    uidPopupMenu->exec(event->globalPos());
  }

  if (!sigList->selectedItems().isEmpty()) {
    signPopupMenu->exec(event->globalPos());
  }
}

void KeyPairUIDTab::slotAddSignSingle() {
  auto selected_uids = getUIDSelected();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one UID before doing this operation."));
    return;
  }

  auto keySignDialog = new KeyUIDSignDialog(mKey, selected_uids, this);
  keySignDialog->show();
}

void KeyPairUIDTab::slotDelUIDSingle() {
  auto selected_uids = getUIDSelected();
  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one UID before doing this operation."));
    return;
  }

  QString keynames;

  keynames.append(QString::fromStdString(selected_uids->front()));
  keynames.append("<br/>");

  int ret = QMessageBox::warning(
      this, tr("Deleting UID"),
      "<b>" + tr("Are you sure that you want to delete the following uid?") +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!UidOperator::GetInstance().revUID(mKey, selected_uids->front())) {
      QMessageBox::critical(nullptr, tr("Operation Failed"),
                            tr("An error occurred during the operation."));
    }
  }
}

void KeyPairUIDTab::createSignPopupMenu() {
  signPopupMenu = new QMenu(this);

  auto* delSignAct = new QAction(tr("Delete(Revoke) Key Signature"), this);
  connect(delSignAct, SIGNAL(triggered()), this, SLOT(slotDelSign()));

  signPopupMenu->addAction(delSignAct);
}

void KeyPairUIDTab::slotDelSign() {
  auto selected_signs = getSignSelected();
  if (selected_signs->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one Key Signature before doing this operation."));
    return;
  }

  if (!GpgKeyGetter::GetInstance()
           .GetKey(selected_signs->front().first)
           .good()) {
    QMessageBox::critical(
        nullptr, tr("Invalid Operation"),
        tr("To delete the signature, you need to have its corresponding public "
           "key in the local database."));
    return;
  }

  QString keynames;

  keynames.append(QString::fromStdString(selected_signs->front().second));
  keynames.append("<br/>");

  int ret = QMessageBox::warning(
      this, tr("Deleting Key Signature"),
      "<b>" +
          tr("Are you sure that you want to delete the following signature?") +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!GpgKeyManager::GetInstance().revSign(mKey, selected_signs)) {
      QMessageBox::critical(nullptr, tr("Operation Failed"),
                            tr("An error occurred during the operation."));
    }
  }
}

}  // namespace GpgFrontend::UI
