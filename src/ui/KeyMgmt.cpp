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

#include "ui/KeyMgmt.h"

#include <utility>

#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExportor.h"
#include "gpg/function/GpgKeyOpera.h"
#include "ui/SignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {
KeyMgmt::KeyMgmt(QWidget* parent) : QMainWindow(parent) {
  /* the list of Keys available*/
  mKeyList = new KeyList(true, this);

  mKeyList->addListGroupTab(_("All"), KeyListRow::SECRET_OR_PUBLIC_KEY);

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

  mKeyList->addListGroupTab(
      _("No Master Key"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return !key.has_master_key() &&
               !(key.revoked() || key.disabled() || key.expired());
      });

  mKeyList->addListGroupTab(
      _("Revoked"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool { return key.revoked(); });

  mKeyList->addListGroupTab(
      _("Expired"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool { return key.expired(); });

  setCentralWidget(mKeyList);
  mKeyList->setDoubleClickedAction([this](const GpgKey& key, QWidget* parent) {
    new KeyDetailsDialog(key, parent);
  });

  mKeyList->slotRefresh();

  createActions();
  createMenus();
  createToolBars();
  connect(this, SIGNAL(signalStatusBarChanged(QString)), this->parent(),
          SLOT(slotSetStatusBarText(QString)));

  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    int width = settings.lookup("window.icon_size.width");
    int height = settings.lookup("window.icon_size.height");

    this->setIconSize(QSize(width, height));

  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("icon_size");
  }

  // icon_style
  try {
    int s_icon_style = settings.lookup("window.icon_style");
    auto icon_style = static_cast<Qt::ToolButtonStyle>(s_icon_style);
    this->setToolButtonStyle(icon_style);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("icon_style");
  }

  auto pos = QPoint(50, 50);
  LOG(INFO) << "parent" << parent;
  if (parent) pos += parent->pos();
  LOG(INFO) << "pos default" << pos.x() << pos.y();
  auto size = QSize(900, 600);

  try {
    int x, y, width, height;
    x = settings.lookup("window.key_management.position.x");
    y = settings.lookup("window.key_management.position.y");
    width = settings.lookup("window.key_management.size.width");
    height = settings.lookup("window.key_management.size.height");
    pos = QPoint(x, y);
    size = QSize(width, height);

    std::string window_state =
        settings.lookup("window.key_management.window_state");

    // state sets pos & size of dock-widgets
    this->restoreState(
        QByteArray::fromBase64(QByteArray::fromStdString(window_state)));

  } catch (...) {
    LOG(WARNING) << "cannot read pos or size from settings";
  }

  this->resize(size);
  this->move(pos);

  setWindowTitle(_("KeyPair Management"));
  mKeyList->addMenuAction(deleteSelectedKeysAct);
  mKeyList->addMenuAction(showKeyDetailsAct);

  connect(this, SIGNAL(signalKeyStatusUpdated()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));
}

void KeyMgmt::createActions() {
  openKeyFileAct = new QAction(_("Open"), this);
  openKeyFileAct->setShortcut(QKeySequence(_("Ctrl+O")));
  openKeyFileAct->setToolTip(_("Open Key File"));
  connect(importKeyFromFileAct, &QAction::triggered, this,
          [&]() { CommonUtils::GetInstance()->slotImportKeyFromFile(this); });

  closeAct = new QAction(_("Close"), this);
  closeAct->setShortcut(QKeySequence(_("Ctrl+Q")));
  closeAct->setIcon(QIcon(":exit.png"));
  closeAct->setToolTip(_("Close"));
  connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));

  generateKeyPairAct = new QAction(_("New Keypair"), this);
  generateKeyPairAct->setShortcut(QKeySequence(_("Ctrl+N")));
  generateKeyPairAct->setIcon(QIcon(":key_generate.png"));
  generateKeyPairAct->setToolTip(_("Generate KeyPair"));
  connect(generateKeyPairAct, SIGNAL(triggered()), this,
          SLOT(slotGenerateKeyDialog()));

  generateSubKeyAct = new QAction(_("New Subkey"), this);
  generateSubKeyAct->setShortcut(QKeySequence(_("Ctrl+Shift+N")));
  generateSubKeyAct->setIcon(QIcon(":key_generate.png"));
  generateSubKeyAct->setToolTip(_("Generate Subkey For Selected KeyPair"));
  connect(generateSubKeyAct, SIGNAL(triggered()), this,
          SLOT(slotGenerateSubKey()));

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

  exportKeyToClipboardAct = new QAction(_("Export To Clipboard"), this);
  exportKeyToClipboardAct->setIcon(QIcon(":export_key_to_clipboard.png"));
  exportKeyToClipboardAct->setToolTip(_("Export Selected Key(s) To Clipboard"));
  connect(exportKeyToClipboardAct, SIGNAL(triggered()), this,
          SLOT(slotExportKeyToClipboard()));

  exportKeyToFileAct = new QAction(_("Export To File"), this);
  exportKeyToFileAct->setIcon(QIcon(":export_key_to_file.png"));
  exportKeyToFileAct->setToolTip(_("Export Selected Key(s) To File"));
  connect(exportKeyToFileAct, SIGNAL(triggered()), this,
          SLOT(slotExportKeyToFile()));

  exportKeyAsOpenSSHFormat = new QAction(_("Export As OpenSSH"), this);
  exportKeyAsOpenSSHFormat->setIcon(QIcon(":ssh-key.png"));
  exportKeyAsOpenSSHFormat->setToolTip(
      _("Export Selected Key(s) As OpenSSH Format to File"));
  connect(exportKeyAsOpenSSHFormat, SIGNAL(triggered()), this,
          SLOT(slotExportAsOpenSSHFormat()));

  deleteSelectedKeysAct = new QAction(_("Delete Selected Key(s)"), this);
  deleteSelectedKeysAct->setToolTip(_("Delete the Selected keys"));
  connect(deleteSelectedKeysAct, SIGNAL(triggered()), this,
          SLOT(slotDeleteSelectedKeys()));

  deleteCheckedKeysAct = new QAction(_("Delete Checked Key(s)"), this);
  deleteCheckedKeysAct->setToolTip(_("Delete the Checked keys"));
  deleteCheckedKeysAct->setIcon(QIcon(":button_delete.png"));
  connect(deleteCheckedKeysAct, SIGNAL(triggered()), this,
          SLOT(slotDeleteCheckedKeys()));

  showKeyDetailsAct = new QAction(_("Show Key Details"), this);
  showKeyDetailsAct->setToolTip(_("Show Details for this Key"));
  connect(showKeyDetailsAct, SIGNAL(triggered()), this,
          SLOT(slotShowKeyDetails()));
}

