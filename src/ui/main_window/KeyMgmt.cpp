/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "KeyMgmt.h"

#include <cassert>

#include "core/function/KeyPackageOperator.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/function/openpgp/support/KeyGenerationOpSupport.h"
#include "core/function/openpgp/support/KeyImportExportOpSupport.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/model/GpgImportInformation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "function/GenerateRevocationCert.h"
#include "function/SetOwnerTrustLevel.h"
#include "ui/UIModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/import_export/ExportKeyPackageDialog.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"
#include "ui/dialog/key_generate/KeyGenerateDialog.h"
#include "ui/dialog/key_generate/SubkeyGenerateDialog.h"
#include "ui/dialog/keypair_details/KeySetExpireDateDialog.h"
#include "ui/dialog/keypair_details/KeyUIDSignDialog.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/main_window/MainWindow.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

KeyMgmt::KeyMgmt(QWidget* parent)
    : GeneralMainWindow("key_management", parent) {
  /* the list of Keys available*/
  key_list_ = new KeyList(kGpgFrontendDefaultChannel, KeyMenuAbility::kALL,
                          GpgKeyTableColumn::kALL, this);

  // The Key Management window is the home for category management.
  key_list_->SetCategoryManagementEnabled(true);

  // The table layout is per window: this one is wide and shows the full column
  // set, the ToolBox dock is narrow and compact.
  key_list_->SetPersistenceScope("keymgmt");

  key_list_->AddListGroupTab(tr("All"), "all",
                             GpgKeyTableDisplayMode::kPUBLIC_KEY |
                                 GpgKeyTableDisplayMode::kPRIVATE_KEY);

  key_list_->AddListGroupTab(
      tr("Key Group"), "key_group", GpgKeyTableDisplayMode::kPUBLIC_KEY,
      [](const GpgAbstractKey* key) -> bool {
        return key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP;
      });

  key_list_->AddListGroupTab(
      tr("Only Public Key"), "only_public_key",
      GpgKeyTableDisplayMode::kPUBLIC_KEY,
      [](const GpgAbstractKey* key) -> bool {
        return !key->IsPrivateKey() &&
               !(key->IsRevoked() || key->IsDisabled() || key->IsExpired());
      });

  key_list_->AddListGroupTab(
      tr("Has Private Key"), "has_private_key",
      GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgAbstractKey* key) -> bool {
        return key->IsPrivateKey() &&
               !(key->IsRevoked() || key->IsDisabled() || key->IsExpired());
      });

  key_list_->AddListGroupTab(
      tr("Expiring Soon"), "expiring_soon",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgAbstractKey* key) -> bool { return IsKeyExpiringSoon(key); });

  key_list_->AddListGroupTab(
      tr("No Primary Key"), "no_primary_key",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgAbstractKey* key) -> bool {
        if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) return false;
        return !dynamic_cast<const GpgKey*>(key)->IsHasMasterKey() &&
               !(key->IsRevoked() || key->IsDisabled() || key->IsExpired());
      });

  key_list_->AddListGroupTab(
      tr("Revoked"), "revoked",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY,

      [](const GpgAbstractKey* key) -> bool { return key->IsRevoked(); });

  key_list_->AddListGroupTab(
      tr("Expired"), "expired",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY,

      [](const GpgAbstractKey* key) -> bool { return key->IsExpired(); });

  key_list_->AddListGroupTab(
      tr("Disabled"), "disabled",
      GpgKeyTableDisplayMode::kPUBLIC_KEY |
          GpgKeyTableDisplayMode::kPRIVATE_KEY,
      [](const GpgAbstractKey* key) -> bool { return key->IsDisabled(); });

  // Per-window order for the integrated (built-in) tabs; custom categories use
  // a separate, shared order (see KeyList).
  key_list_->SetTabOrderSettingsKey("keys/keymgmt_tab_order");
  key_list_->RebuildCategoryTabs();

  setCentralWidget(key_list_);

  key_list_->SlotRefresh();

  create_actions();
  create_menus();
  create_tool_bars();

  connect(this, &KeyMgmt::SignalStatusBarChanged,
          qobject_cast<MainWindow*>(this->parent()),
          &MainWindow::SlotSetStatusBarText);

  this->statusBar()->show();

  setWindowTitle(tr("KeyPair Management"));
  setAcceptDrops(true);

  const bool state_restored = restoreWindowState();
  if (!state_restored) {
    QTimer::singleShot(0, this, [this]() -> void { apply_default_layout(); });
  }

  popup_menu_ = new QMenu(this);

  // Lead with the single most common action; double-click / Enter triggers it.
  popup_menu_->addAction(show_key_details_act_);
  popup_menu_->setDefaultAction(show_key_details_act_);
  popup_menu_->addSeparator();

  // Clipboard shortcuts, grouped so they never crowd the operations below.
  copy_menu_ = popup_menu_->addMenu(tr("Copy"));
  copy_menu_->setIcon(QIcon(":/icons/edit.png"));
  copy_menu_->addAction(copy_fingerprint_act_);
  copy_menu_->addAction(copy_key_id_act_);
  copy_menu_->addAction(copy_email_act_);
  copy_menu_->addSeparator();
  copy_menu_->addAction(copy_public_key_act_);

  // Everything that mutates the key itself, in lifecycle order.
  popup_key_ops_menu_ = popup_menu_->addMenu(tr("Key Operations"));
  popup_key_ops_menu_->setIcon(QIcon(":/icons/key.png"));
  popup_key_ops_menu_->addAction(certify_key_act_);
  popup_key_ops_menu_->addAction(set_expiry_act_);
  popup_key_ops_menu_->addAction(generate_subkey_act_);
  popup_key_ops_menu_->addAction(set_owner_trust_of_key_act_);
  popup_key_ops_menu_->addSeparator();
  popup_key_ops_menu_->addAction(generate_revoke_cert_act_);

  // Keyserver actions share their own submenu; it hides wholesale when the
  // key-server module is not loaded.
  popup_keyserver_menu_ = popup_menu_->addMenu(tr("Keyserver"));
  popup_keyserver_menu_->setIcon(QIcon(":/icons/web.png"));
  popup_keyserver_menu_->addAction(import_key_from_key_server_act_);
  popup_keyserver_menu_->addAction(publish_key_to_key_server_act_);
  popup_keyserver_menu_->addAction(refresh_selected_from_key_server_act_);

  add_key_2_category_menu_ = popup_menu_->addMenu(tr("Category"));
  popup_menu_->addSeparator();

  popup_menu_->addAction(delete_selected_keys_act_);

  connect(key_list_, &KeyList::SignalRequestContextMenu, this,
          &KeyMgmt::slot_popup_menu_by_key_list);

  status_summary_label_ = new QLabel(this);
  statusBar()->addPermanentWidget(status_summary_label_);
  refresh_status_summary();

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyMgmt::refresh_status_summary);

  connect(this, &KeyMgmt::SignalKeyStatusUpdated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshStatusBar, this,
          [=](const QString& message, int timeout) {
            statusBar()->showMessage(message, timeout);
          });
}

