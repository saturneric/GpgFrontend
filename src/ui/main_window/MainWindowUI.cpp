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

#include "MainWindow.h"

void MainWindow::createActions() {
    /* Main Menu
      */
    newTabAct = new QAction(tr("&New"), this);
    newTabAct->setIcon(QIcon(":misc_doc.png"));
    QList<QKeySequence> newTabActShortcutList;
    newTabActShortcutList.append(QKeySequence(Qt::CTRL + Qt::Key_N));
    newTabActShortcutList.append(QKeySequence(Qt::CTRL + Qt::Key_T));
    newTabAct->setShortcuts(newTabActShortcutList);
    newTabAct->setToolTip(tr("Open a new file"));
    connect(newTabAct, SIGNAL(triggered()), edit, SLOT(slotNewTab()));

    openAct = new QAction(tr("&Open..."), this);
    openAct->setIcon(QIcon(":fileopen.png"));
    openAct->setShortcut(QKeySequence::Open);
    openAct->setToolTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), edit, SLOT(slotOpen()));

    browserAct = new QAction(tr("&Browser"), this);
    browserAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    browserAct->setToolTip(tr("Open a file browser"));
    connect(browserAct, SIGNAL(triggered()), this, SLOT(slotOpenFileTab()));

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setIcon(QIcon(":filesave.png"));
    saveAct->setShortcut(QKeySequence::Save);
    saveAct->setToolTip(tr("Save the current File"));
    connect(saveAct, SIGNAL(triggered()), edit, SLOT(slotSave()));

    saveAsAct = new QAction(tr("Save &As") + "...", this);
    saveAsAct->setIcon(QIcon(":filesaveas.png"));
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    saveAsAct->setToolTip(tr("Save the current File as..."));
    connect(saveAsAct, SIGNAL(triggered()), edit, SLOT(slotSaveAs()));

    printAct = new QAction(tr("&Print"), this);
    printAct->setIcon(QIcon(":fileprint.png"));
    printAct->setShortcut(QKeySequence::Print);
    printAct->setToolTip(tr("Print Document"));
    connect(printAct, SIGNAL(triggered()), edit, SLOT(slotPrint()));

    closeTabAct = new QAction(tr("&Close"), this);
    closeTabAct->setShortcut(QKeySequence::Close);
    closeTabAct->setToolTip(tr("Close file"));
    connect(closeTabAct, SIGNAL(triggered()), edit, SLOT(slotCloseTab()));

    quitAct = new QAction(tr("&Quit"), this);
    quitAct->setShortcut(QKeySequence::Quit);
    quitAct->setIcon(QIcon(":exit.png"));
    quitAct->setToolTip(tr("Quit Program"));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

    /* Edit Menu
     */
    undoAct = new QAction(tr("&Undo"), this);
    undoAct->setShortcut(QKeySequence::Undo);
    undoAct->setToolTip(tr("Undo Last Edit Action"));
    connect(undoAct, SIGNAL(triggered()), edit, SLOT(slotUndo()));

    redoAct = new QAction(tr("&Redo"), this);
    redoAct->setShortcut(QKeySequence::Redo);
    redoAct->setToolTip(tr("Redo Last Edit Action"));
    connect(redoAct, SIGNAL(triggered()), edit, SLOT(slotRedo()));

    zoomInAct = new QAction(tr("Zoom In"), this);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAct, SIGNAL(triggered()), edit, SLOT(slotZoomIn()));

    zoomOutAct = new QAction(tr("Zoom Out"), this);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAct, SIGNAL(triggered()), edit, SLOT(slotZoomOut()));

    pasteAct = new QAction(tr("&Paste"), this);
    pasteAct->setIcon(QIcon(":button_paste.png"));
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setToolTip(tr("Paste Text From Clipboard"));
    connect(pasteAct, SIGNAL(triggered()), edit, SLOT(slotPaste()));

    cutAct = new QAction(tr("Cu&t"), this);
    cutAct->setIcon(QIcon(":button_cut.png"));
    cutAct->setShortcut(QKeySequence::Cut);
    cutAct->setToolTip(tr("Cut the current selection's contents to the "
                          "clipboard"));
    connect(cutAct, SIGNAL(triggered()), edit, SLOT(slotCut()));

    copyAct = new QAction(tr("&Copy"), this);
    copyAct->setIcon(QIcon(":button_copy.png"));
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setToolTip(tr("Copy the current selection's contents to the "
                           "clipboard"));
    connect(copyAct, SIGNAL(triggered()), edit, SLOT(slotCopy()));

    quoteAct = new QAction(tr("&Quote"), this);
    quoteAct->setIcon(QIcon(":quote.png"));
    quoteAct->setToolTip(tr("Quote whole text"));
    connect(quoteAct, SIGNAL(triggered()), edit, SLOT(slotQuote()));

    selectAllAct = new QAction(tr("Select &All"), this);
    selectAllAct->setIcon(QIcon(":edit.png"));
    selectAllAct->setShortcut(QKeySequence::SelectAll);
    selectAllAct->setToolTip(tr("Select the whole text"));
    connect(selectAllAct, SIGNAL(triggered()), edit, SLOT(slotSelectAll()));

    findAct = new QAction(tr("&Find"), this);
    findAct->setShortcut(QKeySequence::Find);
    findAct->setToolTip(tr("Find a word"));
    connect(findAct, SIGNAL(triggered()), this, SLOT(slotFind()));

    cleanDoubleLinebreaksAct = new QAction(tr("Remove &spacing"), this);
    cleanDoubleLinebreaksAct->setIcon(QIcon(":format-line-spacing-triple.png"));
    //cleanDoubleLineBreaksAct->setShortcut(QKeySequence::SelectAll);
    cleanDoubleLinebreaksAct->setToolTip(tr("Remove double linebreaks, e.g. in pasted text from webmailer"));
    connect(cleanDoubleLinebreaksAct, SIGNAL(triggered()), this, SLOT(slotCleanDoubleLinebreaks()));

    openSettingsAct = new QAction(tr("Se&ttings"), this);
    openSettingsAct->setToolTip(tr("Open settings dialog"));
    openSettingsAct->setShortcut(QKeySequence::Preferences);
    connect(openSettingsAct, SIGNAL(triggered()), this, SLOT(slotOpenSettingsDialog()));

    /* Crypt Menu
     */
    encryptAct = new QAction(tr("&Encrypt"), this);
    encryptAct->setIcon(QIcon(":encrypted.png"));
    encryptAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
    encryptAct->setToolTip(tr("Encrypt Message"));
    connect(encryptAct, SIGNAL(triggered()), this, SLOT(slotEncrypt()));

    encryptSignAct = new QAction(tr("&Encrypt &Sign"), this);
    encryptSignAct->setIcon(QIcon(":encrypted_signed.png"));
    encryptSignAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_E));
    encryptSignAct->setToolTip(tr("Encrypt and Sign Message"));
    connect(encryptSignAct, SIGNAL(triggered()), this, SLOT(slotEncryptSign()));

    decryptAct = new QAction(tr("&Decrypt"), this);
    decryptAct->setIcon(QIcon(":decrypted.png"));
    decryptAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    decryptAct->setToolTip(tr("Decrypt Message"));
    connect(decryptAct, SIGNAL(triggered()), this, SLOT(slotDecrypt()));

    decryptVerifyAct = new QAction(tr("&Decrypt &Verify"), this);
    decryptVerifyAct->setIcon(QIcon(":decrypted_verified.png"));
    decryptVerifyAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D));
    decryptVerifyAct->setToolTip(tr("Decrypt and Verify Message"));
    connect(decryptVerifyAct, SIGNAL(triggered()), this, SLOT(slotDecryptVerify()));

    /*
     * File encryption submenu
     */
    fileEncryptAct = new QAction(tr("&Encrypt File"), this);
    fileEncryptAct->setToolTip(tr("Encrypt File"));
    connect(fileEncryptAct, SIGNAL(triggered()), this, SLOT(slotFileEncryptCustom()));

    fileDecryptAct = new QAction(tr("&Decrypt File"), this);
    fileDecryptAct->setToolTip(tr("Decrypt File"));
    connect(fileDecryptAct, SIGNAL(triggered()), this, SLOT(slotFileDecryptCustom()));

    fileSignAct = new QAction(tr("&Sign File"), this);
    fileSignAct->setToolTip(tr("Sign File"));
    connect(fileSignAct, SIGNAL(triggered()), this, SLOT(slotFileSignCustom()));

    fileVerifyAct = new QAction(tr("&Verify File"), this);
    fileVerifyAct->setToolTip(tr("Verify File"));
    connect(fileVerifyAct, SIGNAL(triggered()), this, SLOT(slotFileVerifyCustom()));


    signAct = new QAction(tr("&Sign"), this);
    signAct->setIcon(QIcon(":signature.png"));
    signAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
    signAct->setToolTip(tr("Sign Message"));
    connect(signAct, SIGNAL(triggered()), this, SLOT(slotSign()));

    verifyAct = new QAction(tr("&Verify"), this);
    verifyAct->setIcon(QIcon(":verify.png"));
    verifyAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V));
    verifyAct->setToolTip(tr("Verify Message"));
    connect(verifyAct, SIGNAL(triggered()), this, SLOT(slotVerify()));

    /* Key Menu
     */

    importKeyFromEditAct = new QAction(tr("&Editor"), this);
    importKeyFromEditAct->setIcon(QIcon(":txt.png"));
    importKeyFromEditAct->setToolTip(tr("Import New Key From Editor"));
    connect(importKeyFromEditAct, SIGNAL(triggered()), this, SLOT(slotImportKeyFromEdit()));

    openKeyManagementAct = new QAction(tr("Manage &Keys"), this);
    openKeyManagementAct->setIcon(QIcon(":keymgmt.png"));
    openKeyManagementAct->setToolTip(tr("Open Keymanagement"));
    connect(openKeyManagementAct, SIGNAL(triggered()), this, SLOT(slotOpenKeyManagement()));

    /*
     * About Menu
     */
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setIcon(QIcon(":help.png"));
    aboutAct->setToolTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(slotAbout()));

    /*
     * Check Update Menu
    */
    checkUpdateAct = new QAction(tr("&Check for Updates"), this);
    checkUpdateAct->setIcon(QIcon(":help.png"));
    checkUpdateAct->setToolTip(tr("Check for updates"));
    connect(checkUpdateAct, SIGNAL(triggered()), this, SLOT(slotCheckUpdate()));

    startWizardAct = new QAction(tr("Open &Wizard"), this);
    startWizardAct->setToolTip(tr("Open the wizard"));
    connect(startWizardAct, SIGNAL(triggered()), this, SLOT(slotStartWizard()));

    /* Popup-Menu-Action for KeyList
     */
    appendSelectedKeysAct = new QAction(tr("Append Selected Key(s) To Text"), this);
    appendSelectedKeysAct->setToolTip(tr("Append The Selected Keys To Text in Editor"));
    connect(appendSelectedKeysAct, SIGNAL(triggered()), this, SLOT(slotAppendSelectedKeys()));

    copyMailAddressToClipboardAct = new QAction(tr("Copy Email"), this);
    copyMailAddressToClipboardAct->setToolTip(tr("Copy selected Email to clipboard"));
    connect(copyMailAddressToClipboardAct, SIGNAL(triggered()), this, SLOT(slotCopyMailAddressToClipboard()));

    // TODO: find central place for shared actions, to avoid code-duplication with keymgmt.cpp
    showKeyDetailsAct = new QAction(tr("Show Key Details"), this);
    showKeyDetailsAct->setToolTip(tr("Show Details for this Key"));
    connect(showKeyDetailsAct, SIGNAL(triggered()), this, SLOT(slotShowKeyDetails()));

    refreshKeysFromKeyserverAct = new QAction(tr("Refresh Key From Key Server"), this);
    refreshKeysFromKeyserverAct->setToolTip(tr("Refresh key from default key server"));
    connect(refreshKeysFromKeyserverAct, SIGNAL(triggered()), this, SLOT(refreshKeysFromKeyserver()));

    uploadKeyToServerAct = new QAction(tr("Upload Public Key(s) To Server"), this);
    uploadKeyToServerAct->setToolTip(tr("Upload The Selected Public Keys To Server"));
    connect(uploadKeyToServerAct, SIGNAL(triggered()), this, SLOT(uploadKeyToServer()));
    
    /* Key-Shortcuts for Tab-Switchung-Action
    */
    switchTabUpAct = new QAction(this);
    switchTabUpAct->setShortcut(QKeySequence::NextChild);
    connect(switchTabUpAct, SIGNAL(triggered()), edit, SLOT(slotSwitchTabUp()));
    this->addAction(switchTabUpAct);

    switchTabDownAct = new QAction(this);
    switchTabDownAct->setShortcut(QKeySequence::PreviousChild);
    connect(switchTabDownAct, SIGNAL(triggered()), edit, SLOT(slotSwitchTabDown()));
    this->addAction(switchTabDownAct);

    cutPgpHeaderAct = new QAction(tr("Remove PGP Header"), this);
    connect(cutPgpHeaderAct, SIGNAL(triggered()), this, SLOT(slotCutPgpHeader()));

    addPgpHeaderAct = new QAction(tr("Add PGP Header"), this);
    connect(addPgpHeaderAct, SIGNAL(triggered()), this, SLOT(slotAddPgpHeader()));
}

