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

#include "ui/KeyMgmt.h"

#include <utility>

KeyMgmt::KeyMgmt(GpgME::GpgContext *ctx, QWidget *parent )  : QMainWindow(parent)
{
    mCtx = ctx;

    /* the list of Keys available*/
    mKeyList = new KeyList(mCtx);
    mKeyList->setColumnWidth(2, 250);
    mKeyList->setColumnWidth(3, 250);
    setCentralWidget(mKeyList);
    mKeyList->setDoubleClickedAction([this] (const GpgKey &key, QWidget *parent) {
        new KeyDetailsDialog(mCtx, key, parent);
    });

    createActions();
    createMenus();
    createToolBars();
    connect(this,SIGNAL(signalStatusBarChanged(QString)),this->parent(),SLOT(slotSetStatusBarText(QString)));

    /* Restore the iconstyle */
    QSettings settings;
    settings.sync();
    QSize iconSize = settings.value("toolbar/iconsize", QSize(32, 32)).toSize();
    Qt::ToolButtonStyle buttonStyle = static_cast<Qt::ToolButtonStyle>(settings.value("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon).toUInt());
    this->setIconSize(iconSize);
    this->setToolButtonStyle(buttonStyle);

    // state sets pos & size of dock-widgets
    this->restoreState(settings.value("keymgmt/windowState").toByteArray());

    // Restore window size & location
    if (settings.value("window/windowSave").toBool()) {
        QPoint pos = settings.value("keymgmt/pos", QPoint(100, 100)).toPoint();
        QSize size = settings.value("keymgmt/size", QSize(800, 450)).toSize();
        this->resize(size);
        this->move(pos);
    } else {
        this->resize(QSize(1000, 600));
    }

    setWindowTitle(tr("KeyPairs Management"));
    mKeyList->addMenuAction(deleteSelectedKeysAct);
    mKeyList->addMenuAction(showKeyDetailsAct);
}

void KeyMgmt::createActions()
{
    closeAct = new QAction(tr("&Close Key Management"), this);
    closeAct->setShortcut(tr("Ctrl+Q"));
    closeAct->setIcon(QIcon(":exit.png"));
    closeAct->setToolTip(tr("Close Key Management"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));

    generateKeyPairAct = new QAction(tr("Generate KeyPair"), this);
    generateKeyPairAct->setIcon(QIcon(":key_generate.png"));
    generateKeyPairAct->setToolTip(tr("Generate KeyPair"));
    connect(generateKeyPairAct, SIGNAL(triggered()), this, SLOT(slotGenerateKeyDialog()));

    generateSubKeyAct = new QAction(tr("Generate Subkey For Selected"), this);
    generateSubKeyAct->setIcon(QIcon(":key_generate.png"));
    generateSubKeyAct->setToolTip(tr("Generate Subkey For Selected KeyPair"));
    connect(generateSubKeyAct, SIGNAL(triggered()), this, SLOT(slotGenerateSubKey()));

    importKeyFromFileAct = new QAction(tr("&File"), this);
    importKeyFromFileAct->setIcon(QIcon(":import_key_from_file.png"));
    importKeyFromFileAct->setToolTip(tr("Import New Key From File"));
    connect(importKeyFromFileAct, SIGNAL(triggered()), this, SLOT(slotImportKeyFromFile()));

    importKeyFromClipboardAct = new QAction(tr("&Clipboard"), this);
    importKeyFromClipboardAct->setIcon(QIcon(":import_key_from_clipboard.png"));
    importKeyFromClipboardAct->setToolTip(tr("Import New Key From Clipboard"));
    connect(importKeyFromClipboardAct, SIGNAL(triggered()), this, SLOT(slotImportKeyFromClipboard()));

    importKeyFromKeyServerAct = new QAction(tr("&Keyserver"), this);
    importKeyFromKeyServerAct->setIcon(QIcon(":import_key_from_server.png"));
    importKeyFromKeyServerAct->setToolTip(tr("Import New Key From Keyserver"));
    connect(importKeyFromKeyServerAct, SIGNAL(triggered()), this, SLOT(slotImportKeyFromKeyServer()));

    exportKeyToClipboardAct = new QAction(tr("Export To &Clipboard"), this);
    exportKeyToClipboardAct->setIcon(QIcon(":export_key_to_clipboard.png"));
    exportKeyToClipboardAct->setToolTip(tr("Export Selected Key(s) To Clipboard"));
    connect(exportKeyToClipboardAct, SIGNAL(triggered()), this, SLOT(slotExportKeyToClipboard()));

    exportKeyToFileAct = new QAction(tr("Export To &File"), this);
    exportKeyToFileAct->setIcon(QIcon(":export_key_to_file.png"));
    exportKeyToFileAct->setToolTip(tr("Export Selected Key(s) To File"));
    connect(exportKeyToFileAct, SIGNAL(triggered()), this, SLOT(slotExportKeyToFile()));

    deleteSelectedKeysAct = new QAction(tr("Delete Selected Key(s)"), this);
    deleteSelectedKeysAct->setToolTip(tr("Delete the Selected keys"));
    connect(deleteSelectedKeysAct, SIGNAL(triggered()), this, SLOT(slotDeleteSelectedKeys()));

    deleteCheckedKeysAct = new QAction(tr("Delete Checked Key(s)"), this);
    deleteCheckedKeysAct->setToolTip(tr("Delete the Checked keys"));
    deleteCheckedKeysAct->setIcon(QIcon(":button_cancel.png"));
    connect(deleteCheckedKeysAct, SIGNAL(triggered()), this, SLOT(slotDeleteCheckedKeys()));

    showKeyDetailsAct = new QAction(tr("Show Keydetails"), this);
    showKeyDetailsAct->setToolTip(tr("Show Details for this Key"));
    connect(showKeyDetailsAct, SIGNAL(triggered()), this, SLOT(slotShowKeyDetails()));
}

void KeyMgmt::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(closeAct);

    keyMenu = menuBar()->addMenu(tr("&Key"));
    generateKeyMenu = keyMenu->addMenu(tr("&Generate Key"));
    generateKeyMenu->addAction(generateKeyPairAct);
    generateKeyMenu->addAction(generateSubKeyAct);

    importKeyMenu = keyMenu->addMenu(tr("&Import Key From..."));
    importKeyMenu->addAction(importKeyFromFileAct);
    importKeyMenu->addAction(importKeyFromClipboardAct);
    importKeyMenu->addAction(importKeyFromKeyServerAct);
    keyMenu->addAction(exportKeyToFileAct);
    keyMenu->addAction(exportKeyToClipboardAct);
    keyMenu->addSeparator();
    keyMenu->addAction(deleteCheckedKeysAct);
}

void KeyMgmt::createToolBars()
{
    QToolBar *keyToolBar = addToolBar(tr("Key"));
    keyToolBar->setObjectName("keytoolbar");

    // add button with popup menu for import
    auto* generateToolButton = new QToolButton(this);
    generateToolButton->setMenu(generateKeyMenu);
    generateToolButton->setPopupMode(QToolButton::InstantPopup);
    generateToolButton->setIcon(QIcon(":key_generate.png"));
    generateToolButton->setToolTip(tr("Generate key"));
    generateToolButton->setText(tr("Generate key"));
    generateToolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    keyToolBar->addWidget(generateToolButton);

    // add button with popup menu for import
    auto* toolButton = new QToolButton(this);
    toolButton->setMenu(importKeyMenu);
    toolButton->setPopupMode(QToolButton::InstantPopup);
    toolButton->setIcon(QIcon(":key_import.png"));
    toolButton->setToolTip(tr("Import key"));
    toolButton->setText(tr("Import key from"));
    toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    keyToolBar->addWidget(toolButton);

    keyToolBar->addSeparator();
    keyToolBar->addAction(deleteCheckedKeysAct);
    keyToolBar->addSeparator();
    keyToolBar->addAction(exportKeyToFileAct);
    keyToolBar->addAction(exportKeyToClipboardAct);

}

void KeyMgmt::slotImportKeys(QByteArray inBuffer)
{
    GpgImportInformation result = mCtx->importKey(std::move(inBuffer));
    new KeyImportDetailDialog(mCtx, result, this);

}

void KeyMgmt::slotImportKeyFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Key"), "", tr("Key Files") + " (*.asc *.txt);;"+tr("Keyring files")+" (*.gpg);;All Files (*)");
    if (! fileName.isNull()) {
        QFile file;
        file.setFileName(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << tr("Couldn't Open File: ") + fileName;
            return;
        }
        QByteArray inBuffer = file.readAll();
        slotImportKeys(inBuffer);
        file.close();
    }
}

