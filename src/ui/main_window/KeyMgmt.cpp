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
#include "ui/UIModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/import_export/ExportKeyPackageDialog.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"
#include "ui/dialog/key_generate/KeyGenerateDialog.h"
#include "ui/dialog/key_generate/SubkeyGenerateDialog.h"
#include "ui/dialog/keypair_details/KeySetExpireDateDialog.h"
#include "ui/dialog/keypair_details/KeyUIDSignDialog.h"
#include "ui/function/ExportKey.h"
#include "ui/function/GenerateRevocationCert.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/SetOwnerTrustLevel.h"
#include "ui/main_window/MainWindow.h"
#include "ui/main_window/ToolBarHelper.h"
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

  // Order matters: the popup owns the submenus the menu bar and the tool bar
  // then reuse (Category, Key Operations), and the View menu needs the tool
  // bar's toggle action, which only exists once the bar does.
  create_actions();
  create_popup_menu();
  create_menus();
  create_tool_bars();

  // Both need the bar to exist: one styles it, the other measures it.
  init_window_style();
  align_chrome_insets();

  // QMainWindow has no "bar moved" signal; the bar turning vertical is what
  // actually tells us it left the top edge.
  connect(key_tool_bar_, &QToolBar::orientationChanged, this,
          [this](Qt::Orientation) { align_chrome_insets(); });
  connect(key_tool_bar_, &QToolBar::visibilityChanged, this,
          [this](bool) { align_chrome_insets(); });

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalAppearanceSettingsChanged, this, [this]() {
            reloadAppearanceSettings();
            apply_tool_bar_appearance();
          });

  // KeyMgmt is normally parented to the MainWindow, but nothing enforces it —
  // connecting to a null receiver would only warn at runtime.
  if (auto* main_window = qobject_cast<MainWindow*>(this->parent());
      main_window != nullptr) {
    connect(this, &KeyMgmt::SignalStatusBarChanged, main_window,
            &MainWindow::SlotSetStatusBarText);
  }

  this->statusBar()->show();

  setWindowTitle(tr("KeyPair Management"));
  setAcceptDrops(true);

  const bool state_restored = restoreWindowState();
  if (!state_restored) {
    QTimer::singleShot(0, this, [this]() -> void { apply_default_layout(); });
  }

  connect(key_list_, &KeyList::SignalRequestContextMenu, this,
          &KeyMgmt::slot_popup_menu_by_key_list);

  connect(key_list_, &KeyList::SignalRequestShowDetails, this,
          &KeyMgmt::SlotShowKeyDetails);

  // Everything that can change what the actions should look like.
  connect(key_list_, &KeyList::SignalSelectionChanged, this,
          &KeyMgmt::update_key_action_state);
  connect(key_list_, &KeyList::SignalKeyChecked, this,
          &KeyMgmt::update_key_action_state);
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this, [this]() {
            // The keyring-wide facts have to be current before the pass that
            // reads them.
            refresh_keyring_summary_flags();
            update_key_action_state();
          });

  refresh_keyring_summary_flags();
  update_key_action_state();

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

void KeyMgmt::create_popup_menu() {
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

  // Saving this one key to a file, beside the clipboard entries: the two are
  // the same intent through different channels.
  popup_export_menu_ = popup_menu_->addMenu(tr("Export"));
  popup_export_menu_->setIcon(QIcon(":/icons/key_export.png"));
  popup_export_menu_->addAction(export_public_key_act_);
  popup_export_menu_->addAction(export_private_key_act_);
  popup_export_menu_->addSeparator();
  popup_export_menu_->addAction(export_key_as_open_ssh_format_);

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
  popup_keyserver_menu_->setIcon(QIcon(":/icons/server.png"));
  popup_keyserver_menu_->addAction(import_key_from_key_server_act_);
  popup_keyserver_menu_->addAction(publish_key_to_key_server_act_);
  popup_keyserver_menu_->addAction(refresh_selected_from_key_server_act_);

  add_key_2_category_menu_ = popup_menu_->addMenu(tr("Category"));
  popup_menu_->addSeparator();

  popup_menu_->addAction(delete_selected_keys_act_);

  // Flat, and including the submenu children: those are exactly the actions
  // shared with the menu bar, so a per-menu walk would miss them.
  popup_actions_ = {
      show_key_details_act_,
      copy_fingerprint_act_,
      copy_key_id_act_,
      copy_email_act_,
      copy_public_key_act_,
      export_public_key_act_,
      export_private_key_act_,
      export_key_as_open_ssh_format_,
      certify_key_act_,
      set_expiry_act_,
      generate_subkey_act_,
      set_owner_trust_of_key_act_,
      generate_revoke_cert_act_,
      import_key_from_key_server_act_,
      publish_key_to_key_server_act_,
      refresh_selected_from_key_server_act_,
      delete_selected_keys_act_,
  };

  // What is worth offering when the click landed on nothing: the ways to get a
  // key into an empty keyring. Reuses the Import Key submenu built for the menu
  // bar rather than a second copy of it.
  empty_area_menu_ = new QMenu(this);
  empty_area_menu_->addAction(generate_key_pair_act_);
  empty_area_menu_->addAction(import_key_from_clipboard_act_);
  empty_area_menu_->addSeparator();
  empty_area_menu_->addAction(open_key_file_act_);
  empty_area_menu_->addAction(import_keys_from_key_package_act_);
  empty_area_menu_->addSeparator();
  empty_area_menu_->addAction(refresh_keys_act_);
}

