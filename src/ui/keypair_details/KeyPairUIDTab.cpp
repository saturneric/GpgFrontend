//
// Created by eric on 2021/5/22.
//

#include "ui/keypair_details/KeyPairUIDTab.h"

KeyPairUIDTab::KeyPairUIDTab(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent) : QWidget(parent), mKey(key) {

    mCtx = ctx;

    createUIDList();
    createSignList();
    createManageUIDMenu();

    auto uidButtonsLayout = new QGridLayout();

    auto addUIDButton = new QPushButton(tr("New UID"));
    auto manageUIDButton = new QPushButton(tr("Manage UID"));

    manageUIDButton->setMenu(manageUIDMenu);

    uidButtonsLayout->addWidget(addUIDButton, 0, 1);
    uidButtonsLayout->addWidget(manageUIDButton, 0, 2);


//    auto sigButtonsLayout = new QGridLayout();
//    auto manageSigButton = new QPushButton(tr("Manage Signature"));
//
//    sigButtonsLayout->addWidget(addSigButton, 0, 1);
//    sigButtonsLayout->addWidget(manageSigButton, 0, 2);

    auto gridLayout = new QGridLayout();
    gridLayout->addWidget(uidList, 0, 0);
    gridLayout->addLayout(uidButtonsLayout, 1, 0);

    gridLayout->addWidget(sigList, 2, 0);
//    gridLayout->addLayout(sigButtonsLayout, 3, 0);

    setLayout(gridLayout);

    connect(mCtx, SIGNAL(signalKeyDBChanged()), this, SLOT(slotRefreshUIDList()));
    connect(mCtx, SIGNAL(signalKeyDBChanged()), this, SLOT(slotRefreshSigList()));
    connect(uidList, SIGNAL(itemSelectionChanged()), this, SLOT(slotRefreshSigList()));
//    connect(addSigButton, SIGNAL(clicked(bool)), this, SLOT(slotAddSign()));

    slotRefreshUIDList();
    slotRefreshSigList();
}

void KeyPairUIDTab::createUIDList() {
    uidList = new QTableWidget(this);
    uidList->setColumnCount(3);
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
    labels << tr("Operate") << tr("Name") << tr("Email") << tr("Comment");
    uidList->setHorizontalHeaderLabels(labels);
    uidList->horizontalHeader()->setStretchLastSection(true);
}

void KeyPairUIDTab::createSignList() {
    sigList = new QTableWidget(this);
    sigList->setColumnCount(4);
    sigList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    sigList->verticalHeader()->hide();
    sigList->setShowGrid(false);
    sigList->setSelectionBehavior(QAbstractItemView::SelectRows);

    // tableitems not editable
    sigList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // no focus (rectangle around tableitems)
    // may be it should focus on whole row
    sigList->setFocusPolicy(Qt::NoFocus);
    sigList->setAlternatingRowColors(true);

    QStringList labels;
    labels << tr("Type") << tr("Pubkey Id") << tr("Create Time") << tr("Valid Time");
    sigList->setHorizontalHeaderLabels(labels);
    sigList->horizontalHeader()->setStretchLastSection(true);

}

void KeyPairUIDTab::slotRefreshUIDList() {

    int row = 0;

    uidList->clearContents();
    uidList->setRowCount(mKey.uids.size());
    uidList->setSelectionMode(QAbstractItemView::SingleSelection);

    for(const auto& uid : mKey.uids) {

        auto *tmp0 = new QTableWidgetItem(uid.name);
        uidList->setItem(row, 1, tmp0);

        auto *tmp1 = new QTableWidgetItem(uid.email);
        uidList->setItem(row, 2, tmp1);

        auto *tmp2 = new QTableWidgetItem(uid.comment);
        uidList->setItem(row, 3, tmp2);

        auto *tmp3 = new QTableWidgetItem(QString::number(row));
        tmp3->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        tmp3->setTextAlignment(Qt::AlignCenter);
        tmp3->setCheckState(Qt::Unchecked);
        uidList->setItem(row, 0, tmp3);

        row++;
    }
}

void KeyPairUIDTab::slotRefreshSigList() {

    sigList->clearContents();

    int row = 0;
    for(const auto& uid : mKey.uids) {

        // Only Show Selected UID's Signatures
        if(!uidList->item(row, 0)->isSelected())
            continue;

        sigList->setRowCount(uid.signatures.size());

        for(const auto &sig : uid.signatures) {
            auto *tmp0 = new QTableWidgetItem(sig.pubkey_algo);
            sigList->setItem(row, 0, tmp0);

            auto *tmp2 = new QTableWidgetItem(sig.uid);
            sigList->setItem(row, 1, tmp2);

            auto *tmp3 = new QTableWidgetItem(sig.create_time.toString());
            sigList->setItem(row, 2, tmp3);

            auto *tmp4 = new QTableWidgetItem(sig.expire_time.toString());
            sigList->setItem(row, 3, tmp4);

            row++;
        }

        break;

    }
}

void KeyPairUIDTab::slotAddSign() {

    QVector<UID> selected_uids;

    getUIDChecked(selected_uids);

    if(selected_uids.isEmpty()) {
        auto emptyUIDMsg = new QMessageBox();
        emptyUIDMsg->setText("Please select one or more UIDs before doing this operation.");
        emptyUIDMsg->exec();
        return;
    }

    auto keySignDialog = new KeySignDialog(mCtx, mKey, selected_uids, this);
    keySignDialog->show();
}

void KeyPairUIDTab::getUIDChecked(QVector<UID> &selected_uids) {

    auto &uids = mKey.uids;

    for (int i = 0; i < uidList->rowCount(); i++) {
        if (uidList->item(i, 0)->checkState() == Qt::Checked) {
            selected_uids.push_back(uids[i]);
        }
    }
}

void KeyPairUIDTab::createManageUIDMenu() {

    manageUIDMenu = new QMenu(this);

    auto *signUIDAct = new QAction(tr("Sign Selected UID(s)"), this);
    connect(signUIDAct, SIGNAL(triggered()), this, SLOT(slotAddSign()));

    manageUIDMenu->addAction(signUIDAct);
}
