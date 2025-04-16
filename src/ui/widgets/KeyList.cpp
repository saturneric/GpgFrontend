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

#include "ui/widgets/KeyList.h"

#include <cstddef>

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgAbstractKeyGetter.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/KeyGroupCreationDialog.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"

//
#include "ui_KeyList.h"

namespace GpgFrontend::UI {

KeyList::KeyList(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyList>()),
      model_(GpgAbstractKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetGpgKeyTableModel()),
      global_column_filter_(static_cast<GpgKeyTableColumn>(
          GetSettings()
              .value("keys/global_columns_filter",
                     static_cast<unsigned int>(GpgKeyTableColumn::kALL))
              .toUInt())) {
  ui_->setupUi(this);
}

KeyList::KeyList(int channel, KeyMenuAbility menu_ability,
                 GpgKeyTableColumn fixed_columns_filter, QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyList>()),
      current_gpg_context_channel_(channel),
      menu_ability_(menu_ability),
      model_(GpgAbstractKeyGetter::GetInstance(channel).GetGpgKeyTableModel()),
      fixed_columns_filter_(fixed_columns_filter),
      global_column_filter_(static_cast<GpgKeyTableColumn>(
          GetSettings()
              .value("keys/global_columns_filter",
                     static_cast<unsigned int>(GpgKeyTableColumn::kALL))
              .toUInt())) {
  ui_->setupUi(this);
  init();
}

void KeyList::init() {
  ui_->menuWidget->setHidden(menu_ability_ == KeyMenuAbility::kNONE);
  ui_->refreshKeyListButton->setHidden(~menu_ability_ &
                                       KeyMenuAbility::kREFRESH);
  ui_->syncButton->setHidden(~menu_ability_ & KeyMenuAbility::kSYNC_PUBLIC_KEY);
  ui_->checkALLButton->setHidden(~menu_ability_ & KeyMenuAbility::kCHECK_ALL);
  ui_->uncheckButton->setHidden(~menu_ability_ & KeyMenuAbility::kUNCHECK_ALL);
  ui_->columnTypeButton->setHidden(~menu_ability_ &
                                   KeyMenuAbility::kCOLUMN_FILTER);
  ui_->searchBarEdit->setHidden(~menu_ability_ & KeyMenuAbility::kSEARCH_BAR);
  ui_->switchContextButton->setHidden(~menu_ability_ &
                                      KeyMenuAbility::kKEY_DATABASE);
  ui_->keyGroupButton->setHidden(~menu_ability_ & KeyMenuAbility::kKEY_GROUP);

  auto* gpg_context_menu = new QMenu(this);
  auto* gpg_context_groups = new QActionGroup(this);
  gpg_context_groups->setExclusive(true);
  auto key_db_infos = GetGpgKeyDatabaseInfos();

  for (auto& key_db_info : key_db_infos) {
    auto channel = key_db_info.channel;
    auto key_db_name = key_db_info.name;

    LOG_D() << "context grt channel: " << channel
            << "database name: " << key_db_name;

    auto* switch_context_action =
        new QAction(QString("%1: %2").arg(channel).arg(key_db_name), this);
    switch_context_action->setCheckable(true);
    switch_context_action->setChecked(channel == current_gpg_context_channel_);
    connect(switch_context_action, &QAction::toggled, this,
            [this, channel](bool checked) {
              if (checked) {
                current_gpg_context_channel_ = channel;
                ui_->channelLcdNumber->display(channel);
                emit SignalRefreshDatabase();
              }
            });
    gpg_context_groups->addAction(switch_context_action);
    gpg_context_menu->addAction(switch_context_action);
  }
  ui_->switchContextButton->setMenu(gpg_context_menu);

  auto* column_type_menu = new QMenu(this);

  key_id_column_action_ = new QAction(tr("Key ID"), this);
  key_id_column_action_->setCheckable(true);
  key_id_column_action_->setChecked(
      (global_column_filter_ & GpgKeyTableColumn::kKEY_ID) !=
      GpgKeyTableColumn::kNONE);
  connect(key_id_column_action_, &QAction::toggled, this, [=](bool checked) {
    UpdateKeyTableColumnType(
        checked ? global_column_filter_ | GpgKeyTableColumn::kKEY_ID
                : global_column_filter_ & ~GpgKeyTableColumn::kKEY_ID);
  });

  algo_column_action_ = new QAction(tr("Algorithm"), this);
  algo_column_action_->setCheckable(true);
  algo_column_action_->setChecked(
      (global_column_filter_ & GpgKeyTableColumn::kALGO) !=
      GpgKeyTableColumn::kNONE);
  connect(algo_column_action_, &QAction::toggled, this, [=](bool checked) {
    UpdateKeyTableColumnType(
        checked ? global_column_filter_ | GpgKeyTableColumn::kALGO
                : global_column_filter_ & ~GpgKeyTableColumn::kALGO);
  });

  owner_trust_column_action_ = new QAction(tr("Owner Trust"), this);
  owner_trust_column_action_->setCheckable(true);
  owner_trust_column_action_->setChecked(
      (global_column_filter_ & GpgKeyTableColumn::kOWNER_TRUST) !=
      GpgKeyTableColumn::kNONE);
  connect(
      owner_trust_column_action_, &QAction::toggled, this, [=](bool checked) {
        UpdateKeyTableColumnType(
            checked ? global_column_filter_ | GpgKeyTableColumn::kOWNER_TRUST
                    : global_column_filter_ & ~GpgKeyTableColumn::kOWNER_TRUST);
      });

  create_date_column_action_ = new QAction(tr("Create Date"), this);
  create_date_column_action_->setCheckable(true);
  create_date_column_action_->setChecked(
      (global_column_filter_ & GpgKeyTableColumn::kCREATE_DATE) !=
      GpgKeyTableColumn::kNONE);
  connect(
      create_date_column_action_, &QAction::toggled, this, [=](bool checked) {
        UpdateKeyTableColumnType(
            checked ? global_column_filter_ | GpgKeyTableColumn::kCREATE_DATE
                    : global_column_filter_ & ~GpgKeyTableColumn::kCREATE_DATE);
      });

  subkeys_number_column_action_ = new QAction(tr("Subkey(s)"), this);
  subkeys_number_column_action_->setCheckable(true);
  subkeys_number_column_action_->setChecked(
      (global_column_filter_ & GpgKeyTableColumn::kSUBKEYS_NUMBER) !=
      GpgKeyTableColumn::kNONE);
  connect(
      subkeys_number_column_action_, &QAction::toggled, this,
      [=](bool checked) {
        UpdateKeyTableColumnType(
            checked
                ? global_column_filter_ | GpgKeyTableColumn::kSUBKEYS_NUMBER
                : global_column_filter_ & ~GpgKeyTableColumn::kSUBKEYS_NUMBER);
      });

  comment_column_action_ = new QAction(tr("Comment"), this);
  comment_column_action_->setCheckable(true);
  comment_column_action_->setChecked(
      (global_column_filter_ & GpgKeyTableColumn::kCOMMENT) !=
      GpgKeyTableColumn::kNONE);
  connect(comment_column_action_, &QAction::toggled, this, [=](bool checked) {
    UpdateKeyTableColumnType(
        checked ? global_column_filter_ | GpgKeyTableColumn::kCOMMENT
                : global_column_filter_ & ~GpgKeyTableColumn::kCOMMENT);
  });

  if ((fixed_columns_filter_ & GpgKeyTableColumn::kKEY_ID) !=
      GpgKeyTableColumn::kNONE) {
    column_type_menu->addAction(key_id_column_action_);
  }

  if ((fixed_columns_filter_ & GpgKeyTableColumn::kALGO) !=
      GpgKeyTableColumn::kNONE) {
    column_type_menu->addAction(algo_column_action_);
  }
  if ((fixed_columns_filter_ & GpgKeyTableColumn::kCREATE_DATE) !=
      GpgKeyTableColumn::kNONE) {
    column_type_menu->addAction(create_date_column_action_);
  }

  if ((fixed_columns_filter_ & GpgKeyTableColumn::kOWNER_TRUST) !=
      GpgKeyTableColumn::kNONE) {
    column_type_menu->addAction(owner_trust_column_action_);
  }

  if ((fixed_columns_filter_ & GpgKeyTableColumn::kSUBKEYS_NUMBER) !=
      GpgKeyTableColumn::kNONE) {
    column_type_menu->addAction(subkeys_number_column_action_);
  }

  if ((fixed_columns_filter_ & GpgKeyTableColumn::kCOMMENT) !=
      GpgKeyTableColumn::kNONE) {
    column_type_menu->addAction(comment_column_action_);
  }

  ui_->columnTypeButton->setMenu(column_type_menu);

  ui_->keyGroupTab->clear();

  auto forbid_all_gnupg_connection =
      GetSettings()
          .value("network/forbid_all_gnupg_connection", false)
          .toBool();

  // forbidden networks connections
  if (forbid_all_gnupg_connection) ui_->syncButton->setDisabled(true);

  // register key database refresh signal
  connect(this, &KeyList::SignalRefreshDatabase, UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyList::SlotRefresh);
  connect(UISignalStation::GetInstance(), &UISignalStation::SignalUIRefresh,
          this, &KeyList::SlotRefreshUI);

  // register key database sync signal for refresh button
  connect(ui_->refreshKeyListButton, &QPushButton::clicked, this,
          &KeyList::SignalRefreshDatabase);

  connect(ui_->uncheckButton, &QPushButton::clicked, this,
          &KeyList::uncheck_all);
  connect(ui_->checkALLButton, &QPushButton::clicked, this,
          &KeyList::check_all);
  connect(ui_->syncButton, &QPushButton::clicked, this,
          &KeyList::slot_sync_with_key_server);
  connect(ui_->searchBarEdit, &QLineEdit::textChanged, this,
          &KeyList::filter_by_keyword);
  connect(this, &KeyList::SignalRefreshStatusBar,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshStatusBar);
  connect(this, &KeyList::SignalColumnTypeChange, this, [=]() {
    GetSettings().setValue("keys/global_columns_filter",
                           static_cast<unsigned int>(global_column_filter_));
  });
  connect(ui_->keyGroupButton, &QPushButton::clicked, this,
          &KeyList::slot_new_key_group);
  connect(this, &KeyList::SignalKeyChecked, this, [=]() {
    auto keys = GetCheckedKeys();

    ui_->keyGroupButton->setDisabled(keys.empty());
    for (const auto& key : keys) {
      if (!key->IsHasEncrCap()) ui_->keyGroupButton->setDisabled(true);
    }
  });

  setAcceptDrops(true);

  ui_->refreshKeyListButton->setText(tr("Refresh"));
  ui_->refreshKeyListButton->setToolTip(
      tr("Refresh the key list to synchronize changes."));
  ui_->syncButton->setText(tr("Sync Public Key"));
  ui_->syncButton->setToolTip(
      tr("Sync public key with your default keyserver."));
  ui_->uncheckButton->setText(tr("Uncheck ALL"));
  ui_->uncheckButton->setToolTip(
      tr("Cancel all checked items in the current tab at once."));
  ui_->checkALLButton->setText(tr("Check ALL"));
  ui_->checkALLButton->setToolTip(
      tr("Check all items in the current tab at once"));
  ui_->searchBarEdit->setPlaceholderText(tr("Search for keys..."));
}

auto KeyList::AddListGroupTab(
    const QString& name, const QString& id, GpgKeyTableDisplayMode display_mode,
    GpgKeyTableProxyModel::KeyFilter search_filter,
    GpgKeyTableColumn custom_columns_filter) -> KeyTable* {
  auto* key_table =
      new KeyTable(this, model_, display_mode, custom_columns_filter,
                   std::move(search_filter));

  key_table->setObjectName(id);
  ui_->keyGroupTab->addTab(key_table, name);

  connect(this, &KeyList::SignalColumnTypeChange, key_table,
          &KeyTable::SignalColumnTypeChange);
  connect(key_table, &KeyTable::SignalKeyChecked, this, [=]() {
    if (sender() != ui_->keyGroupTab->currentWidget()) return;
    emit SignalKeyChecked();
  });

  UpdateKeyTableColumnType(global_column_filter_);
  return key_table;
}

void KeyList::SlotRefresh() {
  ui_->refreshKeyListButton->setDisabled(true);
  ui_->syncButton->setDisabled(true);

  LOG_D() << "request new key table module, current gpg context channel: "
          << current_gpg_context_channel_;
  model_ = GpgAbstractKeyGetter::GetInstance(current_gpg_context_channel_)
               .GetGpgKeyTableModel();

  for (int i = 0; i < ui_->keyGroupTab->count(); i++) {
    auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->widget(i));
    key_table->RefreshModel(model_);
  }