auto KeyMgmt::make_action(const QString& text, const QString& icon,
                          const QString& tip,
                          const QContainer<QKeySequence>& shortcuts)
    -> QAction* {
  auto* action = new QAction(text, this);

  if (!icon.isEmpty()) action->setIcon(QIcon(icon));

  action->setToolTip(tip);
  action->setStatusTip(tip);
  action->setProperty("gfBaseToolTip", tip);

  if (!shortcuts.isEmpty()) {
    action->setShortcuts(
        QList<QKeySequence>{shortcuts.cbegin(), shortcuts.cend()});
  }

  return action;
}

void KeyMgmt::create_actions() {
  open_key_file_act_ = make_action(tr("Open"), {}, tr("Open Key File"),
                                   {QKeySequence(Qt::CTRL | Qt::Key_O)});
  connect(open_key_file_act_, &QAction::triggered, this, [this]() {
    CommonUtils::GetInstance()->SlotImportKeyFromFile(
        this, key_list_->GetCurrentGpgContextChannel());
  });

  close_act_ = make_action(tr("Close"), ":/icons/exit.png", tr("Close"),
                           {QKeySequence(Qt::CTRL | Qt::Key_Q)});
  connect(close_act_, &QAction::triggered, this, &KeyMgmt::close);

  generate_key_pair_act_ =
      make_action(tr("New Keypair"), ":/icons/key_generate.png",
                  tr("Generate KeyPair"), {QKeySequence(Qt::CTRL | Qt::Key_N)});
  generate_key_pair_act_->setDisabled(
      !IsOpSupported<GenerateKeyTag>(key_list_->GetCurrentGpgContextChannel()));
  connect(generate_key_pair_act_, &QAction::triggered, this,
          &KeyMgmt::SlotGenerateKeyDialog);

  generate_subkey_act_ =
      make_action(tr("New Subkey"), ":/icons/key_generate.png",
                  tr("Generate Subkey For Selected KeyPair"),
                  {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N)});
  // Engine support and selection state are both applied by
  // update_key_action_state(); a setDisabled() here would be overwritten on the
  // first selection change anyway.
  connect(generate_subkey_act_, &QAction::triggered, this,
          &KeyMgmt::SlotGenerateSubKey);

  import_key_from_file_act_ =
      make_action(tr("File"), ":/icons/import_key_from_file.png",
                  tr("Import New Key From File"));
  connect(import_key_from_file_act_, &QAction::triggered, this, [this]() {
    CommonUtils::GetInstance()->SlotImportKeyFromFile(
        this, key_list_->GetCurrentGpgContextChannel());
  });

  import_key_from_clipboard_act_ =
      make_action(tr("Clipboard"), ":/icons/import_key_from_clipboard.png",
                  tr("Import New Key From Clipboard"), {QKeySequence::Paste});
  import_key_from_clipboard_act_->setShortcutContext(Qt::WindowShortcut);
  addAction(import_key_from_clipboard_act_);
  connect(import_key_from_clipboard_act_, &QAction::triggered, this, [this]() {
    CommonUtils::GetInstance()->SlotImportKeyFromClipboard(
        this, key_list_->GetCurrentGpgContextChannel());
  });

  import_keys_from_key_package_act_ =
      make_action(tr("Key Package"), ":/icons/key_package.png",
                  tr("Import Key(s) From a Key Package"));
  connect(import_keys_from_key_package_act_, &QAction::triggered, this,
          &KeyMgmt::SlotImportKeyPackage);

  export_key_to_clipboard_act_ = make_action(
      tr("Export To Clipboard"), ":/icons/export_key_to_clipboard.png",
      tr("Export Checked Key(s) To Clipboard"));
  connect(export_key_to_clipboard_act_, &QAction::triggered, this,
          &KeyMgmt::SlotExportKeyToClipboard);

  export_key_to_file_act_ =
      make_action(tr("Export As Key Package"), ":/icons/key_package.png",
                  tr("Export Checked Key(s) To a Key Package"));
  connect(export_key_to_file_act_, &QAction::triggered, this,
          &KeyMgmt::SlotExportKeyToKeyPackage);

  export_key_as_open_ssh_format_ =
      make_action(tr("Export As OpenSSH"), ":/icons/ssh-key.png",
                  tr("Export a single key in OpenSSH format to a file"));
  connect(export_key_as_open_ssh_format_, &QAction::triggered, this,
          &KeyMgmt::SlotExportAsOpenSSHFormat);

  // Window-scoped rather than handled in the table's keyPressEvent: this way
  // the shortcut works wherever focus is, it shows up beside the menu entry,
  // and one gating pass can disable both at once.
  delete_selected_keys_act_ =
      make_action(tr("Delete Selected Keys"), ":/icons/trash.png",
                  tr("Delete the Selected keys"), {QKeySequence::Delete});
  delete_selected_keys_act_->setShortcutContext(Qt::WindowShortcut);
  addAction(delete_selected_keys_act_);
  connect(delete_selected_keys_act_, &QAction::triggered, this,
          &KeyMgmt::SlotDeleteSelectedKeys);

  refresh_keys_act_ = make_action(tr("Refresh Key List"), {},
                                  tr("Re-read the keyring from disk"),
                                  {QKeySequence(Qt::Key_F5)});
  refresh_keys_act_->setShortcutContext(Qt::WindowShortcut);
  addAction(refresh_keys_act_);
  connect(refresh_keys_act_, &QAction::triggered, key_list_,
          &KeyList::SlotRefresh);

  focus_search_act_ = make_action(
      tr("Find Key"), {}, tr("Jump to the search box"), {QKeySequence::Find});
  focus_search_act_->setShortcutContext(Qt::WindowShortcut);
  addAction(focus_search_act_);
  connect(focus_search_act_, &QAction::triggered, key_list_,
          &KeyList::FocusSearchBar);

  delete_checked_keys_act_ =
      make_action(tr("Delete Checked Keys"), ":/icons/trash.png",
                  tr("Delete the Checked keys"));
  connect(delete_checked_keys_act_, &QAction::triggered, this,
          &KeyMgmt::SlotDeleteCheckedKeys);

  show_key_details_act_ =
      make_action(tr("Show Key Details"), ":/icons/detail.png",
                  tr("Show Details for this Key"));
  connect(show_key_details_act_, &QAction::triggered, this,
          &KeyMgmt::SlotShowKeyDetails);

  set_owner_trust_of_key_act_ =
      make_action(tr("Set Owner Trust Level"), ":/icons/stairs.png",
                  tr("Set how much you trust this key to certify others"));
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

  // Every gated action, by the rule that governs it. Anything missing here is
  // simply never gated — it stays enabled regardless of the selection.
  action_map_ = {
      {KeyAction::kShowDetails, show_key_details_act_},
      {KeyAction::kCopyFingerprint, copy_fingerprint_act_},
      {KeyAction::kCopyKeyId, copy_key_id_act_},
      {KeyAction::kCopyEmail, copy_email_act_},
      {KeyAction::kCopyPublicKey, copy_public_key_act_},
      {KeyAction::kCertify, certify_key_act_},
      {KeyAction::kSetExpiry, set_expiry_act_},
      {KeyAction::kGenerateSubkey, generate_subkey_act_},
      {KeyAction::kSetOwnerTrust, set_owner_trust_of_key_act_},
      {KeyAction::kGenerateRevokeCert, generate_revoke_cert_act_},
      {KeyAction::kDeleteSelected, delete_selected_keys_act_},
      {KeyAction::kDeleteChecked, delete_checked_keys_act_},
      {KeyAction::kExportPackage, export_key_to_file_act_},
      {KeyAction::kExportClipboard, export_key_to_clipboard_act_},
      {KeyAction::kExportOpenSsh, export_key_as_open_ssh_format_},
      {KeyAction::kExportPublicKey, export_public_key_act_},
      {KeyAction::kExportPrivateKey, export_private_key_act_},
      {KeyAction::kKeyserverSearch, import_key_from_key_server_act_},
      {KeyAction::kKeyserverPublish, publish_key_to_key_server_act_},
      {KeyAction::kKeyserverRefresh, refresh_selected_from_key_server_act_},
      {KeyAction::kBulkSetOwnerTrust, bulk_set_owner_trust_act_},
      {KeyAction::kBulkExtendExpiry, bulk_extend_expiry_act_},
      {KeyAction::kBackupAllPrivate, backup_all_private_keys_act_},
  };
}

