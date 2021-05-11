/*
 *      keymgmt.h
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

#ifndef __KEYMGMT_H__
#define __KEYMGMT_H__

#include "keylist.h"
#include "keygenthread.h"
#include "keydetailsdialog.h"
#include "keyimportdetaildialog.h"
#include "keyserverimportdialog.h"
#include "keygendialog.h"
#include <QtGui>

QT_BEGIN_NAMESPACE
class QMainWindow;
class iostream;
class QFileDialog;
class QIcon;
class QAction;
class QApplication;
QT_END_NAMESPACE

class KeyMgmt : public QMainWindow
{
    Q_OBJECT

public:
    KeyMgmt(GpgME::GpgContext* ctx, QWidget *parent = 0);
    QAction *importKeyFromClipboardAct;
    QAction *importKeyFromFileAct;
    QAction *importKeyFromKeyServerAct;

public slots:
    void slotImportKeyFromFile();
    void slotImportKeyFromClipboard();
    void slotImportKeyFromKeyServer();
    void slotImportKeys(QByteArray inBuffer);
    void slotExportKeyToFile();
    void slotExportKeyToClipboard();
    void slotDeleteSelectedKeys();
    void slotDeleteCheckedKeys();
    void slotGenerateKeyDialog();
    void slotShowKeyDetails();

signals:
    void signalStatusBarChanged(QString);

private:
    void createMenus();
    void createActions();
    void createToolBars();
    void deleteKeysWithWarning(QStringList *uidList);

    KeyList *mKeyList;
    GpgME::GpgContext *mCtx;
    QMenu *fileMenu;
    QMenu *keyMenu;
    QMenu *importKeyMenu;
    QAction *exportKeyToFileAct;
    QAction *exportKeyToClipboardAct;
    QAction *deleteCheckedKeysAct;
    QAction *deleteSelectedKeysAct;
    QAction *generateKeyDialogAct;
    QAction *closeAct;
    QAction *showKeyDetailsAct;
    QMessageBox msgbox;
    KeyServerImportDialog *importDialog;

protected:
    void closeEvent(QCloseEvent *event);
};

#endif // __KEYMGMT_H__