void KeyMgmt::create_actions() {
  open_key_file_act_ = new QAction(tr("Open"), this);
  open_key_file_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
  open_key_file_act_->setToolTip(tr("Open Key File"));
  connect(open_key_file_act_, &QAction::triggered, this, [this]() {
    CommonUtils::GetInstance()->SlotImportKeyFromFile(
        this, key_list_->GetCurrentGpgContextChannel());
  });

  close_act_ = new QAction(tr("Close"), this);
  close_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
  close_act_->setIcon(QIcon(":/icons/exit.png"));
  close_act_->setToolTip(tr("Close"));
  connect(close_act_, &QAction::triggered, this, &KeyMgmt::close);

  generate_key_pair_act_ = new QAction(tr("New Keypair"), this);
  generate_key_pair_act_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
  generate_key_pair_act_->setIcon(QIcon(":/icons/key_generate.png"));
  generate_key_pair_act_->setToolTip(tr("Generate KeyPair"));
  generate_key_pair_act_->setDisabled(
      !IsOpSupported<GenerateKeyTag>(key_list_->GetCurrentGpgContextChannel()));
  connect(generate_key_pair_act_, &QAction::triggered, this,
          &KeyMgmt::SlotGenerateKeyDialog);

  generate_subkey_act_ = new QAction(tr("New Subkey"), this);
  generate_subkey_act_->setShortcut(
      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
  generate_subkey_act_->setIcon(QIcon(":/icons/key_generate.png"));
  generate_subkey_act_->setToolTip(tr("Generate Subkey For Selected KeyPair"));
  generate_subkey_act_->setDisabled(!IsOpSupported<GenerateSubKeyTag>(
      key_list_->GetCurrentGpgContextChannel()));
  connect(generate_subkey_act_, &QAction::triggered, this,
          &KeyMgmt::SlotGenerateSubKey);

  import_key_from_file_act_ = new QAction(tr("File"), this);
  import_key_from_file_act_->setIcon(QIcon(":/icons/import_key_from_file.png"));
  import_key_from_file_act_->setToolTip(tr("Import New Key From File"));
  connect(import_key_from_file_act_, &QAction::triggered, this, [this]() {
    CommonUtils::GetInstance()->SlotImportKeyFromFile(
        this, key_list_->GetCurrentGpgContextChannel());
  });

  import_key_from_clipboard_act_ = new QAction(tr("Clipboard"), this);
  import_key_from_clipboard_act_->setIcon(
      QIcon(":/icons/import_key_from_clipboard.png"));
  import_key_from_clipboard_act_->setToolTip(
      tr("Import New Key From Clipboard"));
  import_key_from_clipboard_act_->setShortcut(QKeySequence::Paste);
  import_key_from_clipboard_act_->setShortcutContext(Qt::WindowShortcut);
  addAction(import_key_from_clipboard_act_);
  connect(import_key_from_clipboard_act_, &QAction::triggered, this, [this]() {
    CommonUtils::GetInstance()->SlotImportKeyFromClipboard(
        this, key_list_->GetCurrentGpgContextChannel());
  });

  import_keys_from_key_package_act_ = new QAction(tr("Key Package"), this);
  import_keys_from_key_package_act_->setIcon(QIcon(":/icons/key_package.png"));
  import_keys_from_key_package_act_->setToolTip(
      tr("Import Key(s) From a Key Package"));
  connect(import_keys_from_key_package_act_, &QAction::triggered, this,
          &KeyMgmt::SlotImportKeyPackage);

  export_key_to_clipboard_act_ = new QAction(tr("Export To Clipboard"), this);
  export_key_to_clipboard_act_->setIcon(
      QIcon(":/icons/export_key_to_clipboard.png"));
  export_key_to_clipboard_act_->setToolTip(
      tr("Export Checked Key(s) To Clipboard"));
  connect(export_key_to_clipboard_act_, &QAction::triggered, this,
          &KeyMgmt::SlotExportKeyToClipboard);

  export_key_to_file_act_ = new QAction(tr("Export As Key Package"), this);
  export_key_to_file_act_->setIcon(QIcon(":/icons/key_package.png"));
  export_key_to_file_act_->setToolTip(
      tr("Export Checked Key(s) To a Key Package"));
  connect(export_key_to_file_act_, &QAction::triggered, this,
          &KeyMgmt::SlotExportKeyToKeyPackage);

  auto if_export_as_ssh_supported =
      IsOpSupported<ExportKeyAsOpenSSHFormatOpTag>(
          key_list_->GetCurrentGpgContextChannel());

  export_key_as_open_ssh_format_ = new QAction(tr("Export As OpenSSH"), this);
  export_key_as_open_ssh_format_->setIcon(QIcon(":/icons/ssh-key.png"));
  export_key_as_open_ssh_format_->setToolTip(
      tr("Export Checked Key As OpenSSH Format to File"));
  export_key_as_open_ssh_format_->setVisible(if_export_as_ssh_supported);
  connect(export_key_as_open_ssh_format_, &QAction::triggered, this,
          &KeyMgmt::SlotExportAsOpenSSHFormat);

  delete_selected_keys_act_ = new QAction(tr("Delete Selected Key(s)"), this);
  delete_selected_keys_act_->setIcon(QIcon(":/icons/button_delete.png"));
  delete_selected_keys_act_->setToolTip(tr("Delete the Selected keys"));
  connect(delete_selected_keys_act_, &QAction::triggered, this,
          &KeyMgmt::SlotDeleteSelectedKeys);

  delete_checked_keys_act_ = new QAction(tr("Delete Checked Key(s)"), this);
  delete_checked_keys_act_->setIcon(QIcon(":/icons/button_delete.png"));
  delete_checked_keys_act_->setToolTip(tr("Delete the Checked keys"));

  connect(delete_checked_keys_act_, &QAction::triggered, this,
          &KeyMgmt::SlotDeleteCheckedKeys);

  show_key_details_act_ = new QAction(tr("Show Key Details"), this);
  show_key_details_act_->setIcon(QIcon(":/icons/detail.png"));
  show_key_details_act_->setToolTip(tr("Show Details for this Key"));
  connect(show_key_details_act_, &QAction::triggered, this,
          &KeyMgmt::SlotShowKeyDetails);

  set_owner_trust_of_key_act_ = new QAction(tr("Set Owner Trust Level"), this);
  set_owner_trust_of_key_act_->setIcon(QIcon(":/icons/stairs.png"));
  set_owner_trust_of_key_act_->setToolTip(tr("Set Owner Trust Level"));
  set_owner_trust_of_key_act_->setData(QVariant("set_owner_trust_level"));
  connect(set_owner_trust_of_key_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedKey();
    if (key == nullptr) return;
    auto* function = new SetOwnerTrustLevel(this);
    function->Exec(key_list_->GetCurrentGpgContextChannel(), key);
    function->deleteLater();
  });

  create_quick_actions();
  create_keyserver_actions();
  create_bulk_actions();
}

