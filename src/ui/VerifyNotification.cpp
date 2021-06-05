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

#include "ui/VerifyNotification.h"

VerifyNotification::VerifyNotification(QWidget *parent, GpgME::GpgContext *ctx, KeyList *keyList, QTextEdit *edit) :
        QWidget(parent), mCtx(ctx), mKeyList(keyList), mTextpage(edit) {

    verifyLabel = new QLabel(this);
    infoBoard = new QTextEdit(this);
    infoBoard->setReadOnly(true);
    infoBoard->setFixedHeight(160);

    this->setFixedHeight(170);

    connect(mCtx, SIGNAL(signalKeyInfoChanged()), this, SLOT(slotRefresh()));
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
    detailsButton = new QPushButton(tr("Details"), this);
    detailsButton->setMenu(detailMenu);
    auto *notificationWidgetLayout = new QHBoxLayout(this);
    notificationWidgetLayout->addWidget(infoBoard);
    notificationWidgetLayout->addWidget(detailsButton);
    this->setLayout(notificationWidgetLayout);
}

void VerifyNotification::slotImportFromKeyserver() {
    auto *importDialog = new KeyServerImportDialog(mCtx, mKeyList, this);
    importDialog->slotImport(*keysNotInList);
}

void VerifyNotification::setInfoBoard(const QString &text, verify_label_status verifyLabelStatus) {
    QString color;
    infoBoard->setText(text);
    switch (verifyLabelStatus) {
        case VERIFY_ERROR_OK:
            color = "#ccffcc";
            break;
        case VERIFY_ERROR_WARN:
            color = "#ececba";
            break;
        case VERIFY_ERROR_CRITICAL:
            color = "#ff8080";
            break;
        default:
            break;
    }

    infoBoard->setAutoFillBackground(true);
    QPalette status = infoBoard->palette();
    status.setColor(QPalette::Text, color);
    infoBoard->setPalette(status);
    infoBoard->setFont(QFont("Times", 10, QFont::Bold));
}

void VerifyNotification::showImportAction(bool visible) {
    importFromKeyserverAct->setVisible(visible);
}

void VerifyNotification::slotShowVerifyDetails() {
    QByteArray text = mTextpage->toPlainText().toUtf8();
    GpgME::GpgContext::preventNoDataErr(&text);
    new VerifyDetailsDialog(this, mCtx, mKeyList, &text);
}

bool VerifyNotification::slotRefresh() {
    verify_label_status verifyStatus = VERIFY_ERROR_OK;

    QByteArray text = mTextpage->toPlainText().toUtf8();
    GpgME::GpgContext::preventNoDataErr(&text);
    int textIsSigned = GpgME::GpgContext::textIsSigned(text);

    gpgme_signature_t sign = mCtx->verify(&text);

    if (sign == nullptr) {
        return false;
    }

    QString verifyLabelText;
    QTextStream textSteam(&verifyLabelText);
    bool unknownKeyFound = false;
    bool canContinue = true;


    textSteam << "Signed At " << QDateTime::fromTime_t(sign->timestamp).toString() << endl;

    textSteam << endl << "It Contains:" << endl;

    while (sign && canContinue) {

        switch (gpg_err_code(sign->status)) {
            case GPG_ERR_BAD_SIGNATURE:
                textIsSigned = 3;
                verifyStatus = VERIFY_ERROR_CRITICAL;
                textSteam << tr("One or More Bad Signatures.") << endl;
                canContinue = false;
                break;
            case GPG_ERR_NO_ERROR:
                textSteam << tr("A ");
                if(sign->summary & GPGME_SIGSUM_GREEN) {
                    textSteam << tr("Good ");
                }
                if(sign->summary & GPGME_SIGSUM_RED) {
                    textSteam << tr("Bad ");
                }
                if(sign->summary & GPGME_SIGSUM_SIG_EXPIRED) {
                    textSteam << tr("Expired ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_MISSING) {
                    textSteam << tr("Missing Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_REVOKED) {
                    textSteam << tr("Revoked Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_EXPIRED) {
                    textSteam << tr("Expired Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_CRL_MISSING) {
                    textSteam << tr("Missing CRL's ");
                }

                if(sign->summary & GPGME_SIGSUM_VALID) {
                    textSteam << tr("Signature Fully Valid.") << endl;
                } else {
                    textSteam << tr("Signature NOT Fully Valid.") << endl;
                }

                if(!(sign->status & GPGME_SIGSUM_KEY_MISSING)) {
                   unknownKeyFound = printSigner(textSteam, sign);
                } else {
                    textSteam << tr("Key is NOT present with ID 0x") << QString(sign->fpr) << endl;
                }

                break;
            case GPG_ERR_NO_PUBKEY:
                verifyStatus = VERIFY_ERROR_WARN;
                textSteam << tr("A signature could NOT be verified due to a Missing Key\n");
                unknownKeyFound = true;
                break;
            case GPG_ERR_CERT_REVOKED:
                verifyStatus = VERIFY_ERROR_WARN;
                textSteam << tr("A signature is valid but the key used to verify the signature has been revoked\n");
                unknownKeyFound = printSigner(textSteam, sign);
                break;
            case GPG_ERR_SIG_EXPIRED:
                verifyStatus = VERIFY_ERROR_WARN;
                textSteam << tr("A signature is valid but expired\n");
                unknownKeyFound = printSigner(textSteam, sign);
                break;
            case GPG_ERR_KEY_EXPIRED:
                verifyStatus = VERIFY_ERROR_WARN;
                textSteam << tr("A signature is valid but the key used to verify the signature has expired.\n");
                unknownKeyFound = printSigner(textSteam, sign);
                break;
            case GPG_ERR_GENERAL:
                verifyStatus = VERIFY_ERROR_CRITICAL;
                textSteam << tr("There was some other error which prevented the signature verification.\n");
                canContinue = false;
                break;
            default:
                verifyStatus = VERIFY_ERROR_WARN;
                textSteam << tr("Error for key with fingerprint ") <<
                                       GpgME::GpgContext::beautifyFingerprint(QString(sign->fpr));
        }
        textSteam << endl;
        sign = sign->next;
    }


    // If an unknown key is found, enable the importfromkeyserveraction
    this->showImportAction(unknownKeyFound);

    // Remove the last linebreak
    verifyLabelText.remove(verifyLabelText.length() - 1, 1);

    setInfoBoard(verifyLabelText, verifyStatus);

    return true;
}

bool VerifyNotification::printSigner(QTextStream &stream, gpgme_signature_t sign) {
    bool keyFound = true;
    stream << tr("Signed By: ");
    auto key = mCtx->getKeyByFpr(sign->fpr);
    if(!key.good) {
        stream << "<Unknown>";
        keyFound = false;
    }
    stream << key.name;
    if (!key.email.isEmpty()) {
        stream << "<" << key.email <<  ">";
    }

    stream << endl;

    return keyFound;

}