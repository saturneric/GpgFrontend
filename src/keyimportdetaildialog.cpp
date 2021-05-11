/*
 *      keyimportdetailsdialog.cpp
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

#include "keyimportdetaildialog.h"

KeyImportDetailDialog::KeyImportDetailDialog(GpgME::GpgContext* ctx, GpgImportInformation result, QWidget *parent)
    : QDialog(parent)
{
    mCtx = ctx;
    mResult = result;
    // If no key for import found, just show a message
    if (mResult.considered == 0) {
        QMessageBox::information(0, tr("Key import details"), tr("No keys found to import"));
        return;
    }

    QVBoxLayout *mvbox = new QVBoxLayout();

    this->createGeneralInfoBox();
    mvbox->addWidget(generalInfoBox);

    this->createKeysTable();
    mvbox->addWidget(keysTable);

    this->createButtonBox();
    mvbox->addWidget(buttonBox);

    this->setLayout(mvbox);
    this->setWindowTitle(tr("Key import details"));
    this->resize(QSize(600,300));
    this->setModal(true);
    this->exec();
}

void KeyImportDetailDialog::createGeneralInfoBox()
{
    // GridBox for general import information
    generalInfoBox = new QGroupBox(tr("Genral key import info"));
    QGridLayout *generalInfoBoxLayout = new QGridLayout(generalInfoBox);

    generalInfoBoxLayout->addWidget(new QLabel(tr("Considered:")),1,0);
    generalInfoBoxLayout->addWidget(new QLabel(QString::number(mResult.considered)),1,1);
    int row=2;
    if (mResult.unchanged != 0){
        generalInfoBoxLayout->addWidget(new QLabel(tr("Public unchanged:")),row,0);
        generalInfoBoxLayout->addWidget(new QLabel(QString::number(mResult.unchanged)),row,1);
        row++;
    }
    if (mResult.imported != 0){
        generalInfoBoxLayout->addWidget(new QLabel(tr("Imported:")),row,0);
        generalInfoBoxLayout->addWidget(new QLabel(QString::number(mResult.imported)),row,1);
        row++;
    }
    if (mResult.not_imported != 0){
        generalInfoBoxLayout->addWidget(new QLabel(tr("Not imported:")),row,0);
        generalInfoBoxLayout->addWidget(new QLabel(QString::number(mResult.not_imported)),row,1);
        row++;
    }
    if (mResult.secret_read != 0){
        generalInfoBoxLayout->addWidget(new QLabel(tr("Private read:")),row,0);
        generalInfoBoxLayout->addWidget(new QLabel(QString::number(mResult.secret_read)),row,1);
        row++;
    }
    if (mResult.secret_imported != 0){
        generalInfoBoxLayout->addWidget(new QLabel(tr("Private imported:")),row,0);
        generalInfoBoxLayout->addWidget(new QLabel(QString::number(mResult.secret_imported)),row,1);
        row++;
    }
    if (mResult.secret_unchanged != 0){
        generalInfoBoxLayout->addWidget(new QLabel(tr("Private unchanged:")),row,0);
        generalInfoBoxLayout->addWidget(new QLabel(QString::number(mResult.secret_unchanged)),row,1);
        row++;
    }
}

void KeyImportDetailDialog::createKeysTable()
{  
    keysTable = new QTableWidget(this);
    keysTable->setRowCount(0);
    keysTable->setColumnCount(4);
    keysTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Nothing is selectable
    keysTable->setSelectionMode(QAbstractItemView::NoSelection);

    QStringList headerLabels;
    headerLabels  << tr("Name") << tr("Email") << tr("Status") << tr("Fingerprint");
    keysTable->verticalHeader()->hide();

    keysTable->setHorizontalHeaderLabels(headerLabels);
    int row = 0;
    foreach (GpgImportedKey impKey, mResult.importedKeys) {
        keysTable->setRowCount(row+1);
        GpgKey key = mCtx->getKeyByFpr(impKey.fpr);
        keysTable->setItem(row, 0, new QTableWidgetItem(key.name));
        keysTable->setItem(row, 1, new QTableWidgetItem(key.email));
        keysTable->setItem(row, 2 ,new QTableWidgetItem(getStatusString(impKey.importStatus)));
        keysTable->setItem(row, 3, new QTableWidgetItem(impKey.fpr));
        row++;
    }
    keysTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    keysTable->horizontalHeader()->setStretchLastSection(true);
    keysTable->resizeColumnsToContents();
}

QString KeyImportDetailDialog::getStatusString(int keyStatus)
{
    QString statusString;
    // keystatus is greater than 15, if key is private
    if (keyStatus > 15) {
        statusString.append(tr("private"));
        keyStatus=keyStatus-16;
    } else {
        statusString.append(tr("public"));
    }
    if (keyStatus == 0) {
        statusString.append(", "+tr("unchanged"));
    } else {
        if (keyStatus == 1) {
            statusString.append(", "+tr("new key"));
        } else {
            if (keyStatus > 7) {
                statusString.append(", "+tr("new subkey"));
                keyStatus=keyStatus-8;
            }
            if (keyStatus > 3) {
                statusString.append(", "+tr("new signature"));
                keyStatus=keyStatus-4;
            }
            if (keyStatus > 1) {
                statusString.append(", "+tr("new uid"));
                keyStatus=keyStatus-2;
            }
        }
    }
    return statusString;
}

void KeyImportDetailDialog::createButtonBox()
{
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(close()));
}
