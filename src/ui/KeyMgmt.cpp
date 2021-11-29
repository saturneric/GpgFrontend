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

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExportor.h"
#include "gpg/function/GpgKeyOpera.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {
KeyMgmt::KeyMgmt(QWidget* parent)
    : QMainWindow(parent),
      appPath(qApp->applicationDirPath()),
      settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini",
               QSettings::IniFormat) {
  /* the list of Keys available*/
  mKeyList = new KeyList();
  mKeyList->setColumnWidth(2, 250);
  mKeyList->setColumnWidth(3, 250);
  setCentralWidget(mKeyList);
  mKeyList->setDoubleClickedAction([this](const GpgKey& key, QWidget* parent) {
    new KeyDetailsDialog(key, parent);
  });

  createActions();
  createMenus();
  createToolBars();
  connect(this, SIGNAL(signalStatusBarChanged(QString)), this->parent(),
          SLOT(slotSetStatusBarText(QString)));

  /* Restore the iconstyle */
  this->settings.sync();

  QSize iconSize = settings.value("toolbar/iconsize", QSize(24, 24)).toSize();
  settings.setValue("toolbar/iconsize", iconSize);

  Qt::ToolButtonStyle buttonStyle = static_cast<Qt::ToolButtonStyle>(
      settings.value("toolbar/iconstyle", Qt::ToolButtonTextUnderIcon)
          .toUInt());
  this->setIconSize(iconSize);
  this->setToolButtonStyle(buttonStyle);

  // state sets pos & size of dock-widgets
  this->restoreState(this->settings.value("keymgmt/windowState").toByteArray());

  qDebug() << "windows/windowSave"
           << this->settings.value("window/windowSave").toBool();

  // Restore window size & location
  if (this->settings.value("keymgmt/setWindowSize").toBool()) {
    QPoint pos = settings.value("keymgmt/pos", QPoint(100, 100)).toPoint();
    QSize size = settings.value("keymgmt/size", QSize(900, 600)).toSize();
    qDebug() << "Settings size" << size << "pos" << pos;
    this->setMinimumSize(size);
    this->move(pos);
  } else {
    qDebug() << "Use default min windows size and pos";
    QPoint defaultPoint(100, 100);
    QSize defaultMinSize(900, 600);
    this->setMinimumSize(defaultMinSize);
    this->move(defaultPoint);
    this->settings.setValue("keymgmt/pos", defaultPoint);
    this->settings.setValue("keymgmt/size", defaultMinSize);
    this->settings.setValue("keymgmt/setWindowSize", true);
  }

  setWindowTitle(tr("Key Pair Management"));
  mKeyList->addMenuAction(deleteSelectedKeysAct);
  mKeyList->addMenuAction(showKeyDetailsAct);

  connect(this, SIGNAL(signalKeyStatusUpdated()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));
}

