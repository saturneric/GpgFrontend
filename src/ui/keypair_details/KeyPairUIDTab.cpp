//
// Created by eric on 2021/5/22.
//

#include "ui/keypair_details/KeyPairUIDTab.h"

KeyPairUIDTab::KeyPairUIDTab(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent) : QWidget(parent), key(key) {

    mCtx = ctx;

    createUIDList();
    createSignList();

    auto uidButtonsLayout = new QGridLayout();

    auto addUIDButton = new QPushButton(tr("New UID"));
    auto manageUIDButton = new QPushButton(tr("Manage UID"));

    uidButtonsLayout->addWidget(addUIDButton, 0, 1);
    uidButtonsLayout->addWidget(manageUIDButton, 0, 2);


    auto sigButtonsLayout = new QGridLayout();

    auto addSigButton = new QPushButton("New Signature");
    auto manageSigButton = new QPushButton(tr("Manage Signature"));

    sigButtonsLayout->addWidget(addSigButton, 0, 1);
    sigButtonsLayout->addWidget(manageSigButton, 0, 2);

    auto gridLayout = new QGridLayout();
    gridLayout->addWidget(uidList, 0, 0);
    gridLayout->addLayout(uidButtonsLayout, 1, 0);

    gridLayout->addWidget(sigList, 2, 0);
    gridLayout->addLayout(sigButtonsLayout, 3, 0);

    setLayout(gridLayout);

    connect(mCtx, SIGNAL(signalKeyDBChanged()), this, SLOT(slotRefreshUIDList()));
    connect(mCtx, SIGNAL(signalKeyDBChanged()), this, SLOT(slotRefreshSigList()));

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

    // tableitems not editable
    uidList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // no focus (rectangle around tableitems)
    // may be it should focus on whole row
    uidList->setFocusPolicy(Qt::NoFocus);
    uidList->setAlternatingRowColors(true);

    QStringList labels;
    labels << tr("Name") << tr("Email") << tr("Comment");
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

    // tableitems not editable
    sigList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // no focus (rectangle around tableitems)
    // may be it should focus on whole row
    sigList->setFocusPolicy(Qt::NoFocus);
    sigList->setAlternatingRowColors(true);

    QStringList labels;
    labels << tr("Type") << tr("Name") << tr("Pubkey Id") << tr("Create Time") << tr("Valid Time");
    sigList->setHorizontalHeaderLabels(labels);
    sigList->horizontalHeader()->setStretchLastSection(true);
}

void KeyPairUIDTab::slotRefreshUIDList() {
    int row = 0;

    uidList->clearContents();
    uidList->setRowCount(key.uids.size());
    uidList->setSelectionMode(QAbstractItemView::SingleSelection);

    for(const auto& uid : key.uids) {

        auto *tmp0 = new QTableWidgetItem(uid.name);
        uidList->setItem(row, 0, tmp0);

        auto *tmp1 = new QTableWidgetItem(uid.email);
        uidList->setItem(row, 1, tmp1);

        auto *tmp2 = new QTableWidgetItem(uid.comment);
        uidList->setItem(row, 2, tmp2);

        row++;
    }




}

void KeyPairUIDTab::slotRefreshSigList() {
    int row = 0;

    sigList->clearContents();

    for(const auto& uid : key.uids) {
        row += uid.signatures.size();
    }
    sigList->setRowCount(row);

    row = 0;
    for(const auto& uid : key.uids) {

        // Only Show Selected UID's Signatures
        if(!uidList->item(row, 0)->isSelected())
            continue;

        for(const auto &sig : uid.signatures) {
            auto *tmp0 = new QTableWidgetItem(sig.pubkey_algo);
            uidList->setItem(row, 0, tmp0);

            auto *tmp1 = new QTableWidgetItem(sig.name);
            uidList->setItem(row, 1, tmp1);

            auto *tmp2 = new QTableWidgetItem(sig.uid);
            uidList->setItem(row, 2, tmp2);

            auto *tmp3 = new QTableWidgetItem(sig.create_time.toString());
            uidList->setItem(row, 3, tmp3);

            auto *tmp4 = new QTableWidgetItem(sig.expire_time.toString());
            uidList->setItem(row, 4, tmp4);

            row++;
        }
    }
}