void KeyMgmt::create_quick_actions() {
  copy_fingerprint_act_ =
      make_action(tr("Copy Fingerprint"), {},
                  tr("Copy this key's full fingerprint to the clipboard"));
  connect(copy_fingerprint_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedKey();
    if (key == nullptr) return;
    copy_text_to_clipboard(key->Fingerprint(), tr("Fingerprint"));
  });

  copy_key_id_act_ = make_action(tr("Copy Key ID"), {},
                                 tr("Copy this key's ID to the clipboard"));
  connect(copy_key_id_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedKey();
    if (key == nullptr) return;
    copy_text_to_clipboard(key->ID(), tr("Key ID"));
  });

  copy_email_act_ =
      make_action(tr("Copy Email"), {},
                  tr("Copy this key's email address to the clipboard"));
  connect(copy_email_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedKey();
    if (key == nullptr) return;
    copy_text_to_clipboard(key->Email(), tr("Email"));
  });

  copy_public_key_act_ =
      make_action(tr("Copy Public Key Block"), {},
                  tr("Copy this key's armored public key block to the "
                     "clipboard, ready to paste or send"));
  connect(copy_public_key_act_, &QAction::triggered, this, [this]() {
    // Armored blocks concatenate, so several selected keys give one paste-able
    // chunk rather than needing one copy each.
    auto keys = key_list_->GetSelectedKeys();
    if (keys.empty()) return;
    export_keys_to_clipboard(keys);
  });

  certify_key_act_ = make_action(tr("Certify Key..."), {},
                                 tr("Sign this key's identity with your key"));
  connect(certify_key_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedGpgKey();
    if (key == nullptr) return;
    new KeyUIDSignDialog(key_list_->GetCurrentGpgContextChannel(), key,
                         key->UID(), this);
  });

  set_expiry_act_ =
      make_action(tr("Set Expiry..."), {}, tr("Change when this key expires"));
  connect(set_expiry_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedGpgKey();
    if (key == nullptr) return;
    auto* dialog = new KeySetExpireDateDialog(
        key_list_->GetCurrentGpgContextChannel(), key, this);
    dialog->show();
  });

  // Saving one key to a file used to require opening the key details dialog
  // and finding its Operations tab; these put it one click from the key list.
  export_public_key_act_ =
      make_action(tr("Export Public Key..."), {},
                  tr("Save this key's public half to a file you can send"));
  connect(export_public_key_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedGpgKey();
    if (key == nullptr) return;
    (new ExportKey(this))
        ->ExecPublic(key_list_->GetCurrentGpgContextChannel(), key);
  });

  export_private_key_act_ = make_action(
      tr("Export Private Key..."), {},
      tr("Save this key's private half to a file — keep it to yourself"));
  connect(export_private_key_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedGpgKey();
    if (key == nullptr) return;
    (new ExportKey(this))
        ->ExecPrivate(key_list_->GetCurrentGpgContextChannel(), key);
  });

  generate_revoke_cert_act_ = make_action(
      tr("Generate Revocation Certificate..."), {},
      tr("Create a certificate that can revoke this key, to store somewhere "
         "safe in case you ever lose control of it"));
  connect(generate_revoke_cert_act_, &QAction::triggered, this, [this]() {
    auto key = key_list_->GetSelectedGpgKey();
    if (key == nullptr) return;
    auto* function = new GenerateRevocationCert(this);
    function->Exec(key_list_->GetCurrentGpgContextChannel(), key);
  });
}