void KeyMgmt::create_quick_actions() {
  copy_fingerprint_act_ = new QAction(tr("Copy Fingerprint"), this);
  connect(copy_fingerprint_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedKey();
    if (key == nullptr) return;
    copy_text_to_clipboard(key->Fingerprint(), tr("Fingerprint"));
  });

  copy_key_id_act_ = new QAction(tr("Copy Key ID"), this);
  connect(copy_key_id_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedKey();
    if (key == nullptr) return;
    copy_text_to_clipboard(key->ID(), tr("Key ID"));
  });

  copy_email_act_ = new QAction(tr("Copy Email"), this);
  connect(copy_email_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedKey();
    if (key == nullptr) return;
    copy_text_to_clipboard(key->Email(), tr("Email"));
  });

  copy_public_key_act_ = new QAction(tr("Copy Public Key Block"), this);
  connect(copy_public_key_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedKey();
    if (key == nullptr) return;
    export_keys_to_clipboard({key});
  });

  certify_key_act_ = new QAction(tr("Certify Key..."), this);
  certify_key_act_->setToolTip(tr("Sign this key's identity with your key"));
  connect(certify_key_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedGpgKey();
    if (key == nullptr) return;
    new KeyUIDSignDialog(key_list_->GetCurrentGpgContextChannel(), key,
                         key->UID(), this);
  });

  set_expiry_act_ = new QAction(tr("Set Expiry..."), this);
  connect(set_expiry_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedGpgKey();
    if (key == nullptr) return;
    auto* dialog = new KeySetExpireDateDialog(
        key_list_->GetCurrentGpgContextChannel(), key, this);
    dialog->show();
  });

  generate_revoke_cert_act_ =
      new QAction(tr("Generate Revocation Certificate..."), this);
  connect(generate_revoke_cert_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedGpgKey();
    if (key == nullptr) return;
    auto* function = new GenerateRevocationCert(this);
    function->Exec(key_list_->GetCurrentGpgContextChannel(), key);
  });
}

void KeyMgmt::create_keyserver_actions() {
  import_key_from_key_server_act_ =
      new QAction(tr("Search Keyserver..."), this);
  import_key_from_key_server_act_->setIcon(QIcon(":/icons/web.png"));
  import_key_from_key_server_act_->setToolTip(
      tr("Search a keyserver and import keys"));
  connect(import_key_from_key_server_act_, &QAction::triggered, this, [this]() {
    Module::TriggerEvent("REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT",
                         {
                             {"fingerprint", GFBuffer(QString{})},
                             {"parent", GFBuffer(RegisterQObject(this))},
                         });
  });

  publish_key_to_key_server_act_ =
      new QAction(tr("Publish Key to Keyserver..."), this);
  publish_key_to_key_server_act_->setToolTip(
      tr("Upload the checked public key(s) to the default keyserver"));
  connect(publish_key_to_key_server_act_, &QAction::triggered, this, [this]() {
    auto keys = key_list_->GetCheckedKeys();
    if (keys.empty()) {
      auto selected = key_list_->GetSelectedKey();
      if (selected != nullptr) keys = {selected};
    }
    publish_keys_to_key_server(keys);
  });

  refresh_selected_from_key_server_act_ =
      new QAction(tr("Refresh Selected from Keyserver"), this);
  connect(refresh_selected_from_key_server_act_, &QAction::triggered, this,
          [this]() {
            auto keys = key_list_->GetCheckedKeys();
            if (keys.empty()) {
              auto selected = key_list_->GetSelectedKey();
              if (selected != nullptr) keys = {selected};
            }
            if (keys.empty()) return;
            key_list_->SyncKeysFromKeyServer(keys);
          });
}

