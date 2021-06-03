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

KeyPairUIDTab::KeyPairUIDTab(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent) : QWidget(parent), mKey(key) {

    mCtx = ctx;

    createUIDList();
    createSignList();
    createManageUIDMenu();
    createUIDPopupMenu();
    createSignPopupMenu();

    auto uidButtonsLayout = new QGridLayout();

    auto addUIDButton = new QPushButton(tr("New UID"));
    auto manageUIDButton = new QPushButton(tr("UID Management"));

    manageUIDButton->setMenu(manageSelectedUIDMenu);

    uidButtonsLayout->addWidget(addUIDButton, 0, 1);
    uidButtonsLayout->addWidget(manageUIDButton, 0, 2);

    auto gridLayout = new QGridLayout();

    gridLayout->addWidget(uidList, 0, 0);
    gridLayout->addLayout(uidButtonsLayout, 1, 0);

    auto uidGroupBox = new QGroupBox();
    uidGroupBox->setLayout(gridLayout);
    uidGroupBox->setTitle("UIDs");

    auto signGridLayout = new QGridLayout();
    signGridLayout->addWidget(sigList, 0, 0);

    auto signGroupBox = new QGroupBox();
    signGroupBox->setLayout(signGridLayout);
    signGroupBox->setTitle("Signature of Selected UID");

    auto vboxLayout = new QVBoxLayout();
    vboxLayout->addWidget(uidGroupBox);
    vboxLayout->addWidget(signGroupBox);

    connect(addUIDButton, SIGNAL(clicked(bool)), this, SLOT(slotAddUID()));
    connect(mCtx, SIGNAL(signalKeyInfoChanged()), this, SLOT(slotRefreshUIDList()));
    connect(uidList, SIGNAL(itemSelectionChanged()), this, SLOT(slotRefreshSigList()));

    setLayout(vboxLayout);
    setAttribute(Qt::WA_DeleteOnClose, true);

    slotRefreshUIDList();
}

void KeyPairUIDTab::createUIDList() {

    uidList = new QTableWidget(this);
    uidList->setColumnCount(4);
    uidList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    uidList->verticalHeader()->hide();
    uidList->setShowGrid(false);
    uidList->setSelectionBehavior(QAbstractItemView::SelectRows);
    uidList->setSelectionMode( QAbstractItemView::SingleSelection );

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
    sigList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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
    labels << tr("Key ID") << tr("Name") << tr("Email") << tr("Create Date") << tr("Expired Date");
    sigList->setHorizontalHeaderLabels(labels);
    sigList->horizontalHeader()->setStretchLastSection(false);

}

void KeyPairUIDTab::slotRefreshUIDList() {

    int row = 0;

    uidList->setSelectionMode(QAbstractItemView::SingleSelection);

    this->buffered_uids.clear();

    for(const auto &uid : mKey.uids) {
        if(uid.invalid || uid.revoked) {
            continue;
        }
        this->buffered_uids.push_back(&uid);
    }

    uidList->setRowCount(buffered_uids.size());

    for(const auto& uid : buffered_uids) {

        auto *tmp0 = new QTableWidgetItem(uid->name);
        uidList->setItem(row, 1, tmp0);

        auto *tmp1 = new QTableWidgetItem(uid->email);
        uidList->setItem(row, 2, tmp1);

        auto *tmp2 = new QTableWidgetItem(uid->comment);
        uidList->setItem(row, 3, tmp2);

        auto *tmp3 = new QTableWidgetItem(QString::number(row));
        tmp3->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        tmp3->setTextAlignment(Qt::AlignCenter);
        tmp3->setCheckState(Qt::Unchecked);
        uidList->setItem(row, 0, tmp3);

        row++;
    }

    if(uidList->rowCount() > 0) {
        uidList->selectRow(0);
    }

    slotRefreshSigList();
}

void KeyPairUIDTab::slotRefreshSigList() {

    int uidRow = 0, sigRow = 0;
    for(const auto& uid : buffered_uids) {

        // Only Show Selected UID Signatures
        if(!uidList->item(uidRow++, 0)->isSelected()) {
            continue;
        }

        buffered_signatures.clear();

        for(const auto &sig : uid->signatures) {
            if(sig.invalid || sig.revoked) {
                continue;
            }
            buffered_signatures.push_back(&sig);
        }

        sigList->setRowCount(buffered_signatures.size());

        for(const auto &sig : buffered_signatures) {

            auto *tmp0 = new QTableWidgetItem(sig->keyid);
            sigList->setItem(sigRow, 0, tmp0);

            if(gpgme_err_code(sig->status) == GPG_ERR_NO_PUBKEY) {
                auto *tmp2 = new QTableWidgetItem("<Unknown>");
                sigList->setItem(sigRow, 1, tmp2);

                auto *tmp3 = new QTableWidgetItem("<Unknown>");
                sigList->setItem(sigRow, 2, tmp3);
            } else {
                auto *tmp2 = new QTableWidgetItem(sig->name);
                sigList->setItem(sigRow, 1, tmp2);

                auto *tmp3 = new QTableWidgetItem(sig->email);
                sigList->setItem(sigRow, 2, tmp3);
            }

            auto *tmp4 = new QTableWidgetItem(sig->create_time.toString());
            sigList->setItem(sigRow, 3, tmp4);

            auto *tmp5 = new QTableWidgetItem(sig->expire_time.toTime_t() == 0 ? tr("Never Expires")  : sig->expire_time.toString());
            tmp5->setTextAlignment(Qt::AlignCenter);
            sigList->setItem(sigRow, 4, tmp5);

            sigRow++;
        }

        break;
    }
}

