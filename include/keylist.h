/*
 *      keylist.h
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

#ifndef __KEYLIST_H__
#define __KEYLIST_H__

#include <include/GPG4USB.h>

#include "gpgcontext.h"
#include "keyimportdetaildialog.h"

QT_BEGIN_NAMESPACE
class QWidget;
class QVBoxLayout;
class QLabel;
class QTableWidget;
class QMenu;
QT_END_NAMESPACE

class KeyList : public QWidget
{
    Q_OBJECT

public:
    KeyList(GpgME::GpgContext *ctx, QWidget *parent = 0);
    void setColumnWidth(int row, int size);
    void addMenuAction(QAction *act);

    QStringList *getChecked();
    QStringList *getPrivateChecked();
    QStringList *getAllPrivateKeys();

    void setChecked(QStringList *keyIds);
    //QStringList *getPrivateChecked();
    QStringList *getSelected();
    void markKeys(QStringList *keyIds);
    bool containsPrivateKeys();

public slots:
    void slotRefresh();
    void uploadKeyToServer(QByteArray *keys);

private:
    void importKeys(QByteArray inBuffer);
    GpgME::GpgContext *mCtx;
    QTableWidget *mKeyList;
    QMenu *popupMenu;
    QNetworkAccessManager *qnam;

private slots:
    void uploadFinished();

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent* event);};

#endif // __KEYLIST_H__
