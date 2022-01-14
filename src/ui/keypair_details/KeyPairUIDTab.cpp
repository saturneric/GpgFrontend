/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
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
#include "gpg/function/UIDOperator.h"
#include "ui/SignalStation.h"
#include "ui/widgets/TOFUInfoPage.h"

namespace GpgFrontend::UI {

KeyPairUIDTab::KeyPairUIDTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), mKey(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  createUIDList();
  createSignList();
  createManageUIDMenu();
  createUIDPopupMenu();
  createSignPopupMenu();

  auto uidButtonsLayout = new QGridLayout();

  auto addUIDButton = new QPushButton(_("New UID"));
  auto manageUIDButton = new QPushButton(_("UID Management"));

  if (mKey.has_master_key()) {
    manageUIDButton->setMenu(manageSelectedUIDMenu);
  } else {
    manageUIDButton->setDisabled(true);
  }

  uidButtonsLayout->addWidget(addUIDButton, 0, 1);
  uidButtonsLayout->addWidget(manageUIDButton, 0, 2);

  auto grid_layout = new QGridLayout();

  grid_layout->addWidget(uidList, 0, 0);
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
  tofuTabs = new QTabWidget(this);
  tofu_vbox_layout->addWidget(tofuTabs);
#endif

  auto sign_grid_layout = new QGridLayout();
  sign_grid_layout->addWidget(sigList, 0, 0);
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

  connect(addUIDButton, SIGNAL(clicked(bool)), this, SLOT(slotAddUID()));
  connect(uidList, SIGNAL(itemSelectionChanged()), this,
          SLOT(slotRefreshTOFUInfo()));
  connect(uidList, SIGNAL(itemSelectionChanged()), this,
          SLOT(slotRefreshSigList()));

  // Key Database Refresh
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slotRefreshKey()));

  connect(this, SIGNAL(signalUpdateUIDInfo()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));

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
  labels << _("Select") << _("Name") << _("Email") << _("Comment");
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
  labels << _("Key ID") << _("Name") << _("Email") << _("Create Date (UTC)")
         << _("Expired Date (UTC)");
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

    if (!row) {
      for (auto i = 0; i < uidList->columnCount(); i++) {
        uidList->item(row, i)->setForeground(QColor(65, 105, 255));
      }
    }

    row++;
  }

  if (uidList->rowCount() > 0) {
    uidList->selectRow(0);
  }

  slotRefreshSigList();
  slotRefreshTOFUInfo();
}

void KeyPairUIDTab::slotRefreshTOFUInfo() {
  if (this->tofuTabs == nullptr) return;

  int uidRow = 0;
  tofuTabs->clear();
  for (const auto& uid : buffered_uids) {
    // Only Show Selected UID Signatures
    if (!uidList->item(uidRow++, 0)->isSelected()) {
      continue;
    }
    auto tofu_infos = uid.tofu_infos();
    LOG(INFO) << "tofu info size" << tofu_infos->size();
    if (tofu_infos->empty()) {
      tofuTabs->hide();
    } else {
      tofuTabs->show();
    }
    int index = 1;
    for (const auto& tofu_info : *tofu_infos) {
      tofuTabs->addTab(new TOFUInfoPage(tofu_info, this),
                       QString(_("TOFU %1")).arg(index++));
    }
  }
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

      auto* tmp4 = new QTableWidgetItem(QLocale::system().toString(
          QDateTime::fromTime_t(to_time_t(sig.create_time()))));
      sigList->setItem(sigRow, 3, tmp4);

      auto* tmp5 = new QTableWidgetItem(
          boost::posix_time::to_time_t(
              boost::posix_time::ptime(sig.expire_time())) == 0
              ? _("Never Expires")
              : QLocale::system().toString(
                    QDateTime::fromTime_t(to_time_t(sig.expire_time()))));
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
        nullptr, _("Invalid Operation"),
        _("Please select one or more UIDs before doing this operation."));
    return;
  }

  auto keySignDialog =
      new KeyUIDSignDialog(mKey, std::move(selected_uids), this);
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

  auto* signUIDAct = new QAction(_("Sign Selected UID(s)"), this);
  connect(signUIDAct, SIGNAL(triggered()), this, SLOT(slotAddSign()));
  auto* delUIDAct = new QAction(_("Delete Selected UID(s)"), this);
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
    QMessageBox::information(nullptr, _("Successful Operation"),
                             _("Successfully added a new UID."));
  } else if (result == -1) {
    QMessageBox::critical(nullptr, _("Operation Failed"),
                          _("An error occurred during the operation."));
  }
}

