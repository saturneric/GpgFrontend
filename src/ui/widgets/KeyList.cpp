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
#include "core/model/GpgImportInformation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/KeyGroupCreationDialog.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"

//
#include "ui_KeyList.h"

namespace GpgFrontend::UI {

namespace {

auto IsOwnerTrustSupported(int channel) -> bool {
  // In release 2.1.x, only the GnuPG engine is supported, and it supports owner
  // trust.
  return true;
}

auto ApplyEngineColumnFilter(int channel, GpgKeyTableColumn columns)
    -> GpgKeyTableColumn {
  if (!IsOwnerTrustSupported(channel)) {
    columns = columns & ~GpgKeyTableColumn::kOWNER_TRUST;
  }

  return columns;
}

}  // namespace

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

auto KeyList::has_ability(KeyMenuAbility ability) const -> bool {
  using T = std::underlying_type_t<KeyMenuAbility>;

  return (static_cast<T>(menu_ability_) & static_cast<T>(ability)) !=
         static_cast<T>(KeyMenuAbility::kNONE);
}

void KeyList::init_ui_visibility() {
  ui_->menuWidget->setHidden(menu_ability_ == KeyMenuAbility::kNONE);

  ui_->refreshKeyListButton->setHidden(!has_ability(KeyMenuAbility::kREFRESH));
  ui_->syncButton->setHidden(!has_ability(KeyMenuAbility::kSYNC_PUBLIC_KEY));
  ui_->checkALLButton->setHidden(!has_ability(KeyMenuAbility::kCHECK_ALL));
  ui_->uncheckButton->setHidden(!has_ability(KeyMenuAbility::kUNCHECK_ALL));
  ui_->columnTypeButton->setHidden(
      !has_ability(KeyMenuAbility::kCOLUMN_FILTER));
  ui_->searchBarEdit->setHidden(!has_ability(KeyMenuAbility::kSEARCH_BAR));
  ui_->switchContextButton->setHidden(
      !has_ability(KeyMenuAbility::kKEY_DATABASE));
  ui_->keyGroupButton->setHidden(!has_ability(KeyMenuAbility::kKEY_GROUP));
}

void KeyList::init_ui_style() {
  setObjectName(QStringLiteral("KeyList"));

  ui_->menuWidget->setObjectName(QStringLiteral("KeyListMenu"));
  ui_->keyGroupTab->setObjectName(QStringLiteral("KeyGroupTab"));

  ui_->searchBarEdit->setClearButtonEnabled(true);
  ui_->searchBarEdit->setMinimumHeight(30);

  const auto setup_tool_button = [](QToolButton* button) {
    button->setMinimumHeight(28);
    button->setFocusPolicy(Qt::NoFocus);
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIconSize(QSize(16, 16));
    button->setAutoRaise(false);
  };

  setup_tool_button(ui_->refreshKeyListButton);
  setup_tool_button(ui_->syncButton);
  setup_tool_button(ui_->uncheckButton);
  setup_tool_button(ui_->checkALLButton);
  setup_tool_button(ui_->columnTypeButton);
  setup_tool_button(ui_->keyGroupButton);
  setup_tool_button(ui_->switchContextButton);

  ui_->columnTypeButton->setPopupMode(QToolButton::InstantPopup);
  ui_->switchContextButton->setPopupMode(QToolButton::InstantPopup);

  ui_->keyGroupTab->setDocumentMode(true);
  ui_->keyGroupTab->setUsesScrollButtons(true);
  ui_->keyGroupTab->setElideMode(Qt::ElideRight);
  ui_->keyGroupTab->tabBar()->setExpanding(false);
  ui_->keyGroupTab->tabBar()->setDrawBase(false);
}

void KeyList::init_texts() {
  ui_->refreshKeyListButton->setText(tr("Refresh"));
  ui_->refreshKeyListButton->setToolTip(
      tr("Refresh the key list to synchronize changes."));

  ui_->syncButton->setText(tr("Sync Public Key"));
  ui_->syncButton->setToolTip(
      tr("Sync public keys with the default keyserver."));

  ui_->uncheckButton->setText(tr("Uncheck All"));
  ui_->uncheckButton->setToolTip(tr("Uncheck all keys in the current tab."));

  ui_->checkALLButton->setText(tr("Check All"));
  ui_->checkALLButton->setToolTip(tr("Check all keys in the current tab."));

  ui_->searchBarEdit->setPlaceholderText(
      tr("Search keys by user ID, key ID, fingerprint..."));

  ui_->columnTypeButton->setText(tr("Columns"));
  ui_->columnTypeButton->setToolTip(tr("Choose visible key table columns."));

  ui_->keyGroupButton->setText(tr("New Key Group"));
  ui_->keyGroupButton->setToolTip(
      tr("Create a key group from checked encryption-capable keys."));

  ui_->switchContextButton->setText(tr("Key Databases"));
  ui_->switchContextButton->setToolTip(tr("Switch between key databases."));
}

void KeyList::init_context_menu() {
  auto* gpg_context_menu = new QMenu(this);
  auto* gpg_context_groups = new QActionGroup(this);
  gpg_context_groups->setExclusive(true);

  const auto key_db_infos = GetGpgKeyDatabaseInfos();

  for (const auto& key_db_info : key_db_infos) {
    const auto channel = key_db_info.channel;
    const auto key_db_name = key_db_info.name;

    auto* action = new QAction(QString("%1  ·  %2")
                                   .arg(key_db_name)
                                   .arg(tr("Channel %1").arg(channel)),
                               this);

    action->setCheckable(true);
    action->setChecked(channel == current_gpg_context_channel_);

    connect(action, &QAction::toggled, this, [this, channel](bool checked) {
      if (!checked) return;

      current_gpg_context_channel_ = channel;
      ui_->channelLcdNumber->display(channel);

      init_column_menu();
      UpdateKeyTableColumnType(global_column_filter_);

      emit SignalRefreshDatabase();
    });

    gpg_context_groups->addAction(action);
    gpg_context_menu->addAction(action);
  }

  if (gpg_context_menu->isEmpty()) {
    auto* empty_action =
        gpg_context_menu->addAction(tr("No key database available"));
    empty_action->setEnabled(false);
  }

  ui_->switchContextButton->setMenu(gpg_context_menu);
}

void KeyList::init_column_menu() {
  auto* column_type_menu = new QMenu(this);

  auto add_column_action = [this, column_type_menu](QAction*& action,
                                                    const QString& title,
                                                    GpgKeyTableColumn column) {
    action = new QAction(title, this);
    action->setCheckable(true);
    action->setChecked((global_column_filter_ & column) !=
                       GpgKeyTableColumn::kNONE);

    connect(action, &QAction::toggled, this, [this, column](bool checked) {
      UpdateKeyTableColumnType(checked ? global_column_filter_ | column
                                       : global_column_filter_ & ~column);
    });

    auto effective_fixed_columns = ApplyEngineColumnFilter(
        current_gpg_context_channel_, fixed_columns_filter_);

    if ((effective_fixed_columns & column) != GpgKeyTableColumn::kNONE) {
      column_type_menu->addAction(action);
    }
  };

  add_column_action(key_id_column_action_, tr("Key ID"),
                    GpgKeyTableColumn::kKEY_ID);
  add_column_action(algo_column_action_, tr("Algorithm"),
                    GpgKeyTableColumn::kALGO);
  add_column_action(create_date_column_action_, tr("Create Date"),
                    GpgKeyTableColumn::kCREATE_DATE);
  add_column_action(owner_trust_column_action_, tr("Owner Trust"),
                    GpgKeyTableColumn::kOWNER_TRUST);
  add_column_action(subkeys_number_column_action_, tr("Subkeys"),
                    GpgKeyTableColumn::kSUBKEYS_NUMBER);
  add_column_action(comment_column_action_, tr("Comment"),
                    GpgKeyTableColumn::kCOMMENT);

  if (column_type_menu->isEmpty()) {
    auto* empty_action = column_type_menu->addAction(tr("No optional columns"));
    empty_action->setEnabled(false);
  }

  ui_->columnTypeButton->setMenu(column_type_menu);
}

void KeyList::init_signals() {
  connect(this, &KeyList::SignalRefreshDatabase, UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyList::SlotRefresh);
  connect(UISignalStation::GetInstance(), &UISignalStation::SignalUIRefresh,
          this, &KeyList::SlotRefreshUI);

  connect(ui_->refreshKeyListButton, &QPushButton::clicked, this,
          &KeyList::SignalRefreshDatabase);

  connect(ui_->uncheckButton, &QPushButton::clicked, this,
          &KeyList::uncheck_all);
  connect(ui_->checkALLButton, &QPushButton::clicked, this,
          &KeyList::check_all);
  connect(ui_->syncButton, &QPushButton::clicked, this,
          &KeyList::slot_sync_with_key_server);

  search_timer_ = new QTimer(this);
  search_timer_->setSingleShot(true);
  search_timer_->setInterval(180);

  connect(ui_->searchBarEdit, &QLineEdit::textChanged, this,
          [this]() { search_timer_->start(); });

  connect(search_timer_, &QTimer::timeout, this, &KeyList::filter_by_keyword);

  connect(this, &KeyList::SignalRefreshStatusBar,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshStatusBar);

  connect(this, &KeyList::SignalColumnTypeChange, this, [this]() {
    GetSettings().setValue("keys/global_columns_filter",
                           static_cast<unsigned int>(global_column_filter_));
  });

  connect(ui_->keyGroupButton, &QPushButton::clicked, this,
          &KeyList::slot_new_key_group);

  connect(this, &KeyList::SignalKeyChecked, this,
          [this]() { update_action_state(); });
}

void KeyList::update_action_state() {
  const bool has_key_table =
      ui_ != nullptr && ui_->keyGroupTab != nullptr &&
      qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget()) != nullptr;

