//
// Created by eric on 2021/5/24.
//

#include "ui/keypair_details/KeyUIDSignDialog.h"

KeyUIDSignDialog::KeyUIDSignDialog(GpgME::GpgContext *ctx, const GpgKey &key, const QVector<UID> &uid, QWidget *parent) :
        mKey(key), mCtx(ctx), mUids(uid), QDialog(parent) {

    mKeyList = new KeyList(ctx,
                           KeyListRow::ONLY_SECRET_KEY,
                           KeyListColumn::NAME | KeyListColumn::EmailAddress,
                           this);

    mKeyList->setExcludeKeys({key.id});
    mKeyList->slotRefresh();

    signKeyButton = new QPushButton("Sign");

    /**
     * A DateTime after 5 Years is recommend.
     */
    expiresEdit = new QDateTimeEdit(QDateTime::currentDateTime().addYears(5));
    expiresEdit->setMinimumDateTime(QDateTime::currentDateTime());

    /**
     * Note further that the OpenPGP protocol uses 32 bit values for timestamps
     * and thus can only encode dates up to the year 2106.
     */
    expiresEdit->setMaximumDate(QDate(2106, 1, 1));

    nonExpireCheck = new QCheckBox("Non Expired");
    nonExpireCheck->setTristate(false);

    connect(nonExpireCheck, &QCheckBox::stateChanged, this, [this] (int state) -> void {
        if(state == 0)
            expiresEdit->setDisabled(false);
        else
            expiresEdit->setDisabled(true);
    });

    auto layout = new QGridLayout();

    auto timeLayout = new QGridLayout();

    layout->addWidget(mKeyList, 0, 0);
    layout->addWidget(signKeyButton, 2, 0, Qt::AlignRight);
    timeLayout->addWidget(new QLabel(tr("Expired Time")), 0, 0);
    timeLayout->addWidget(expiresEdit, 0, 1);
    timeLayout->addWidget(nonExpireCheck, 0, 2);
    layout->addLayout(timeLayout, 1, 0);

    connect(signKeyButton, SIGNAL(clicked(bool)), this, SLOT(slotSignKey(bool)));

    this->setLayout(layout);
    this->setModal(true);
    this->setWindowTitle(tr("Sign For Key's UID(s)"));
    this->adjustSize();

    setAttribute(Qt::WA_DeleteOnClose, true);
}

void KeyUIDSignDialog::slotSignKey(bool clicked) {

    // Set Signers
    QVector<GpgKey> keys;
    mKeyList->getCheckedKeys(keys);
    mCtx->setSigners(keys);

    const auto expires = expiresEdit->dateTime();

    for(const auto &uid : mUids) {
        // Sign For mKey
        if (!mCtx->signKey(mKey, uid.uid, &expires)) {
            QMessageBox::critical(nullptr,
                                  tr("Operation Unsuccessful"),
                                  QString("%1 <%2>"+tr(" signature operation failed for UID ") + "%3")
                                  .arg(mKey.name, mKey.email, uid.uid));
        }

    }

    QMessageBox::information(nullptr,
                             tr("Operation Complete"),
                             tr("The signature operation of the UID is complete"));

    this->close();
}
