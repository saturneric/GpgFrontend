/*
 *      mainwindow.h
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

#ifndef __GPGWIN_H__
#define __GPGWIN_H__

#include "gpg/GpgConstants.h"
#include "ui/Attachments.h"
#include "ui/KeyMgmt.h"
#include "ui/TextEdit.h"
#include "ui/FileEncryptionDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/AboutDialog.h"
#include "ui/VerifyNotification.h"
#include "ui/FindWidget.h"
#include "ui/Wizard.h"


/**
 * @brief
 *
 */
class MainWindow : public QMainWindow {
Q_OBJECT

public:
    /**
     * @brief
     *
     */
    MainWindow();

public slots:

    void slotSetStatusBarText(const QString& text);

protected:
    /**
     * @details Close event shows a save dialog, if there are unsaved documents on exit.
     * @param event
     */
    void closeEvent(QCloseEvent *event) override;

private slots:

    /**
     * @details encrypt the text of currently active textedit-page
     * with the currently checked keys
     */
    void slotEncrypt();

    /**
     * @details Show a passphrase dialog and decrypt the text of currently active tab.
     */
    void slotDecrypt();

    /**
     * @details Sign the text of currently active tab with the checked private keys
     */
    void slotSign();

    /**
     * @details Verify the text of currently active tab and show verify information.
     * If document is signed with a key, which is not in keylist, show import missing
     * key from keyserver in Menu of verifynotification.
     */
    void slotVerify();

    /**
     * @details Show the details of the first of the first of selected keys
     */
    void slotShowKeyDetails();

    /**
     * @details Refresh key information of selected keys from default keyserver
     */
    void refreshKeysFromKeyserver();

    /**
      * @details upload the selected key to the keyserver
      */
    void uploadKeyToServer();

    /**
     * @details Open find widget.
     */
    void slotFind();

    /**
      * @details start the wizard
      */
    void slotStartWizard();

    /**
     * @details Import keys from currently active tab to keylist if possible.
     */
    void slotImportKeyFromEdit();

    /**
     * @details Append the selected keys to currently active textedit.
     */
    void slotAppendSelectedKeys();

    /**
     * @details Copy the mailaddress of selected key to clipboard.
     * Method for keylists contextmenu.
     */
    void slotCopyMailAddressToClipboard();

    /**
     * @details Open key management dialog.
     */
    void slotOpenKeyManagement();

    /**
     * @details Open about-dialog.
     */
    void slotAbout();

    /**
     * @details Open dialog for encrypting file.
     */
    void slotFileEncrypt();

    /**
     * @details Open dialog for decrypting file.
     */
    void slotFileDecrypt();

    /**
     * @details Open dialog for signing file.
     */
    void slotFileSign();

    /**
     * @details Open dialog for verifying file.
     */
    void slotFileVerify();

    /**
     * @details Open settings-dialog.
     */
    void slotOpenSettingsDialog();

    /**
     * @details Open online-tutorial in default browser.
     */
    static void slotOpenTutorial();

    /**
     * @details Open integrated help in new tab.
     */
    void slotOpenHelp();

    /**
     * @details Open integrated help in new tab with the specified page.
     */
    void slotOpenHelp(const QString& page);

    /**
     * @details Show a warn message in status bar, if there are files in attachment folder.
     */
    void slotCheckAttachmentFolder();

    /**
     * @details Open online translation tutorial in default browser.
     */
    static void slotOpenTranslate();

    /**
     * @details Replace double linebreaks by single linebreaks in currently active tab.
     */
    void slotCleanDoubleLinebreaks();

    /**
     * @details Cut the existing PGP header and footer from current tab.
     */
    void slotCutPgpHeader();

    /**
     * @details Add PGP header and footer to current tab.
     */
    void slotAddPgpHeader();

//    void dropEvent(QDropEvent *event);

    /**
     * @details Disable tab related actions, if number of tabs is 0.
     * @param number number of the opened tabs and -1, if no tab is opened
     */
    void slotDisableTabActions(int number);

    /**
     * @details get value of member restartNeeded to needed.
     * @param needed true, if application has to be restarted
     */
    void slotSetRestartNeeded(bool needed);

private:
    /**
     * @details Create actions for the main-menu and the context-menu of the keylist.
     */
    void createActions();

    /**
     * @details create the menu of the main-window.
     */
    void createMenus();

    /**
     * @details Create edit-, crypt- and key-toolbars.
     */
    void createToolBars();

    /**
     * @details Create statusbar of mainwindow.
     */
    void createStatusBar();

    /**
     * @details Create keylist- and attachment-dockwindows.
     */
    void createDockWindows();

    /**
     * @details Create attachment-dockwindow.
     */
    void createAttachmentDock();

    /**
     * @details close attachment-dockwindow.
     */
    void closeAttachmentDock();

    /**
     * @details Load settings from ini-file.
     */
    void restoreSettings();