void KeyPairUIDTab::slotDelUID() {
  auto selected_uids = getUIDChecked();

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
      LOG(INFO) << "KeyPairUIDTab::slotDelUID UID" << uid;
      if (!UIDOperator::GetInstance().RevUID(mKey, uid)) {
        QMessageBox::critical(
            nullptr, _("Operation Failed"),
            QString(_("An error occurred during the delete %1 operation."))
                .arg(uid.c_str()));
      }
    }
    emit signalUpdateUIDInfo();
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
      this, _("Set Primary UID"),
      "<b>" +
          QString(_("Are you sure that you want to set the Primary UID to?")) +
          "</b><br/><br/>" + keynames + +"<br/>" +
          _("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    if (!UIDOperator::GetInstance().SetPrimaryUID(mKey,
                                                  selected_uids->front())) {
      QMessageBox::critical(nullptr, _("Operation Failed"),
                            _("An error occurred during the operation."));
    } else {
      emit signalUpdateUIDInfo();
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
  return uids;
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

  auto* serPrimaryUIDAct = new QAction(_("Set As Primary"), this);
  connect(serPrimaryUIDAct, SIGNAL(triggered()), this,
          SLOT(slotSetPrimaryUID()));
  auto* signUIDAct = new QAction(_("Sign UID"), this);
  connect(signUIDAct, SIGNAL(triggered()), this, SLOT(slotAddSignSingle()));
  auto* delUIDAct = new QAction(_("Delete UID"), this);
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

  //  if (!sigList->selectedItems().isEmpty()) {
  //    signPopupMenu->exec(event->globalPos());
  //  }
}

void KeyPairUIDTab::slotAddSignSingle() {
  auto selected_uids = getUIDSelected();

  if (selected_uids->empty()) {
    QMessageBox::information(
        nullptr, _("Invalid Operation"),
        _("Please select one UID before doing this operation."));
    return;
  }

  auto keySignDialog =
      new KeyUIDSignDialog(mKey, std::move(selected_uids), this);
  keySignDialog->show();
}

void KeyPairUIDTab::slotDelUIDSingle() {
  auto selected_uids = getUIDSelected();
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
    if (!UIDOperator::GetInstance().RevUID(mKey, selected_uids->front())) {
      QMessageBox::critical(nullptr, _("Operation Failed"),
                            _("An error occurred during the operation."));
    } else {
      emit signalUpdateUIDInfo();
    }
  }
}

void KeyPairUIDTab::createSignPopupMenu() {
  signPopupMenu = new QMenu(this);

  auto* delSignAct = new QAction(_("Delete(Revoke) Key Signature"), this);
  connect(delSignAct, SIGNAL(triggered()), this, SLOT(slotDelSign()));

  signPopupMenu->addAction(delSignAct);
}

void KeyPairUIDTab::slotDelSign() {
  auto selected_signs = getSignSelected();
  if (selected_signs->empty()) {
    QMessageBox::information(
        nullptr, _("Invalid Operation"),
        _("Please select one Key Signature before doing this operation."));
    return;
  }

  if (!GpgKeyGetter::GetInstance()
           .GetKey(selected_signs->front().first)
           .good()) {
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
    if (!GpgKeyManager::GetInstance().RevSign(mKey, selected_signs)) {
      QMessageBox::critical(nullptr, _("Operation Failed"),
                            _("An error occurred during the operation."));
    }
  }
}
void KeyPairUIDTab::slotRefreshKey() {
  this->mKey = GpgKeyGetter::GetInstance().GetKey(this->mKey.id());
  this->slotRefreshUIDList();
  this->slotRefreshTOFUInfo();
  this->slotRefreshSigList();
}

}  // namespace GpgFrontend::UI