void MainWindow::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
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

    editMenu = menuBar()->addMenu(tr("&Edit"));
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

    fileEncMenu = new QMenu(tr("&File..."));
    fileEncMenu->addAction(fileEncryptAct);
    fileEncMenu->addAction(fileDecryptAct);
    fileEncMenu->addAction(fileSignAct);
    fileEncMenu->addAction(fileVerifyAct);

    cryptMenu = menuBar()->addMenu(tr("&Crypt"));
    cryptMenu->addAction(encryptAct);
    cryptMenu->addAction(encryptSignAct);
    cryptMenu->addAction(decryptAct);
    cryptMenu->addAction(decryptVerifyAct);
    cryptMenu->addSeparator();
    cryptMenu->addAction(signAct);
    cryptMenu->addAction(verifyAct);
    cryptMenu->addSeparator();
    cryptMenu->addMenu(fileEncMenu);

    keyMenu = menuBar()->addMenu(tr("&Keys"));
    importKeyMenu = keyMenu->addMenu(tr("&Import Key"));
    importKeyMenu->setIcon(QIcon(":key_import.png"));
    importKeyMenu->addAction(keyMgmt->importKeyFromFileAct);
    importKeyMenu->addAction(importKeyFromEditAct);
    importKeyMenu->addAction(keyMgmt->importKeyFromClipboardAct);
    importKeyMenu->addAction(keyMgmt->importKeyFromKeyServerAct);
    importKeyMenu->addAction(keyMgmt->importKeyFromKeyServerAct);
    keyMenu->addAction(openKeyManagementAct);

    steganoMenu = menuBar()->addMenu(tr("&Steganography"));
    steganoMenu->addAction(cutPgpHeaderAct);
    steganoMenu->addAction(addPgpHeaderAct);

    // Hide menu, when steganography menu is disabled in settings
    if (!settings.value("advanced/steganography").toBool()) {
        this->menuBar()->removeAction(steganoMenu->menuAction());
    }

    viewMenu = menuBar()->addMenu(tr("&View"));

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(startWizardAct);
    helpMenu->addSeparator();
    helpMenu->addAction(checkUpdateAct);
    helpMenu->addAction(aboutAct);

}

