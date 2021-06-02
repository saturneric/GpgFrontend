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
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/keypair_details/KeyPairSubkeyTab.h"

KeyPairSubkeyTab::KeyPairSubkeyTab(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent) : mCtx(ctx), mKey(key), QWidget(parent) {

    createSubkeyList();

    listBox = new QGroupBox("Subkey List");
    detailBox = new QGroupBox("Detail of Selected Subkey");

    auto uidButtonsLayout = new QGridLayout();

    auto addSubkeyButton = new QPushButton(tr("Generate A New Subkey"));
    if(!mKey.is_private_key) {
        addSubkeyButton->setDisabled(true);
        setHidden(addSubkeyButton);
    }

    uidButtonsLayout->addWidget(addSubkeyButton, 0, 1);

    auto *baseLayout = new QVBoxLayout();

    auto subkeyListLayout = new QGridLayout();
    subkeyListLayout->addWidget(subkeyList, 0, 0);
    subkeyListLayout->addLayout(uidButtonsLayout, 1, 0);

    auto *subkeyDetailLayout = new QGridLayout();

    subkeyDetailLayout->addWidget(new QLabel(tr("Key ID: ")), 0, 0);
    subkeyDetailLayout->addWidget(new QLabel(tr("Algorithm: ")), 1, 0);
    subkeyDetailLayout->addWidget(new QLabel(tr("Key size:")), 2, 0);
    subkeyDetailLayout->addWidget(new QLabel(tr("Usage: ")), 3, 0);
    subkeyDetailLayout->addWidget(new QLabel(tr("Expires on: ")), 4, 0);
    subkeyDetailLayout->addWidget(new QLabel(tr("Last Update: ")), 5, 0);

    keyidVarLabel = new QLabel();
    keySizeVarLabel = new QLabel();
    expireVarLabel = new QLabel();
    algorithmVarLabel = new QLabel();
    createdVarLabel = new QLabel();
    usageVarLabel = new QLabel();

    subkeyDetailLayout->addWidget(keyidVarLabel, 0, 1);
    subkeyDetailLayout->addWidget(keySizeVarLabel, 2, 1);
    subkeyDetailLayout->addWidget(expireVarLabel, 4, 1);
    subkeyDetailLayout->addWidget(algorithmVarLabel, 1, 1);
    subkeyDetailLayout->addWidget(createdVarLabel, 5, 1);
    subkeyDetailLayout->addWidget(usageVarLabel, 3, 1);

    listBox->setLayout(subkeyListLayout);
    detailBox->setLayout(subkeyDetailLayout);

    baseLayout->addWidget(listBox);
    baseLayout->addWidget(detailBox);
    baseLayout->addStretch();

    connect(addSubkeyButton, SIGNAL(clicked(bool)), this, SLOT(slotAddSubkey()));
    connect(mCtx, SIGNAL(signalKeyInfoChanged()), this, SLOT(slotRefreshSubkeyList()));
    connect(subkeyList, SIGNAL(itemSelectionChanged()), this, SLOT(slotRefreshSubkeyDetail()));

    setLayout(baseLayout);
    setAttribute(Qt::WA_DeleteOnClose, true);

    slotRefreshSubkeyList();

}

void KeyPairSubkeyTab::createSubkeyList() {
    subkeyList = new QTableWidget(this);

    subkeyList->setColumnCount(5);
    subkeyList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    subkeyList->verticalHeader()->hide();
    subkeyList->setShowGrid(false);
    subkeyList->setSelectionBehavior(QAbstractItemView::SelectRows);

    // tableitems not editable
    subkeyList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // no focus (rectangle around tableitems)
    // may be it should focus on whole row
    subkeyList->setFocusPolicy(Qt::NoFocus);
    subkeyList->setAlternatingRowColors(true);

    QStringList labels;
    labels << tr("Subkey ID") << tr("Key Size") << tr("Algo") << tr("Create Date") << tr("Expire Date");

    subkeyList->setHorizontalHeaderLabels(labels);
    subkeyList->horizontalHeader()->setStretchLastSection(false);
}

void KeyPairSubkeyTab::slotRefreshSubkeyList() {
    int row = 0;

    subkeyList->setSelectionMode(QAbstractItemView::SingleSelection);

    this->buffered_subkeys.clear();

    for(const auto &subkeys : mKey.subKeys) {
        if(subkeys.disabled || subkeys.revoked) {
            continue;
        }
        this->buffered_subkeys.push_back(&subkeys);
    }

    subkeyList->setRowCount(buffered_subkeys.size());

    for(const auto& subkeys : buffered_subkeys) {

        auto *tmp0 = new QTableWidgetItem(subkeys->id);
        tmp0->setTextAlignment(Qt::AlignCenter);
        subkeyList->setItem(row, 0, tmp0);

        auto *tmp1 = new QTableWidgetItem(QString::number(subkeys->length));
        tmp1->setTextAlignment(Qt::AlignCenter);
        subkeyList->setItem(row, 1, tmp1);

        auto *tmp2 = new QTableWidgetItem(subkeys->pubkey_algo);
        tmp2->setTextAlignment(Qt::AlignCenter);
        subkeyList->setItem(row, 2, tmp2);

        auto *tmp3= new QTableWidgetItem(subkeys->timestamp.toString());
        tmp3->setTextAlignment(Qt::AlignCenter);
        subkeyList->setItem(row, 3, tmp3);

        auto *tmp4= new QTableWidgetItem(subkeys->expires.toTime_t() == 0 ? tr("Never Expire") : subkeys->expires.toString());
        tmp4->setTextAlignment(Qt::AlignCenter);
        subkeyList->setItem(row, 4, tmp4);

        row++;
    }

    if(subkeyList->rowCount() > 0) {
        subkeyList->selectRow(0);
    }
}

void KeyPairSubkeyTab::slotAddSubkey() {
    auto dialog = new SubkeyGenerateDialog(mCtx, mKey, this);
    dialog->show();
}

void KeyPairSubkeyTab::slotRefreshSubkeyDetail() {

    int row = 0;

    for(int i = 0 ; i < subkeyList->rowCount(); i++) {
        if(subkeyList->item(row, 0)->isSelected()) break;
        row++;
    }

    auto key = buffered_subkeys[row];

    keyidVarLabel->setText(key->id);
    keySizeVarLabel->setText(QString::number(key->length));
    expireVarLabel->setText(key->expires.toTime_t() == 0 ? tr("Never Expire")  : key->expires.toString());
    algorithmVarLabel->setText(key->pubkey_algo);
    createdVarLabel->setText(key->timestamp.toString());

    QString usage;
    QTextStream usage_steam(&usage);

    if(key->can_certify)
        usage_steam << "Cert ";
    if(key->can_encrypt)
        usage_steam << "Encr ";
    if(key->can_sign)
        usage_steam << "Sign ";
    if(key->can_authenticate)
        usage_steam << "Auth ";

    usageVarLabel->setText(usage);
}

void KeyPairSubkeyTab::createSubkeyOperaMenu() {
    subkeyOperaMenu = new QMenu();
    auto *revokeSubkeyAct = new QAction(tr("Revoke Subkey"));
    auto *editSubkeyAct = new QAction(tr("Edit Subkey"));

    subkeyOperaMenu->addAction(revokeSubkeyAct);
    subkeyOperaMenu->addAction(editSubkeyAct);
}

void KeyPairSubkeyTab::slotEditSubkey() {

}

void KeyPairSubkeyTab::slotRevokeSubkey() {

}
