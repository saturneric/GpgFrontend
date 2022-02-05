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

#include "ui/KeyMgmt.h"

#include <utility>

#include "core/function/GpgKeyGetter.h"
#include "core/function/GpgKeyImportExporter.h"
#include "core/function/GpgKeyOpera.h"
#include "core/key_package/KeyPackageOperator.h"
#include "ui/SignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/import_export/ExportKeyPackageDialog.h"
#include "ui/key_generate/SubkeyGenerateDialog.h"
#include "ui/main_window/MainWindow.h"
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {
KeyMgmt::KeyMgmt(QWidget* parent) : QMainWindow(parent) {
  /* the list of Keys available*/
  key_list_ = new KeyList(KeyMenuAbility::ALL, this);

  key_list_->AddListGroupTab(_("All"), KeyListRow::SECRET_OR_PUBLIC_KEY);

  key_list_->AddListGroupTab(
      _("Only Public Key"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return !key.IsPrivateKey() &&
               !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  key_list_->AddListGroupTab(
      _("Has Private Key"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return key.IsPrivateKey() &&
               !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  key_list_->AddListGroupTab(
      _("No Primary Key"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool {
        return !key.IsHasMasterKey() &&
               !(key.IsRevoked() || key.IsDisabled() || key.IsExpired());
      });

  key_list_->AddListGroupTab(
      _("Revoked"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool { return key.IsRevoked(); });

  key_list_->AddListGroupTab(
      _("Expired"), KeyListRow::SECRET_OR_PUBLIC_KEY,
      KeyListColumn::TYPE | KeyListColumn::NAME | KeyListColumn::EmailAddress |
          KeyListColumn::Usage | KeyListColumn::Validity,
      [](const GpgKey& key) -> bool { return key.IsExpired(); });

  setCentralWidget(key_list_);
  key_list_->SetDoubleClickedAction([this](const GpgKey& key, QWidget* parent) {
    new KeyDetailsDialog(key, parent);
  });

  key_list_->SlotRefresh();

  create_actions();
  create_menus();
  create_tool_bars();

  connect(this, &KeyMgmt::SignalStatusBarChanged,
          qobject_cast<MainWindow*>(this->parent()),
          &MainWindow::SlotSetStatusBarText);

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
  this->statusBar()->show();

  setWindowTitle(_("KeyPair Management"));
  key_list_->AddMenuAction(delete_selected_keys_act_);
  key_list_->AddMenuAction(show_key_details_act_);

  connect(this, &KeyMgmt::SignalKeyStatusUpdated, SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh);
  connect(SignalStation::GetInstance(), &SignalStation::SignalRefreshStatusBar,
          this, [=](const QString& message, int timeout) {
            statusBar()->showMessage(message, timeout);
          });
}

void KeyMgmt::create_actions() {
  open_key_file_act_ = new QAction(_("Open"), this);
  open_key_file_act_->setShortcut(QKeySequence(_("Ctrl+O")));
  open_key_file_act_->setToolTip(_("Open Key File"));
  connect(open_key_file_act_, &QAction::triggered, this,
          [&]() { CommonUtils::GetInstance()->SlotImportKeyFromFile(this); });

  close_act_ = new QAction(_("Close"), this);
  close_act_->setShortcut(QKeySequence(_("Ctrl+Q")));
  close_act_->setIcon(QIcon(":exit.png"));
  close_act_->setToolTip(_("Close"));
  connect(close_act_, &QAction::triggered, this, &KeyMgmt::close);

  generate_key_pair_act_ = new QAction(_("New Keypair"), this);
  generate_key_pair_act_->setShortcut(QKeySequence(_("Ctrl+N")));
  generate_key_pair_act_->setIcon(QIcon(":key_generate.png"));
  generate_key_pair_act_->setToolTip(_("Generate KeyPair"));
  connect(generate_key_pair_act_, &QAction::triggered, this,
          &KeyMgmt::SlotGenerateKeyDialog);

  generate_subkey_act_ = new QAction(_("New Subkey"), this);
  generate_subkey_act_->setShortcut(QKeySequence(_("Ctrl+Shift+N")));
  generate_subkey_act_->setIcon(QIcon(":key_generate.png"));
  generate_subkey_act_->setToolTip(_("Generate Subkey For Selected KeyPair"));
  connect(generate_subkey_act_, &QAction::triggered, this,
          &KeyMgmt::SlotGenerateSubKey);

  import_key_from_file_act_ = new QAction(_("File"), this);
  import_key_from_file_act_->setIcon(QIcon(":import_key_from_file.png"));
  import_key_from_file_act_->setToolTip(_("Import New Key From File"));
  connect(import_key_from_file_act_, &QAction::triggered, this,
          [&]() { CommonUtils::GetInstance()->SlotImportKeyFromFile(this); });

  import_key_from_clipboard_act_ = new QAction(_("Clipboard"), this);
  import_key_from_clipboard_act_->setIcon(
      QIcon(":import_key_from_clipboard.png"));
  import_key_from_clipboard_act_->setToolTip(
      _("Import New Key From Clipboard"));
  connect(import_key_from_clipboard_act_, &QAction::triggered, this, [&]() {
    CommonUtils::GetInstance()->SlotImportKeyFromClipboard(this);
  });

  import_key_from_key_server_act_ = new QAction(_("Keyserver"), this);
  import_key_from_key_server_act_->setIcon(
      QIcon(":import_key_from_server.png"));
  import_key_from_key_server_act_->setToolTip(
      _("Import New Key From Keyserver"));
  connect(import_key_from_key_server_act_, &QAction::triggered, this, [&]() {
    CommonUtils::GetInstance()->SlotImportKeyFromKeyServer(this);
  });

  import_keys_from_key_package_act_ = new QAction(_("Key Package"), this);
  import_keys_from_key_package_act_->setIcon(QIcon(":key_package.png"));
  import_keys_from_key_package_act_->setToolTip(
      _("Import Key(s) From a Key Package"));
  connect(import_keys_from_key_package_act_, &QAction::triggered, this,
          &KeyMgmt::SlotImportKeyPackage);

  export_key_to_clipboard_act_ = new QAction(_("Export To Clipboard"), this);
  export_key_to_clipboard_act_->setIcon(QIcon(":export_key_to_clipboard.png"));
  export_key_to_clipboard_act_->setToolTip(
      _("Export Selected Key(s) To Clipboard"));
  connect(export_key_to_clipboard_act_, &QAction::triggered, this,
          &KeyMgmt::SlotExportKeyToClipboard);

  export_key_to_file_act_ = new QAction(_("Export To Key Package"), this);
  export_key_to_file_act_->setIcon(QIcon(":key_package.png"));
  export_key_to_file_act_->setToolTip(
      _("Export Checked Key(s) To a Key Package"));
  connect(export_key_to_file_act_, &QAction::triggered, this,
          &KeyMgmt::SlotExportKeyToKeyPackage);

  export_key_as_open_ssh_format_ = new QAction(_("Export As OpenSSH"), this);
  export_key_as_open_ssh_format_->setIcon(QIcon(":ssh-key.png"));
  export_key_as_open_ssh_format_->setToolTip(
      _("Export Selected Key(s) As OpenSSH Format to File"));
  connect(export_key_as_open_ssh_format_, &QAction::triggered, this,
          &KeyMgmt::SlotExportAsOpenSSHFormat);

  delete_selected_keys_act_ = new QAction(_("Delete Selected Key(s)"), this);
  delete_selected_keys_act_->setToolTip(_("Delete the Selected keys"));
  connect(delete_selected_keys_act_, &QAction::triggered, this,
          &KeyMgmt::SlotDeleteSelectedKeys);

  delete_checked_keys_act_ = new QAction(_("Delete Checked Key(s)"), this);
  delete_checked_keys_act_->setToolTip(_("Delete the Checked keys"));
  delete_checked_keys_act_->setIcon(QIcon(":button_delete.png"));
  connect(delete_checked_keys_act_, &QAction::triggered, this,
          &KeyMgmt::SlotDeleteCheckedKeys);

  show_key_details_act_ = new QAction(_("Show Key Details"), this);
  show_key_details_act_->setToolTip(_("Show Details for this Key"));
  connect(show_key_details_act_, &QAction::triggered, this,
          &KeyMgmt::SlotShowKeyDetails);
}

void KeyMgmt::create_menus() {
  file_menu_ = menuBar()->addMenu(_("File"));
  file_menu_->addAction(open_key_file_act_);
  file_menu_->addAction(close_act_);

  key_menu_ = menuBar()->addMenu(_("Key"));
  generate_key_menu_ = key_menu_->addMenu(_("Generate Key"));
  generate_key_menu_->addAction(generate_key_pair_act_);
  generate_key_menu_->addAction(generate_subkey_act_);

  import_key_menu_ = key_menu_->addMenu(_("Import Key"));
  import_key_menu_->addAction(import_key_from_file_act_);
  import_key_menu_->addAction(import_key_from_clipboard_act_);
  import_key_menu_->addAction(import_key_from_key_server_act_);
  import_key_menu_->addAction(import_keys_from_key_package_act_);

  key_menu_->addAction(export_key_to_file_act_);
  key_menu_->addAction(export_key_to_clipboard_act_);
  key_menu_->addAction(export_key_as_open_ssh_format_);
  key_menu_->addSeparator();
  key_menu_->addAction(delete_checked_keys_act_);
}

void KeyMgmt::create_tool_bars() {
  QToolBar* keyToolBar = addToolBar(_("Key"));
  keyToolBar->setObjectName("keytoolbar");

  // add button with popup menu for import
  auto* generateToolButton = new QToolButton(this);
  generateToolButton->setMenu(generate_key_menu_);
  generateToolButton->setPopupMode(QToolButton::InstantPopup);
  generateToolButton->setIcon(QIcon(":key_generate.png"));
  generateToolButton->setText(_("Generate"));
  generateToolButton->setToolTip(_("Generate A New Keypair or Subkey"));
  generateToolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  keyToolBar->addWidget(generateToolButton);

  // add button with popup menu for import
  auto* toolButton = new QToolButton(this);
  toolButton->setMenu(import_key_menu_);
  toolButton->setPopupMode(QToolButton::InstantPopup);
  toolButton->setIcon(QIcon(":key_import.png"));
  toolButton->setToolTip(_("Import key"));
  toolButton->setText(_("Import Key"));
  toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  keyToolBar->addWidget(toolButton);

  keyToolBar->addSeparator();
  keyToolBar->addAction(delete_checked_keys_act_);
  keyToolBar->addSeparator();
  keyToolBar->addAction(export_key_to_file_act_);
  keyToolBar->addAction(export_key_to_clipboard_act_);
  keyToolBar->addAction(export_key_as_open_ssh_format_);
}

void KeyMgmt::SlotDeleteSelectedKeys() {
  delete_keys_with_warning(key_list_->GetSelected());
}

void KeyMgmt::SlotDeleteCheckedKeys() {
  delete_keys_with_warning(key_list_->GetChecked());
}

void KeyMgmt::delete_keys_with_warning(KeyIdArgsListPtr uidList) {
  /**
   * TODO: Different Messages for private/public key, check if
   * more than one selected... compare to seahorse "delete-dialog"
   */

  if (uidList->empty()) return;
  QString keynames;
  for (const auto& key_id : *uidList) {
    auto key = GpgKeyGetter::GetInstance().GetKey(key_id);
    if (!key.IsGood()) continue;
    keynames.append(QString::fromStdString(key.GetName()));
    keynames.append("<i> &lt;");
    keynames.append(QString::fromStdString(key.GetEmail()));
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
    GpgKeyOpera::GetInstance().DeleteKeys(std::move(uidList));
    emit SignalKeyStatusUpdated();
  }
}

void KeyMgmt::SlotShowKeyDetails() {
  auto keys_selected = key_list_->GetSelected();
  if (keys_selected->empty()) return;

  auto key = GpgKeyGetter::GetInstance().GetKey(keys_selected->front());

  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }

  new KeyDetailsDialog(key);
}

void KeyMgmt::SlotExportKeyToKeyPackage() {
  auto keys_checked = key_list_->GetChecked();
  if (keys_checked->empty()) {
    QMessageBox::critical(
        this, _("Forbidden"),
        _("Please check some keys before doing this operation."));
    return;
  }
  auto dialog = new ExportKeyPackageDialog(std::move(keys_checked), this);
  dialog->exec();
  emit SignalStatusBarChanged(QString(_("key(s) exported")));
}

void KeyMgmt::SlotExportKeyToClipboard() {
  auto keys_checked = key_list_->GetChecked();
  if (keys_checked->empty()) {
    QMessageBox::critical(
        this, _("Forbidden"),
        _("Please check some keys before doing this operation."));
    return;
  }

  ByteArrayPtr key_export_data = nullptr;
  if (!GpgKeyImportExporter::GetInstance().ExportKeys(keys_checked,
                                                      key_export_data)) {
    return;
  }
  QApplication::clipboard()->setText(QString::fromStdString(*key_export_data));
}

void KeyMgmt::SlotGenerateKeyDialog() {
  auto* keyGenDialog = new KeyGenDialog(this);
  keyGenDialog->show();
}

void KeyMgmt::closeEvent(QCloseEvent* event) {
  SlotSaveWindowState();
  QMainWindow::closeEvent(event);
}

void KeyMgmt::SlotGenerateSubKey() {
  auto keys_selected = key_list_->GetSelected();
  if (keys_selected->empty()) {
    QMessageBox::information(
        this, _("Invalid Operation"),
        _("Please select one KeyPair before doing this operation."));
    return;
  }
  const auto key = GpgKeyGetter::GetInstance().GetKey(keys_selected->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }
  if (!key.IsPrivateKey()) {
    QMessageBox::critical(this, _("Invalid Operation"),
                          _("If a key pair does not have a private key then "
                            "it will not be able to generate sub-keys."));
    return;
  }

  auto dialog = new SubkeyGenerateDialog(key.GetId(), this);
  dialog->show();
}
void KeyMgmt::SlotSaveWindowState() {
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

  GlobalSettingStation::GetInstance().SyncSettings();
}

void KeyMgmt::SlotExportAsOpenSSHFormat() {
  ByteArrayPtr key_export_data = nullptr;
  auto keys_checked = key_list_->GetChecked();

  if (keys_checked->empty()) {
    QMessageBox::critical(
        this, _("Forbidden"),
        _("Please select a key before performing this operation. If you select "
          "multiple keys, only the first key will be exported."));
    return;
  }

  auto key = GpgKeyGetter::GetInstance().GetKey(keys_checked->front());
  if (!GpgKeyImportExporter::GetInstance().ExportKeyOpenSSH(key,
                                                            key_export_data)) {
    QMessageBox::critical(this, _("Error"), _("An error occur in exporting."));
    return;
  }

  if (key_export_data->empty()) {
    QMessageBox::critical(
        this, _("Error"),
        _("This key may not be able to export as OpenSSH format. Please check "
          "the key-size of the subkey(s) used to sign."));
    return;
  }

  key = GpgKeyGetter::GetInstance().GetKey(keys_checked->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, _("Error"), _("Key Not Found."));
    return;
  }
  QString fileString = QString::fromStdString(
      key.GetName() + " " + key.GetEmail() + "(" + key.GetId() + ").pub");

  QString file_name = QFileDialog::getSaveFileName(
      this, _("Export OpenSSH Key To File"), fileString,
      QString(_("OpenSSH Public Key Files")) + " (*.pub);;All Files (*)");

  if (!file_name.isEmpty()) {
    write_buffer_to_file(file_name.toStdString(), *key_export_data);
    emit SignalStatusBarChanged(QString(_("key(s) exported")));
  }
}

void KeyMgmt::SlotImportKeyPackage() {
  auto key_package_file_name = QFileDialog::getOpenFileName(
      this, _("Import Key Package"), {},
      QString(_("Key Package")) + " (*.gfepack);;All Files (*)");

  auto key_file_name = QFileDialog::getOpenFileName(
      this, _("Import Key Package Passphrase File"), {},
      QString(_("Key Package Passphrase File")) + " (*.key);;All Files (*)");

  if(key_package_file_name.isEmpty() || key_file_name.isEmpty())
    return;

  GpgImportInformation info;

  if (KeyPackageOperator::ImportKeyPackage(key_package_file_name.toStdString(),
                                           key_file_name.toStdString(), info)) {
    emit SignalStatusBarChanged(QString(_("key(s) imported")));
    emit SignalKeyStatusUpdated();

    auto dialog = new KeyImportDetailDialog(info, false, this);
    dialog->exec();
  } else {
    QMessageBox::critical(this, _("Error"),
                          _("An error occur in importing key package."));
  }
}

}  // namespace GpgFrontend::UI
