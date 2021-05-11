/*
 *      verifydetailbox.h
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

#ifndef __VERIFYKEYDETAILBOX_H__
#define __VERIFYKEYDETAILBOX_H__

#include "keylist.h"
#include "keyserverimportdialog.h"
#include <QDialog>
#include <QGroupBox>

class VerifyKeyDetailBox: public QGroupBox
{
    Q_OBJECT
public:
    explicit VerifyKeyDetailBox(QWidget *parent, GpgME::GpgContext* ctx, KeyList* mKeyList,  gpgme_signature_t signature);

private slots:
    void slotImportFormKeyserver();

private:
    GpgME::GpgContext* mCtx;
    KeyList* mKeyList;
    QString beautifyFingerprint(QString fingerprint);
    QString fpr;
};

#endif // __VERIFYKEYDETAILBOX_H__

