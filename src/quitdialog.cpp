/*
 *
 *      keymgmt.cpp
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

#include "quitdialog.h"

QuitDialog::QuitDialog(QWidget *parent, QHash<int, QString> unsavedDocs)
    : QDialog(parent)
{
    setWindowTitle(tr("Unsaved files"));
    setModal(true);
    discarded =false;

    /*
     * Table of unsaved documents
     */
    QHashIterator<int, QString> i (unsavedDocs);
    int row = 0;
    mFileList = new QTableWidget(this);
    mFileList->horizontalHeader()->hide();
    mFileList->setColumnCount(3);
    mFileList->setColumnWidth(0, 20);
    mFileList->setColumnHidden(2, true);
    mFileList->verticalHeader()->hide();
    mFileList->setShowGrid(false);
    mFileList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mFileList->setFocusPolicy(Qt::NoFocus);
    mFileList->horizontalHeader()->setStretchLastSection( true );
    // fill the table
    i.toBack(); //jump to the end of list to fill the table backwards
    while (i.hasPrevious()) {
        i.previous();
        mFileList->setRowCount(mFileList->rowCount()+1);

        // checkbox in front of filename
        QTableWidgetItem *tmp0 = new QTableWidgetItem();
        tmp0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
        tmp0->setCheckState(Qt::Checked);
        mFileList->setItem(row, 0, tmp0);

        // filename
        QTableWidgetItem *tmp1 = new QTableWidgetItem(i.value());
        mFileList->setItem(row, 1, tmp1);

        // tab-index in hidden column
        QTableWidgetItem *tmp2 = new QTableWidgetItem(QString::number(i.key()));
        mFileList->setItem(row, 2, tmp2);
        ++row;
    }
    /*
     *  Warnbox with icon and text
     */
    QPixmap *pixmap = new QPixmap(":error.png");
    QLabel *warnicon = new QLabel();
    warnicon->setPixmap(*pixmap);
    QLabel *warnlabel = new QLabel(tr("<h3>%1 files contain unsaved information.<br/>Save the changes before closing?</h3>").arg(row));
    QHBoxLayout *warnBoxLayout = new QHBoxLayout();
    warnBoxLayout->addWidget(warnicon);
    warnBoxLayout->addWidget(warnlabel);
    warnBoxLayout->setAlignment(Qt::AlignLeft);
    QWidget *warnBox = new QWidget(this);
    warnBox->setLayout(warnBoxLayout);

    /*
     *  Two labels on top and under the filelist
     */
    QLabel *checkLabel = new QLabel(tr("Check the files you want to save:"));
    QLabel *notelabel = new QLabel(tr("<b>Note:</b> If you don't save these files, all changes are lost.<br/>"));

    /*
     *  Buttonbox
     */
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Discard |QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QPushButton* btnNoKey = buttonBox->button(QDialogButtonBox::Discard);
    connect(btnNoKey, SIGNAL(clicked()), SLOT(slotMyDiscard()));

    /*
     *  Set the layout
     */
    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->addWidget(warnBox);
    vbox->addWidget(checkLabel);
    vbox->addWidget(mFileList);
    vbox->addWidget(notelabel);
    vbox->addWidget(buttonBox);
    this->setLayout(vbox);
}


void QuitDialog::slotMyDiscard()
{
    discarded =true;
    reject();
}

bool QuitDialog::isDiscarded()
{
    return discarded;
}

QList <int> QuitDialog::getTabIdsToSave()
{
    QList <int> tabIdsToSave;
    for (int i = 0; i < mFileList->rowCount(); i++) {
        if (mFileList->item(i, 0)->checkState() == Qt::Checked) {
            tabIdsToSave << mFileList->item(i, 2)->text().toInt();
        }
    }
    return tabIdsToSave;
}

