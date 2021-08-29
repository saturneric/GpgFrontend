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

#ifndef GPGFRONTEND_KEYPAIRUIDTAB_H
#define GPGFRONTEND_KEYPAIRUIDTAB_H

#include "GpgFrontend.h"
#include "gpg/GpgContext.h"

#include "KeyUIDSignDialog.h"
#include "KeyNewUIDDialog.h"

class KeyPairUIDTab : public QWidget {
Q_OBJECT

public:

    KeyPairUIDTab(GpgFrontend::GpgContext *ctx, const GpgKey &key, QWidget *parent);

private:

    void createUIDList();

    void createSignList();

    void createManageUIDMenu();

    void createUIDPopupMenu();

    void createSignPopupMenu();

    void getUIDChecked(QVector<GpgUID> &uids);

    bool getUIDSelected(GpgUID &uid);

    bool getSignSelected(GpgKeySignature &signature);

    GpgFrontend::GpgContext *mCtx;
    const GpgKey &mKey;
    QTableWidget *uidList{};
    QTableWidget *sigList{};
    QMenu *manageSelectedUIDMenu;
    QMenu *uidPopupMenu;
    QMenu *signPopupMenu;
    QVector<const GpgUID *> buffered_uids;
    QVector<const GpgKeySignature *> buffered_signatures;

private slots:

    void slotRefreshUIDList();

    void slotRefreshSigList();

    void slotAddSign();

    void slotAddSignSingle();

    void slotAddUID();

    void slotDelUID();

    void slotDelUIDSingle();

    void slotSetPrimaryUID();

    void slotDelSign();

    static void slotAddUIDResult(int result);

protected:

    void contextMenuEvent(QContextMenuEvent *event) override;

};


#endif //GPGFRONTEND_KEYPAIRUIDTAB_H
