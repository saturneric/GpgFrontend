/*
 *      keylist.cpp
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

#include "keylist.h"

KeyList::KeyList(GpgME::GpgContext *ctx, QWidget *parent)
        : QWidget(parent)
{
    mCtx = ctx;

    mKeyList = new QTableWidget(this);
    mKeyList->setColumnCount(6);
    mKeyList->verticalHeader()->hide();
    mKeyList->setShowGrid(false);
    mKeyList->setColumnWidth(0, 24);
    mKeyList->setColumnWidth(1, 20);
    mKeyList->sortByColumn(2, Qt::AscendingOrder);
    mKeyList->setSelectionBehavior(QAbstractItemView::SelectRows);
    // hide id and fingerprint of key
    mKeyList->setColumnHidden(4, true);
    mKeyList->setColumnHidden(5, true);

    // tableitems not editable
    mKeyList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // no focus (rectangle around tableitems)
    // may be it should focus on whole row
    mKeyList->setFocusPolicy(Qt::NoFocus);

    mKeyList->setAlternatingRowColors(true);

    QStringList labels;
    labels << "" << "" << tr("Name") << tr("EMail") << "id" << "fpr";
    mKeyList->setHorizontalHeaderLabels(labels);
    mKeyList->horizontalHeader()->setStretchLastSection(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mKeyList);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    setLayout(layout);

    popupMenu = new QMenu(this);
    connect(mCtx, SIGNAL(signalKeyDBChanged()), this, SLOT(slotRefresh()));
    setAcceptDrops(true);
    slotRefresh();
}

void KeyList::slotRefresh()
{
    QStringList *keyList;
    keyList = getChecked();
    // while filling the table, sort enabled causes errors
    mKeyList->setSortingEnabled(false);
    mKeyList->clearContents();

    GpgKeyList keys = mCtx->listKeys();
    mKeyList->setRowCount(keys.size());

    int row = 0;
    GpgKeyList::iterator it = keys.begin();
    while (it != keys.end()) {

        QTableWidgetItem *tmp0 = new QTableWidgetItem();
        tmp0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        tmp0->setCheckState(Qt::Unchecked);
        mKeyList->setItem(row, 0, tmp0);

        if (it->privkey) {
            QTableWidgetItem *tmp1 = new QTableWidgetItem(QIcon(":kgpg_key2.png"), "");
            mKeyList->setItem(row, 1, tmp1);
        }
        QTableWidgetItem *tmp2 = new QTableWidgetItem(it->name);
        tmp2->setToolTip(it->name);
        mKeyList->setItem(row, 2, tmp2);
        QTableWidgetItem *tmp3 = new QTableWidgetItem(it->email);
        tmp3->setToolTip(it->email);
        // strike out expired keys
        if(it->expired || it->revoked) {
            QFont strike = tmp2->font();
            strike.setStrikeOut(true);
            tmp2->setFont(strike);
            tmp3->setFont(strike);
        }
        mKeyList->setItem(row, 3, tmp3);
        QTableWidgetItem *tmp4 = new QTableWidgetItem(it->id);
        mKeyList->setItem(row, 4, tmp4);
        QTableWidgetItem *tmp5 = new QTableWidgetItem(it->fpr);
        mKeyList->setItem(row, 5, tmp5);


        it++;
        ++row;
    }
    mKeyList->setSortingEnabled(true);
    setChecked(keyList);
}

QStringList *KeyList::getChecked()
{
    QStringList *ret = new QStringList();
    for (int i = 0; i < mKeyList->rowCount(); i++) {
        if (mKeyList->item(i, 0)->checkState() == Qt::Checked) {
            *ret << mKeyList->item(i, 4)->text();
        }
    }
    return ret;
}

QStringList *KeyList::getAllPrivateKeys()
{
    QStringList *ret = new QStringList();
    for (int i = 0; i < mKeyList->rowCount(); i++) {
        if (mKeyList->item(i, 1)) {
            *ret << mKeyList->item(i, 4)->text();
        }
    }
    return ret;
}

QStringList *KeyList::getPrivateChecked()
{
    QStringList *ret = new QStringList();
    for (int i = 0; i < mKeyList->rowCount(); i++) {
        if ((mKeyList->item(i, 0)->checkState() == Qt::Checked) && (mKeyList->item(i, 1))) {
            *ret << mKeyList->item(i, 4)->text();
        }
    }
    return ret;
}

void KeyList::setChecked(QStringList *keyIds)
{
    if (!keyIds->isEmpty()) {
        for (int i = 0; i < mKeyList->rowCount(); i++) {
            if (keyIds->contains(mKeyList->item(i, 4)->text()))  {
                mKeyList->item(i, 0)->setCheckState(Qt::Checked);
            }
        }
    }
}

QStringList *KeyList::getSelected()
{
    QStringList *ret = new QStringList();

    for (int i = 0; i < mKeyList->rowCount(); i++) {
        if (mKeyList->item(i, 0)->isSelected() == 1) {
            *ret << mKeyList->item(i, 4)->text();
        }
    }
    return ret;
}

bool KeyList::containsPrivateKeys()
{
    for (int i = 0; i < mKeyList->rowCount(); i++) {
        if (mKeyList->item(i, 1)) {
            return  true;
        }
    }
    return false;
}

void KeyList::setColumnWidth(int row, int size)
{
    mKeyList->setColumnWidth(row, size);
}

void KeyList::contextMenuEvent(QContextMenuEvent *event)
{
    popupMenu->exec(event->globalPos());
}

void KeyList::addMenuAction(QAction *act)
{
    popupMenu->addAction(act);
}

void KeyList::dropEvent(QDropEvent* event)
{
//    importKeyDialog();
    QSettings settings;

    QDialog *dialog = new QDialog();

    dialog->setWindowTitle(tr("Import Keys"));
    QLabel *label;
    label = new QLabel(tr("You've dropped something on the keylist.\n gpg4usb will now try to import key(s).")+"\n");

    // "always import keys"-CheckBox
    QCheckBox *checkBox = new QCheckBox(tr("Always import without bothering."));
    if (settings.value("general/confirmImportKeys").toBool()) checkBox->setCheckState(Qt::Unchecked);

    // Buttons for ok and cancel
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->addWidget(label);
    vbox->addWidget(checkBox);
    vbox->addWidget(buttonBox);

    dialog->setLayout(vbox);

    if (settings.value("general/confirmImportKeys",Qt::Checked).toBool())
    {
        dialog->exec();
        if (dialog->result() == QDialog::Rejected) {
            return;
        }
        if (checkBox->isChecked()){
           settings.setValue("general/confirmImportKeys", false);
        } else {
            settings.setValue("general/confirmImportKeys", true);

        }
    }

    if (event->mimeData()->hasUrls())
    {
        foreach (QUrl tmp, event->mimeData()->urls())
        {
                QFile file;
                file.setFileName(tmp.toLocalFile());
                if (!file.open(QIODevice::ReadOnly)) {
                    qDebug() << tr("Couldn't Open File: ") + tmp.toString();
                }
                QByteArray inBuffer = file.readAll();
                this->importKeys(inBuffer);
                file.close();
        }
   } else  {
            QByteArray inBuffer(event->mimeData()->text().toUtf8());
            this->importKeys(inBuffer);
  }
}

void KeyList::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

/** set background color for Keys and put them to top
 *
 */