void KeyMgmt::create_keyserver_actions() {
  import_key_from_key_server_act_ = make_action(
      tr("Search Keyserver..."), ":/icons/import_key_from_server.png",
      tr("Search a keyserver and import keys"));
  connect(import_key_from_key_server_act_, &QAction::triggered, this, [this]() {
    // Seed the search with the selected key so the context-menu entry searches
    // the key the user clicked; stays blank when nothing is selected.
    QString fpr;
    auto selected = key_list_->GetSelectedKey();
    if (selected != nullptr) fpr = selected->Fingerprint();

    Module::TriggerEvent("REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT",
                         {
                             {"fingerprint", GFBuffer(fpr)},
                             {"parent", GFBuffer(RegisterQObject(this))},
                         });
  });

  publish_key_to_key_server_act_ = make_action(
      tr("Publish Key to Keyserver..."), {},
      tr("Upload the checked public key(s) to the key server configured as the "
         "default"));
  connect(publish_key_to_key_server_act_, &QAction::triggered, this,
          [this]() { publish_keys_to_key_server(target_keys()); });

  refresh_selected_from_key_server_act_ =
      make_action(tr("Refresh Selected from Keyserver"), {},
                  tr("Fetch the latest copy of these key(s) from the key "
                     "server, picking up new signatures and revocations"));
  connect(refresh_selected_from_key_server_act_, &QAction::triggered, this,
          [this]() {
            auto keys = target_keys();
            if (keys.empty()) return;
            key_list_->SyncKeysFromKeyServer(keys);
          });
}

