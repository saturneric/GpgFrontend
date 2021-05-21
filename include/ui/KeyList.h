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

#ifndef __KEYLIST_H__
#define __KEYLIST_H__

#include "gpg/GpgContext.h"
#include "KeyImportDetailDialog.h"


class KeyList : public QWidget {
Q_OBJECT

public:
    explicit KeyList(GpgME::GpgContext *ctx, QWidget *parent = nullptr);

    void setColumnWidth(int row, int size);

    void addMenuAction(QAction *act);

    QStringList *getChecked();

    QStringList *getPrivateChecked();

    QStringList *getAllPrivateKeys();

    void setChecked(QStringList *keyIds);

    //QStringList *getPrivateChecked();
    QStringList *getSelected();

    [[maybe_unused]] static void markKeys(QStringList *keyIds);

    [[maybe_unused]] bool containsPrivateKeys();

public slots:

    void slotRefresh();

    void uploadKeyToServer(QByteArray *keys);

private:
    void importKeys(QByteArray inBuffer);

    GpgME::GpgContext *mCtx;
    QTableWidget *mKeyList;
    QMenu *popupMenu;
    QNetworkAccessManager *qnam{};

private slots:

    void uploadFinished();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dropEvent(QDropEvent *event) override;
};

#endif // __KEYLIST_H__