  emit SignalRefreshStatusBar(tr("Refreshing Key List..."), 3000);
  this->SlotRefreshUI();
}

void KeyList::SlotRefreshUI() {
  emit SignalRefreshStatusBar(tr("Key List Refreshed."), 1000);
  ui_->refreshKeyListButton->setDisabled(false);
  ui_->syncButton->setDisabled(false);
  emit SignalKeyChecked();
}

auto KeyList::GetCheckedKeys() -> GpgAbstractKeyPtrList {
  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());

  assert(key_table != nullptr);
  if (key_table == nullptr) return {};

  return key_table->GetCheckedKeys();
}

auto KeyList::GetCheckedPrivateKey() -> GpgAbstractKeyPtrList {
  auto ret = GpgAbstractKeyPtrList{};

  auto keys = GetCheckedKeys();
  for (const auto& key : keys) {
    if (key->IsPrivateKey()) ret.push_back(key);
  }

  return ret;
}

auto KeyList::GetCheckedPublicKey() -> GpgAbstractKeyPtrList {
  auto ret = GpgAbstractKeyPtrList{};

  auto keys = GetCheckedKeys();
  for (const auto& key : keys) {
    if (!key->IsPrivateKey()) ret.push_back(key);
  }

  return ret;
}

void KeyList::SetChecked(const KeyIdArgsList& key_ids,
                         const KeyTable& key_table) {
  if (!key_ids.empty()) {
    for (int i = 0; i < key_table.GetRowCount(); i++) {
      if (std::find(
              key_ids.begin(), key_ids.end(),
              key_table.GetKeyByIndex(key_table.model()->index(i, 0))->ID()) !=
          key_ids.end()) {
        key_table.SetRowChecked(i);
      }
    }
  }
}