void KeyMgmt::create_bulk_actions() {
  bulk_set_owner_trust_act_ =
      make_action(tr("Set Owner Trust for Checked Keys..."), {},
                  tr("Give every checked key the same owner trust level"));
  connect(bulk_set_owner_trust_act_, &QAction::triggered, this,
          &KeyMgmt::bulk_set_owner_trust);

  bulk_extend_expiry_act_ =
      make_action(tr("Extend Expiry of Checked Keys..."), {},
                  tr("Push back the expiry date of every checked private key "
                     "in one step"));
  connect(bulk_extend_expiry_act_, &QAction::triggered, this,
          &KeyMgmt::bulk_extend_expiry);

  backup_all_private_keys_act_ = make_action(
      tr("Back Up All Private Keys..."), {},
      tr("Export every private key in this keyring to a single key package"));
  connect(backup_all_private_keys_act_, &QAction::triggered, this,
          &KeyMgmt::backup_all_private_keys);
}

void KeyMgmt::create_menus() {
  // Split by what an entry does, not by what it acts on: everything the popup
  // menu offers now has exactly one home in the menu bar as well, so a user who
  // never right-clicks can still find it. The actions are the *same* QAction
  // objects the popup uses, which is what keeps the two from drifting apart —
  // Qt is happy for one action to sit in several menus.
  file_menu_ = menuBar()->addMenu(tr("File"));
  file_menu_->addAction(open_key_file_act_);
  file_menu_->addAction(close_act_);

  edit_menu_ = menuBar()->addMenu(tr("Edit"));
  edit_menu_->addAction(copy_fingerprint_act_);
  edit_menu_->addAction(copy_key_id_act_);
  edit_menu_->addAction(copy_email_act_);
  edit_menu_->addSeparator();
  edit_menu_->addAction(copy_public_key_act_);
  edit_menu_->addSeparator();
  edit_menu_->addAction(focus_search_act_);

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
  export_key_menu_->addSeparator();
  export_key_menu_->addAction(export_public_key_act_);
  export_key_menu_->addAction(export_private_key_act_);

  // The same QMenu object as the popup's Category submenu, so both copies stay
  // current — which is why populate_key_category_menu() is driven from
  // update_key_action_state() rather than only on right-click.
  key_menu_->addMenu(add_key_2_category_menu_);

  // Built as a submenu so the tool bar's Delete button and this entry are the
  // same QMenu object, the way Import and Export already work.
  key_menu_->addSeparator();
  delete_menu_ = key_menu_->addMenu(tr("Delete"));
  delete_menu_->setIcon(QIcon(":/icons/trash.png"));
  delete_menu_->addAction(delete_selected_keys_act_);
  delete_menu_->addAction(delete_checked_keys_act_);

  opera_menu_ = menuBar()->addMenu(tr("Operations"));
  opera_menu_->addAction(show_key_details_act_);
  opera_menu_->addSeparator();
  opera_menu_->addAction(certify_key_act_);
  opera_menu_->addAction(set_expiry_act_);
  opera_menu_->addAction(generate_subkey_act_);
  opera_menu_->addAction(set_owner_trust_of_key_act_);
  opera_menu_->addSeparator();
  opera_menu_->addAction(generate_revoke_cert_act_);
  opera_menu_->addSeparator();

  bulk_menu_ = opera_menu_->addMenu(tr("Bulk"));
  bulk_menu_->addAction(bulk_set_owner_trust_act_);
  bulk_menu_->addAction(bulk_extend_expiry_act_);
  bulk_menu_->addSeparator();
  bulk_menu_->addAction(backup_all_private_keys_act_);

  keyserver_menu_ = menuBar()->addMenu(tr("Keyserver"));
  keyserver_menu_->addAction(import_key_from_key_server_act_);
  keyserver_menu_->addAction(publish_key_to_key_server_act_);
  keyserver_menu_->addAction(refresh_selected_from_key_server_act_);
  // Visibility is derived from its entries by sync_submenu_visibility().
}