void KeyList::markKeys(QStringList *keyIds)
{
    foreach(QString id, *keyIds) {
        qDebug() << "marked: " << id;
     }
}

void KeyList::importKeys(QByteArray inBuffer)
{
    GpgImportInformation result = mCtx->importKey(inBuffer);
    new KeyImportDetailDialog(mCtx, result, this);
}

void KeyList::uploadKeyToServer(QByteArray *keys)
{
    QUrl reqUrl("http://localhost:11371/pks/add");
    qnam = new QNetworkAccessManager(this);

    QUrl params;
    keys->replace("\n", "%0D%0A")
            .replace("(", "%28")
            .replace(")", "%29")
            .replace("/", "%2F")
            .replace(":", "%3A")
            .replace("+","%2B")
            .replace(' ', '+');

    QUrlQuery q;

    q.addQueryItem("keytext", *keys);

    params = q.query(QUrl::FullyEncoded).toUtf8();

    QNetworkRequest req(reqUrl);

    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QNetworkReply *reply = qnam->post(req,params.toEncoded());
    connect(reply, SIGNAL(finished()),
            this, SLOT(uploadFinished()));
    qDebug() << "REQURL: " << reqUrl;
    qDebug() << "PARAMS.ENCODED: " << params.toEncoded();
}

void KeyList::uploadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QByteArray response = reply->readAll();
    qDebug() << "RESPNOSE: " << response.data();
    //reply->readAll();
    qDebug() << "ERROR: " << reply->error();
    if (reply->error()) {
        qDebug() << "Error while contacting keyserver!";
        return;
    } else {
        qDebug() << "Success while contacting keyserver!";
    }

    reply->deleteLater();
    reply = 0;
}
