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

#include "ui/keypair_details/KeyPairDetailTab.h"

KeyPairDetailTab::KeyPairDetailTab(GpgME::GpgContext *ctx, const GpgKey &mKey, QWidget *parent) : mKey(mKey), QWidget(parent) {

    mCtx = ctx;
    keyid = new QString(mKey.id);

    ownerBox = new QGroupBox(tr("Owner details"));
    keyBox = new QGroupBox(tr("Key details"));
    fingerprintBox = new QGroupBox(tr("Fingerprint"));
    additionalUidBox = new QGroupBox(tr("Additional Uids"));

    nameVarLabel = new QLabel(mKey.name);
    nameVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    emailVarLabel = new QLabel(mKey.email);
    emailVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    commentVarLabel = new QLabel(mKey.comment);
    commentVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    keyidVarLabel = new QLabel(mKey.id);
    keyidVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QString usage;
    QTextStream usage_steam(&usage);

    if(mKey.can_certify)
        usage_steam << "Cert ";
    if(mKey.can_encrypt)
        usage_steam << "Encr ";
    if(mKey.can_sign)
        usage_steam << "Sign ";
    if(mKey.can_authenticate)
        usage_steam << "Auth ";

    usageVarLabel = new QLabel(usage);

    QString keySizeVal, keyExpireVal, keyCreateTimeVal, keyAlgoVal;

    keySizeVal = QString::number(mKey.length);

    if (mKey.expires.toTime_t() == 0) {
        keyExpireVal = tr("Never Expires");
    } else {
        keyExpireVal = mKey.expires.toString();
    }


    keyAlgoVal = mKey.pubkey_algo;
    keyCreateTimeVal = mKey.create_time.toString();

    keySizeVarLabel = new QLabel(keySizeVal);
    expireVarLabel = new QLabel(keyExpireVal);
    createdVarLabel = new QLabel(keyCreateTimeVal);
    algorithmVarLabel = new QLabel(keyAlgoVal);

    // Show the situation that master key not exists.
    masterKeyExistVarLabel = new QLabel(mKey.has_master_key ? "Exists" : "Not Exists");
    if(!mKey.has_master_key){
        auto paletteExpired = masterKeyExistVarLabel->palette();
        paletteExpired.setColor(masterKeyExistVarLabel->foregroundRole(), Qt::red);
        masterKeyExistVarLabel->setPalette(paletteExpired);
    } else {
        auto paletteValid = masterKeyExistVarLabel->palette();
        paletteValid.setColor(masterKeyExistVarLabel->foregroundRole(), Qt::darkGreen);
        masterKeyExistVarLabel->setPalette(paletteValid);
    }

    if(mKey.expired){
        auto paletteExpired = expireVarLabel->palette();
        paletteExpired.setColor(expireVarLabel->foregroundRole(), Qt::red);
        expireVarLabel->setPalette(paletteExpired);
    } else {
        auto paletteValid = expireVarLabel->palette();
        paletteValid.setColor(expireVarLabel->foregroundRole(), Qt::darkGreen);
        expireVarLabel->setPalette(paletteValid);
    }

    auto *mvbox = new QVBoxLayout();
    auto *vboxKD = new QGridLayout();
    auto *vboxOD = new QGridLayout();

    vboxOD->addWidget(new QLabel(tr("Name:")), 0, 0);
    vboxOD->addWidget(new QLabel(tr("Email Address:")), 1, 0);
    vboxOD->addWidget(new QLabel(tr("Comment:")), 2, 0);
    vboxOD->addWidget(nameVarLabel, 0, 1);
    vboxOD->addWidget(emailVarLabel, 1, 1);
    vboxOD->addWidget(commentVarLabel, 2, 1);

    vboxKD->addWidget(new QLabel(tr("Key ID: ")), 0, 0);
    vboxKD->addWidget(new QLabel(tr("Algorithm: ")), 1, 0);
    vboxKD->addWidget(new QLabel(tr("Key size:")), 2, 0);
    vboxKD->addWidget(new QLabel(tr("Usage: ")), 3, 0);
    vboxKD->addWidget(new QLabel(tr("Expires on: ")), 4, 0);
    vboxKD->addWidget(new QLabel(tr("Last Update: ")), 5, 0);
    vboxKD->addWidget(new QLabel(tr("Existence: ")), 6, 0);


    vboxKD->addWidget(keySizeVarLabel, 2, 1);
    vboxKD->addWidget(expireVarLabel, 4, 1);
    vboxKD->addWidget(algorithmVarLabel, 1, 1);
    vboxKD->addWidget(createdVarLabel, 5, 1);
    vboxKD->addWidget(keyidVarLabel, 0, 1);
    vboxKD->addWidget(usageVarLabel, 3, 1);
    vboxKD->addWidget(masterKeyExistVarLabel, 6, 1);

    ownerBox->setLayout(vboxOD);
    mvbox->addWidget(ownerBox);
    keyBox->setLayout(vboxKD);
    mvbox->addWidget(keyBox);

    fingerPrintVarLabel = new QLabel(beautifyFingerprint(mKey.fpr));
    fingerPrintVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fingerPrintVarLabel->setStyleSheet("margin-left: 0; margin-right: 5;");
    auto *hboxFP = new QHBoxLayout();

    hboxFP->addWidget(fingerPrintVarLabel);

    auto *copyFingerprintButton = new QPushButton(tr("Copy"));
    copyFingerprintButton->setFlat(true);
    copyFingerprintButton->setToolTip(tr("copy fingerprint to clipboard"));
    connect(copyFingerprintButton, SIGNAL(clicked()), this, SLOT(slotCopyFingerprint()));

    hboxFP->addWidget(copyFingerprintButton);

    fingerprintBox->setLayout(hboxFP);
    mvbox->addWidget(fingerprintBox);
    mvbox->addStretch();

    if (mKey.is_private_key) {
        auto *privKeyBox = new QGroupBox(tr("Operations"));
        auto *vboxPK = new QVBoxLayout();

        auto *exportButton = new QPushButton(tr("Export Private Key"));
        vboxPK->addWidget(exportButton);
        connect(exportButton, SIGNAL(clicked()), this, SLOT(slotExportPrivateKey()));

        auto *editExpiresButton = new QPushButton(tr("Modify Expiration Datetime"));
        vboxPK->addWidget(editExpiresButton);
        connect(editExpiresButton, SIGNAL(clicked()), this, SLOT(slotModifyEditDatetime()));

        privKeyBox->setLayout(vboxPK);
        mvbox->addWidget(privKeyBox);


    }

    if ((mKey.expired) || (mKey.revoked)) {
        auto *expBox = new QHBoxLayout();
        QIcon icon = QIcon::fromTheme("dialog-warning");
        QPixmap pixmap = icon.pixmap(QSize(32, 32), QIcon::Normal, QIcon::On);

        auto *expLabel = new QLabel();
        auto *iconLabel = new QLabel();
        if (mKey.expired) {
            expLabel->setText(tr("Warning: The master key of the key pair has expired."));
        }
        if (mKey.revoked) {
            expLabel->setText(tr("Warning: The master key of the key pair has been revoked"));
        }

        iconLabel->setPixmap(pixmap);
        QFont font = expLabel->font();
        font.setBold(true);
        expLabel->setFont(font);
        expLabel->setAlignment(Qt::AlignCenter);
        expBox->addWidget(iconLabel);
        expBox->addWidget(expLabel);
        mvbox->addLayout(expBox);
    }

    connect(mCtx, SIGNAL(signalKeyInfoChanged()), this, SLOT(slotRefreshKeyInfo()));

    setAttribute(Qt::WA_DeleteOnClose, true);
    setLayout(mvbox);
}