[[maybe_unused]] auto KeyList::ContainsPrivateKeys() -> bool {
  if (ui_->keyGroupTab->size().isEmpty()) return false;
  auto* key_table =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());

  for (int i = 0; i < key_table->rowCount(); i++) {
    if (key_table->item(i, 1) != nullptr) {
      return true;
    }
  }
  return false;
}

void KeyList::SetColumnWidth(int row, int size) {
  if (ui_->keyGroupTab->size().isEmpty()) return;
  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());
  key_table->setColumnWidth(row, size);
}

void KeyList::contextMenuEvent(QContextMenuEvent* event) {
  if (ui_->keyGroupTab->count() == 0) return;
  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());

  if (key_table == nullptr) {
    FLOG_D("m_key_list_ is nullptr, key group tab number: %d",
           ui_->keyGroupTab->count());
    return;
  }

  if (key_table->GetRowSelected() >= 0) {
    emit SignalRequestContextMenu(event, key_table);
  }
}

void KeyList::dropEvent(QDropEvent* event) {
  auto* dialog = new QDialog();

  dialog->setWindowTitle(tr("Import Keys"));
  QLabel* label;
  label = new QLabel(tr("You've dropped something on the table.") + "\n " +
                     tr("GpgFrontend will now try to import key(s).") + "\n");

  // "always import keys"-CheckBox
  auto* check_box = new QCheckBox(tr("Always import without bothering."));

  auto confirm_import_keys =
      GetSettings().value("basic/confirm_import_keys", true).toBool();
  if (confirm_import_keys) check_box->setCheckState(Qt::Checked);

  // Buttons for ok and cancel
  auto* button_box =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(button_box, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

  auto* vbox = new QVBoxLayout();
  vbox->addWidget(label);
  vbox->addWidget(check_box);
  vbox->addWidget(button_box);

  dialog->setLayout(vbox);

  if (confirm_import_keys) {
    dialog->exec();
    if (dialog->result() == QDialog::Rejected) return;

    auto settings = GetSettings();
    settings.setValue("basic/confirm_import_keys", check_box->isChecked());
  }

  if (event->mimeData()->hasUrls()) {
    for (const QUrl& tmp : event->mimeData()->urls()) {
      QFile file;
      file.setFileName(tmp.toLocalFile());
      if (!file.open(QIODevice::ReadOnly)) {
        LOG_W() << "couldn't open file: " << tmp.toString();
      }
      auto in_buffer = file.readAll();
      this->import_keys(in_buffer);
      file.close();
    }
  } else {
    auto in_buffer(event->mimeData()->text().toUtf8());
    this->import_keys(in_buffer);
  }
}

void KeyList::dragEnterEvent(QDragEnterEvent* event) {
  event->acceptProposedAction();
}

void KeyList::import_keys(const QByteArray& in_buffer) {
  LOG_D() << "importing keys to channel:" << current_gpg_context_channel_;
  auto result = GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
                    .ImportKey(GFBuffer(in_buffer));

  auto* connection = new QMetaObject::Connection;
  *connection = connect(
      UISignalStation::GetInstance(),
      &UISignalStation::SignalKeyDatabaseRefreshDone, this, [=]() {
        new KeyImportDetailDialog(current_gpg_context_channel_, result, this);
        QObject::disconnect(*connection);
        delete connection;
      });

  emit SignalRefreshDatabase();
}

auto KeyList::GetSelectedKey() -> GpgAbstractKeyPtr {
  auto k = GetSelectedKeys();
  if (k.empty()) return nullptr;
  return k.front();
}

auto KeyList::GetSelectedGpgKey() -> GpgKeyPtr {
  auto k = GetSelectedGpgKeys();
  if (k.empty()) return nullptr;
  return k.front();
}

auto KeyList::GetSelectedGpgKeys() -> GpgKeyPtrList {
  auto keys = GetSelectedKeys();
  auto g_keys = GpgKeyPtrList{};
  for (const auto& key : keys) {
    if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) continue;
    g_keys.push_back(qSharedPointerDynamicCast<GpgKey>(key));
  }
  return g_keys;
}

