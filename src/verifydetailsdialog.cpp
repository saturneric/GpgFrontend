/*
 *      verifydetailsdialog.cpp
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#include "verifydetailsdialog.h"

VerifyDetailsDialog::VerifyDetailsDialog(QWidget *parent, GpgME::GpgContext* ctx, KeyList* keyList, QByteArray* inputData, QByteArray* inputSignature) :
    QDialog(parent)
{
    mCtx = ctx;
    mKeyList = keyList;
    //mTextpage = edit;
    mInputData = inputData;
    mInputSignature = inputSignature;

    this->setWindowTitle(tr("Signaturedetails"));

    connect(mCtx, SIGNAL(keyDBChanged()), this, SLOT(slotRefresh()));
    mainLayout = new QHBoxLayout();
    this->setLayout(mainLayout);

    mVbox = new QWidget();
    slotRefresh();

    this->exec();
}

void VerifyDetailsDialog::slotRefresh()
{
    mVbox->close();

    mVbox = new QWidget();
    QVBoxLayout *mVboxLayout = new QVBoxLayout(mVbox);
    mainLayout->addWidget(mVbox);

    // Button Box for close button
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    // Get signature information of current text
    //QByteArray text = mTextpage->toPlainText().toUtf8();
    //mCtx->preventNoDataErr(&text);
    gpgme_signature_t sign;
    if(mInputSignature != 0) {
        sign = mCtx->verify(mInputData, mInputSignature);
    } else {
        sign = mCtx->verify(mInputData);
    }

    if(sign==0) {
       mVboxLayout->addWidget(new QLabel(tr("No valid input found")));
       mVboxLayout->addWidget(buttonBox);
       return;
    }

    // Get timestamp of signature of current text
    QDateTime timestamp;
    timestamp.setTime_t(sign->timestamp);

    // Set the title widget depending on sign status
    if(gpg_err_code(sign->status) == GPG_ERR_BAD_SIGNATURE) {
        mVboxLayout->addWidget(new QLabel(tr("Error Validating signature")));
    } else if (mInputSignature != 0) {
        mVboxLayout->addWidget(new QLabel(tr("File was signed on <br/> %1 by:<br/>").arg(timestamp.toString(Qt::SystemLocaleLongDate))));
    } else {
        switch (mCtx->textIsSigned(*mInputData))
        {
            case 2:
            {
                mVboxLayout->addWidget(new QLabel(tr("Text was completely signed on <br/> %1 by:<br/>").arg(timestamp.toString(Qt::SystemLocaleLongDate))));                break;
            }
            case 1:
            {
                mVboxLayout->addWidget(new QLabel(tr("Text was partially signed on <br/> %1 by:<br/>").arg(timestamp.toString(Qt::SystemLocaleLongDate))));                break;
            }
        }
    }
    // Add informationbox for every single key
    while (sign) {
        VerifyKeyDetailBox *sbox = new VerifyKeyDetailBox(this,mCtx,mKeyList,sign);
        sign = sign->next;
        mVboxLayout->addWidget(sbox);
    }

    mVboxLayout->addWidget(buttonBox);
}