void KeyMgmt::create_bulk_actions() {
  bulk_set_owner_trust_act_ =
      new QAction(tr("Set Owner Trust for Checked Keys..."), this);
  connect(bulk_set_owner_trust_act_, &QAction::triggered, this,
          &KeyMgmt::bulk_set_owner_trust);

  bulk_extend_expiry_act_ =
      new QAction(tr("Extend Expiry of Checked Keys..."), this);
  connect(bulk_extend_expiry_act_, &QAction::triggered, this,
          &KeyMgmt::bulk_extend_expiry);

  backup_all_private_keys_act_ =
      new QAction(tr("Back Up All Private Keys..."), this);
  connect(backup_all_private_keys_act_, &QAction::triggered, this,
          &KeyMgmt::backup_all_private_keys);
}

void KeyMgmt::create_menus() {
  file_menu_ = menuBar()->addMenu(tr("File"));
  file_menu_->addAction(open_key_file_act_);
  file_menu_->addAction(close_act_);

  key_menu_ = menuBar()->addMenu(tr("Key"));
  generate_key_menu_ = key_menu_->addMenu(tr("Generate Key"));
  generate_key_menu_->addAction(generate_key_pair_act_);
  generate_key_menu_->addAction(generate_subkey_act_);

  import_key_menu_ = key_menu_->addMenu(tr("Import Key"));
  import_key_menu_->addAction(import_key_from_file_act_);
  import_key_menu_->addAction(import_key_from_clipboard_act_);
  import_key_menu_->addAction(import_keys_from_key_package_act_);

  export_key_menu_ = key_menu_->addMenu(tr("Export Key"));
  export_key_menu_->addAction(export_key_to_file_act_);
  export_key_menu_->addAction(export_key_to_clipboard_act_);
  export_key_menu_->addAction(export_key_as_open_ssh_format_);

  keyserver_menu_ = key_menu_->addMenu(tr("Keyserver"));
  keyserver_menu_->addAction(import_key_from_key_server_act_);
  keyserver_menu_->addAction(publish_key_to_key_server_act_);
  keyserver_menu_->addAction(refresh_selected_from_key_server_act_);
  keyserver_menu_->menuAction()->setVisible(
      Module::IsEventListening("REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT"));

  bulk_menu_ = key_menu_->addMenu(tr("Bulk"));
  bulk_menu_->addAction(bulk_set_owner_trust_act_);
  bulk_menu_->addAction(bulk_extend_expiry_act_);
  bulk_menu_->addSeparator();
  bulk_menu_->addAction(backup_all_private_keys_act_);

  key_menu_->addSeparator();
  key_menu_->addAction(delete_checked_keys_act_);
}

void KeyMgmt::create_tool_bars() {
  QToolBar* key_tool_bar = addToolBar(tr("Key"));
  key_tool_bar->setObjectName("keytoolbar");

  // generate key pair
  key_tool_bar->addAction(generate_key_pair_act_);
  key_tool_bar->addSeparator();

  // add button with popup menu for import
  auto* import_tool_button = new QToolButton(this);
  import_tool_button->setMenu(import_key_menu_);
  import_tool_button->setPopupMode(QToolButton::InstantPopup);
  import_tool_button->setIcon(QIcon(":/icons/key_import.png"));
  import_tool_button->setToolTip(tr("Import key"));
  import_tool_button->setText(tr("Import Key"));
  import_tool_button->setToolButtonStyle(icon_style_);
  key_tool_bar->addWidget(import_tool_button);

  auto* export_tool_button = new QToolButton(this);
  export_tool_button->setMenu(export_key_menu_);
  export_tool_button->setPopupMode(QToolButton::InstantPopup);
  export_tool_button->setIcon(QIcon(":/icons/key_export.png"));
  export_tool_button->setToolTip(tr("Export Key"));
  export_tool_button->setText(tr("Export Key"));
  export_tool_button->setToolButtonStyle(icon_style_);
  key_tool_bar->addWidget(export_tool_button);

  key_tool_bar->addAction(delete_checked_keys_act_);
}

void KeyMgmt::SlotDeleteSelectedKeys() {
  delete_keys_with_warning(key_list_->GetSelectedKeys());
}

void KeyMgmt::SlotDeleteCheckedKeys() {
  delete_keys_with_warning(key_list_->GetCheckedKeys());
}

void KeyMgmt::delete_keys_with_warning(const GpgAbstractKeyPtrList& keys) {
  if (keys.empty()) return;

  QString keynames;
  for (const auto& key : keys) {
    if (!key->IsGood()) continue;
    keynames.append(key->Name());
    keynames.append("<i> &lt;");
    keynames.append(key->Email());
    keynames.append("&gt; </i><br/>");
  }

  int const ret = QMessageBox::warning(
      this, tr("Deleting Keys"),
      "<b>" + tr("Are you sure that you want to delete the following keys?") +
          "</b><br/><br/>" + keynames + +"<br/>" +
          tr("The action can not be undone."),
      QMessageBox::No | QMessageBox::Yes);

  if (ret == QMessageBox::Yes) {
    KeyManagementOperation::GetInstance(
        key_list_->GetCurrentGpgContextChannel())
        .DeleteKeys(keys);
    emit SignalKeyStatusUpdated();
  }
}

void KeyMgmt::SlotShowKeyDetails() {
  auto keys = key_list_->GetSelectedKeys();
  if (keys.isEmpty()) return;

  CommonUtils::OpenDetailsDialogByKey(
      this, key_list_->GetCurrentGpgContextChannel(), keys.front());
}

void KeyMgmt::SlotExportKeyToKeyPackage() {
  auto keys = key_list_->GetCheckedKeys();
  if (keys.empty()) {
    QMessageBox::critical(
        this, tr("Forbidden"),
        tr("Please check some keys before doing this operation."));
    return;
  }

  auto* dialog = new ExportKeyPackageDialog(
      key_list_->GetCurrentGpgContextChannel(), keys, this);
  dialog->exec();
  emit SignalStatusBarChanged(tr("key(s) exported"));
}

