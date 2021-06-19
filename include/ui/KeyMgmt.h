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

#ifndef __KEYMGMT_H__
#define __KEYMGMT_H__

#include "ui/widgets/KeyList.h"
#include "ui/keygen/KeygenThread.h"
#include "ui/keypair_details/KeyDetailsDialog.h"
#include "KeyImportDetailDialog.h"
#include "KeyServerImportDialog.h"
#include "ui/keygen/KeygenDialog.h"


class KeyMgmt : public QMainWindow {
Q_OBJECT

public:
    explicit KeyMgmt(GpgME::GpgContext *ctx, QWidget *parent = nullptr);

    QAction *importKeyFromClipboardAct{};
    QAction *importKeyFromFileAct{};
    QAction *importKeyFromKeyServerAct{};

    QAction *generateKeyPairAct{};
    QAction *generateSubKeyAct{};

public slots:

    void slotGenerateSubKey();

    void slotImportKeyFromFile();

    void slotImportKeyFromClipboard();

    void slotImportKeyFromKeyServer();

    void slotImportKeys(QByteArray inBuffer);

    void slotExportKeyToFile();

    void slotExportKeyToClipboard();

    void slotDeleteSelectedKeys();

    void slotDeleteCheckedKeys();

    void slotGenerateKeyDialog();

    void slotShowKeyDetails();

signals:

    void signalStatusBarChanged(QString);

private:
    void createMenus();

    void createActions();

    void createToolBars();

    void deleteKeysWithWarning(QStringList *uidList);

    KeyList *mKeyList;
    GpgME::GpgContext *mCtx;
    QMenu *fileMenu{};
    QMenu *keyMenu{};
    QMenu *generateKeyMenu{};
    QMenu *importKeyMenu{};
    QAction *exportKeyToFileAct{};
    QAction *exportKeyToClipboardAct{};
    QAction *deleteCheckedKeysAct{};
    QAction *deleteSelectedKeysAct{};
    QAction *generateKeyDialogAct{};
    QAction *closeAct{};
    QAction *showKeyDetailsAct{};
    KeyServerImportDialog *importDialog{};

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // __KEYMGMT_H__
