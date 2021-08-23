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
#include "ui/help/VersionCheckThread.h"

MainWindow::MainWindow()
        : appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
                   QSettings::IniFormat) {

    networkAccessManager = new QNetworkAccessManager(this);

    auto waitingDialog = new WaitingDialog(tr("Loading Gnupg"), this);

    // Init Gnupg
    auto ctx_thread = QThread::create([&]() { mCtx = new GpgME::GpgContext(); });
    ctx_thread->start();
    while (ctx_thread->isRunning())
        QApplication::processEvents();
    waitingDialog->close();
    ctx_thread->deleteLater();

    QString baseUrl = "https://api.github.com/repos/saturneric/gpgfrontend/releases/latest";

    QNetworkRequest request;
    request.setUrl(QUrl(baseUrl));

    QNetworkReply *replay = networkAccessManager->get(request);

    auto version_thread = new VersionCheckThread(replay);

    connect(version_thread, SIGNAL(finished()), version_thread, SLOT(deleteLater()));
    connect(version_thread, SIGNAL(upgradeVersion(const QString &, const QString &)), this, SLOT(slotVersionUpgrade(const QString &, const QString &)));

    version_thread->start();

    // Check Context Status
    if (!mCtx->isGood()) {
        QMessageBox::critical(
                nullptr, tr("ENV Loading Failed"),
                tr("Gnupg is not installed correctly, please follow the ReadME "
                   "instructions to install gnupg and then open GPGFrontend."));
        QCoreApplication::quit();
        exit(0);
    }

    /* get path were app was started */
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    edit = new TextEdit(this);
    setCentralWidget(edit);

    /* the list of Keys available*/
    mKeyList = new KeyList(mCtx, KeyListRow::SECRET_OR_PUBLIC_KEY,
                           KeyListColumn::TYPE | KeyListColumn::NAME |
                           KeyListColumn::EmailAddress |
                           KeyListColumn::Usage | KeyListColumn::Validity,
                           this);
    mKeyList->setFilter([](const GpgKey &key) -> bool {
        if (key.revoked || key.disabled || key.expired)
            return false;
        else
            return true;
    });
    mKeyList->slotRefresh();

    infoBoard = new InfoBoardWidget(this, mCtx, mKeyList);

    /* List of binary Attachments */
    attachmentDockCreated = false;

    /* Variable containing if restart is needed */
    this->slotSetRestartNeeded(false);

    keyMgmt = new KeyMgmt(mCtx, this);
    keyMgmt->hide();
    /* test attachmentdir for files alll 15s */
    auto *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(slotCheckAttachmentFolder()));
    timer->start(5000);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWindows();

    connect(edit->tabWidget, SIGNAL(currentChanged(int)), this,
            SLOT(slotDisableTabActions(int)));

    mKeyList->addMenuAction(appendSelectedKeysAct);
    mKeyList->addMenuAction(copyMailAddressToClipboardAct);
    mKeyList->addMenuAction(showKeyDetailsAct);
    mKeyList->addSeparator();
    mKeyList->addMenuAction(refreshKeysFromKeyserverAct);
    mKeyList->addMenuAction(uploadKeyToServerAct);

    restoreSettings();

    // open filename if provided as first command line parameter
    QStringList args = qApp->arguments();
    if (args.size() > 1) {
        if (!args[1].startsWith("-")) {
            if (QFile::exists(args[1]))
                edit->loadFile(args[1]);
        }
    }
    edit->curTextPage()->setFocus();
    this->setMinimumSize(1200, 700);
    this->setWindowTitle(qApp->applicationName());
    this->show();

    // Show wizard, if the don't show wizard message box wasn't checked
    // and keylist doesn't contain a private key
    qDebug() << "wizard/showWizard"
             << settings.value("wizard/showWizard", true).toBool();
    qDebug() << "wizard/nextPage" << settings.value("wizard/nextPage").isNull();
    if (settings.value("wizard/showWizard", true).toBool() ||
        !settings.value("wizard/nextPage").isNull()) {
        slotStartWizard();
    }

}

void MainWindow::restoreSettings() {
    // state sets pos & size of dock-widgets
    this->restoreState(settings.value("window/windowState").toByteArray());

    // Restore window size & location
    if (settings.value("window/windowSave").toBool()) {
        QPoint pos = settings.value("window/pos", QPoint(100, 100)).toPoint();
        QSize size = settings.value("window/size", QSize(800, 450)).toSize();
        this->resize(size);
        this->move(pos);
    } else {
        this->resize(QSize(800, 450));
        this->move(QPoint(100, 100));
    }

    // Iconsize
    QSize iconSize = settings.value("toolbar/iconsize", QSize(24, 24)).toSize();
    this->setIconSize(iconSize);

    importButton->setIconSize(iconSize);
    fileEncButton->setIconSize(iconSize);
    // set list of keyserver if not defined
    QStringList *keyServerDefaultList;
    keyServerDefaultList = new QStringList("http://keys.gnupg.net");
    keyServerDefaultList->append("https://keyserver.ubuntu.com");
    keyServerDefaultList->append("http://pool.sks-keyservers.net");

    QStringList keyServerList =
            settings.value("keyserver/keyServerList", *keyServerDefaultList)
                    .toStringList();
    settings.setValue("keyserver/keyServerList", keyServerList);

    // set default keyserver, if it's not set
    QString defaultKeyServer = settings
            .value("keyserver/defaultKeyServer",
                   QString("https://keyserver.ubuntu.com"))
            .toString();
    settings.setValue("keyserver/defaultKeyServer", defaultKeyServer);

    // Iconstyle
    Qt::ToolButtonStyle buttonStyle = static_cast<Qt::ToolButtonStyle>(
            settings.value("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon)
                    .toUInt());
    this->setToolButtonStyle(buttonStyle);
    importButton->setToolButtonStyle(buttonStyle);
    fileEncButton->setToolButtonStyle(buttonStyle);

    // Checked Keys
    if (settings.value("keys/saveKeyChecked").toBool()) {
        QStringList keyIds =
                settings.value("keys/savedCheckedKeyList").toStringList();
        mKeyList->setChecked(&keyIds);
    }
}

void MainWindow::saveSettings() {
    // window position and size
    settings.setValue("window/windowState", saveState());
    settings.setValue("window/pos", pos());
    settings.setValue("window/size", size());

    // keyid-list of private checked keys
    if (settings.value("keys/saveKeyChecked").toBool()) {
        QStringList *keyIds = mKeyList->getChecked();
        if (!keyIds->isEmpty()) {
            settings.setValue("keys/savedCheckedKeyList", *keyIds);
        } else {
            settings.setValue("keys/savedCheckedKeyList", "");
        }
    } else {
        settings.remove("keys/savedCheckedKeyList");
    }
}

void MainWindow::closeAttachmentDock() {
    if (!attachmentDockCreated) {
        return;
    }
    attachmentDock->close();
    attachmentDock->deleteLater();
    attachmentDockCreated = false;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    /*
     * ask to save changes, if there are
     * modified documents in any tab
     */
    if (edit->maybeSaveAnyTab()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }

    // clear password from memory
    mCtx->clearPasswordCache();
}