void KeyMgmt::slotImportKeyFromKeyServer()
{
    importDialog = new KeyServerImportDialog(mCtx, mKeyList, this);
    importDialog->show();
}

void KeyMgmt::slotImportKeyFromClipboard()
{
    QClipboard *cb = QApplication::clipboard();
    slotImportKeys(cb->text(QClipboard::Clipboard).toUtf8());
}

void KeyMgmt::slotDeleteSelectedKeys()
{
    deleteKeysWithWarning(mKeyList->getSelected());
}

void KeyMgmt::slotDeleteCheckedKeys()
{
    deleteKeysWithWarning(mKeyList->getChecked());
}

void KeyMgmt::deleteKeysWithWarning(QStringList *uidList)
{
    /**
     * TODO: Different Messages for private/public key, check if
     * more than one selected... compare to seahorse "delete-dialog"
     */

    if (uidList->isEmpty()) {
        return;
    }
    QString keynames;
    for (const auto &uid : *uidList) {
        auto &key = mCtx->getKeyById(uid);
        keynames.append(key.name);
        keynames.append("<i> &lt;");
        keynames.append(key.email);
        keynames.append("&gt; </i><br/>");
    }

    int ret = QMessageBox::warning(this, tr("Deleting Keys"),
                                   "<b>"+tr("Are you sure that you want to delete the following keys?")+"</b><br/><br/>"+keynames+
                                   +"<br/>"+tr("The action can not be undone."),
                                   QMessageBox::No | QMessageBox::Yes);

    if (ret == QMessageBox::Yes) {
        mCtx->deleteKeys(uidList);
    }
}

