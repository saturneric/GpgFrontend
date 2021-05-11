/*
 *      keydetailsdialog.h
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

#ifndef __KEYDETAILSDIALOG_H__
#define __KEYDETAILSDIALOG_H__

#include "gpgcontext.h"
#include <gpgme.h>

QT_BEGIN_NAMESPACE
class QDateTime;
class QVBoxLayout;
class QHBoxLayout;
class QDialogButtonBox;
class QDialog;
class QGroupBox;
class QLabel;
class QGridLayout;
class QPushButton;
QT_END_NAMESPACE

class KeyDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    KeyDetailsDialog(GpgME::GpgContext* ctx, gpgme_key_t key, QWidget *parent = 0);

    /**
     * @details Return QString with a space inserted at every fourth character
     *
     * @param fingerprint The fingerprint to be beautified
     */
    static QString beautifyFingerprint(QString fingerprint);

private slots:
    /**
     * @details Export the key to a file, which is choosen in a file dialog
     */
    void slotExportPrivateKey();

    /**
     * @details Copy the fingerprint to clipboard
     */
    void slotCopyFingerprint();

private:
    QString *keyid; /** The id of the key the details should be shown for */
    GpgME::GpgContext *mCtx; /** The current gpg-context */

    QGroupBox *ownerBox; /** Groupbox containing owner information */
    QGroupBox *keyBox; /** Groupbox containing key information */
    QGroupBox *fingerprintBox; /** Groupbox containing fingerprint information */
    QGroupBox *additionalUidBox; /** Groupbox containing information about additional uids */
    QDialogButtonBox *buttonBox; /** Box containing the close button */

    QLabel *nameVarLabel; /** Label containng the keys name */
    QLabel *emailVarLabel; /** Label containng the keys email */
    QLabel *commentVarLabel; /** Label containng the keys commment */
    QLabel *keySizeVarLabel; /** Label containng the keys keysize */
    QLabel *expireVarLabel; /** Label containng the keys expiration date */
    QLabel *createdVarLabel; /** Label containng the keys creation date */
    QLabel *algorithmVarLabel; /** Label containng the keys algorithm */
    QLabel *keyidVarLabel;  /** Label containng the keys keyid */
    QLabel *fingerPrintVarLabel; /** Label containng the keys fingerprint */
    QLabel *addUserIdsVarLabel; /** Label containng info about keys additional uids */
};

#endif // __KEYDETAILSDIALOG_H__
