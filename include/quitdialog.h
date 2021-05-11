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

#ifndef __QUITDIALOG_H__
#define __QUITDIALOG_H__

#include <GPG4USB.h>

class QuitDialog : public QDialog {
Q_OBJECT

public:
    QuitDialog(QWidget *parent, QHash<int, QString> unsavedDocs);

    bool isDiscarded();

    QList<int> getTabIdsToSave();

private slots:

    void slotMyDiscard();

private:
    [[maybe_unused]] QAction *closeAct;
    [[maybe_unused]] QLabel *nameLabel;
    bool discarded;
    QTableWidget *mFileList;
};

#endif // __QUITDIALOG_H__
