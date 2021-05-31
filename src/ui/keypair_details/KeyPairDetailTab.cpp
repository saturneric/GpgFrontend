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

KeyPairDetailTab::KeyPairDetailTab(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent) : QWidget(parent) {

    mCtx = ctx;
    keyid = new QString(key.id);

    ownerBox = new QGroupBox(tr("Owner details"));
    keyBox = new QGroupBox(tr("Key details"));
    fingerprintBox = new QGroupBox(tr("Fingerprint"));
    additionalUidBox = new QGroupBox(tr("Additional Uids"));

    nameVarLabel = new QLabel(key.name);
    nameVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    emailVarLabel = new QLabel(key.email);
    emailVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    commentVarLabel = new QLabel(key.comment);
    commentVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    keyidVarLabel = new QLabel(key.id);
    keyidVarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QString usage;
    QTextStream usage_steam(&usage);

    if(key.can_certify)
        usage_steam << "Cert ";
    if(key.can_encrypt)
        usage_steam << "Encr ";
    if(key.can_sign)
        usage_steam << "Sign ";
    if(key.can_authenticate)
        usage_steam << "Auth ";

    usageVarLabel = new QLabel(usage);

    QString keySizeVal, keyExpireVal, keyCreateTimeVal, keyAlgoVal;

    keySizeVal = QString::number(key.length);

    if (key.expires.toTime_t() == 0) {
        keyExpireVal = tr("Never Expired");
    } else {
        keyExpireVal = key.expires.toString();
    }

    keyAlgoVal = key.pubkey_algo;
    keyCreateTimeVal = key.create_time.toString();

    keySizeVarLabel = new QLabel(keySizeVal);
    expireVarLabel = new QLabel(keyExpireVal);
    createdVarLabel = new QLabel(keyCreateTimeVal);
    algorithmVarLabel = new QLabel(keyAlgoVal);

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


    vboxKD->addWidget(keySizeVarLabel, 2, 1);
    vboxKD->addWidget(expireVarLabel, 4, 1);
    vboxKD->addWidget(algorithmVarLabel, 1, 1);
    vboxKD->addWidget(createdVarLabel, 5, 1);
    vboxKD->addWidget(keyidVarLabel, 0, 1);
    vboxKD->addWidget(usageVarLabel, 3, 1);

    ownerBox->setLayout(vboxOD);
    mvbox->addWidget(ownerBox);

    keyBox->setLayout(vboxKD);
    mvbox->addWidget(keyBox);

    fingerPrintVarLabel = new QLabel(beautifyFingerprint(key.fpr));
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

    if (key.is_private_key) {
        auto *privKeyBox = new QGroupBox(tr("Private Key"));
        auto *vboxPK = new QVBoxLayout();

        auto *exportButton = new QPushButton(tr("Export Private Key"));
        vboxPK->addWidget(exportButton);
        connect(exportButton, SIGNAL(clicked()), this, SLOT(slotExportPrivateKey()));

        privKeyBox->setLayout(vboxPK);
        mvbox->addWidget(privKeyBox);
    }

    if ((key.expired) || (key.revoked)) {
        auto *expBox = new QHBoxLayout();
        QIcon icon = QIcon::fromTheme("dialog-warning");
        QPixmap pixmap = icon.pixmap(QSize(32, 32), QIcon::Normal, QIcon::On);

        auto *expLabel = new QLabel();
        auto *iconLabel = new QLabel();
        if (key.expired) {
            expLabel->setText(tr("Warning: Key expired"));
        }
        if (key.revoked) {
            expLabel->setText(tr("Warning: Key revoked"));
        }

        iconLabel->setPixmap(pixmap);
        QFont font = expLabel->font();
        font.setBold(true);
        expLabel->setFont(font);
        expBox->addWidget(iconLabel);
        expBox->addWidget(expLabel);
        mvbox->addLayout(expBox);
    }

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

