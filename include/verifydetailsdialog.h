/*
 *      verifydetailsdialog.h
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

#ifndef __VERIFYDETAILSDIALOG_H__
#define __VERIFYDETAILSDIALOG_H__

#include "editorpage.h"
#include "verifykeydetailbox.h"
#include <QDialog>

class VerifyDetailsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VerifyDetailsDialog(QWidget *parent, GpgME::GpgContext* ctx, KeyList* mKeyList, QByteArray* inputData, QByteArray* inputSignature = 0);

private slots:
    void slotRefresh();

private:
    GpgME::GpgContext *mCtx;
    KeyList *mKeyList;
    QHBoxLayout *mainLayout;
    QWidget *mVbox;
    QByteArray* mInputData; /** Data to be verified */
    QByteArray* mInputSignature; /** Data to be verified */
    QDialogButtonBox* buttonBox;
};

#endif // __VERIFYDETAILSDIALOG_H__