void MainWindow::createToolBars() {
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->setObjectName("fileToolBar");
    fileToolBar->addAction(newTabAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->hide();
    viewMenu->addAction(fileToolBar->toggleViewAction());

    cryptToolBar = addToolBar(tr("Crypt"));
    cryptToolBar->setObjectName("cryptToolBar");
    cryptToolBar->addAction(encryptAct);
    cryptToolBar->addAction(encryptSignAct);
    cryptToolBar->addAction(decryptAct);
    cryptToolBar->addAction(decryptVerifyAct);
    cryptToolBar->addAction(signAct);
    cryptToolBar->addAction(verifyAct);
    viewMenu->addAction(cryptToolBar->toggleViewAction());

    keyToolBar = addToolBar(tr("Key"));
    keyToolBar->setObjectName("keyToolBar");
    keyToolBar->addAction(openKeyManagementAct);
    viewMenu->addAction(keyToolBar->toggleViewAction());

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->setObjectName("editToolBar");
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
    editToolBar->addAction(selectAllAct);
    viewMenu->addAction(editToolBar->toggleViewAction());

    specialEditToolBar = addToolBar(tr("Special Edit"));
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
    importButton->setToolTip(tr("Import key from..."));
    importButton->setText(tr("Import key"));
    keyToolBar->addWidget(importButton);

    // Add dropdown menu for file encryption/decryption to crypttoolbar
    fileEncButton = new QToolButton();
    connect(fileEncButton, SIGNAL(clicked(bool)), this, SLOT(slotOpenFileTab()));
    fileEncButton->setPopupMode(QToolButton::InstantPopup);
    fileEncButton->setIcon(QIcon(":fileencryption.png"));
    fileEncButton->setToolTip(tr("Browser to view and operate file"));
    fileEncButton->setText(tr("Browser"));
    fileToolBar->addWidget(fileEncButton);

}

void MainWindow::createStatusBar() {
    auto *statusBarBox = new QWidget();
    auto *statusBarBoxLayout = new QHBoxLayout();
    QPixmap *pixmap;

    // icon which should be shown if there are files in attachments-folder
    pixmap = new QPixmap(":statusbar_icon.png");
    statusBarIcon = new QLabel();
    statusBar()->addWidget(statusBarIcon);

    statusBarIcon->setPixmap(*pixmap);
    statusBar()->insertPermanentWidget(0, statusBarIcon, 0);
    statusBarIcon->hide();
    statusBar()->showMessage(tr("Ready"), 2000);
    statusBarBox->setLayout(statusBarBoxLayout);
}

void MainWindow::createDockWindows() {
    /* KeyList-Dockwindow
     */
    keyListDock = new QDockWidget(tr("Key ToolBox"), this);
    keyListDock->setObjectName("EncryptDock");
    keyListDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    keyListDock->setMinimumWidth(460);
    addDockWidget(Qt::RightDockWidgetArea, keyListDock);
    keyListDock->setWidget(mKeyList);
    viewMenu->addAction(keyListDock->toggleViewAction());

    infoBoardDock = new QDockWidget(tr("Information Board"), this);
    infoBoardDock->setObjectName("Information Board");
    infoBoardDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, infoBoardDock);
    infoBoardDock->setWidget(infoBoard);
    infoBoardDock->widget()->layout()->setContentsMargins(0, 0, 0, 0);
    viewMenu->addAction(infoBoardDock->toggleViewAction());

    /* Attachments-Dockwindow
      */
    if (settings.value("mime/parseMime").toBool()) {
        createAttachmentDock();
    }
}

void MainWindow::createAttachmentDock() {
    if (attachmentDockCreated) {
        return;
    }
    mAttachments = new Attachments();
    attachmentDock = new QDockWidget(tr("Attached files:"), this);
    attachmentDock->setObjectName("AttachmentDock");
    attachmentDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, attachmentDock);
    attachmentDock->setWidget(mAttachments);
    // hide till attachment is decrypted
    viewMenu->addAction(attachmentDock->toggleViewAction());
    attachmentDock->hide();
    attachmentDockCreated = true;
}
