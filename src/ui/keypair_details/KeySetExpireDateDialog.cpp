//
// Created by eric on 2021/6/3.
//

#include "ui/keypair_details/KeySetExpireDateDialog.h"

KeySetExpireDateDialog::KeySetExpireDateDialog(GpgME::GpgContext *ctx, const GpgKey &key, const GpgSubKey *subkey, QWidget *parent) :
QDialog(parent), mKey(key), mSubkey(subkey), mCtx(ctx) {

    QDateTime maxDateTime = QDateTime::currentDateTime().addYears(2);
    dateTimeEdit = new QDateTimeEdit(maxDateTime);
    dateTimeEdit->setMinimumDateTime(QDateTime::currentDateTime());
    dateTimeEdit->setMaximumDateTime(maxDateTime);
    nonExpiredCheck = new QCheckBox();
    nonExpiredCheck->setTristate(false);
    confirmButton = new QPushButton(tr("Confirm"));

    auto *gridLayout = new QGridLayout();
    gridLayout->addWidget(dateTimeEdit, 0, 0, 1, 2);
    gridLayout->addWidget(nonExpiredCheck, 1, 0, 1, 1, Qt::AlignRight);
    gridLayout->addWidget(new QLabel(tr("Never Expire")));
    gridLayout->addWidget(confirmButton, 2, 0);

    connect(nonExpiredCheck, SIGNAL(stateChanged(int)), this, SLOT(slotNonExpiredChecked(int)));
    connect(confirmButton, SIGNAL(clicked(bool)), this, SLOT(slotConfirm()));

    this->setLayout(gridLayout);
    this->setWindowTitle("Edit Expire Datetime");
    this->setModal(true);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
}

void KeySetExpireDateDialog::slotConfirm() {
    QDateTime *expires = nullptr;
    if(this->nonExpiredCheck->checkState() == Qt::Unchecked) {
        expires = new QDateTime(this->dateTimeEdit->dateTime());
    }

    if(!mCtx->setExpire(mKey, mSubkey, expires)) {
        QMessageBox::critical(nullptr,
                              tr("Operation Failed"),
                              tr("An error occurred during the operation."));
    }
    delete expires;
    this->close();
}

void KeySetExpireDateDialog::slotNonExpiredChecked(int state) {
    if(state == 0) {
        this->dateTimeEdit->setDisabled(false);
    } else {
        this->dateTimeEdit->setDisabled(true);
    }
}