void KeyMgmt::create_tool_bars() {
  key_tool_bar_ = addToolBar(tr("Key"));
  // restoreState() matches tool bars by object name, so this string is part of
  // the saved-layout contract: renaming it silently discards the position the
  // user dragged the bar to.
  key_tool_bar_->setObjectName("keytoolbar");
  SetupToolBar(key_tool_bar_, icon_style_, icon_size_);

  // generate key pair
  key_tool_bar_->addAction(generate_key_pair_act_);
  key_tool_bar_->addSeparator();

  // add button with popup menu for import
  import_tool_button_ = new QToolButton(this);
  SetupMenuToolButton(import_tool_button_, import_key_menu_,
                      QIcon(":/icons/key_import.png"), tr("Import Key"),
                      tr("Import key"), icon_style_, icon_size_);
  key_tool_bar_->addWidget(import_tool_button_);

  export_tool_button_ = new QToolButton(this);
  SetupMenuToolButton(export_tool_button_, export_key_menu_,
                      QIcon(":/icons/key_export.png"), tr("Export Key"),
                      tr("Export Key"), icon_style_, icon_size_);
  key_tool_bar_->addWidget(export_tool_button_);

  key_tool_bar_->addSeparator();

  // The per-key half of the bar. Reuses the popup's Key Operations menu rather
  // than building a third parallel structure — one QMenu can back several
  // buttons — so the button and the context menu can never disagree.
  key_tool_bar_->addAction(show_key_details_act_);

  key_ops_tool_button_ = new QToolButton(this);
  SetupMenuToolButton(
      key_ops_tool_button_, popup_key_ops_menu_, QIcon(":/icons/key.png"),
      tr("Key Operations"),
      tr("Certify, set expiry, add a subkey, set trust, revoke"), icon_style_,
      icon_size_);
  key_tool_bar_->addWidget(key_ops_tool_button_);

  // One button rather than the two near-identical delete actions it replaces:
  // side by side they were the widest pair on the bar and carried its only
  // alarming colour, which put the visual weight of the window on the one thing
  // a user is least often here to do. Both actions stay a keystroke away on
  // Del, in the Key menu, and in the context menu.
  key_tool_bar_->addSeparator();
  delete_tool_button_ = new QToolButton(this);
  SetupMenuToolButton(delete_tool_button_, delete_menu_,
                      QIcon(":/icons/trash.png"), tr("Delete"),
                      tr("Delete the selected or the checked keys"),
                      icon_style_, icon_size_);
  key_tool_bar_->addWidget(delete_tool_button_);

  // Built here rather than in create_menus() because it hands out the tool
  // bar's own toggle action, which does not exist until the bar does.
  view_menu_ = menuBar()->addMenu(tr("View"));
  view_menu_->addAction(refresh_keys_act_);
  view_menu_->addSeparator();
  view_menu_->addAction(key_tool_bar_->toggleViewAction());
}

void KeyMgmt::init_window_style() {
  // Owned, not inherited. This window only ever looked styled because it
  // happens to be parented to the MainWindow and a style sheet cascades down
  // the parent chain — and, as the note beside the status-bar hookup says,
  // nothing enforces that parenting.
  setStyleSheet(MainWindowChromeStyleSheet() + QStringLiteral(R"(
/* The key list's tool row is the lower half of this window's chrome, not
   content: same surface as the tool bar above it, with a single hairline under
   the pair, so the two strips read as one block instead of two competing bars.
   Scoped to the object name KeyList sets on that row, so the key list docked
   inside the main window is untouched. */
QWidget#KeyListMenu {
  background: palette(window);
  border-bottom: 1px solid palette(mid);
}
)"));
}

void KeyMgmt::align_chrome_insets() {
  if (key_tool_bar_ == nullptr || key_list_ == nullptr) return;

  // Hidden via the View menu, or docked to a side, the bar leaves no bottom
  // edge for the tool row to continue, so the row goes back to standing on its
  // own. isHidden() rather than isVisible(): during construction nothing is
  // visible yet, and the common case has to be the aligned one.
  if (key_tool_bar_->isHidden() ||
      toolBarArea(key_tool_bar_) != Qt::TopToolBarArea) {
    key_list_->SetChromeInset({3, 3, 3, 3}, {0, 0, 0, 0});
    return;
  }

  const int handle = key_tool_bar_->isMovable()
                         ? style()->pixelMetric(QStyle::PM_ToolBarHandleExtent,
                                                nullptr, key_tool_bar_)
                         : 0;

  // 2px of that is the tool bar padding from the chrome style sheet; the rest
  // is whatever the active style puts around a tool bar item.
  const int inset = handle + 2 +
                    style()->pixelMetric(QStyle::PM_ToolBarItemMargin, nullptr,
                                         key_tool_bar_);

  key_list_->SetChromeInset({0, 0, 0, 0}, {inset, 4, inset, 4});
}

void KeyMgmt::apply_tool_bar_appearance() {
  if (key_tool_bar_ != nullptr) {
    key_tool_bar_->setToolButtonStyle(icon_style_);
    key_tool_bar_->setIconSize(icon_size_);
  }

  const QList<QToolButton*> menu_buttons = {
      import_tool_button_,
      export_tool_button_,
      key_ops_tool_button_,
      delete_tool_button_,
  };

  for (auto* button : menu_buttons) {
    if (button == nullptr) continue;

    button->setToolButtonStyle(icon_style_);
    button->setIconSize(icon_size_);

    // A button added with addWidget() keeps its old size hint otherwise, so an
    // icon-only/text-under-icon switch would leave it the wrong height.
    button->updateGeometry();
    button->update();
  }

  // The bar's metrics just moved, so the row below it has a new edge to match.
  align_chrome_insets();
}

