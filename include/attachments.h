/*
 *      attachments.h
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

#ifndef __ATTACHMENTS_H__
#define __ATTACHMENTS_H__

#include "attachmenttablemodel.h"
#include <QtGui>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableView;
class QHeaderView;
class QVBoxLayout;
class QMenu;
class QMessageBox;
class QContextMenuEvent;
class QFileDialog;
class QUrl;
class QDesktopServices;
class QSettings;
class QApplication;
QT_END_NAMESPACE

class Attachments : public QWidget
{
    Q_OBJECT

public slots:
    void slotSaveFile();
    void slotOpenFile();

public:
    Attachments(QWidget *parent = nullptr);
    void addMimePart(MimePart *mp);

private:
    void createActions();
    void saveByteArrayToFile(QByteArray outBuffer, QString filename);
    QAction *saveFileAct;
    QAction *openFileAct;
    AttachmentTableModel *table;
    QTableView *tableView;
    QSettings settings;

protected:
    void contextMenuEvent(QContextMenuEvent *event);
};

#endif // __ATTACHMENTS_H__
