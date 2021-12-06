/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
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

#include "MainWindow.h"
#include "ui/UserInterfaceUtils.h"

namespace GpgFrontend::UI {

void MainWindow::createActions() {
  /* Main Menu
   */
  newTabAct = new QAction(_("New"), this);
  newTabAct->setIcon(QIcon(":misc_doc.png"));
  QList<QKeySequence> newTabActShortcutList;
  newTabActShortcutList.append(QKeySequence(Qt::CTRL + Qt::Key_N));
  newTabActShortcutList.append(QKeySequence(Qt::CTRL + Qt::Key_T));
  newTabAct->setShortcuts(newTabActShortcutList);
  newTabAct->setToolTip(_("Open a new file"));
  connect(newTabAct, SIGNAL(triggered()), edit, SLOT(slotNewTab()));

  openAct = new QAction(_("Open..."), this);
  openAct->setIcon(QIcon(":fileopen.png"));
  openAct->setShortcut(QKeySequence::Open);
  openAct->setToolTip(_("Open an existing file"));
  connect(openAct, SIGNAL(triggered()), edit, SLOT(slotOpen()));

  browserAct = new QAction(_("File Browser"), this);
  browserAct->setIcon(QIcon(":file-browser.png"));
  browserAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
  browserAct->setToolTip(_("Open a file browser"));
  connect(browserAct, SIGNAL(triggered()), this, SLOT(slotOpenFileTab()));

  saveAct = new QAction(_("Save"), this);
  saveAct->setIcon(QIcon(":filesave.png"));
  saveAct->setShortcut(QKeySequence::Save);
  saveAct->setToolTip(_("Save the current File"));
  connect(saveAct, SIGNAL(triggered()), edit, SLOT(slotSave()));

  saveAsAct = new QAction(QString(_("Save As")) + "...", this);
  saveAsAct->setIcon(QIcon(":filesaveas.png"));
  saveAsAct->setShortcut(QKeySequence::SaveAs);
  saveAsAct->setToolTip(_("Save the current File as..."));
  connect(saveAsAct, SIGNAL(triggered()), edit, SLOT(slotSaveAs()));

  printAct = new QAction(_("Print"), this);
  printAct->setIcon(QIcon(":fileprint.png"));
  printAct->setShortcut(QKeySequence::Print);
  printAct->setToolTip(_("Print Document"));
  connect(printAct, SIGNAL(triggered()), edit, SLOT(slotPrint()));

  closeTabAct = new QAction(_("Close"), this);
  closeTabAct->setShortcut(QKeySequence::Close);
  closeTabAct->setToolTip(_("Close file"));
  connect(closeTabAct, SIGNAL(triggered()), edit, SLOT(slotCloseTab()));

  quitAct = new QAction(_("Quit"), this);
  quitAct->setShortcut(QKeySequence::Quit);
  quitAct->setIcon(QIcon(":exit.png"));
  quitAct->setToolTip(_("Quit Program"));
  connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

  /* Edit Menu
   */
  undoAct = new QAction(_("Undo"), this);
  undoAct->setShortcut(QKeySequence::Undo);
  undoAct->setToolTip(_("Undo Last Edit Action"));
  connect(undoAct, SIGNAL(triggered()), edit, SLOT(slotUndo()));

  redoAct = new QAction(_("Redo"), this);
  redoAct->setShortcut(QKeySequence::Redo);
  redoAct->setToolTip(_("Redo Last Edit Action"));
  connect(redoAct, SIGNAL(triggered()), edit, SLOT(slotRedo()));

  zoomInAct = new QAction(_("Zoom In"), this);
  zoomInAct->setShortcut(QKeySequence::ZoomIn);
  connect(zoomInAct, SIGNAL(triggered()), edit, SLOT(slotZoomIn()));

  zoomOutAct = new QAction(_("Zoom Out"), this);
  zoomOutAct->setShortcut(QKeySequence::ZoomOut);
  connect(zoomOutAct, SIGNAL(triggered()), edit, SLOT(slotZoomOut()));

  pasteAct = new QAction(_("Paste"), this);
  pasteAct->setIcon(QIcon(":button_paste.png"));
  pasteAct->setShortcut(QKeySequence::Paste);
  pasteAct->setToolTip(_("Paste Text From Clipboard"));
  connect(pasteAct, SIGNAL(triggered()), edit, SLOT(slotPaste()));

  cutAct = new QAction(_("Cut"), this);
  cutAct->setIcon(QIcon(":button_cut.png"));
  cutAct->setShortcut(QKeySequence::Cut);
  cutAct->setToolTip(
      _("Cut the current selection's contents to the "
        "clipboard"));
  connect(cutAct, SIGNAL(triggered()), edit, SLOT(slotCut()));

  copyAct = new QAction(_("Copy"), this);
  copyAct->setIcon(QIcon(":button_copy.png"));
  copyAct->setShortcut(QKeySequence::Copy);
  copyAct->setToolTip(
      _("Copy the current selection's contents to the "
        "clipboard"));
  connect(copyAct, SIGNAL(triggered()), edit, SLOT(slotCopy()));

  quoteAct = new QAction(_("Quote"), this);
  quoteAct->setIcon(QIcon(":quote.png"));
  quoteAct->setToolTip(_("Quote whole text"));
  connect(quoteAct, SIGNAL(triggered()), edit, SLOT(slotQuote()));

  selectAllAct = new QAction(_("Select All"), this);
  selectAllAct->setIcon(QIcon(":edit.png"));
  selectAllAct->setShortcut(QKeySequence::SelectAll);
  selectAllAct->setToolTip(_("Select the whole text"));
  connect(selectAllAct, SIGNAL(triggered()), edit, SLOT(slotSelectAll()));

  findAct = new QAction(_("Find"), this);
  findAct->setShortcut(QKeySequence::Find);
  findAct->setToolTip(_("Find a word"));
  connect(findAct, SIGNAL(triggered()), this, SLOT(slotFind()));

  cleanDoubleLinebreaksAct = new QAction(_("Remove spacing"), this);
  cleanDoubleLinebreaksAct->setIcon(QIcon(":format-line-spacing-triple.png"));
  // cleanDoubleLineBreaksAct->setShortcut(QKeySequence::SelectAll);
  cleanDoubleLinebreaksAct->setToolTip(
      _("Remove double linebreaks, e.g. in pasted text from Web Mailer"));
  connect(cleanDoubleLinebreaksAct, SIGNAL(triggered()), this,
          SLOT(slotCleanDoubleLinebreaks()));

  openSettingsAct = new QAction(_("Settings"), this);
  openSettingsAct->setToolTip(_("Open settings dialog"));
  openSettingsAct->setShortcut(QKeySequence::Preferences);
  connect(openSettingsAct, SIGNAL(triggered()), this,
          SLOT(slotOpenSettingsDialog()));

  /* Crypt Menu
   */
  encryptAct = new QAction(_("Encrypt"), this);
  encryptAct->setIcon(QIcon(":encrypted.png"));
  encryptAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
  encryptAct->setToolTip(_("Encrypt Message"));
  connect(encryptAct, SIGNAL(triggered()), this, SLOT(slotEncrypt()));

  encryptSignAct = new QAction(_("Encrypt Sign"), this);
  encryptSignAct->setIcon(QIcon(":encrypted_signed.png"));
  encryptSignAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_E));
  encryptSignAct->setToolTip(_("Encrypt and Sign Message"));
  connect(encryptSignAct, SIGNAL(triggered()), this, SLOT(slotEncryptSign()));

  decryptAct = new QAction(_("Decrypt"), this);
  decryptAct->setIcon(QIcon(":decrypted.png"));
  decryptAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
  decryptAct->setToolTip(_("Decrypt Message"));
  connect(decryptAct, SIGNAL(triggered()), this, SLOT(slotDecrypt()));

  decryptVerifyAct = new QAction(_("Decrypt Verify"), this);
  decryptVerifyAct->setIcon(QIcon(":decrypted_verified.png"));
  decryptVerifyAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D));
  decryptVerifyAct->setToolTip(_("Decrypt and Verify Message"));
  connect(decryptVerifyAct, SIGNAL(triggered()), this,
          SLOT(slotDecryptVerify()));

  /*
   * File encryption submenu
   */
  fileEncryptAct = new QAction(_("Encrypt File"), this);
  fileEncryptAct->setToolTip(_("Encrypt File"));
  connect(fileEncryptAct, SIGNAL(triggered()), this,
          SLOT(slotFileEncryptCustom()));

  fileDecryptAct = new QAction(_("Decrypt File"), this);
  fileDecryptAct->setToolTip(_("Decrypt File"));
  connect(fileDecryptAct, SIGNAL(triggered()), this,
          SLOT(slotFileDecryptCustom()));

  fileSignAct = new QAction(_("Sign File"), this);
  fileSignAct->setToolTip(_("Sign File"));
  connect(fileSignAct, SIGNAL(triggered()), this, SLOT(slotFileSignCustom()));

  fileVerifyAct = new QAction(_("Verify File"), this);
  fileVerifyAct->setToolTip(_("Verify File"));
  connect(fileVerifyAct, SIGNAL(triggered()), this,
          SLOT(slotFileVerifyCustom()));

  signAct = new QAction(_("Sign"), this);
  signAct->setIcon(QIcon(":signature.png"));
  signAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
  signAct->setToolTip(_("Sign Message"));
  connect(signAct, SIGNAL(triggered()), this, SLOT(slotSign()));

  verifyAct = new QAction(_("Verify"), this);
  verifyAct->setIcon(QIcon(":verify.png"));
  verifyAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V));
  verifyAct->setToolTip(_("Verify Message"));
  connect(verifyAct, SIGNAL(triggered()), this, SLOT(slotVerify()));

  /* Key Menu
   */

  importKeyFromFileAct = new QAction(_("File"), this);
  importKeyFromFileAct->setIcon(QIcon(":import_key_from_file.png"));
  importKeyFromFileAct->setToolTip(_("Import New Key From File"));
  connect(importKeyFromFileAct, &QAction::triggered, this,
          [&]() { CommonUtils::GetInstance()->slotImportKeyFromFile(this); });

  importKeyFromClipboardAct = new QAction(_("Clipboard"), this);
  importKeyFromClipboardAct->setIcon(QIcon(":import_key_from_clipboard.png"));
  importKeyFromClipboardAct->setToolTip(_("Import New Key From Clipboard"));
  connect(importKeyFromClipboardAct, &QAction::triggered, this, [&]() {
    CommonUtils::GetInstance()->slotImportKeyFromClipboard(this);
  });

  importKeyFromKeyServerAct = new QAction(_("Keyserver"), this);
  importKeyFromKeyServerAct->setIcon(QIcon(":import_key_from_server.png"));
  importKeyFromKeyServerAct->setToolTip(_("Import New Key From Keyserver"));
  connect(importKeyFromKeyServerAct, &QAction::triggered, this, [&]() {
    CommonUtils::GetInstance()->slotImportKeyFromKeyServer(this);
  });

  importKeyFromEditAct = new QAction(_("Editor"), this);
  importKeyFromEditAct->setIcon(QIcon(":txt.png"));
  importKeyFromEditAct->setToolTip(_("Import New Key From Editor"));
  connect(importKeyFromEditAct, SIGNAL(triggered()), this,
          SLOT(slotImportKeyFromEdit()));

  openKeyManagementAct = new QAction(_("Manage Keys"), this);
  openKeyManagementAct->setIcon(QIcon(":keymgmt.png"));
  openKeyManagementAct->setToolTip(_("Open Key Management"));
  connect(openKeyManagementAct, SIGNAL(triggered()), this,
          SLOT(slotOpenKeyManagement()));

  /*
   * About Menu
   */
  aboutAct = new QAction(_("About"), this);
  aboutAct->setIcon(QIcon(":help.png"));
  aboutAct->setToolTip(_("Show the application's About box"));
  connect(aboutAct, SIGNAL(triggered()), this, SLOT(slotAbout()));

  /*
   * Check Update Menu
   */
  checkUpdateAct = new QAction(_("Check for Updates"), this);
  checkUpdateAct->setIcon(QIcon(":help.png"));
  checkUpdateAct->setToolTip(_("Check for updates"));
  connect(checkUpdateAct, SIGNAL(triggered()), this, SLOT(slotCheckUpdate()));

  startWizardAct = new QAction(_("Open Wizard"), this);
  startWizardAct->setToolTip(_("Open the wizard"));
  connect(startWizardAct, SIGNAL(triggered()), this, SLOT(slotStartWizard()));

  /* Popup-Menu-Action for KeyList
   */
  appendSelectedKeysAct =
      new QAction(_("Append Selected Key(s) To Text"), this);
  appendSelectedKeysAct->setToolTip(
      _("Append The Selected Keys To Text in Editor"));
  connect(appendSelectedKeysAct, SIGNAL(triggered()), this,
          SLOT(slotAppendSelectedKeys()));

  copyMailAddressToClipboardAct = new QAction(_("Copy Email"), this);
  copyMailAddressToClipboardAct->setToolTip(
      _("Copy selected Email to clipboard"));
  connect(copyMailAddressToClipboardAct, SIGNAL(triggered()), this,
          SLOT(slotCopyMailAddressToClipboard()));

  // TODO: find central place for shared actions, to avoid code-duplication with
  // keymgmt.cpp
  showKeyDetailsAct = new QAction(_("Show Key Details"), this);
  showKeyDetailsAct->setToolTip(_("Show Details for this Key"));
  connect(showKeyDetailsAct, SIGNAL(triggered()), this,
          SLOT(slotShowKeyDetails()));

  refreshKeysFromKeyserverAct =
      new QAction(_("Refresh Key From Key Server"), this);
  refreshKeysFromKeyserverAct->setToolTip(
      _("Refresh key from default key server"));
  connect(refreshKeysFromKeyserverAct, SIGNAL(triggered()), this,
          SLOT(refreshKeysFromKeyserver()));

  uploadKeyToServerAct = new QAction(_("Upload Public Key(s) To Server"), this);
  uploadKeyToServerAct->setToolTip(
      _("Upload The Selected Public Keys To Server"));
  connect(uploadKeyToServerAct, SIGNAL(triggered()), this,
          SLOT(uploadKeyToServer()));

  /* Key-Shortcuts for Tab-Switchung-Action
   */
  switchTabUpAct = new QAction(this);
  switchTabUpAct->setShortcut(QKeySequence::NextChild);
  connect(switchTabUpAct, SIGNAL(triggered()), edit, SLOT(slotSwitchTabUp()));
  this->addAction(switchTabUpAct);

  switchTabDownAct = new QAction(this);
  switchTabDownAct->setShortcut(QKeySequence::PreviousChild);
  connect(switchTabDownAct, SIGNAL(triggered()), edit,
          SLOT(slotSwitchTabDown()));
  this->addAction(switchTabDownAct);

  cutPgpHeaderAct = new QAction(_("Remove PGP Header"), this);
  connect(cutPgpHeaderAct, SIGNAL(triggered()), this, SLOT(slotCutPgpHeader()));

  addPgpHeaderAct = new QAction(_("Add PGP Header"), this);
  connect(addPgpHeaderAct, SIGNAL(triggered()), this, SLOT(slotAddPgpHeader()));
}