  if (!has_key_table) {
    ui_->keyGroupButton->setEnabled(false);
    ui_->checkALLButton->setEnabled(false);
    ui_->uncheckButton->setEnabled(false);

    if (!ui_->syncButton->isHidden()) {
      ui_->syncButton->setEnabled(false);
    }

    return;
  }

  ui_->checkALLButton->setEnabled(true);
  ui_->uncheckButton->setEnabled(true);

  const auto keys = GetCheckedKeys();

  bool can_create_group = !keys.empty();
  for (const auto& key : keys) {
    if (key == nullptr || !key->IsHasEncrCap()) {
      can_create_group = false;
      break;
    }
  }

  ui_->keyGroupButton->setEnabled(can_create_group);

  if (!ui_->syncButton->isHidden()) {
    ui_->syncButton->setEnabled(true);
  }
}

void KeyList::init() {
  init_ui_visibility();
  init_ui_style();
  init_context_menu();
  init_column_menu();

  ui_->keyGroupTab->clear();

  ui_->syncButton->setHidden(
      !Module::IsEventListening("REQUEST_GET_PUBLIC_KEY_BY_KEY_ID"));

  ui_->channelLcdNumber->display(current_gpg_context_channel_);

  init_signals();
  init_texts();
  update_action_state();

  setAcceptDrops(true);
}