void KeyMgmt::SlotExportKeyToClipboard() {
  auto keys = key_list_->GetCheckedKeys();
  if (keys.empty()) {
    QMessageBox::critical(
        this, tr("Forbidden"),
        tr("Please check some keys before doing this operation."));
    return;
  }

  export_keys_to_clipboard(keys);
}

void KeyMgmt::export_keys_to_clipboard(const GpgAbstractKeyPtrList& keys) {
  if (keys.empty()) return;

  assert(std::all_of(keys.begin(), keys.end(),
                     [](const auto& key) { return key->IsGood(); }));

  GpgOperaHelper::WaitForOpera(
      this, tr("Exporting"), [=](const OperaWaitingHd& op_hd) {
        KeyImportExportOperation::GetInstance(
            key_list_->GetCurrentGpgContextChannel())
            .ExportKeys(
                keys, false, true, false, false,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                    CommonUtils::RaiseMessageBox(this, err);
                    return;
                  }

                  if (data_obj == nullptr || !data_obj->Check<GFBuffer>()) {
                    FLOG_W("data object checking failed");
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto gf_buffer = ExtractParams<GFBuffer>(data_obj, 0);
                  QApplication::clipboard()->setText(
                      gf_buffer.ConvertToQByteArray());
                });
      });
}

void KeyMgmt::copy_text_to_clipboard(const QString& text, const QString& what) {
  if (text.isEmpty()) {
    emit SignalStatusBarChanged(tr("%1 is empty").arg(what));
    return;
  }
  QApplication::clipboard()->setText(text);
  emit SignalStatusBarChanged(tr("%1 copied to clipboard").arg(what));
}

void KeyMgmt::publish_keys_to_key_server(const GpgAbstractKeyPtrList& keys) {
  if (keys.empty()) {
    QMessageBox::critical(
        this, tr("Forbidden"),
        tr("Please check or select a key before doing this operation."));
    return;
  }

  QString names;
  for (const auto& key : keys) {
    if (key == nullptr || !key->IsGood()) continue;
    names.append("<br/>&nbsp;&nbsp;" + key->Name() + " &lt;" + key->Email() +
                 "&gt;");
  }

  int const ret = QMessageBox::warning(
      this, tr("Publish Key to Keyserver"),
      "<b>" +
          tr("You are about to upload the following public key(s) to the "
             "default keyserver:") +
          "</b>" + names + "<br/><br/>" +
          tr("Publication is <b>permanent and public</b>: the key(s) cannot be "
             "removed from most keyservers once uploaded. Only the public part "
             "is uploaded, never your private key.") +
          "<br/><br/>" + tr("Do you want to proceed?"),
      QMessageBox::No | QMessageBox::Yes);
  if (ret != QMessageBox::Yes) return;

  GpgOperaHelper::WaitForOpera(
      this, tr("Exporting"), [=](const OperaWaitingHd& op_hd) {
        KeyImportExportOperation::GetInstance(
            key_list_->GetCurrentGpgContextChannel())
            .ExportKeys(
                keys, false, true, false, false,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  op_hd();

                  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                    CommonUtils::RaiseMessageBox(this, err);
                    return;
                  }

                  if (data_obj == nullptr || !data_obj->Check<GFBuffer>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto gf_buffer = ExtractParams<GFBuffer>(data_obj, 0);
                  Module::TriggerEvent(
                      "REQUEST_UPLOAD_PUBLIC_KEY",
                      {
                          {"key_text", gf_buffer},
                      },
                      [this](Module::EventIdentifier,
                             Module::Event::ListenerIdentifier,
                             Module::Event::Params p) -> void {
                        if (p["ret"] != "0" || !p["error_msg"].Empty()) {
                          QMessageBox::critical(
                              this, tr("Upload Failed"),
                              p["error_msg"].ConvertToQString());
                          return;
                        }

                        QMessageBox::information(
                            this, tr("Upload Complete"),
                            tr("The public key was uploaded to the "
                               "keyserver.\n\nFingerprint: %1")
                                .arg(p["fingerprint"].ConvertToQString()));
                      });
                });
      });
}

void KeyMgmt::bulk_set_owner_trust() {
  auto keys = key_list_->GetCheckedKeys();
  if (keys.empty()) {
    QMessageBox::critical(
        this, tr("Forbidden"),
        tr("Please check some keys before doing this operation."));
    return;
  }

  QStringList items;
  items << tr("Undefined") << tr("Never") << tr("Marginal") << tr("Full")
        << tr("Ultimate");

  bool ok = false;
  QString item = QInputDialog::getItem(
      this, tr("Modify Owner Trust Level"),
      tr("Trust level to apply to %1 checked key(s):").arg(keys.size()), items,
      0, false, &ok);
  if (!ok || item.isEmpty()) return;

  int trust_level = 1;  // Undefined
  if (item == tr("Ultimate")) {
    trust_level = 5;
  } else if (item == tr("Full")) {
    trust_level = 4;
  } else if (item == tr("Marginal")) {
    trust_level = 3;
  } else if (item == tr("Never")) {
    trust_level = 2;
  }

  const auto channel = key_list_->GetCurrentGpgContextChannel();
  int failed = 0;
  for (const auto& key : keys) {
    if (key == nullptr || !key->IsGood()) continue;
    if (!KeyManagementOperation::GetInstance(channel).SetOwnerTrustLevel(
            key, trust_level)) {
      failed++;
    }
  }

  if (failed > 0) {
    QMessageBox::warning(
        this, tr("Partially Failed"),
        tr("Failed to set owner trust on %1 key(s).").arg(failed));
  }
  emit SignalKeyStatusUpdated();
}