void KeyMgmt::changeEvent(QEvent* event) {
  GeneralMainWindow::changeEvent(event);

  if (event != nullptr && event->type() == QEvent::StyleChange) {
    align_chrome_insets();
  }
}

auto KeyMgmt::target_keys() const -> GpgAbstractKeyPtrList {
  auto keys = key_list_->GetCheckedKeys();
  if (!keys.empty()) return keys;
  return key_list_->GetSelectedKeys();
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
  // Unreachable through the UI — the action is disabled when there is nothing
  // to export — but the slot must still hold its own.
  auto keys = target_keys();
  if (keys.empty()) return;

  auto* dialog = new ExportKeyPackageDialog(
      key_list_->GetCurrentGpgContextChannel(), keys, this);
  dialog->exec();
  emit SignalStatusBarChanged(tr("key(s) exported"));
}

void KeyMgmt::SlotExportKeyToClipboard() {
  export_keys_to_clipboard(target_keys());
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
  if (keys.empty()) return;

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

                        // The target is configurable, so say where it went.
                        const auto key_server =
                            p["key_server"].ConvertToQString();
                        auto message =
                            key_server.isEmpty()
                                ? tr("The public key was uploaded to the key "
                                     "server.")
                                : tr("The public key was uploaded to %1.")
                                      .arg(QUrl(key_server).host());

                        // An HKP upload reports no fingerprint back, so only
                        // claim one when the server actually gave us one.
                        const auto fingerprint =
                            p["fingerprint"].ConvertToQString();
                        if (!fingerprint.isEmpty()) {
                          message +=
                              "\n\n" + tr("Fingerprint: %1").arg(fingerprint);
                        }

                        QMessageBox::information(this, tr("Upload Complete"),
                                                 message);
                      });
                });
      });
}