auto KeyList::AddListGroupTab(const QString& name, const QString& id,
                              GpgKeyTableDisplayMode display_mode,
                              GpgKeyTableProxyModel::KeyFilter search_filter,
                              GpgKeyTableColumn custom_columns_filter)
    -> KeyTable* {
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
    if (key_table == nullptr) continue;

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
  if (ui_ == nullptr || ui_->keyGroupTab == nullptr) return {};

  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());
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
  if (ui_->keyGroupTab->count() == 0) return;

  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());
  if (key_table == nullptr) return;

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
  if (!event->mimeData()->hasUrls() && !event->mimeData()->hasText()) {
    event->ignore();
    return;
  }

  QDialog dialog(this);
  dialog.setWindowTitle(tr("Import Keys"));

  auto* label = new QLabel(tr("You've dropped something on the key list.\n"
                              "GpgFrontend will now try to import key(s)."),
                           &dialog);

  auto* check_box =
      new QCheckBox(tr("Ask before importing keys next time."), &dialog);

  auto confirm_import_keys =
      GetSettings().value("basic/confirm_import_keys", true).toBool();
  check_box->setChecked(confirm_import_keys);

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

  connect(button_box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  auto* vbox = new QVBoxLayout(&dialog);
  vbox->addWidget(label);
  vbox->addWidget(check_box);
  vbox->addWidget(button_box);

  if (confirm_import_keys) {
    if (dialog.exec() != QDialog::Accepted) {
      event->ignore();
      return;
    }

    auto settings = GetSettings();
    settings.setValue("basic/confirm_import_keys", check_box->isChecked());
  }

  QByteArray all_import_data;

  if (event->mimeData()->hasUrls()) {
    for (const QUrl& url : event->mimeData()->urls()) {
      if (!url.isLocalFile()) continue;

      QFile file(url.toLocalFile());
      if (!file.open(QIODevice::ReadOnly)) {
        LOG_W() << "couldn't open file:" << url.toString();
        continue;
      }

      all_import_data += file.readAll();
      all_import_data += '\n';
    }
  } else {
    all_import_data = event->mimeData()->text().toUtf8();
  }

  if (!all_import_data.isEmpty()) {
    import_keys(all_import_data);
  }
}