void KeyMgmt::createMenus() {
  fileMenu = menuBar()->addMenu(_("File"));
  fileMenu->addAction(openKeyFileAct);
  fileMenu->addAction(closeAct);

  keyMenu = menuBar()->addMenu(_("Key"));
  generateKeyMenu = keyMenu->addMenu(_("Generate Key"));
  generateKeyMenu->addAction(generateKeyPairAct);
  generateKeyMenu->addAction(generateSubKeyAct);

  importKeyMenu = keyMenu->addMenu(_("Import Key"));
  importKeyMenu->addAction(importKeyFromFileAct);
  importKeyMenu->addAction(importKeyFromClipboardAct);
  importKeyMenu->addAction(importKeyFromKeyServerAct);

  keyMenu->addAction(exportKeyToFileAct);
  keyMenu->addAction(exportKeyToClipboardAct);
  keyMenu->addAction(exportKeyAsOpenSSHFormat);
  keyMenu->addSeparator();
  keyMenu->addAction(deleteCheckedKeysAct);
}

void KeyMgmt::createToolBars() {
  QToolBar* keyToolBar = addToolBar(_("Key"));
  keyToolBar->setObjectName("keytoolbar");

  // add button with popup menu for import
  auto* generateToolButton = new QToolButton(this);
  generateToolButton->setMenu(generateKeyMenu);
  generateToolButton->setPopupMode(QToolButton::InstantPopup);
  generateToolButton->setIcon(QIcon(":key_generate.png"));
  generateToolButton->setText(_("Generate"));
  generateToolButton->setToolTip(_("Generate A New Keypair or Subkey"));
  generateToolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  keyToolBar->addWidget(generateToolButton);

  // add button with popup menu for import
  auto* toolButton = new QToolButton(this);
  toolButton->setMenu(importKeyMenu);
  toolButton->setPopupMode(QToolButton::InstantPopup);
  toolButton->setIcon(QIcon(":key_import.png"));
  toolButton->setToolTip(_("Import key"));
  toolButton->setText(_("Import Key"));
  toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  keyToolBar->addWidget(toolButton);

  keyToolBar->addSeparator();
  keyToolBar->addAction(deleteCheckedKeysAct);
  keyToolBar->addSeparator();
  keyToolBar->addAction(exportKeyToFileAct);
  keyToolBar->addAction(exportKeyToClipboardAct);
  keyToolBar->addAction(exportKeyAsOpenSSHFormat);
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
      this, _("Deleting Keys"),
      "<b>" +
          QString(
              _("Are you sure that you want to delete the following keys?")) +
          "</b><br/><br/>" + keynames + +"<br/>" +
          _("The action can not be undone."),
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
    QMessageBox::critical(nullptr, _("Error"), _("Key Not Found."));
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
    QMessageBox::critical(nullptr, _("Error"), _("Key Not Found."));
    return;
  }
  QString fileString = QString::fromStdString(key.name() + " " + key.email() +
                                              "(" + key.id() + ")_pub.asc");

  QString file_name = QFileDialog::getSaveFileName(
      this, _("Export Key To File"), fileString,
      QString(_("Key Files")) + " (*.asc *.txt);;All Files (*)");

  write_buffer_to_file(file_name.toStdString(), *key_export_data);

  emit signalStatusBarChanged(QString(_("key(s) exported")));
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