void KeyPairDetailTab::slotExportPrivateKey() {
    // Show a information box with explanation about private key
    int ret = QMessageBox::information(this, tr("Exporting private Key"),
                                       tr("You are about to export your private key.\n"
                                          "This is NOT your public key, so don't give it away.\n"
                                          "Make sure you keep it save."
                                          "Do you really want to export your private key?"),
                                       QMessageBox::Cancel | QMessageBox::Ok);

    // export key, if ok was clicked
    if (ret == QMessageBox::Ok) {
        auto *keyArray = new QByteArray();
        mCtx->exportSecretKey(*keyid, keyArray);
        auto &key = mCtx->getKeyById(*keyid);
        QString fileString = key.name + " " +key.email + "(" +
                             key.id + ")_pub_sec.asc";
        QString fileName = QFileDialog::getSaveFileName(this, tr("Export Key To File"), fileString,
                                                        tr("Key Files") + " (*.asc *.txt);;All Files (*)");
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(nullptr, tr("Export error"), tr("Couldn't open %1 for writing").arg(fileName));
            return;
        }
        QTextStream stream(&file);
        stream << *keyArray;
        file.close();
        delete keyArray;
    }
}

QString KeyPairDetailTab::beautifyFingerprint(QString fingerprint) {
    uint len = fingerprint.length();
    if ((len > 0) && (len % 4 == 0))
        for (uint n = 0; 4 * (n + 1) < len; ++n)
            fingerprint.insert(static_cast<int>(5u * n + 4u), ' ');
    return fingerprint;
}

void KeyPairDetailTab::slotCopyFingerprint() {
    QString fpr = fingerPrintVarLabel->text().trimmed().replace(" ", "");
    QClipboard *cb = QApplication::clipboard();
    cb->setText(fpr);
}

void KeyPairDetailTab::slotModifyEditDatetime() {
    auto dialog = new KeySetExpireDateDialog(mCtx, mKey, nullptr, this);
    dialog->show();
}

void KeyPairDetailTab::slotRefreshKeyInfo() {

    nameVarLabel->setText(mKey.name);
    emailVarLabel->setText(mKey.email);

    commentVarLabel->setText(mKey.comment);
    keyidVarLabel->setText(mKey.id);

    QString usage;
    QTextStream usage_steam(&usage);

    if(mKey.can_certify)
        usage_steam << "Cert ";
    if(mKey.can_encrypt)
        usage_steam << "Encr ";
    if(mKey.can_sign)
        usage_steam << "Sign ";
    if(mKey.can_authenticate)
        usage_steam << "Auth ";

    usageVarLabel->setText(usage);

    QString keySizeVal, keyExpireVal, keyCreateTimeVal, keyAlgoVal;

    keySizeVal = QString::number(mKey.length);

    if (mKey.expires.toTime_t() == 0) {
        keyExpireVal = tr("Never Expired");
    } else {
        keyExpireVal = mKey.expires.toString();
    }

    keyAlgoVal = mKey.pubkey_algo;
    keyCreateTimeVal = mKey.create_time.toString();

    keySizeVarLabel->setText(keySizeVal);
    expireVarLabel->setText(keyExpireVal);
    createdVarLabel->setText(keyCreateTimeVal);
    algorithmVarLabel->setText(keyAlgoVal);

    fingerPrintVarLabel->setText(beautifyFingerprint(mKey.fpr));

}

