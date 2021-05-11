/*
 *      verifynotification.cpp
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

#include "verifynotification.h"

VerifyNotification::VerifyNotification(QWidget *parent, GpgME::GpgContext *ctx, KeyList *keyList,QTextEdit *edit) :
    QWidget(parent)
{
    mCtx = ctx;
    mKeyList = keyList;
    mTextpage = edit;
    verifyLabel = new QLabel(this);

    connect(mCtx, SIGNAL(keyDBChanged()), this, SLOT(slotRefresh()));
    connect(edit, SIGNAL(textChanged()), this, SLOT(close()));

    importFromKeyserverAct = new QAction(tr("Import missing key from Keyserver"), this);
    connect(importFromKeyserverAct, SIGNAL(triggered()), this, SLOT(slotImportFromKeyserver()));

    showVerifyDetailsAct = new QAction(tr("Show detailed verify information"), this);
    connect(showVerifyDetailsAct, SIGNAL(triggered()), this, SLOT(slotShowVerifyDetails()));

    detailMenu = new QMenu(this);
    detailMenu->addAction(showVerifyDetailsAct);
    detailMenu->addAction(importFromKeyserverAct);
    importFromKeyserverAct->setVisible(false);

    keysNotInList = new QStringList();
    detailsButton = new QPushButton(tr("Details"),this);
    detailsButton->setMenu(detailMenu);
    QHBoxLayout *notificationWidgetLayout = new QHBoxLayout(this);
    notificationWidgetLayout->setContentsMargins(10,0,0,0);
    notificationWidgetLayout->addWidget(verifyLabel,2);
    notificationWidgetLayout->addWidget(detailsButton);
    this->setLayout(notificationWidgetLayout);
}

void VerifyNotification::slotImportFromKeyserver()
{
    KeyServerImportDialog *importDialog =new KeyServerImportDialog(mCtx,mKeyList, this);
    importDialog->slotImport(*keysNotInList);
}

void VerifyNotification::setVerifyLabel(QString text, verify_label_status verifyLabelStatus)
{
    QString color;
    verifyLabel->setText(text);
    switch (verifyLabelStatus) {
    case VERIFY_ERROR_OK:       color="#ccffcc";
                                break;
    case VERIFY_ERROR_WARN:     color="#ececba";
                                break;
    case VERIFY_ERROR_CRITICAL: color="#ff8080";
                                break;
    default:
                                break;
    }

    verifyLabel->setAutoFillBackground(true);
    QPalette status = verifyLabel->palette();
    status.setColor(QPalette::Background, color);
    verifyLabel->setPalette(status);
}                    

void VerifyNotification::showImportAction(bool visible)
{
    importFromKeyserverAct->setVisible(visible);
}

void VerifyNotification::slotShowVerifyDetails()
{
    QByteArray text = mTextpage->toPlainText().toUtf8();
    mCtx->preventNoDataErr(&text);
    new VerifyDetailsDialog(this, mCtx, mKeyList, &text);
}

bool VerifyNotification::slotRefresh()
{
    verify_label_status verifyStatus=VERIFY_ERROR_OK;

    QByteArray text = mTextpage->toPlainText().toUtf8();
    mCtx->preventNoDataErr(&text);
    int textIsSigned = mCtx->textIsSigned(text);

    gpgme_signature_t sign = mCtx->verify(&text);

    if (sign == NULL) {
        return false;
    }

    QString verifyLabelText;
    bool unknownKeyFound=false;

    while (sign) {

        switch (gpg_err_code(sign->status))
        {
            case GPG_ERR_NO_PUBKEY:
            {
                verifyStatus=VERIFY_ERROR_WARN;
                verifyLabelText.append(tr("Key not present with id 0x")+QString(sign->fpr));
                this->keysNotInList->append(sign->fpr);
                unknownKeyFound=true;
                break;
            }
            case GPG_ERR_NO_ERROR:
            {
                GpgKey key = mCtx->getKeyByFpr(sign->fpr);
                verifyLabelText.append(key.name);
                if (!key.email.isEmpty()) {
                    verifyLabelText.append("<"+key.email+">");
                }
                break;
            }
            case GPG_ERR_BAD_SIGNATURE:
            {
                textIsSigned = 3;
                verifyStatus=VERIFY_ERROR_CRITICAL;
                GpgKey key = mCtx->getKeyById(sign->fpr);
                verifyLabelText.append(key.name);
                if (!key.email.isEmpty()) {
                    verifyLabelText.append("<"+key.email+">");
                }
                break;
            }
            default:
            {
                //textIsSigned = 3;
                verifyStatus=VERIFY_ERROR_WARN;
                //GpgKey key = mKeyList->getKeyByFpr(sign->fpr);
                verifyLabelText.append(tr("Error for key with fingerprint ")+mCtx->beautifyFingerprint(QString(sign->fpr)));
                break;
            }
        }
        verifyLabelText.append("\n");
        sign = sign->next;
    }

    switch (textIsSigned)
    {
        case 3:
            {
                verifyLabelText.prepend(tr("Error validating signature by: "));
                break;
            }
        case 2:
            {
                verifyLabelText.prepend(tr("Text was completely signed by: "));
                break;
            }
        case 1:
            {
                verifyLabelText.prepend(tr("Text was partially signed by: "));
                break;
            }
    }

    // If an unknown key is found, enable the importfromkeyserveraction
    this->showImportAction(unknownKeyFound);

    // Remove the last linebreak
    verifyLabelText.remove(verifyLabelText.length()-1,1);

    this->setVerifyLabel(verifyLabelText,verifyStatus);

    return true;
}
