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

#include "ui/widgets/KeyList.h"

#include <boost/format.hpp>
#include <mutex>
#include <utility>

#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "ui/SignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui_KeyList.h"

namespace GpgFrontend::UI {

KeyList::KeyList(KeyMenuAbility::AbilityType menu_ability, QWidget* parent)
    : QWidget(parent),
      ui_(std::make_shared<Ui_KeyList>()),
      menu_ability_(menu_ability) {
  init();
}

void KeyList::init() {
  ui_->setupUi(this);

  ui_->menuWidget->setHidden(!menu_ability_);
  ui_->refreshKeyListButton->setHidden(~menu_ability_ &
                                       KeyMenuAbility::REFRESH);
  ui_->syncButton->setHidden(~menu_ability_ & KeyMenuAbility::SYNC_PUBLIC_KEY);
  ui_->uncheckButton->setHidden(~menu_ability_ & KeyMenuAbility::UNCHECK_ALL);

  ui_->keyGroupTab->clear();
  popup_menu_ = new QMenu(this);

  bool forbid_all_gnupg_connection =
      GlobalSettingStation::GetInstance().LookupSettings(
          "network.forbid_all_gnupg_connection", false);

  // forbidden networks connections
  if (forbid_all_gnupg_connection) ui_->syncButton->setDisabled(true);

  // register key database refresh signal
  connect(this, &KeyList::SignalRefreshDatabase, SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh);
  connect(SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyList::SlotRefresh);

  // register key database sync signal for refresh button
  connect(ui_->refreshKeyListButton, &QPushButton::clicked, this,
          &KeyList::SignalRefreshDatabase);

  connect(ui_->uncheckButton, &QPushButton::clicked, this,
          &KeyList::uncheck_all);
  connect(ui_->checkALLButton, &QPushButton::clicked, this,
          &KeyList::check_all);
  connect(ui_->syncButton, &QPushButton::clicked, this,
          &KeyList::slot_sync_with_key_server);
  connect(this, &KeyList::SignalRefreshStatusBar, SignalStation::GetInstance(),
          &SignalStation::SignalRefreshStatusBar);

  setAcceptDrops(true);

  ui_->refreshKeyListButton->setText(_("Refresh"));
  ui_->refreshKeyListButton->setToolTip(
      _("Refresh the key list to synchronize changes."));
  ui_->syncButton->setText(_("Sync Public Key"));
  ui_->syncButton->setToolTip(
      _("Sync public key with your default keyserver."));
  ui_->uncheckButton->setText(_("Uncheck ALL"));
  ui_->uncheckButton->setToolTip(
      _("Cancel all checked items in the current tab at once."));
  ui_->checkALLButton->setText(_("Check ALL"));
  ui_->checkALLButton->setToolTip(
      _("Check all items in the current tab at once"));
}

void KeyList::AddListGroupTab(
    const QString& name, KeyListRow::KeyType selectType,
    KeyListColumn::InfoType infoType,
    const std::function<bool(const GpgKey&)>& filter) {
  SPDLOG_DEBUG("add tab: {}", name.toStdString());

  auto key_list = new QTableWidget(this);
  if (m_key_list_ == nullptr) {
    m_key_list_ = key_list;
  }
  ui_->keyGroupTab->addTab(key_list, name);
  m_key_tables_.emplace_back(key_list, selectType, infoType, filter);

  key_list->setColumnCount(7);
  key_list->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  key_list->verticalHeader()->hide();
  key_list->setShowGrid(false);
  key_list->sortByColumn(2, Qt::AscendingOrder);
  key_list->setSelectionBehavior(QAbstractItemView::SelectRows);
  key_list->setSelectionMode(QAbstractItemView::SingleSelection);

  // table items not editable
  key_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
  // no focus (rectangle around table items)
  // maybe it should focus on whole row
  key_list->setFocusPolicy(Qt::NoFocus);

  key_list->setAlternatingRowColors(true);

  // Hidden Column For Purpose
  if (!(infoType & KeyListColumn::TYPE)) {
    key_list->setColumnHidden(1, true);
  }
  if (!(infoType & KeyListColumn::NAME)) {
    key_list->setColumnHidden(2, true);
  }
  if (!(infoType & KeyListColumn::EmailAddress)) {
    key_list->setColumnHidden(3, true);
  }
  if (!(infoType & KeyListColumn::Usage)) {
    key_list->setColumnHidden(4, true);
  }
  if (!(infoType & KeyListColumn::Validity)) {
    key_list->setColumnHidden(5, true);
  }
  if (!(infoType & KeyListColumn::FingerPrint)) {
    key_list->setColumnHidden(6, true);
  }

  QStringList labels;
  labels << _("Select") << _("Type") << _("Name") << _("Email Address")
         << _("Usage") << _("Trust") << _("Finger Print");

  key_list->setHorizontalHeaderLabels(labels);
  key_list->horizontalHeader()->setStretchLastSection(false);

  connect(key_list, &QTableWidget::doubleClicked, this,
          &KeyList::slot_double_clicked);
}

void KeyList::SlotRefresh() {
  SPDLOG_DEBUG("refresh, address: {}", static_cast<void*>(this));

  ui_->refreshKeyListButton->setDisabled(true);
  ui_->syncButton->setDisabled(true);

  emit SignalRefreshStatusBar(_("Refreshing Key List..."), 3000);
  this->buffered_keys_list_ = GpgKeyGetter::GetInstance().FetchKey();
  this->slot_refresh_ui();
}

KeyIdArgsListPtr KeyList::GetChecked(const KeyTable& key_table) {
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_table.key_list_->rowCount(); i++) {
    if (key_table.key_list_->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(key_table.buffered_keys_[i].GetId());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::GetChecked() {
  auto key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::GetAllPrivateKeys() {
  auto key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 1) && buffered_keys[i].IsPrivateKey()) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::GetPrivateChecked() {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (ui_->keyGroupTab->size().isEmpty()) return ret;

  auto key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if ((key_list->item(i, 0)->checkState() == Qt::Checked) &&
        (key_list->item(i, 1))) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

void KeyList::SetChecked(const KeyIdArgsListPtr& keyIds,
                         const KeyTable& key_table) {
  if (!keyIds->empty()) {
    for (int i = 0; i < key_table.key_list_->rowCount(); i++) {
      if (std::find(keyIds->begin(), keyIds->end(),
                    key_table.buffered_keys_[i].GetId()) != keyIds->end()) {
        key_table.key_list_->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }
}

void KeyList::SetChecked(KeyIdArgsListPtr key_ids) {
  auto key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!m_key_tables_.empty()) {
    for (auto& key_table : m_key_tables_) {
      if (key_table.key_list_ == key_list) {
        key_table.SetChecked(std::move(key_ids));
        break;
      }
    }
  }
}

KeyIdArgsListPtr KeyList::GetSelected() {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (ui_->keyGroupTab->size().isEmpty()) return ret;

  auto key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->isSelected() == 1) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

[[maybe_unused]] bool KeyList::ContainsPrivateKeys() {
  if (ui_->keyGroupTab->size().isEmpty()) return false;
  m_key_list_ = qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());

  for (int i = 0; i < m_key_list_->rowCount(); i++) {
    if (m_key_list_->item(i, 1)) {
      return true;
    }
  }
  return false;
}

void KeyList::SetColumnWidth(int row, int size) {
  if (ui_->keyGroupTab->size().isEmpty()) return;
  m_key_list_ = qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());

  m_key_list_->setColumnWidth(row, size);
}

void KeyList::contextMenuEvent(QContextMenuEvent* event) {
  if (ui_->keyGroupTab->size().isEmpty()) return;
  m_key_list_ = qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());

  if (m_key_list_->selectedItems().length() > 0) {
    popup_menu_->exec(event->globalPos());
  }
}

void KeyList::AddSeparator() { popup_menu_->addSeparator(); }

void KeyList::AddMenuAction(QAction* act) { popup_menu_->addAction(act); }

void KeyList::dropEvent(QDropEvent* event) {
  auto* dialog = new QDialog();

  dialog->setWindowTitle(_("Import Keys"));
  QLabel* label;
  label =
      new QLabel(QString(_("You've dropped something on the table.")) + "\n " +
                 _("GpgFrontend "
                   "will now try to import key(s).") +
                 "\n");

  // "always import keys"-CheckBox
  auto* checkBox = new QCheckBox(_("Always import without bothering."));

  bool confirm_import_keys = GlobalSettingStation::GetInstance().LookupSettings(
      "general.confirm_import_keys", true);
  if (confirm_import_keys) checkBox->setCheckState(Qt::Checked);

  // Buttons for ok and cancel
  auto* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

  auto* vbox = new QVBoxLayout();
  vbox->addWidget(label);
  vbox->addWidget(checkBox);
  vbox->addWidget(buttonBox);

  dialog->setLayout(vbox);

  if (confirm_import_keys) {
    dialog->exec();
    if (dialog->result() == QDialog::Rejected) return;

    auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

    if (!settings.exists("general") ||
        settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
      settings.add("general", libconfig::Setting::TypeGroup);
    auto& general = settings["general"];
    if (!general.exists("confirm_import_keys"))
      general.add("confirm_import_keys", libconfig::Setting::TypeBoolean) =
          checkBox->isChecked();
    else {
      general["confirm_import_keys"] = checkBox->isChecked();
    }
    GlobalSettingStation::GetInstance().SyncSettings();
  }

  if (event->mimeData()->hasUrls()) {
    for (const QUrl& tmp : event->mimeData()->urls()) {
      QFile file;
      file.setFileName(tmp.toLocalFile());
      if (!file.open(QIODevice::ReadOnly)) {
        SPDLOG_ERROR("couldn't open file: {}", tmp.toString().toStdString());
      }
      QByteArray inBuffer = file.readAll();
      this->import_keys(inBuffer);
      file.close();
    }
  } else {
    QByteArray inBuffer(event->mimeData()->text().toUtf8());
    this->import_keys(inBuffer);
  }
}

void KeyList::dragEnterEvent(QDragEnterEvent* event) {
  event->acceptProposedAction();
}

/** set background color for Keys and put them to top
 *
 */
[[maybe_unused]] void KeyList::MarkKeys(QStringList* keyIds) {
  foreach (QString id, *keyIds) {
    spdlog::debug("marked: ", id.toStdString());
  }
}

void KeyList::import_keys(const QByteArray& inBuffer) {
  auto std_buffer = std::make_unique<ByteArray>(inBuffer.toStdString());
  GpgImportInformation result =
      GpgKeyImportExporter::GetInstance().ImportKey(std::move(std_buffer));
  new KeyImportDetailDialog(result, false, this);
}

void KeyList::slot_double_clicked(const QModelIndex& index) {
  if (ui_->keyGroupTab->size().isEmpty()) return;
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;
  if (m_action_ != nullptr) {
    const auto key =
        GpgKeyGetter::GetInstance().GetKey(buffered_keys[index.row()].GetId());
    m_action_(key, this);
  }
}

void KeyList::SetDoubleClickedAction(
    std::function<void(const GpgKey&, QWidget*)> action) {
  this->m_action_ = std::move(action);
}

std::string KeyList::GetSelectedKey() {
  if (ui_->keyGroupTab->size().isEmpty()) return {};
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;

  for (int i = 0; i < m_key_list_->rowCount(); i++) {
    if (m_key_list_->item(i, 0)->isSelected() == 1) {
      return buffered_keys[i].GetId();
    }
  }
  return {};
}

void KeyList::slot_refresh_ui() {
  SPDLOG_DEBUG("refresh: {}", static_cast<void*>(buffered_keys_list_.get()));
  if (buffered_keys_list_ != nullptr) {
    std::lock_guard<std::mutex> guard(buffered_key_list_mutex_);
    for (auto& key_table : m_key_tables_) {
      key_table.Refresh(
          GpgKeyGetter::GetInstance().GetKeysCopy(buffered_keys_list_));
    }
  }
  emit SignalRefreshStatusBar(_("Key List Refreshed."), 1000);
  ui_->refreshKeyListButton->setDisabled(false);
  ui_->syncButton->setDisabled(false);
}

void KeyList::slot_sync_with_key_server() {
  KeyIdArgsList key_ids;
  {
    std::lock_guard<std::mutex> guard(buffered_key_list_mutex_);
    for (const auto& key : *buffered_keys_list_) {
      if (!(key.IsPrivateKey() && key.IsHasMasterKey()))
        key_ids.push_back(key.GetId());
    }
  }

  if (key_ids.empty()) return;

  ui_->refreshKeyListButton->setDisabled(true);
  ui_->syncButton->setDisabled(true);

  emit SignalRefreshStatusBar(_("Syncing Key List..."), 3000);
  CommonUtils::SlotImportKeyFromKeyServer(
      key_ids, [=](const std::string& key_id, const std::string& status,
                   size_t current_index, size_t all_index) {
        SPDLOG_DEBUG("import key: {} {} {} {}", key_id, status, current_index,
                     all_index);
        auto key = GpgKeyGetter::GetInstance().GetKey(key_id);

        boost::format status_str = boost::format(_("Sync [%1%/%2%] %3% %4%")) %
                                   current_index % all_index %
                                   key.GetUIDs()->front().GetUID() % status;
        emit SignalRefreshStatusBar(status_str.str().c_str(), 1500);

        if (current_index == all_index) {
          ui_->syncButton->setDisabled(false);
          ui_->refreshKeyListButton->setDisabled(false);
          emit SignalRefreshStatusBar(_("Key List Sync Done."), 3000);
          emit this->SignalRefreshDatabase();
        }
      });
}

void KeyList::uncheck_all() {
  auto key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!m_key_tables_.empty()) {
    for (auto& key_table : m_key_tables_) {
      if (key_table.key_list_ == key_list) {
        key_table.UncheckALL();
        break;
      }
    }
  }
}

void KeyList::check_all() {
  auto key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!m_key_tables_.empty()) {
    for (auto& key_table : m_key_tables_) {
      if (key_table.key_list_ == key_list) {
        key_table.CheckALL();
        break;
      }
    }
  }
}

KeyIdArgsListPtr& KeyTable::GetChecked() {
  if (checked_key_ids_ == nullptr)
    checked_key_ids_ = std::make_unique<KeyIdArgsList>();
  auto& ret = checked_key_ids_;
  for (int i = 0; i < buffered_keys_.size(); i++) {
    auto key_id = buffered_keys_[i].GetId();
    SPDLOG_DEBUG("i: {} key_id: {}", i, key_id);
    if (key_list_->item(i, 0)->checkState() == Qt::Checked &&
        std::find(ret->begin(), ret->end(), key_id) == ret->end()) {
      ret->push_back(key_id);
    }
  }
  return ret;
}

void KeyTable::SetChecked(KeyIdArgsListPtr key_ids) {
  checked_key_ids_ = std::move(key_ids);
}

void KeyTable::Refresh(KeyLinkListPtr m_keys) {
  auto& checked_key_list = GetChecked();
  // while filling the table, sort enabled causes errors

  key_list_->setSortingEnabled(false);
  key_list_->clearContents();

  // Optimization for copy
  KeyLinkListPtr keys = nullptr;
  if (m_keys == nullptr)
    keys = GpgKeyGetter::GetInstance().FetchKey();
  else
    keys = std::move(m_keys);

  auto it = keys->begin();
  int row_count = 0;

  while (it != keys->end()) {
    SPDLOG_DEBUG("filtering key id: {}", it->GetId());
    if (filter_ != nullptr) {
      if (!filter_(*it)) {
        it = keys->erase(it);
        continue;
      }
    }
    SPDLOG_DEBUG("adding key id: {}", it->GetId());
    if (select_type_ == KeyListRow::ONLY_SECRET_KEY && !it->IsPrivateKey()) {
      it = keys->erase(it);
      continue;
    }
    row_count++;
    it++;
  }

  key_list_->setRowCount(row_count);

  int row_index = 0;
  it = keys->begin();

  buffered_keys_.clear();

  while (it != keys->end()) {
    auto* tmp0 = new QTableWidgetItem(QString::number(row_index));
    tmp0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    tmp0->setTextAlignment(Qt::AlignCenter);
    tmp0->setCheckState(Qt::Unchecked);
    key_list_->setItem(row_index, 0, tmp0);

    QString type_str;
    QTextStream type_steam(&type_str);
    if (it->IsPrivateKey()) {
      type_steam << "pub/sec";
    } else {
      type_steam << "pub";
    }

    if (it->IsPrivateKey() && !it->IsHasMasterKey()) {
      type_steam << "#";
    }

    if (it->IsHasCardKey()) {
      type_steam << "^";
    }

    auto* tmp1 = new QTableWidgetItem(type_str);
    key_list_->setItem(row_index, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(QString::fromStdString(it->GetName()));
    key_list_->setItem(row_index, 2, tmp2);
    auto* tmp3 = new QTableWidgetItem(QString::fromStdString(it->GetEmail()));
    key_list_->setItem(row_index, 3, tmp3);

    QString usage;
    QTextStream usage_steam(&usage);

    if (it->IsHasActualCertificationCapability()) usage_steam << "C";
    if (it->IsHasActualEncryptionCapability()) usage_steam << "E";
    if (it->IsHasActualSigningCapability()) usage_steam << "S";
    if (it->IsHasActualAuthenticationCapability()) usage_steam << "A";

    auto* temp_usage = new QTableWidgetItem(usage);
    temp_usage->setTextAlignment(Qt::AlignCenter);
    key_list_->setItem(row_index, 4, temp_usage);

    auto* temp_validity =
        new QTableWidgetItem(QString::fromStdString(it->GetOwnerTrust()));
    temp_validity->setTextAlignment(Qt::AlignCenter);
    key_list_->setItem(row_index, 5, temp_validity);

    auto* temp_fpr =
        new QTableWidgetItem(QString::fromStdString(it->GetFingerprint()));
    temp_fpr->setTextAlignment(Qt::AlignCenter);
    key_list_->setItem(row_index, 6, temp_fpr);

    // strike out expired keys
    if (it->IsExpired() || it->IsRevoked()) {
      QFont strike = tmp2->font();
      strike.setStrikeOut(true);
      tmp0->setFont(strike);
      temp_usage->setFont(strike);
      temp_fpr->setFont(strike);
      temp_validity->setFont(strike);
      tmp1->setFont(strike);
      tmp2->setFont(strike);
      tmp3->setFont(strike);
    }

    SPDLOG_DEBUG("key id: {} added into key_list_: {}", it->GetId(),
                 static_cast<void*>(this));

    // move to buffered keys
    buffered_keys_.emplace_back(std::move(*it));

    it++;
    ++row_index;
  }

  if (!checked_key_list->empty()) {
    for (int i = 0; i < key_list_->rowCount(); i++) {
      if (std::find(checked_key_list->begin(), checked_key_list->end(),
                    buffered_keys_[i].GetId()) != checked_key_list->end()) {
        key_list_->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }
}

void KeyTable::UncheckALL() const {
  for (int i = 0; i < key_list_->rowCount(); i++) {
    key_list_->item(i, 0)->setCheckState(Qt::Unchecked);
  }
}

void KeyTable::CheckALL() const {
  for (int i = 0; i < key_list_->rowCount(); i++) {
    key_list_->item(i, 0)->setCheckState(Qt::Checked);
  }
}
}  // namespace GpgFrontend::UI
