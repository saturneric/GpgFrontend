/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPGFRONTEND_KEYPAIRSUBKEYTAB_H
#define GPGFRONTEND_KEYPAIRSUBKEYTAB_H

#include "GpgFrontend.h"

#include "gpg/GpgContext.h"
#include "ui/keygen/SubkeyGenerateDialog.h"

class KeyPairSubkeyTab : public QWidget {
Q_OBJECT

public:

    KeyPairSubkeyTab(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent);

private:

    void creatSubkeyList();

    GpgME::GpgContext *mCtx;
    const GpgKey &mKey;
    QTableWidget *subkeyList;
    QVector<const GpgSubKey *> buffered_subkeys;

    QGroupBox *listBox;
    QGroupBox *detailBox;


    QLabel *keySizeVarLabel; /** Label containng the keys keysize */
    QLabel *expireVarLabel; /** Label containng the keys expiration date */
    QLabel *createdVarLabel; /** Label containng the keys creation date */
    QLabel *algorithmVarLabel; /** Label containng the keys algorithm */
    QLabel *keyidVarLabel;  /** Label containng the keys keyid */
    QLabel *fingerPrintVarLabel; /** Label containng the keys fingerprint */
    QLabel *usageVarLabel;


private slots:

    void slotAddSubkey();

    void slotRefreshSubkeyList();

    void slotRefreshSubkeyDetail();

};


#endif //GPGFRONTEND_KEYPAIRSUBKEYTAB_H