void KeyPairUIDTab::slotAddSign() {

    QVector<UID> selected_uids;
    getUIDChecked(selected_uids);

    if(selected_uids.isEmpty()) {
        QMessageBox::information(nullptr,
                                 tr("Invalid Operation"),
                                 tr("Please select one or more UIDs before doing this operation."));
        return;
    }

    auto keySignDialog = new KeyUIDSignDialog(mCtx, mKey, selected_uids, this);
    keySignDialog->show();
}



void KeyPairUIDTab::getUIDChecked(QVector<UID> &selected_uids) {

    auto &uids = buffered_uids;

    for (int i = 0; i < uidList->rowCount(); i++) {
        if (uidList->item(i, 0)->checkState() == Qt::Checked) {
            selected_uids.push_back(*uids[i]);
        }
    }
}

void KeyPairUIDTab::createManageUIDMenu() {

    manageSelectedUIDMenu = new QMenu(this);

    auto *signUIDAct = new QAction(tr("Sign Selected UID(s)"), this);
    connect(signUIDAct, SIGNAL(triggered()), this, SLOT(slotAddSign()));
    auto *delUIDAct = new QAction(tr("Delete Selected UID(s)"), this);
    connect(delUIDAct, SIGNAL(triggered()), this, SLOT(slotDelUID()));

    manageSelectedUIDMenu->addAction(signUIDAct);
    manageSelectedUIDMenu->addAction(delUIDAct);
}

void KeyPairUIDTab::slotAddUID() {
    auto keyNewUIDDialog = new KeyNewUIDDialog(mCtx, mKey, this);
    connect(keyNewUIDDialog, SIGNAL(finished(int)), this, SLOT(slotAddUIDResult(int)));
    connect(keyNewUIDDialog, SIGNAL(finished(int)), keyNewUIDDialog, SLOT(deleteLater()));
    keyNewUIDDialog->show();
}

void KeyPairUIDTab::slotAddUIDResult(int result) {
    if(result == 1) {
        QMessageBox::information(nullptr,
                                 tr("Successful Operation"),
                                 tr("Successfully added a new UID."));
    } else if (result == -1) {
        QMessageBox::critical(nullptr,
                              tr("Operation Failed"),
                              tr("An error occurred during the operation."));
    }
}

void KeyPairUIDTab::slotDelUID() {

    QVector<UID> selected_uids;
    getUIDChecked(selected_uids);

    if(selected_uids.isEmpty()) {
        QMessageBox::information(nullptr,
                                 tr("Invalid Operation"),
                                 tr("Please select one or more UIDs before doing this operation."));
        return;
    }

    QString keynames;
    for (const auto &uid : selected_uids) {
        keynames.append(uid.name);
        keynames.append("<i> &lt;");
        keynames.append(uid.email);
        keynames.append("&gt; </i><br/>");
    }

    int ret = QMessageBox::warning(this, tr("Deleting UIDs"),
                                   "<b>"+tr("Are you sure that you want to delete the following uids?")+"</b><br/><br/>"+keynames+
                                   +"<br/>"+tr("The action can not be undone."),
                                   QMessageBox::No | QMessageBox::Yes);


    bool if_success = true;

    if (ret == QMessageBox::Yes) {
        for(const auto &uid : selected_uids) {
            if(!mCtx->revUID(mKey, uid)) {
                if_success = false;
            }
        }

        if(!if_success) {
            QMessageBox::critical(nullptr,
                                  tr("Operation Failed"),
                                  tr("An error occurred during the operation."));
        }

    }
}

void KeyPairUIDTab::slotSetPrimaryUID() {

    UID selected_uid;

    if(!getUIDSelected(selected_uid)) {
        auto emptyUIDMsg = new QMessageBox();
        emptyUIDMsg->setText("Please select one UID before doing this operation.");
        emptyUIDMsg->exec();
        return;
    }

    QString keynames;

    keynames.append(selected_uid.name);
    keynames.append("<i> &lt;");
    keynames.append(selected_uid.email);
    keynames.append("&gt; </i><br/>");

    int ret = QMessageBox::warning(this, tr("Set Primary UID"),
                                   "<b>"+tr("Are you sure that you want to set the Primary UID to?")+"</b><br/><br/>"+keynames+
                                   +"<br/>"+tr("The action can not be undone."),
                                   QMessageBox::No | QMessageBox::Yes);

    if (ret == QMessageBox::Yes) {
        if(!mCtx->setPrimaryUID(mKey, selected_uid)) {
            QMessageBox::critical(nullptr,
                                  tr("Operation Failed"),
                                  tr("An error occurred during the operation."));
        }
    }
}

