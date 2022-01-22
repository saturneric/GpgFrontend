/**
 * Copyright (C) 2021 Saturneric
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef __KEYMGMT_H__
#define __KEYMGMT_H__

#include "ui/GpgFrontendUI.h"
#include "ui/KeyImportDetailDialog.h"
#include "ui/KeyServerImportDialog.h"
#include "ui/keygen/KeygenDialog.h"
#include "ui/keypair_details/KeyDetailsDialog.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

class KeyMgmt : public QMainWindow {
  Q_OBJECT

 public:
  explicit KeyMgmt(QWidget* parent = nullptr);

 public slots:

  void slotGenerateSubKey();

  void slotExportKeyToKeyPackage();

  void slotExportKeyToClipboard();

  void slotExportAsOpenSSHFormat();

  void slotDeleteSelectedKeys();

  void slotDeleteCheckedKeys();

  void slotGenerateKeyDialog();

  void slotShowKeyDetails();

  void slotSaveWindowState();

  void slotImportKeyPackage();

 signals:

  void signalStatusBarChanged(QString);

  void signalKeyStatusUpdated();

 private:
  void createMenus();

  void createActions();

  void createToolBars();

  void deleteKeysWithWarning(GpgFrontend::KeyIdArgsListPtr uidList);

  KeyList* key_list_;
  QMenu* fileMenu{};
  QMenu* keyMenu{};
  QMenu* generateKeyMenu{};
  QMenu* importKeyMenu{};
  QAction* openKeyFileAct{};
  QAction* exportKeyToFileAct{};
  QAction* exportKeyAsOpenSSHFormat{};
  QAction* exportKeyToClipboardAct{};
  QAction* deleteCheckedKeysAct{};
  QAction* deleteSelectedKeysAct{};
  QAction* generateKeyDialogAct{};
  QAction* generateKeyPairAct{};
  QAction* generateSubKeyAct{};
  QAction* importKeyFromClipboardAct{};
  QAction* importKeyFromFileAct{};
  QAction* importKeyFromKeyServerAct{};
  QAction* importKeysFromKeyPackageAct{};
  QAction* closeAct{};
  QAction* showKeyDetailsAct{};
  KeyServerImportDialog* importDialog{};

 protected:
  void closeEvent(QCloseEvent* event) override;
};

}  // namespace GpgFrontend::UI

#endif  // __KEYMGMT_H__