void KeyMgmt::createActions() {
  openKeyFileAct = new QAction(tr("&Open"), this);
  openKeyFileAct->setShortcut(tr("Ctrl+O"));
  openKeyFileAct->setToolTip(tr("Open Key File"));
  connect(openKeyFileAct, SIGNAL(triggered()), this,
          SLOT(slotImportKeyFromFile()));

  closeAct = new QAction(tr("&Close"), this);
  closeAct->setShortcut(tr("Ctrl+Q"));
  closeAct->setIcon(QIcon(":exit.png"));
  closeAct->setToolTip(tr("Close"));
  connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));

  generateKeyPairAct = new QAction(tr("New Keypair"), this);
  generateKeyPairAct->setShortcut(tr("Ctrl+N"));
  generateKeyPairAct->setIcon(QIcon(":key_generate.png"));
  generateKeyPairAct->setToolTip(tr("Generate KeyPair"));
  connect(generateKeyPairAct, SIGNAL(triggered()), this,
          SLOT(slotGenerateKeyDialog()));

  generateSubKeyAct = new QAction(tr("New Subkey"), this);
  generateSubKeyAct->setShortcut(tr("Ctrl+Shift+N"));
  generateSubKeyAct->setIcon(QIcon(":key_generate.png"));
  generateSubKeyAct->setToolTip(tr("Generate Subkey For Selected KeyPair"));
  connect(generateSubKeyAct, SIGNAL(triggered()), this,
          SLOT(slotGenerateSubKey()));

  importKeyFromFileAct = new QAction(tr("&File"), this);
  importKeyFromFileAct->setIcon(QIcon(":import_key_from_file.png"));
  importKeyFromFileAct->setToolTip(tr("Import New Key From File"));
  connect(importKeyFromFileAct, SIGNAL(triggered()), this,
          SLOT(slotImportKeyFromFile()));

  importKeyFromClipboardAct = new QAction(tr("&Clipboard"), this);
  importKeyFromClipboardAct->setIcon(QIcon(":import_key_from_clipboard.png"));
  importKeyFromClipboardAct->setToolTip(tr("Import New Key From Clipboard"));
  connect(importKeyFromClipboardAct, SIGNAL(triggered()), this,
          SLOT(slotImportKeyFromClipboard()));

  importKeyFromKeyServerAct = new QAction(tr("&Keyserver"), this);
  importKeyFromKeyServerAct->setIcon(QIcon(":import_key_from_server.png"));
  importKeyFromKeyServerAct->setToolTip(tr("Import New Key From Keyserver"));
  connect(importKeyFromKeyServerAct, SIGNAL(triggered()), this,
          SLOT(slotImportKeyFromKeyServer()));

  exportKeyToClipboardAct = new QAction(tr("Export To &Clipboard"), this);
  exportKeyToClipboardAct->setIcon(QIcon(":export_key_to_clipboard.png"));
  exportKeyToClipboardAct->setToolTip(
      tr("Export Selected Key(s) To Clipboard"));
  connect(exportKeyToClipboardAct, SIGNAL(triggered()), this,
          SLOT(slotExportKeyToClipboard()));

  exportKeyToFileAct = new QAction(tr("Export To &File"), this);
  exportKeyToFileAct->setIcon(QIcon(":export_key_to_file.png"));
  exportKeyToFileAct->setToolTip(tr("Export Selected Key(s) To File"));
  connect(exportKeyToFileAct, SIGNAL(triggered()), this,
          SLOT(slotExportKeyToFile()));

  deleteSelectedKeysAct = new QAction(tr("Delete Selected Key(s)"), this);
  deleteSelectedKeysAct->setToolTip(tr("Delete the Selected keys"));
  connect(deleteSelectedKeysAct, SIGNAL(triggered()), this,
          SLOT(slotDeleteSelectedKeys()));

  deleteCheckedKeysAct = new QAction(tr("Delete Checked Key(s)"), this);
  deleteCheckedKeysAct->setToolTip(tr("Delete the Checked keys"));
  deleteCheckedKeysAct->setIcon(QIcon(":button_delete.png"));
  connect(deleteCheckedKeysAct, SIGNAL(triggered()), this,
          SLOT(slotDeleteCheckedKeys()));

  showKeyDetailsAct = new QAction(tr("Show Key Details"), this);
  showKeyDetailsAct->setToolTip(tr("Show Details for this Key"));
  connect(showKeyDetailsAct, SIGNAL(triggered()), this,
          SLOT(slotShowKeyDetails()));
}

void KeyMgmt::createMenus() {
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(openKeyFileAct);
  fileMenu->addAction(closeAct);

  keyMenu = menuBar()->addMenu(tr("&Key"));
  generateKeyMenu = keyMenu->addMenu(tr("&Generate Key"));
  generateKeyMenu->addAction(generateKeyPairAct);
  generateKeyMenu->addAction(generateSubKeyAct);

  importKeyMenu = keyMenu->addMenu(tr("&Import Key"));
  importKeyMenu->addAction(importKeyFromFileAct);
  importKeyMenu->addAction(importKeyFromClipboardAct);
  importKeyMenu->addAction(importKeyFromKeyServerAct);
  keyMenu->addAction(exportKeyToFileAct);
  keyMenu->addAction(exportKeyToClipboardAct);
  keyMenu->addSeparator();
  keyMenu->addAction(deleteCheckedKeysAct);
}

void KeyMgmt::createToolBars() {
  QToolBar* keyToolBar = addToolBar(tr("Key"));
  keyToolBar->setObjectName("keytoolbar");

  // add button with popup menu for import
  auto* generateToolButton = new QToolButton(this);
  generateToolButton->setMenu(generateKeyMenu);
  generateToolButton->setPopupMode(QToolButton::InstantPopup);
  generateToolButton->setIcon(QIcon(":key_generate.png"));
  generateToolButton->setText(tr("Generate"));
  generateToolButton->setToolTip(tr("Generate A New Keypair or Subkey"));
  generateToolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  keyToolBar->addWidget(generateToolButton);

  // add button with popup menu for import
  auto* toolButton = new QToolButton(this);
  toolButton->setMenu(importKeyMenu);
  toolButton->setPopupMode(QToolButton::InstantPopup);
  toolButton->setIcon(QIcon(":key_import.png"));
  toolButton->setToolTip(tr("Import key"));
  toolButton->setText(tr("Import Key"));
  toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  keyToolBar->addWidget(toolButton);

  keyToolBar->addSeparator();
  keyToolBar->addAction(deleteCheckedKeysAct);
  keyToolBar->addSeparator();
  keyToolBar->addAction(exportKeyToFileAct);
  keyToolBar->addAction(exportKeyToClipboardAct);
}

void KeyMgmt::slotImportKeys(const std::string& in_buffer) {
  GpgImportInformation result = GpgKeyImportExportor::GetInstance().ImportKey(
      std::make_unique<ByteArray>(in_buffer));
  emit signalKeyStatusUpdated();
  new KeyImportDetailDialog(result, false, this);
}