void MainWindow::createMenus() {
  fileMenu = menuBar()->addMenu(_("File"));
  fileMenu->addAction(newTabAct);
  fileMenu->addAction(browserAct);
  fileMenu->addAction(openAct);
  fileMenu->addSeparator();
  fileMenu->addAction(saveAct);
  fileMenu->addAction(saveAsAct);
  fileMenu->addSeparator();
  fileMenu->addAction(printAct);
  fileMenu->addSeparator();
  fileMenu->addAction(closeTabAct);
  fileMenu->addAction(quitAct);

  editMenu = menuBar()->addMenu(_("Edit"));
  editMenu->addAction(undoAct);
  editMenu->addAction(redoAct);
  editMenu->addSeparator();
  editMenu->addAction(zoomInAct);
  editMenu->addAction(zoomOutAct);
  editMenu->addSeparator();
  editMenu->addAction(copyAct);
  editMenu->addAction(cutAct);
  editMenu->addAction(pasteAct);
  editMenu->addAction(selectAllAct);
  editMenu->addAction(findAct);
  editMenu->addSeparator();
  editMenu->addAction(quoteAct);
  editMenu->addAction(cleanDoubleLinebreaksAct);
  editMenu->addSeparator();
  editMenu->addAction(openSettingsAct);

  fileEncMenu = new QMenu(_("File..."));
  fileEncMenu->addAction(fileEncryptAct);
  fileEncMenu->addAction(fileDecryptAct);
  fileEncMenu->addAction(fileSignAct);
  fileEncMenu->addAction(fileVerifyAct);

  cryptMenu = menuBar()->addMenu(_("Crypt"));
  cryptMenu->addAction(encryptAct);
  cryptMenu->addAction(encryptSignAct);
  cryptMenu->addAction(decryptAct);
  cryptMenu->addAction(decryptVerifyAct);
  cryptMenu->addSeparator();
  cryptMenu->addAction(signAct);
  cryptMenu->addAction(verifyAct);
  cryptMenu->addSeparator();
  cryptMenu->addMenu(fileEncMenu);

  keyMenu = menuBar()->addMenu(_("Keys"));
  importKeyMenu = keyMenu->addMenu(_("Import Key"));
  importKeyMenu->setIcon(QIcon(":key_import.png"));
  importKeyMenu->addAction(importKeyFromFileAct);
  importKeyMenu->addAction(importKeyFromEditAct);
  importKeyMenu->addAction(importKeyFromClipboardAct);
  importKeyMenu->addAction(importKeyFromKeyServerAct);
  keyMenu->addAction(openKeyManagementAct);

  steganoMenu = menuBar()->addMenu(_("Steganography"));
  steganoMenu->addAction(cutPgpHeaderAct);
  steganoMenu->addAction(addPgpHeaderAct);

#ifdef ADVANCED_SUPPORT
  // Hide menu, when steganography menu is disabled in settings
  if (!settings.value("advanced/steganography").toBool()) {
    this->menuBar()->removeAction(steganoMenu->menuAction());
  }
#endif

  viewMenu = menuBar()->addMenu(_("View"));

  helpMenu = menuBar()->addMenu(_("Help"));
  helpMenu->addAction(startWizardAct);
  helpMenu->addSeparator();
  helpMenu->addAction(checkUpdateAct);
  helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBars() {
  fileToolBar = addToolBar(_("File"));
  fileToolBar->setObjectName("fileToolBar");
  fileToolBar->addAction(newTabAct);
  fileToolBar->addAction(openAct);
  fileToolBar->addAction(saveAct);
  fileToolBar->addAction(browserAct);
  viewMenu->addAction(fileToolBar->toggleViewAction());

  cryptToolBar = addToolBar(_("Crypt"));
  cryptToolBar->setObjectName("cryptToolBar");
  cryptToolBar->addAction(encryptAct);
  cryptToolBar->addAction(encryptSignAct);
  cryptToolBar->addAction(decryptAct);
  cryptToolBar->addAction(decryptVerifyAct);
  cryptToolBar->addAction(signAct);
  cryptToolBar->addAction(verifyAct);
  viewMenu->addAction(cryptToolBar->toggleViewAction());

  keyToolBar = addToolBar(_("Key"));
  keyToolBar->setObjectName("keyToolBar");
  keyToolBar->addAction(openKeyManagementAct);
  viewMenu->addAction(keyToolBar->toggleViewAction());

  editToolBar = addToolBar(_("Edit"));
  editToolBar->setObjectName("editToolBar");
  editToolBar->addAction(copyAct);
  editToolBar->addAction(pasteAct);
  editToolBar->addAction(selectAllAct);
  editToolBar->hide();
  viewMenu->addAction(editToolBar->toggleViewAction());

  specialEditToolBar = addToolBar(_("Special Edit"));
  specialEditToolBar->setObjectName("specialEditToolBar");
  specialEditToolBar->addAction(quoteAct);
  specialEditToolBar->addAction(cleanDoubleLinebreaksAct);
  specialEditToolBar->hide();
  viewMenu->addAction(specialEditToolBar->toggleViewAction());

  // Add dropdown menu for key import to keytoolbar
  importButton = new QToolButton();
  importButton->setMenu(importKeyMenu);
  importButton->setPopupMode(QToolButton::InstantPopup);
  importButton->setIcon(QIcon(":key_import.png"));
  importButton->setToolTip(_("Import key from..."));
  importButton->setText(_("Import key"));
  keyToolBar->addWidget(importButton);
}

void MainWindow::createStatusBar() {
  auto* statusBarBox = new QWidget();
  auto* statusBarBoxLayout = new QHBoxLayout();
  // QPixmap* pixmap;

  // icon which should be shown if there are files in attachments-folder
  //  pixmap = new QPixmap(":statusbar_icon.png");
  //  statusBarIcon = new QLabel();
  //  statusBar()->addWidget(statusBarIcon);
  //
  //  statusBarIcon->setPixmap(*pixmap);
  //  statusBar()->insertPermanentWidget(0, statusBarIcon, 0);
  statusBar()->showMessage(_("Ready"), 2000);
  statusBarBox->setLayout(statusBarBoxLayout);
}

void MainWindow::createDockWindows() {
  /* KeyList-Dock window
   */
  keyListDock = new QDockWidget(_("Key ToolBox"), this);
  keyListDock->setObjectName("EncryptDock");
  keyListDock->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea);
  keyListDock->setMinimumWidth(460);
  addDockWidget(Qt::RightDockWidgetArea, keyListDock);

  mKeyList->addListGroupTab(
      _("Default"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return !(key.revoked() || key.disabled() || key.expired());
      });

  mKeyList->addListGroupTab(
      _("Only Public Key"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return !key.is_private_key() &&
               !(key.revoked() || key.disabled() || key.expired());
      });

  mKeyList->addListGroupTab(
      _("Has Private Key"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return key.is_private_key() &&
               !(key.revoked() || key.disabled() || key.expired());
      });

  keyListDock->setWidget(mKeyList);
  viewMenu->addAction(keyListDock->toggleViewAction());

  infoBoardDock = new QDockWidget(_("Information Board"), this);
  infoBoardDock->setObjectName("Information Board");
  infoBoardDock->setAllowedAreas(Qt::BottomDockWidgetArea);
  addDockWidget(Qt::BottomDockWidgetArea, infoBoardDock);
  infoBoardDock->setWidget(infoBoard);
  infoBoardDock->widget()->layout()->setContentsMargins(0, 0, 0, 0);
  viewMenu->addAction(infoBoardDock->toggleViewAction());
}

}  // namespace GpgFrontend::UI