void KeyMgmt::bulk_extend_expiry() {
  auto checked = key_list_->GetCheckedKeys();
  GpgKeyPtrList keys;
  for (const auto& key : checked) {
    if (key == nullptr || !key->IsPrivateKey()) continue;
    if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) continue;
    keys.push_back(qSharedPointerDynamicCast<GpgKey>(key));
  }

  if (keys.empty()) {
    QMessageBox::critical(
        this, tr("Forbidden"),
        tr("Please check some private keys before doing this operation."));
    return;
  }

  QDialog dialog(this);
  dialog.setWindowTitle(tr("Extend Expiry of Checked Keys"));
  auto* layout = new QVBoxLayout(&dialog);
  layout->addWidget(new QLabel(
      tr("New expiry date to apply to %1 private key(s):").arg(keys.size())));

  auto* expires_edit =
      new QDateTimeEdit(QDateTime::currentDateTime().addYears(2));
  expires_edit->setMinimumDateTime(QDateTime::currentDateTime());
  expires_edit->setMaximumDate(QDate(2106, 1, 1));
  layout->addWidget(expires_edit);

  auto* never_check = new QCheckBox(tr("Never expires"));
  layout->addWidget(never_check);
  connect(never_check, &QCheckBox::toggled, expires_edit,
          &QDateTimeEdit::setDisabled);

  auto* buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted) return;

  const std::optional<QDateTime> expires =
      never_check->isChecked() ? std::nullopt
                               : std::make_optional(expires_edit->dateTime());
  const auto channel = key_list_->GetCurrentGpgContextChannel();

  GpgOperaHelper::WaitForOpera(
      this, tr("Extending Expiry"), [=](const OperaWaitingHd& op_hd) {
        // gpg --edit-key holds a process-wide keyring lock, so the keys must be
        // updated one after another rather than concurrently.
        int failed = 0;
        for (const auto& key : keys) {
          if (key == nullptr) continue;
          if (KeyManagementOperation::GetInstance(channel).SetExpire(
                  key, {}, expires) != GPG_ERR_NO_ERROR) {
            failed++;
          }
        }
        op_hd();

        if (failed > 0) {
          QMessageBox::warning(
              this, tr("Partially Failed"),
              tr("Failed to update expiry on %1 key(s).").arg(failed));
        }
        emit SignalKeyStatusUpdated();
      });
}

void KeyMgmt::backup_all_private_keys() {
  GpgAbstractKeyPtrList keys;
  for (const auto& key :
       GpgKeyRepository::GetInstance(key_list_->GetCurrentGpgContextChannel())
           .Fetch()) {
    if (key != nullptr && key->IsPrivateKey() && key->IsGood()) {
      keys.push_back(key);
    }
  }

  if (keys.empty()) {
    QMessageBox::information(
        this, tr("No Private Keys"),
        tr("There are no private keys in this keyring to back up."));
    return;
  }

  auto* dialog = new ExportKeyPackageDialog(
      key_list_->GetCurrentGpgContextChannel(), keys, this);
  dialog->exec();
  emit SignalStatusBarChanged(tr("private key(s) backed up"));
}

void KeyMgmt::SlotGenerateKeyDialog() {
  new KeyGenerateDialog(key_list_->GetCurrentGpgContextChannel(), this);
}

void KeyMgmt::SlotGenerateSubKey() {
  auto key = key_list_->GetSelectedGpgKey();
  if (key == nullptr) return;

  if (!key->IsPrivateKey()) {
    QMessageBox::critical(this, tr("Invalid Operation"),
                          tr("If a key pair does not have a private key then "
                             "it will not be able to generate sub-keys."));
    return;
  }

  new SubkeyGenerateDialog(key_list_->GetCurrentGpgContextChannel(), key, this);
}

void KeyMgmt::SlotExportAsOpenSSHFormat() {
  auto keys = key_list_->GetCheckedKeys();
  if (keys.empty()) {
    QMessageBox::critical(
        this, tr("Forbidden"),
        tr("Please check a key before performing this operation."));
    return;
  }

  assert(std::all_of(keys.begin(), keys.end(),
                     [](const auto& key) { return key->IsGood(); }));

  if (keys.size() > 1) {
    QMessageBox::critical(this, tr("Forbidden"),
                          tr("This operation accepts just a single key."));
    return;
  }

  GpgOperaHelper::WaitForOpera(
      this, tr("Exporting"), [this, keys](const OperaWaitingHd& op_hd) {
        KeyImportExportOperation::GetInstance(
            key_list_->GetCurrentGpgContextChannel())
            .ExportKeys(
                keys, false, true, false, true,
                [=](GpgError err, const DataObjectPtr& data_obj) {
                  // stop waiting
                  op_hd();

                  if (CheckGpgError(err) == GPG_ERR_USER_1) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                    CommonUtils::RaiseMessageBox(this, err);
                    return;
                  }

                  if (data_obj == nullptr || !data_obj->Check<GFBuffer>()) {
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Unknown error occurred"));
                    return;
                  }

                  auto gf_buffer = ExtractParams<GFBuffer>(data_obj, 0);
                  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                    CommonUtils::RaiseMessageBox(this, err);
                    return;
                  }

                  if (gf_buffer.Empty()) {
                    QMessageBox::critical(
                        this, tr("Error"),
                        tr("This key may not be able to export as OpenSSH "
                           "format. "
                           "Please check the key-size of the subkey(s) used "
                           "to "
                           "sign."));
                    return;
                  }

                  QString const file_name = QFileDialog::getSaveFileName(
                      this, tr("Export OpenSSH Key To File"), "authorized_keys",
                      tr("OpenSSH Public Key Files") + "All Files (*)");

                  if (!file_name.isEmpty()) {
                    WriteFileGFBuffer(file_name, gf_buffer);
                    emit SignalStatusBarChanged(tr("key(s) exported"));
                  }
                });
      });
}

