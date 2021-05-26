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
#include "ui/KeyImportDetailDialog.h"


struct KeyListRow {

    using KeyType = unsigned int;

    static const KeyType SECRET_OR_PUBLIC_KEY = 0;
    static const KeyType ONLY_SECRET_KEY = 1;

};

struct KeyListColumn {

    using InfoType = unsigned int;

    static constexpr InfoType ALL = ~0;
    static constexpr InfoType TYPE = 1 << 0;
    static constexpr InfoType NAME = 1 << 1;
    static constexpr InfoType EmailAddress = 1 << 2;
    static constexpr InfoType Usage = 1 << 3;
    static constexpr InfoType Validity = 1 << 4;
    static constexpr InfoType FingerPrint = 1 << 5;

};


class KeyList : public QWidget {
Q_OBJECT

public:

    explicit KeyList(GpgME::GpgContext *ctx,
                     KeyListRow::KeyType selectType = KeyListRow::SECRET_OR_PUBLIC_KEY,
                     KeyListColumn::InfoType infoType = KeyListColumn::ALL,
                     QWidget *parent = nullptr);

    void setExcludeKeys(std::initializer_list<QString> key_ids);

    void setColumnWidth(int row, int size);

    void addMenuAction(QAction *act);

    QStringList *getChecked();

    void getCheckedKeys(QVector<GpgKey> &keys);

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
    QVector<GpgKey> buffered_keys;
    KeyListRow::KeyType mSelectType;
    KeyListColumn::InfoType mInfoType;
    QVector<QString> excluded_key_ids;


private slots:

    void uploadFinished();


protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dropEvent(QDropEvent *event) override;
};

#endif // __KEYLIST_H__