    /**
     * @details Save settings to ini-file.
     */
    void saveSettings();

    /**
     * @brief
     *
     * @param message
     */
    void parseMime(QByteArray *message);

    /**
     * @brief return true, if restart is needed
     */
    bool getRestartNeeded() const;

    TextEdit *edit; /** Tabwidget holding the edit-windows */
    QMenu *fileMenu; /** Submenu for file-operations*/
    QMenu *editMenu; /** Submenu for text-operations*/
    QMenu *cryptMenu; /** Submenu for crypt-operations */
    QMenu *fileEncMenu; /** Submenu for file crypt operations */
    QMenu *helpMenu; /** Submenu for help-operations */
    QMenu *keyMenu; /** Submenu for key-operations */
    QMenu *viewMenu; /** Submenu for view operations */
    QMenu *importKeyMenu; /** Sumenu for import operations */
    QMenu *steganoMenu; /** Submenu for steganographic operations*/
    QToolBar *cryptToolBar; /** Toolbar holding crypt actions */
    QToolBar *fileToolBar; /** Toolbar holding file actions */
    QToolBar *editToolBar; /** Toolbar holding edit actions */
    QToolBar *specialEditToolBar; /** Toolbar holding special edit actions */
    QToolBar *keyToolBar; /** Toolbar holding key operations */
    QToolButton *importButton; /** Toolbutton for import dropdown menu in toolbar */
    QToolButton *fileEncButton; /** Toolbutton for file cryption dropdown menu in toolbar */
    QDockWidget *keylistDock; /** Encrypt Dock*/
    QDockWidget *attachmentDock; /** Attachment Dock */
    [[maybe_unused]] QDialog *genkeyDialog; /** Dialog for key generation */

    QAction *newTabAct; /** Action to create new tab */
    QAction *switchTabUpAct; /** Action to switch tab up*/
    QAction *switchTabDownAct; /** Action to switch tab down */
    QAction *openAct; /** Action to open file */
    QAction *saveAct; /** Action to save file */
    QAction *saveAsAct; /** Action to save file as */
    QAction *printAct; /** Action to print */
    QAction *closeTabAct; /** Action to print */
    QAction *quitAct; /** Action to quit application */
    QAction *encryptAct; /** Action to encrypt text */
    QAction *decryptAct; /** Action to decrypt text */
    QAction *signAct; /** Action to sign text */
    QAction *verifyAct; /** Action to verify text */
    QAction *importKeyFromEditAct; /** Action to import key from edit */
    QAction *cleanDoubleLinebreaksAct; /** Action to remove double line breaks */

    QAction *appendSelectedKeysAct; /** Action to append selected keys to edit */
    QAction *copyMailAddressToClipboardAct; /** Action to copy mail to clipboard */
    QAction *openKeyManagementAct; /** Action to open key management */
    QAction *copyAct; /** Action to copy text */
    QAction *quoteAct; /** Action to quote text */
    QAction *cutAct; /** Action to cut text */
    QAction *pasteAct; /** Action to paste text */
    QAction *selectallAct; /** Action to select whole text */
    QAction *findAct; /** Action to find text */
    QAction *undoAct; /** Action to undo last action */
    QAction *redoAct; /** Action to redo last action */
    QAction *zoomInAct; /** Action to zoom in */
    QAction *zoomOutAct; /** Action to zoom out */
    QAction *aboutAct; /** Action to open about dialog */
    QAction *fileEncryptAct; /** Action to open dialog for encrypting file */
    QAction *fileDecryptAct; /** Action to open dialog for decrypting file */
    QAction *fileSignAct; /** Action to open dialog for signing file */
    QAction *fileVerifyAct; /** Action to open dialog for verifying file */
    QAction *openSettingsAct; /** Action to open settings dialog */
    QAction *openTranslateAct; /** Action to open translate doc*/
    QAction *openTutorialAct; /** Action to open tutorial */
    QAction *openHelpAct; /** Action to open tutorial */
    QAction *showKeyDetailsAct; /** Action to open key-details dialog */
    QAction *refreshKeysFromKeyserverAct; /** Action to refresh a key from keyserver */
    QAction *uploadKeyToServerAct; /** Action to append selected keys to edit */
    QAction *startWizardAct; /** Action to open the wizard */
    QAction *cutPgpHeaderAct; /** Action for cutting the PGP header */
    QAction *addPgpHeaderAct; /** Action for adding the PGP header */

    QLabel *statusBarIcon; /**< TODO */
    QSettings settings; /**< TODO */
    KeyList *mKeyList; /**< TODO */
    Attachments *mAttachments; /**< TODO */
    GpgME::GpgContext *mCtx; /**< TODO */
    KeyMgmt *keyMgmt; /**< TODO */
    KeyServerImportDialog *importDialog; /**< TODO */
    bool attachmentDockCreated;
    bool restartNeeded;
};

#endif // __GPGWIN_H__