void KeyList::slot_sync_with_key_server() {
  auto keys = GetCheckedPublicKey();
  if (keys.empty()) {
    QMessageBox::StandardButton const reply = QMessageBox::question(
        this, QCoreApplication::tr("Sync All Public Key"),
        QCoreApplication::tr("You have not checked any public keys that you "
                             "want to synchronize, do you want to synchronize "
                             "all local public keys from the key server?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    keys = model_->GetAllKeys();
  }

  auto key_ids = ConvertKey2GpgKeyIdList(current_gpg_context_channel_, keys);
  if (key_ids.empty()) return;

  ui_->refreshKeyListButton->setDisabled(true);
  ui_->syncButton->setDisabled(true);

  emit SignalRefreshStatusBar(tr("Syncing Key List..."), 3000);

  CommonUtils::SlotImportKeyFromKeyServer(
      current_gpg_context_channel_, key_ids,
      [=](const QString& key_id, const QString& status, size_t current_index,
          size_t all_index) {
        auto status_str = tr("Sync [%1/%2] %3 %4")
                              .arg(current_index)
                              .arg(all_index)
                              .arg(key_id)
                              .arg(status);
        emit SignalRefreshStatusBar(status_str, 1500);

        if (current_index == all_index) {
          ui_->syncButton->setDisabled(false);
          ui_->refreshKeyListButton->setDisabled(false);
          emit SignalRefreshStatusBar(tr("Key List Sync Done."), 3000);
          emit this->SignalRefreshDatabase();
        }
      });
}

void KeyList::filter_by_keyword() {
  auto keyword = ui_->searchBarEdit->text();
  keyword = keyword.trimmed();

  for (int i = 0; i < ui_->keyGroupTab->count(); i++) {
    auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->widget(i));
    // refresh arguments
    key_table->SetFilterKeyword(keyword.toLower());
  }

  // refresh ui
  SlotRefreshUI();
}

void KeyList::uncheck_all() {
  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());
  if (key_table == nullptr) return;
  key_table->UncheckAll();
}