void KeyMgmt::slotShowKeyDetails()
{
    if (mKeyList->getSelected()->isEmpty()) {
        return;
    }

    auto &key = mCtx->getKeyById(mKeyList->getSelected()->first());

    new KeyDetailsDialog(mCtx, key);
}

void KeyMgmt::slotExportKeyToFile()
{
    auto *keyArray = new QByteArray();
    if (!mCtx->exportKeys(mKeyList->getChecked(), keyArray)) {
        return;
    }
    auto &key = mCtx->getKeyById(mKeyList->getSelected()->first());
    QString fileString = key.name + " " + key.email+ "(" + key.id+ ")_pub.asc";

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Key To File"), fileString, tr("Key Files") + " (*.asc *.txt);;All Files (*)");
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);
    stream << *keyArray;
    file.close();
    delete keyArray;
    emit signalStatusBarChanged(QString(tr("key(s) exported")));
}

void KeyMgmt::slotExportKeyToClipboard()
{
    auto *keyArray = new QByteArray();
    QClipboard *cb = QApplication::clipboard();
    if (!mCtx->exportKeys(mKeyList->getChecked(), keyArray)) {
        return;
    }
    cb->setText(*keyArray);
    delete keyArray;
}

void KeyMgmt::slotGenerateKeyDialog()
{
    auto *keyGenDialog = new KeyGenDialog(mCtx,this);
    keyGenDialog->show();
}

void KeyMgmt::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    //settings.setValue("geometry", saveGeometry());
    settings.setValue("keymgmt/windowState", saveState());
    settings.setValue("keymgmt/pos", pos());
    settings.setValue("keymgmt/size", size());

    QMainWindow::closeEvent(event);
}

void KeyMgmt::slotGenerateSubKey() {
    auto selectedList = mKeyList->getSelected();
    if(selectedList->empty()) {
        QMessageBox::information(nullptr,
                                 tr("Invalid Operation"),
                                 tr("Please select one KeyPair before doing this operation."));
        return;
    }
    const auto &key = mCtx->getKeyById(selectedList->first());
    if(!key.is_private_key) {
        QMessageBox::critical(nullptr,
                              tr("Invalid Operation"),
                              tr("If a key pair does not have a private key then it will not be able to generate sub-keys."));
        return;
    }

    auto dialog = new SubkeyGenerateDialog(mCtx, key, this);
    dialog->show();
}