void KeyMgmt::slotImportKeyFromFile() {
  QString file_name = QFileDialog::getOpenFileName(
      this, tr("Open Key"), "",
      tr("Key Files") + " (*.asc *.txt);;" + tr("Keyring files") +
          " (*.gpg);;All Files (*)");
  if (!file_name.isNull()) {
    slotImportKeys(read_all_data_in_file(file_name.toStdString()));
  }
}

void KeyMgmt::slotImportKeyFromKeyServer() {
  importDialog = new KeyServerImportDialog(mKeyList, false, this);
  importDialog->show();
}

void KeyMgmt::slotImportKeyFromClipboard() {
  QClipboard* cb = QApplication::clipboard();
  slotImportKeys(cb->text(QClipboard::Clipboard).toUtf8().toStdString());
}

void KeyMgmt::slotDeleteSelectedKeys() {
  deleteKeysWithWarning(mKeyList->getSelected());
}

void KeyMgmt::slotDeleteCheckedKeys() {
  deleteKeysWithWarning(mKeyList->getChecked());
}

void KeyMgmt::deleteKeysWithWarning(KeyIdArgsListPtr key_ids) {
  /**
   * TODO: Different Messages for private/public key, check if
   * more than one selected... compare to seahorse "delete-dialog"
   */

  LOG(INFO) << "KeyMgmt::deleteKeysWithWarning Called";

  if (key_ids->empty()) return;
  QString keynames;
  for (const auto& key_id : *key_ids) {
    auto key = GpgKeyGetter::GetInstance().GetKey(key_id);
    if (!key.good()) continue;
    keynames.append(QString::fromStdString(key.name()));
    keynames.append("<i> &lt;");
    keynames.append(QString::fromStdString(key.email()));
    keynames.append("&gt; </i><br/>");
  }

  int ret = QMessageBox::warning(
      this, tr("Deleting Keys"),
      "<b>" + tr("Are you sure that you want to delete the following keys?") +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    GpgKeyOpera::GetInstance().DeleteKeys(std::move(key_ids));
    emit signalKeyStatusUpdated();
  }
}

void KeyMgmt::slotShowKeyDetails() {
  auto keys_selected = mKeyList->getSelected();
  if (keys_selected->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(keys_selected->front());

  if (!key.good()) {
    QMessageBox::critical(nullptr, tr("Error"), tr("Key Not Found."));
    return;
  }

  new KeyDetailsDialog(key);
}

void KeyMgmt::slotExportKeyToFile() {
  ByteArrayPtr key_export_data = nullptr;
  auto keys_checked = mKeyList->getChecked();
  if (!GpgKeyImportExportor::GetInstance().ExportKeys(keys_checked,
                                                      key_export_data)) {
    return;
  }
  auto key =
      GpgKeyGetter::GetInstance().GetKey(mKeyList->getSelected()->front());
  if (!key.good()) {
    QMessageBox::critical(nullptr, tr("Error"), tr("Key Not Found."));
    return;
  }
  QString fileString = QString::fromStdString(key.name() + " " + key.email() +
                                              "(" + key.id() + ")_pub.asc");

  QString file_name = QFileDialog::getSaveFileName(
      this, tr("Export Key To File"), fileString,
      tr("Key Files") + " (*.asc *.txt);;All Files (*)");

  write_buffer_to_file(file_name.toStdString(), *key_export_data);

  emit signalStatusBarChanged(QString(tr("key(s) exported")));
}

void KeyMgmt::slotExportKeyToClipboard() {
  ByteArrayPtr key_export_data = nullptr;
  auto keys_checked = mKeyList->getChecked();
  if (!GpgKeyImportExportor::GetInstance().ExportKeys(keys_checked,
                                                      key_export_data)) {
    return;
  }
  QApplication::clipboard()->setText(QString::fromStdString(*key_export_data));
}

void KeyMgmt::slotGenerateKeyDialog() {
  auto* keyGenDialog = new KeyGenDialog(this);
  keyGenDialog->show();
}

void KeyMgmt::closeEvent(QCloseEvent* event) { QMainWindow::closeEvent(event); }

void KeyMgmt::slotGenerateSubKey() {
  auto keys_selected = mKeyList->getSelected();
  if (keys_selected->empty()) {
    QMessageBox::information(
        nullptr, tr("Invalid Operation"),
        tr("Please select one KeyPair before doing this operation."));
    return;
  }
  const auto key = GpgKeyGetter::GetInstance().GetKey(keys_selected->front());
  if (!key.good()) {
    QMessageBox::critical(nullptr, tr("Error"), tr("Key Not Found."));
    return;
  }
  if (!key.is_private_key()) {
    QMessageBox::critical(nullptr, tr("Invalid Operation"),
                          tr("If a key pair does not have a private key then "
                             "it will not be able to generate sub-keys."));
    return;
  }

  auto dialog = new SubkeyGenerateDialog(key.id(), this);
  dialog->show();
}

}  // namespace GpgFrontend::UI