void KeyMgmt::SlotImportKeyPackage() {
  auto key_package_file_name = QFileDialog::getOpenFileName(
      this, tr("Import Key Package"), {}, tr("Key Package") + " (*.gfpack)");

  if (key_package_file_name.isEmpty()) return;

  // max file size is 32 mb
  QFileInfo key_package_file_info(key_package_file_name);

  if (!key_package_file_info.isFile() || !key_package_file_info.isReadable()) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  if (key_package_file_info.size() > static_cast<qint64>(32 * 1024 * 1024)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("The target file is too large for a key package."));
    return;
  }

  auto key_file_name = QFileDialog::getOpenFileName(
      this, tr("Import Key Package Passphrase File"), {},
      tr("Key Package Passphrase File") + " (*.key)");

  if (key_file_name.isEmpty()) return;

  // max file size is 1 mb
  QFileInfo key_file_info(key_file_name);

  if (!key_file_info.isFile() || !key_file_info.isReadable()) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  if (key_file_info.size() > static_cast<qint64>(1024 * 1024)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("The target file is too large for a key package passphrase."));
    return;
  }

  bool ok;
  auto pin = QInputDialog::getText(this, tr("Enter PIN"),
                                   tr("Please enter PIN to decrypt the Key:"),
                                   QLineEdit::Password, QString(), &ok);
  if (!ok || pin.isEmpty()) return;

  GFBuffer buf(pin);
  pin.fill('X');
  pin.clear();

  QPointer<KeyMgmt> self = this;

  Thread::Task::TaskCallback cb = [=](int /*ret*/,
                                      const DataObjectPtr& data_object) {
    if (!data_object->Check<QString, QSharedPointer<GpgImportInformation>>()) {
      return;
    }

    auto msg = ExtractParams<QString>(data_object, 0);
    auto info =
        ExtractParams<QSharedPointer<GpgImportInformation>>(data_object, 1);

    if (!info) {
      QMessageBox::critical(self, tr("Error"), msg);
      return;
    }

    if (!self) return;

    emit SignalStatusBarChanged(tr("key(s) imported"));
    emit SignalKeyStatusUpdated();

    auto* connection = new QMetaObject::Connection;
    *connection =
        connect(UISignalStation::GetInstance(),
                &UISignalStation::SignalKeyDatabaseRefreshDone, self, [=]() {
                  (new KeyImportDetailDialog(
                      key_list_->GetCurrentGpgContextChannel(), info, self));
                  QObject::disconnect(*connection);
                  delete connection;
                });
  };

  auto* task = new Thread::Task{
      [=](const DataObjectPtr& data_object) -> int {
        auto [err, info] =
            KeyPackageOperator::GetInstance(
                key_list_->GetCurrentGpgContextChannel())
                .ImportKeyPackage(key_package_file_name, key_file_name, buf);
        data_object->Swap({err, info});
        return 0;
      },
      "import_key_package", TransferParams(), cb};

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(task);
}

void KeyMgmt::dragEnterEvent(QDragEnterEvent* event) {
  const auto* mime = event->mimeData();
  if (mime->hasUrls() || mime->hasText()) {
    event->acceptProposedAction();
  }
}

void KeyMgmt::dropEvent(QDropEvent* event) {
  const auto* mime = event->mimeData();
  const auto channel = key_list_->GetCurrentGpgContextChannel();

  if (mime->hasUrls()) {
    for (const auto& url : mime->urls()) {
      if (!url.isLocalFile()) continue;
      auto [succ, buffer] = ReadFileGFBuffer(url.toLocalFile());
      if (!succ) continue;
      CommonUtils::GetInstance()->SlotImportKeys(this, channel, buffer);
    }
    event->acceptProposedAction();
    return;
  }

  if (mime->hasText()) {
    CommonUtils::GetInstance()->SlotImportKeys(
        this, channel, GFBuffer(mime->text().toLatin1()));
    event->acceptProposedAction();
  }
}

void KeyMgmt::slot_popup_menu_by_key_list(QContextMenuEvent* event,
                                          KeyTable* key_table) {
  if (event == nullptr || key_table == nullptr) return;

  auto keys = key_table->GetSelectedKeys();
  if (keys.isEmpty()) return;

  const auto channel = key_list_->GetCurrentGpgContextChannel();

  auto if_owner_trust_level_supported =
      IsOpSupported<SetOwnerTrustLevelOpTag>(channel);
  set_owner_trust_of_key_act_->setVisible(if_owner_trust_level_supported);

  const auto& key = keys.front();
  const bool is_gpg_key = key->KeyType() == GpgAbstractKeyType::kGPG_KEY;
  const bool is_private = key->IsPrivateKey();

  generate_subkey_act_->setVisible(is_gpg_key && is_private);

  // Certify makes sense only for someone else's key, and only when the engine
  // can sign; the other per-key operations act on your own keys.
  certify_key_act_->setVisible(is_gpg_key && !is_private &&
                               IsOpSupported<SignKeyOpTag>(channel));
  set_expiry_act_->setVisible(is_gpg_key && is_private);
  generate_revoke_cert_act_->setVisible(is_gpg_key && is_private);
  copy_email_act_->setVisible(!key->Email().isEmpty());

  // The Key Operations submenu is only useful for a real key; hide it whole
  // for key groups so the menu stays short.
  popup_key_ops_menu_->menuAction()->setVisible(is_gpg_key);

  const bool keyserver_available =
      Module::IsEventListening("REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT");
  import_key_from_key_server_act_->setVisible(keyserver_available);
  publish_key_to_key_server_act_->setVisible(
      keyserver_available &&
      Module::IsEventListening("REQUEST_UPLOAD_PUBLIC_KEY"));
  refresh_selected_from_key_server_act_->setVisible(
      keyserver_available &&
      Module::IsEventListening("REQUEST_GET_PUBLIC_KEY_BY_KEY_ID"));
  // Whole keyserver submenu disappears when the module is not loaded.
  popup_keyserver_menu_->menuAction()->setVisible(keyserver_available);

  // Membership acts on all checked keys when any are checked, else the
  // right-clicked selection.
  auto targets = key_list_->GetCheckedKeys();
  if (targets.isEmpty()) targets = keys;
  populate_key_category_menu(channel, key_table->objectName(), targets);

  popup_menu_->exec(event->globalPos());
}

