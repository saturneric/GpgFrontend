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

#include "ui/keypair_details/KeyPairSubkeyTab.h"

KeyPairSubkeyTab::KeyPairSubkeyTab(GpgME::GpgContext *ctx, const GpgKey &key, QWidget *parent) : mCtx(ctx), key(key), QWidget(parent) {

}

void KeyPairSubkeyTab::creatSubkeyList() {
    subkeyList = new QTableWidget(this);
    subkeyList->setColumnCount(5);
    subkeyList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    subkeyList->verticalHeader()->hide();
    subkeyList->setShowGrid(false);
    subkeyList->setSelectionBehavior(QAbstractItemView::SelectRows);

    // tableitems not editable
    subkeyList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // no focus (rectangle around tableitems)
    // may be it should focus on whole row
    subkeyList->setFocusPolicy(Qt::NoFocus);
    subkeyList->setAlternatingRowColors(true);

    QStringList labels;
    labels << tr("Subkey ID") << tr("Key Size") << tr("Algo") << tr("Create Time") << tr("Expire Time");

    subkeyList->setHorizontalHeaderLabels(labels);
    subkeyList->horizontalHeader()->setStretchLastSection(true);
}