void KeyMgmt::closeEvent(QCloseEvent* event) {
  slotSaveWindowState();
  QMainWindow::closeEvent(event);
}

void KeyMgmt::slotGenerateSubKey() {
  auto keys_selected = mKeyList->getSelected();
  if (keys_selected->empty()) {
    QMessageBox::information(
        nullptr, _("Invalid Operation"),
        _("Please select one KeyPair before doing this operation."));
    return;
  }
  const auto key = GpgKeyGetter::GetInstance().GetKey(keys_selected->front());
  if (!key.good()) {
    QMessageBox::critical(nullptr, _("Error"), _("Key Not Found."));
    return;
  }
  if (!key.is_private_key()) {
    QMessageBox::critical(nullptr, _("Invalid Operation"),
                          _("If a key pair does not have a private key then "
                            "it will not be able to generate sub-keys."));
    return;
  }

  auto dialog = new SubkeyGenerateDialog(key.id(), this);
  dialog->show();
}
void KeyMgmt::slotSaveWindowState() {
  auto& settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("window") ||
      settings.lookup("window").getType() != libconfig::Setting::TypeGroup)
    settings.add("window", libconfig::Setting::TypeGroup);

  auto& window = settings["window"];

  if (!window.exists("key_management") ||
      window.lookup("key_management").getType() !=
          libconfig::Setting::TypeGroup)
    window.add("key_management", libconfig::Setting::TypeGroup);

  auto& key_management = window["key_management"];

  if (!key_management.exists("position") ||
      key_management.lookup("position").getType() !=
          libconfig::Setting::TypeGroup) {
    auto& position =
        key_management.add("position", libconfig::Setting::TypeGroup);
    position.add("x", libconfig::Setting::TypeInt) = pos().x();
    position.add("y", libconfig::Setting::TypeInt) = pos().y();
  } else {
    key_management["position"]["x"] = pos().x();
    key_management["position"]["y"] = pos().y();
  }

  if (!key_management.exists("size") ||
      key_management.lookup("size").getType() !=
          libconfig::Setting::TypeGroup) {
    auto& size = key_management.add("size", libconfig::Setting::TypeGroup);
    size.add("width", libconfig::Setting::TypeInt) = QWidget::width();
    size.add("height", libconfig::Setting::TypeInt) = QWidget::height();
  } else {
    key_management["size"]["width"] = QWidget::width();
    key_management["size"]["height"] = QWidget::height();
  }

  if (!key_management.exists("window_state"))
    key_management.add("window_state", libconfig::Setting::TypeString) =
        saveState().toBase64().toStdString();

  GlobalSettingStation::GetInstance().Sync();
}

void KeyMgmt::slotExportAsOpenSSHFormat() {
  ByteArrayPtr key_export_data = nullptr;
  auto keys_checked = mKeyList->getChecked();

  if (keys_checked->empty()) {
    QMessageBox::critical(nullptr, _("Error"), _("No Key Checked."));
    return;
  }

  auto key = GpgKeyGetter::GetInstance().GetKey(keys_checked->front());
  if (!GpgKeyImportExportor::GetInstance().ExportKeyOpenSSH(key,
                                                            key_export_data)) {
    QMessageBox::critical(nullptr, _("Error"),
                          _("An error occur in exporting."));
    return;
  }

  if (key_export_data->empty()) {
    QMessageBox::critical(
        nullptr, _("Error"),
        _("This key may not be able to export as OpenSSH format. Please check "
          "the key-size of the subkey(s) used to sign."));
    return;
  }

  key = GpgKeyGetter::GetInstance().GetKey(keys_checked->front());
  if (!key.good()) {
    QMessageBox::critical(nullptr, _("Error"), _("Key Not Found."));
    return;
  }
  QString fileString = QString::fromStdString(key.name() + " " + key.email() +
                                              "(" + key.id() + ").pub");

  QString file_name = QFileDialog::getSaveFileName(
      this, _("Export OpenSSH Key To File"), fileString,
      QString(_("OpenSSH Public Key Files")) + " (*.pub);;All Files (*)");

  if (!file_name.isEmpty()) {
    write_buffer_to_file(file_name.toStdString(), *key_export_data);
    emit signalStatusBarChanged(QString(_("key(s) exported")));
  }
}

}  // namespace GpgFrontend::UI