void KeyMgmt::bulk_set_owner_trust() {
  auto keys = target_keys();
  if (keys.empty()) return;

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
  auto targets = target_keys();
  if (targets.empty()) return;

  GpgKeyPtrList keys;
  for (const auto& key : targets) {
    if (key == nullptr || !key->IsPrivateKey()) continue;
    if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) continue;
    keys.push_back(qSharedPointerDynamicCast<GpgKey>(key));
  }

  if (keys.empty()) {
    // Not the same as "nothing selected", and the difference is the whole
    // answer: expiry is set by whoever owns the key, so someone else's public
    // key can never be extended no matter how many you pick.
    QMessageBox::information(
        this, tr("Nothing to Extend"),
        tr("None of the %n selected key(s) has a private key, so their expiry "
           "cannot be changed. Expiry is set on keys you own.",
           "", static_cast<int>(targets.size())));
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
  // OpenSSH carries one key per file, so this is the one export that stays
  // single-key; the action is disabled unless exactly one key is targeted.
  auto keys = target_keys();
  if (keys.empty() || keys.size() > 1) return;

  assert(std::all_of(keys.begin(), keys.end(),
                     [](const auto& key) { return key->IsGood(); }));

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

auto KeyMgmt::build_key_action_context() const -> KeyActionContext {
  const auto channel = key_list_->GetCurrentGpgContextChannel();
  const auto selected = key_list_->GetSelectedKeys();
  const auto checked = key_list_->GetCheckedKeys();

  KeyActionContext ctx;
  ctx.selected_count = static_cast<int>(selected.size());
  ctx.checked_count = static_cast<int>(checked.size());

  if (ctx.selected_count == 1) {
    const auto& key = selected.front();
    if (key != nullptr) {
      ctx.selection_is_gpg_key = key->KeyType() == GpgAbstractKeyType::kGPG_KEY;
      ctx.selection_is_private = key->IsPrivateKey();
      ctx.selection_has_email = !key->Email().isEmpty();
    }
  }

  ctx.sign_supported = IsOpSupported<SignKeyOpTag>(channel);
  ctx.owner_trust_supported = IsOpSupported<SetOwnerTrustLevelOpTag>(channel);
  ctx.subkey_generation_supported = IsOpSupported<GenerateSubKeyTag>(channel);
  ctx.ssh_export_supported =
      IsOpSupported<ExportKeyAsOpenSSHFormatOpTag>(channel);

  const bool keyserver_available =
      Module::IsEventListening("REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT");
  ctx.keyserver_search_available = keyserver_available;
  ctx.keyserver_upload_available =
      keyserver_available &&
      Module::IsEventListening("REQUEST_UPLOAD_PUBLIC_KEY");
  ctx.keyserver_fetch_available =
      keyserver_available &&
      Module::IsEventListening("REQUEST_GET_PUBLIC_KEY_BY_KEY_ID");

  const auto targets = !checked.isEmpty() ? checked : selected;
  ctx.any_target_private_key =
      std::any_of(targets.cbegin(), targets.cend(), [](const auto& key) {
        return key != nullptr && key->IsPrivateKey() &&
               key->KeyType() == GpgAbstractKeyType::kGPG_KEY;
      });

  // Read from the cached flag rather than the keyring: this runs on every
  // selection change, including every arrow-key press, and Fetch() copies the
  // whole key list under a lock to answer one yes-or-no question.
  ctx.any_private_key_in_keyring = keyring_has_private_key_;

  return ctx;
}

void KeyMgmt::refresh_keyring_summary_flags() {
  const auto keys =
      GpgKeyRepository::GetInstance(key_list_->GetCurrentGpgContextChannel())
          .Fetch();

  keyring_has_private_key_ = std::any_of(
      keys.cbegin(), keys.cend(),
      [](const auto& key) { return key != nullptr && key->IsPrivateKey(); });
}

void KeyMgmt::update_key_action_state() {
  const auto ctx = build_key_action_context();

  for (auto it = action_map_.cbegin(); it != action_map_.cend(); ++it) {
    auto* action = it.value();
    if (action == nullptr) continue;

    const auto state = EvaluateKeyAction(it.key(), ctx);

    action->setVisible(state.supported);
    action->setEnabled(state.enabled);

    // The base text was stashed by make_action(); without it the reason would
    // accumulate onto the tooltip every time the selection changed.
    const auto base = action->property("gfBaseToolTip").toString();
    QString tip = base;
    if (!state.enabled && !state.reason.isEmpty()) {
      tip = base.isEmpty() ? state.reason : base + "\n" + state.reason;
    }
    action->setToolTip(tip);
    action->setStatusTip(tip);
  }

  auto targets = key_list_->GetCheckedKeys();
  if (targets.isEmpty()) targets = key_list_->GetSelectedKeys();
  populate_key_category_menu(key_list_->GetCurrentGpgContextChannel(),
                             key_list_->GetCurrentCategoryId(), targets);

  sync_submenu_visibility();

  // A tool button is not a menu entry: nothing propagates the state of the menu
  // behind it, so the Delete button would stay lit with nothing to delete.
  // Runs after sync_submenu_visibility(), which is what decides whether the
  // menu has any usable entry left at all.
  if (delete_tool_button_ != nullptr && delete_menu_ != nullptr) {
    delete_tool_button_->setVisible(delete_menu_->menuAction()->isVisible());
    delete_tool_button_->setEnabled(delete_selected_keys_act_->isEnabled() ||
                                    delete_checked_keys_act_->isEnabled());
  }
}

void KeyMgmt::sync_submenu_visibility() {
  // A submenu whose every entry is hidden is an empty arrow the user can only
  // learn is empty by opening it.
  const QVector<QMenu*> menus = {
      popup_key_ops_menu_, popup_export_menu_, popup_keyserver_menu_,
      keyserver_menu_,     export_key_menu_,   generate_key_menu_,
      import_key_menu_,    copy_menu_,         bulk_menu_,
      delete_menu_};

  for (auto* menu : menus) {
    if (menu == nullptr) continue;

    const auto actions = menu->actions();
    const bool any_visible =
        std::any_of(actions.cbegin(), actions.cend(), [](QAction* action) {
          return action != nullptr && !action->isSeparator() &&
                 action->isVisible();
        });
    menu->menuAction()->setVisible(any_visible);
  }
}

void KeyMgmt::slot_popup_menu_by_key_list(QContextMenuEvent* event,
                                          KeyTable* key_table) {
  if (event == nullptr || key_table == nullptr) return;

  auto keys = key_table->GetSelectedKeys();
  if (keys.isEmpty()) {
    // Right-clicking empty space used to do nothing at all, which is worst
    // exactly when the table is empty and the user is looking for a way in.
    if (empty_area_menu_ != nullptr) {
      empty_area_menu_->exec(event->globalPos());
    }
    return;
  }

  // The shared, authoritative pass. Both the menu bar and this popup see it.
  update_key_action_state();

  // Popup-only override. A greyed-out entry is right in a menu bar — it teaches
  // what exists — and wrong in a context menu, where it is just noise beside
  // the handful of things you can actually do to the key under the cursor. So
  // hide here whatever the shared pass just disabled.
  //
  // Safe because QMenu::exec() runs a nested, *blocking* event loop with the
  // menu bar unreachable: nobody can observe the overridden state, and the
  // guard puts it back the moment exec() returns, including when a triggered
  // slot throws or opens a dialog.
  QVector<QAction*> hidden;
  for (auto* action : popup_actions_) {
    if (action == nullptr || action->isEnabled() || !action->isVisible()) {
      continue;
    }
    action->setVisible(false);
    hidden.push_back(action);
  }
  sync_submenu_visibility();

  const auto restore = qScopeGuard([&]() {
    for (auto* action : hidden) action->setVisible(true);
    sync_submenu_visibility();
  });

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