void KeyList::check_all() {
  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());
  if (key_table == nullptr) return;
  key_table->CheckAll();
}

void KeyList::UpdateKeyTableColumnType(GpgKeyTableColumn column_type) {
  global_column_filter_ = column_type;
  emit SignalColumnTypeChange(fixed_columns_filter_ & global_column_filter_);
}

auto KeyList::GetCurrentGpgContextChannel() const -> int {
  return current_gpg_context_channel_;
}

auto KeyList::GetSelectedKeys() -> GpgAbstractKeyPtrList {
  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());

  assert(key_table != nullptr);
  if (key_table == nullptr) return {};

  return key_table->GetSelectedKeys();
}

void KeyList::slot_new_key_group() {
  auto keys = GetCheckedKeys();

  QStringList proper_key_ids;
  for (const auto& key : keys) {
    if (!key->IsHasEncrCap()) continue;
    proper_key_ids.append(key->ID());
  }

  if (proper_key_ids.isEmpty()) return;

  auto* dialog =
      new KeyGroupCreationDialog(current_gpg_context_channel_, proper_key_ids);
  dialog->exec();
}

void KeyList::UpdateKeyTableFilter(
    int index, const GpgKeyTableProxyModel::KeyFilter& filter) {
  if (ui_->keyGroupTab->size().isEmpty() ||
      ui_->keyGroupTab->widget(index) == nullptr) {
    return;
  }

  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->widget(index));
  key_table->SetFilter(filter);
}

void KeyList::RefreshKeyTable(int index) {
  if (ui_->keyGroupTab->size().isEmpty() ||
      ui_->keyGroupTab->widget(index) == nullptr) {
    return;
  }

  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->widget(index));
  key_table->RefreshProxyModel();
}

void KeyList::Init(int channel, KeyMenuAbility menu_ability,
                   GpgKeyTableColumn fixed_column_filter) {
  current_gpg_context_channel_ = channel;
  menu_ability_ = menu_ability;
  fixed_columns_filter_ = fixed_column_filter;
  model_ = GpgAbstractKeyGetter::GetInstance(channel).GetGpgKeyTableModel();

  init();
}
}  // namespace GpgFrontend::UI