bool KeyPairUIDTab::getUIDSelected(UID &uid) {
    auto &uids = buffered_uids;
    for (int i = 0; i < uidList->rowCount(); i++) {
        if (uidList->item(i, 0)->isSelected()) {
            uid = *uids[i];
            return true;
        }
    }
    return false;
}

bool KeyPairUIDTab::getSignSelected(Signature &signature) {
    auto &signatures = buffered_signatures;
    for (int i = 0; i < sigList->rowCount(); i++) {
        if (sigList->item(i, 0)->isSelected()) {
            signature = *signatures[i];
            return true;
        }
    }
    return false;
}

void KeyPairUIDTab::createUIDPopupMenu() {

    uidPopupMenu = new QMenu(this);

    auto *serPrimaryUIDAct = new QAction(tr("Set As Primary"), this);
    connect(serPrimaryUIDAct, SIGNAL(triggered()), this, SLOT(slotSetPrimaryUID()));
    auto *signUIDAct = new QAction(tr("Sign UID"), this);
    connect(signUIDAct, SIGNAL(triggered()), this, SLOT(slotAddSignSingle()));
    auto *delUIDAct = new QAction(tr("Delete UID"), this);
    connect(delUIDAct, SIGNAL(triggered()), this, SLOT(slotDelUIDSingle()));

    uidPopupMenu->addAction(serPrimaryUIDAct);
    uidPopupMenu->addAction(signUIDAct);
    uidPopupMenu->addAction(delUIDAct);
}

void KeyPairUIDTab::contextMenuEvent(QContextMenuEvent *event) {
    if (uidList->selectedItems().length() > 0 && sigList->selectedItems().isEmpty()) {
        uidPopupMenu->exec(event->globalPos());
    }

    if (!sigList->selectedItems().isEmpty()) {
        signPopupMenu->exec(event->globalPos());
    }
}

void KeyPairUIDTab::slotAddSignSingle() {

    UID selected_uid;

    if(!getUIDSelected(selected_uid)) {
        QMessageBox::information(nullptr,
                              tr("Invalid Operation"),
                              tr("Please select one UID before doing this operation."));
        return;
    }

    auto selected_uids = QVector<UID>({ selected_uid });
    auto keySignDialog = new KeyUIDSignDialog(mCtx, mKey, selected_uids, this);
    keySignDialog->show();
}

void KeyPairUIDTab::slotDelUIDSingle() {
    UID selected_uid;

    if(!getUIDSelected(selected_uid)) {
        QMessageBox::information(nullptr,
                                 tr("Invalid Operation"),
                                 tr("Please select one UID before doing this operation."));
        return;
    }

    QString keynames;

    keynames.append(selected_uid.name);
    keynames.append("<i> &lt;");
    keynames.append(selected_uid.email);
    keynames.append("&gt; </i><br/>");

    int ret = QMessageBox::warning(this, tr("Deleting UID"),
                                   "<b>"+tr("Are you sure that you want to delete the following uid?")+"</b><br/><br/>"+keynames+
                                   +"<br/>"+tr("The action can not be undone."),
                                   QMessageBox::No | QMessageBox::Yes);

    if (ret == QMessageBox::Yes) {
        if(!mCtx->revUID(mKey, selected_uid)) {
            QMessageBox::critical(nullptr,
                                  tr("Operation Failed"),
                                  tr("An error occurred during the operation."));
        }
    }
}

void KeyPairUIDTab::createSignPopupMenu() {
    signPopupMenu = new QMenu(this);

    auto *delSignAct = new QAction(tr("Delete(Revoke) Signature"), this);
    connect(delSignAct, SIGNAL(triggered()), this, SLOT(slotDelSign()));

    signPopupMenu->addAction(delSignAct);
}

void KeyPairUIDTab::slotDelSign() {
    Signature selected_sign;

    if(!getSignSelected(selected_sign)) {
        QMessageBox::information(nullptr,
                                 tr("Invalid Operation"),
                                 tr("Please select one Signature before doing this operation."));
        return;
    }

    if(gpgme_err_code(selected_sign.status) == GPG_ERR_NO_PUBKEY) {
        QMessageBox::critical(nullptr,
                                 tr("Invalid Operation"),
                                 tr("To delete the signature, you need to have its corresponding public key in the local database."));
        return;
    }

    QString keynames;

    keynames.append(selected_sign.name);
    keynames.append("<i> &lt;");
    keynames.append(selected_sign.email);
    keynames.append("&gt; </i><br/>");

    int ret = QMessageBox::warning(this, tr("Deleting Signature"),
                                   "<b>"+tr("Are you sure that you want to delete the following signature?")+"</b><br/><br/>"+keynames+
                                   +"<br/>"+tr("The action can not be undone."),
                                   QMessageBox::No | QMessageBox::Yes);

    if (ret == QMessageBox::Yes) {
        if(!mCtx->revSign(mKey, selected_sign)) {
            QMessageBox::critical(nullptr,
                                  tr("Operation Failed"),
                                  tr("An error occurred during the operation."));
        }
    }
}