void KeyList::dragEnterEvent(QDragEnterEvent* event) {
  const auto* mime = event->mimeData();

  if (mime->hasUrls() || mime->hasText()) {
    event->acceptProposedAction();
    return;
  }

  event->ignore();
}

void KeyList::import_keys(const QByteArray& in_buffer) {
  LOG_D() << "importing keys to channel:" << current_gpg_context_channel_;
  auto result = GpgKeyImportExporter::GetInstance(current_gpg_context_channel_)
                    .ImportKey(GFBuffer(in_buffer));

  auto connection = QSharedPointer<QMetaObject::Connection>::create();
  *connection = connect(UISignalStation::GetInstance(),
                        &UISignalStation::SignalKeyDatabaseRefreshDone, this,
                        [this, result, connection]() {
                          auto* dialog = new KeyImportDetailDialog(
                              current_gpg_context_channel_, result, this);
                          dialog->show();

                          QObject::disconnect(*connection);
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

void KeyList::sync_keys_from_key_server(
    const KeyIdArgsList& key_ids,
    const std::function<void(const QString&, const QString&, size_t, size_t)>&
        callback) const {
  // LOOP
  decltype(key_ids.size()) current_index = 1;
  decltype(key_ids.size()) all_index = key_ids.size();

  auto channel = current_gpg_context_channel_;

  for (const auto& key_id : key_ids) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
        ->PostTask(new Thread::Task(
            [=](const DataObjectPtr&) -> int {
              // rate limit
              QThread::msleep(200);
              // call
              Module::TriggerEvent(
                  "REQUEST_GET_PUBLIC_KEY_BY_KEY_ID",
                  {
                      {"key_id", GFBuffer{key_id}},
                  },
                  [key_id, channel, callback, current_index, all_index](
                      const Module::EventIdentifier&,
                      const Module::Event::ListenerIdentifier&,
                      Module::Event::Params p) {
                    QString status;

                    if (p["ret"] != "0" || !p["error_msg"].Empty()) {
                      LOG_E()
                          << "An error occurred trying to get data from key:"
                          << key_id << "error message: "
                          << p["error_msg"].ConvertToQString() << "reply data: "
                          << p["reply_data"].ConvertToQString();
                      status = p["error_msg"].ConvertToQString() +
                               p["reply_data"].ConvertToQString();
                    } else if (p.contains("key_data")) {
                      const auto key_data = p["key_data"];
                      LOG_D() << "got key data of key " << key_id
                              << " from key server: "
                              << key_data.ConvertToQString();

                      auto result =
                          GpgKeyImportExporter::GetInstance(channel).ImportKey(
                              GFBuffer(key_data));
                      if (result->imported == 1) {
                        status = tr("The key has been updated");
                      } else {
                        status = tr("No need to update the key");
                      }
                    }

                    callback(key_id, status, current_index, all_index);
                  });

              return 0;
            },
            QString("key_%1_import_task").arg(key_id)));

    current_index++;
  }
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

  sync_keys_from_key_server(
      key_ids, [=](const QString& key_id, const QString& status,
                   size_t current_index, size_t all_index) {
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
  auto keyword = ui_->searchBarEdit->text().trimmed().toLower();

  for (int i = 0; i < ui_->keyGroupTab->count(); i++) {
    auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->widget(i));
    if (key_table == nullptr) continue;

    key_table->SetFilterKeyword(keyword);
  }

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

  const auto effective_fixed_columns = ApplyEngineColumnFilter(
      current_gpg_context_channel_, fixed_columns_filter_);

  emit SignalColumnTypeChange(effective_fixed_columns & global_column_filter_);
}

auto KeyList::GetCurrentGpgContextChannel() const -> int {
  return current_gpg_context_channel_;
}

auto KeyList::GetSelectedKeys() -> GpgAbstractKeyPtrList {
  if (ui_ == nullptr || ui_->keyGroupTab == nullptr) return {};

  auto* key_table = qobject_cast<KeyTable*>(ui_->keyGroupTab->currentWidget());
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

void KeyList::InitAfter(int channel, KeyMenuAbility menu_ability,
                        GpgKeyTableColumn fixed_columns_filter) {
  current_gpg_context_channel_ = channel;
  menu_ability_ = menu_ability;
  fixed_columns_filter_ = fixed_columns_filter;
  model_ = GpgAbstractKeyGetter::GetInstance(channel).GetGpgKeyTableModel();

  init();
}

}  // namespace GpgFrontend::UI