void KeyMgmt::populate_key_category_menu(int channel,
                                         const QString& current_tab_id,
                                         const GpgAbstractKeyPtrList& keys) {
  if (add_key_2_category_menu_ == nullptr) return;
  add_key_2_category_menu_->clear();

  add_key_2_category_menu_->setDisabled(keys.isEmpty());
  if (keys.isEmpty()) return;

  QStringList key_ids;
  for (const auto& k : keys) {
    if (k != nullptr) key_ids << k->ID();
  }

  add_key_2_category_menu_->setTitle(
      key_ids.size() > 1 ? tr("Category (%1 keys)").arg(key_ids.size())
                         : tr("Category"));

  auto& repo = KeyCategoryRepository::GetInstance(channel);

  // When viewing a custom category tab, offer to remove the keys from it.
  if (current_tab_id.startsWith("cat:")) {
    const bool any_member = std::any_of(
        key_ids.cbegin(), key_ids.cend(),
        [&](const QString& id) { return repo.Contains(current_tab_id, id); });
    if (any_member) {
      auto* remove_act =
          add_key_2_category_menu_->addAction(tr("Remove From This Category"));
      connect(remove_act, &QAction::triggered, this, [=]() {
        auto& r = KeyCategoryRepository::GetInstance(channel);
        for (const auto& id : key_ids)
          r.RemoveKeyFromCategory(current_tab_id, id);
        CommonUtils::GetInstance()->NotifyCategoriesChanged();
      });
      add_key_2_category_menu_->addSeparator();
    }
  }

  for (const auto& c : repo.Fetch()) {
    if (c.builtin) continue;

    const auto category_id = c.id;
    const bool all_members = std::all_of(
        key_ids.cbegin(), key_ids.cend(),
        [&](const QString& id) { return repo.Contains(category_id, id); });

    auto* act = add_key_2_category_menu_->addAction(c.name);
    act->setCheckable(true);
    act->setChecked(all_members);
    connect(act, &QAction::triggered, this, [=](bool checked) {
      auto& r = KeyCategoryRepository::GetInstance(channel);
      for (const auto& id : key_ids) {
        if (checked) {
          r.AddKey2Category(category_id, id);
        } else {
          r.RemoveKeyFromCategory(category_id, id);
        }
      }
      CommonUtils::GetInstance()->NotifyCategoriesChanged();
    });
  }

  add_key_2_category_menu_->addSeparator();
  auto* new_act = add_key_2_category_menu_->addAction(tr("New Category..."));
  connect(new_act, &QAction::triggered, this, [=]() {
    bool ok = false;
    auto name =
        QInputDialog::getText(this, tr("New Category"), tr("Category name:"),
                              QLineEdit::Normal, QString{}, &ok)
            .trimmed();
    if (!ok || name.isEmpty()) return;

    auto& r = KeyCategoryRepository::GetInstance(channel);
    const auto id = r.AddCategory(name);
    for (const auto& key_id : key_ids) r.AddKey2Category(id, key_id);
    CommonUtils::GetInstance()->NotifyCategoriesChanged();
  });
}

void KeyMgmt::refresh_status_summary() {
  if (status_summary_label_ == nullptr) return;

  int total = 0;
  int private_keys = 0;
  int expiring_soon = 0;
  int unusable = 0;

  for (const auto& key :
       GpgKeyRepository::GetInstance(key_list_->GetCurrentGpgContextChannel())
           .Fetch()) {
    if (key == nullptr) continue;

    total++;
    if (key->IsPrivateKey()) private_keys++;
    if (key->IsRevoked() || key->IsExpired() || key->IsDisabled()) {
      unusable++;
    } else if (IsKeyExpiringSoon(key.get())) {
      expiring_soon++;
    }
  }

  status_summary_label_->setText(
      tr("%1 keys · %2 private · %3 expiring soon · %4 expired or revoked")
          .arg(total)
          .arg(private_keys)
          .arg(expiring_soon)
          .arg(unusable));

  status_summary_label_->setToolTip(
      expiring_soon > 0 ? tr("%1 key(s) expire within %2 days. See the "
                             "\"Expiring Soon\" tab.")
                              .arg(expiring_soon)
                              .arg(GetKeyExpiringSoonDays())
                        : tr("No key expires within the next %1 days.")
                              .arg(GetKeyExpiringSoonDays()));
}

namespace {

auto ClampInt(int value, int min, int max) -> int {
  return std::clamp(value, min, max);
}

auto CurrentAvailableGeometry(QWidget* widget) -> QRect {
  const auto* screen = widget != nullptr && widget->screen() != nullptr
                           ? widget->screen()
                           : QGuiApplication::primaryScreen();

  if (screen == nullptr) {
    return QRect(0, 0, 1200, 760);
  }

  return screen->availableGeometry();
}

}  // namespace

void KeyMgmt::apply_default_layout() {
  const QRect available = CurrentAvailableGeometry(this);

  constexpr double kWindowScale = 0.88;
  constexpr double kTargetAspect = 4.0 / 3.0;

  int target_width =
      ClampInt(static_cast<int>(available.width() * kWindowScale), 800, 1280);

  int target_height =
      ClampInt(static_cast<int>(target_width / kTargetAspect), 600, 960);

  if (target_height > static_cast<int>(available.height() * kWindowScale)) {
    target_height =
        ClampInt(static_cast<int>(available.height() * kWindowScale), 560, 900);

    target_width =
        ClampInt(static_cast<int>(target_height * kTargetAspect), 760, 1280);
  }

  setMinimumSize(
      QSize(std::min(760, static_cast<int>(available.width() * 0.90)),
            std::min(560, static_cast<int>(available.height() * 0.90))));

  resize(target_width, target_height);
  movePosition2CenterOfParent();
}

}  // namespace GpgFrontend::UI
