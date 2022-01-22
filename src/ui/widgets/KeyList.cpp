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
#include <utility>

#include "gpg/GpgCoreInit.h"
#include "gpg/function/GpgKeyGetter.h"
#include "ui/SignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/settings/GlobalSettingStation.h"
#include "ui_KeyList.h"

namespace GpgFrontend::UI {

int KeyList::key_list_id = 2048;

KeyList::KeyList(KeyMenuAbility::AbilityType menu_ability, QWidget* parent)
    : QWidget(parent),
      _m_key_list_id(key_list_id++),
      ui(std::make_shared<Ui_KeyList>()),
      menu_ability_(menu_ability) {
  init();
}

void KeyList::init() {
#ifdef GPG_STANDALONE_MODE
  LOG(INFO) << "GPG_STANDALONE_MODE Enabled";
  auto gpg_path = GpgFrontend::UI::GlobalSettingStation::GetInstance()
                      .GetStandaloneGpgBinDir();
  auto db_path = GpgFrontend::UI::GlobalSettingStation::GetInstance()
                     .GetStandaloneDatabaseDir();
  GpgContext::CreateInstance(
      _m_key_list_id, std::make_unique<GpgContext>(true, db_path.string(), true,
                                                   gpg_path.string()));
#else
  new_default_settings_channel(_m_key_list_id);
#endif

  ui->setupUi(this);

  ui->menuWidget->setHidden(!menu_ability_);
  ui->refreshKeyListButton->setHidden(~menu_ability_ & KeyMenuAbility::REFRESH);
  ui->syncButton->setHidden(~menu_ability_ & KeyMenuAbility::SYNC_PUBLIC_KEY);
  ui->uncheckButton->setHidden(~menu_ability_ & KeyMenuAbility::UNCHECK_ALL);

  ui->keyGroupTab->clear();
  popupMenu = new QMenu(this);

  // register key database refresh signal
  connect(this, &KeyList::signalRefreshDatabase, SignalStation::GetInstance(),
          &SignalStation::KeyDatabaseRefresh);
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slotRefresh()));
  connect(ui->refreshKeyListButton, &QPushButton::clicked, this,
          &KeyList::slotRefresh);
  connect(ui->uncheckButton, &QPushButton::clicked, this,
          &KeyList::slotUncheckALL);
  connect(ui->checkALLButton, &QPushButton::clicked, this,
          &KeyList::slotCheckALL);
  connect(ui->syncButton, &QPushButton::clicked, this,
          &KeyList::slotSyncWithKeyServer);
  connect(this, &KeyList::signalRefreshStatusBar, SignalStation::GetInstance(),
          &SignalStation::signalRefreshStatusBar);

  setAcceptDrops(true);

  ui->refreshKeyListButton->setText(_("Refresh"));
  ui->refreshKeyListButton->setToolTip(
      _("Refresh the key list to synchronize changes."));
  ui->syncButton->setText(_("Sync Public Key"));
  ui->syncButton->setToolTip(_("Sync public key with your default keyserver."));
  ui->uncheckButton->setText(_("Uncheck ALL"));
  ui->uncheckButton->setToolTip(
      _("Cancel all checked items in the current tab at once."));
  ui->checkALLButton->setText(_("Check ALL"));
  ui->checkALLButton->setToolTip(
      _("Check all items in the current tab at once"));
}

void KeyList::addListGroupTab(
    const QString& name, KeyListRow::KeyType selectType,
    KeyListColumn::InfoType infoType,
    const std::function<bool(const GpgKey&)>& filter) {
  LOG(INFO) << _("Called") << name.toStdString();

  auto key_list = new QTableWidget(this);
  if (mKeyList == nullptr) {
    mKeyList = key_list;
  }
  ui->keyGroupTab->addTab(key_list, name);
  mKeyTables.emplace_back(key_list, selectType, infoType, filter);

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
         << _("Usage") << _("Validity") << _("Finger Print");

  key_list->setHorizontalHeaderLabels(labels);
  key_list->horizontalHeader()->setStretchLastSection(false);

  connect(key_list, &QTableWidget::doubleClicked, this,
          &KeyList::slotDoubleClicked);
}

void KeyList::slotRefresh() {
  LOG(INFO) << _("Called") << "_m_key_list_id" << _m_key_list_id;
  emit signalRefreshStatusBar(_("Refreshing Key List..."), 3000);
  auto thread = QThread::create([this, _id = _m_key_list_id]() {
    std::lock_guard<std::mutex> guard(buffered_key_list_mutex);
    _buffered_keys_list = nullptr;
    // buffered keys list
    _buffered_keys_list = GpgKeyGetter::GetInstance(_id).FetchKey();
  });
  connect(thread, &QThread::finished, this, &KeyList::slotRefreshUI);
  connect(thread, &QThread::finished, thread, &QThread::deleteLater);
  ui->refreshKeyListButton->setDisabled(true);
  ui->syncButton->setDisabled(true);
  thread->start();
}

KeyIdArgsListPtr KeyList::getChecked(const KeyTable& key_table) {
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_table.key_list->rowCount(); i++) {
    if (key_table.key_list->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(key_table.buffered_keys[i].GetId());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::getChecked() {
  auto key_list = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[ui->keyGroupTab->currentIndex()].buffered_keys;
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::getAllPrivateKeys() {
  auto key_list = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[ui->keyGroupTab->currentIndex()].buffered_keys;
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 1) && buffered_keys[i].IsPrivateKey()) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::getPrivateChecked() {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (ui->keyGroupTab->size().isEmpty()) return ret;

  auto key_list = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[ui->keyGroupTab->currentIndex()].buffered_keys;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if ((key_list->item(i, 0)->checkState() == Qt::Checked) &&
        (key_list->item(i, 1))) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

void KeyList::setChecked(const KeyIdArgsListPtr& keyIds,
                         const KeyTable& key_table) {
  if (!keyIds->empty()) {
    for (int i = 0; i < key_table.key_list->rowCount(); i++) {
      if (std::find(keyIds->begin(), keyIds->end(),
                    key_table.buffered_keys[i].GetId()) != keyIds->end()) {
        key_table.key_list->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }
}

void KeyList::setChecked(KeyIdArgsListPtr key_ids) {
  auto key_list = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!mKeyTables.empty()) {
    for (auto& key_table : mKeyTables) {
      if (key_table.key_list == key_list) {
        key_table.SetChecked(std::move(key_ids));
        break;
      }
    }
  }
}

KeyIdArgsListPtr KeyList::getSelected() {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (ui->keyGroupTab->size().isEmpty()) return ret;

  auto key_list = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[ui->keyGroupTab->currentIndex()].buffered_keys;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->isSelected() == 1) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

[[maybe_unused]] bool KeyList::containsPrivateKeys() {
  if (ui->keyGroupTab->size().isEmpty()) return false;
  mKeyList = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());

  for (int i = 0; i < mKeyList->rowCount(); i++) {
    if (mKeyList->item(i, 1)) {
      return true;
    }
  }
  return false;
}

void KeyList::setColumnWidth(int row, int size) {
  if (ui->keyGroupTab->size().isEmpty()) return;
  mKeyList = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());

  mKeyList->setColumnWidth(row, size);
}

void KeyList::contextMenuEvent(QContextMenuEvent* event) {
  if (ui->keyGroupTab->size().isEmpty()) return;
  mKeyList = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());

  if (mKeyList->selectedItems().length() > 0) {
    popupMenu->exec(event->globalPos());
  }
}

void KeyList::addSeparator() { popupMenu->addSeparator(); }

void KeyList::addMenuAction(QAction* act) { popupMenu->addAction(act); }

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

  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  bool confirm_import_keys = true;
  try {
    confirm_import_keys = settings.lookup("general.confirm_import_keys");
    LOG(INFO) << "confirm_import_keys" << confirm_import_keys;
    if (confirm_import_keys) checkBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("confirm_import_keys");
  }

  // Buttons for ok and cancel
  auto* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

  auto* vbox = new QVBoxLayout();
  vbox->addWidget(label);
  vbox->addWidget(checkBox);
  vbox->addWidget(buttonBox);

  dialog->setLayout(vbox);

  if (confirm_import_keys) {
    dialog->exec();
    if (dialog->result() == QDialog::Rejected) return;

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
        LOG(INFO) << _("Couldn't Open File") << ":"
                  << tmp.toString().toStdString();
      }
      QByteArray inBuffer = file.readAll();
      this->importKeys(inBuffer);
      file.close();
    }
  } else {
    QByteArray inBuffer(event->mimeData()->text().toUtf8());
    this->importKeys(inBuffer);
  }
}

void KeyList::dragEnterEvent(QDragEnterEvent* event) {
  event->acceptProposedAction();
}

/** set background color for Keys and put them to top
 *
 */
[[maybe_unused]] void KeyList::markKeys(QStringList* keyIds) {
  foreach (QString id, *keyIds) { qDebug() << "marked: " << id; }
}

void KeyList::importKeys(const QByteArray& inBuffer) {
  auto std_buffer = std::make_unique<ByteArray>(inBuffer.toStdString());
  GpgImportInformation result =
      GpgKeyImportExporter::GetInstance(_m_key_list_id)
          .ImportKey(std::move(std_buffer));
  new KeyImportDetailDialog(result, false, this);
}

void KeyList::slotDoubleClicked(const QModelIndex& index) {
  if (ui->keyGroupTab->size().isEmpty()) return;
  const auto& buffered_keys =
      mKeyTables[ui->keyGroupTab->currentIndex()].buffered_keys;
  if (mAction != nullptr) {
    const auto key = GpgKeyGetter::GetInstance(_m_key_list_id)
                         .GetKey(buffered_keys[index.row()].GetId());
    mAction(key, this);
  }
}

void KeyList::setDoubleClickedAction(
    std::function<void(const GpgKey&, QWidget*)> action) {
  this->mAction = std::move(action);
}

std::string KeyList::getSelectedKey() {
  if (ui->keyGroupTab->size().isEmpty()) return {};
  const auto& buffered_keys =
      mKeyTables[ui->keyGroupTab->currentIndex()].buffered_keys;

  for (int i = 0; i < mKeyList->rowCount(); i++) {
    if (mKeyList->item(i, 0)->isSelected() == 1) {
      return buffered_keys[i].GetId();
    }
  }
  return {};
}

void KeyList::slotRefreshUI() {
  LOG(INFO) << _("Called") << _buffered_keys_list.get();
  if (_buffered_keys_list != nullptr) {
    std::lock_guard<std::mutex> guard(buffered_key_list_mutex);
    for (auto& key_table : mKeyTables) {
      key_table.Refresh(GpgKeyGetter::GetKeysCopy(_buffered_keys_list));
    }
  }
  emit signalRefreshStatusBar(_("Key List Refreshed."), 1000);
  ui->refreshKeyListButton->setDisabled(false);
  ui->syncButton->setDisabled(false);
}

void KeyList::slotSyncWithKeyServer() {
  KeyIdArgsList key_ids;
  {
    std::lock_guard<std::mutex> guard(buffered_key_list_mutex);
    for (const auto& key : *_buffered_keys_list) {
      if (!(key.IsPrivateKey() && key.IsHasMasterKey()))
        key_ids.push_back(key.GetId());
    }
  }

  ui->refreshKeyListButton->setDisabled(true);
  ui->syncButton->setDisabled(true);

  emit signalRefreshStatusBar(_("Syncing Key List..."), 3000);
  CommonUtils::slotImportKeyFromKeyServer(
      _m_key_list_id, key_ids,
      [=](const std::string& key_id, const std::string& status,
          size_t current_index, size_t all_index) {
        LOG(INFO) << _("Called") << key_id << status << current_index
                  << all_index;
        auto key = GpgKeyGetter::GetInstance(_m_key_list_id).GetKey(key_id);

        boost::format status_str = boost::format(_("Sync [%1%/%2%] %3% %4%")) %
                                   current_index % all_index %
                                   key.GetUIDs()->front().GetUID() % status;
        emit signalRefreshStatusBar(status_str.str().c_str(), 1500);

        if (current_index == all_index) {
          ui->syncButton->setDisabled(false);
          ui->refreshKeyListButton->setDisabled(false);
          emit signalRefreshStatusBar(_("Key List Sync Done."), 3000);
          emit signalRefreshDatabase();
        }
      });
}

void KeyList::slotUncheckALL() {
  auto key_list = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!mKeyTables.empty()) {
    for (auto& key_table : mKeyTables) {
      if (key_table.key_list == key_list) {
        key_table.UncheckALL();
        break;
      }
    }
  }
}

void KeyList::slotCheckALL() {
  auto key_list = qobject_cast<QTableWidget*>(ui->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!mKeyTables.empty()) {
    for (auto& key_table : mKeyTables) {
      if (key_table.key_list == key_list) {
        key_table.CheckALL();
        break;
      }
    }
  }
}

KeyIdArgsListPtr& KeyTable::GetChecked() {
  LOG(INFO) << "called";
  if (checked_key_ids_ == nullptr)
    checked_key_ids_ = std::make_unique<KeyIdArgsList>();
  auto& ret = checked_key_ids_;
  for (int i = 0; i < key_list->rowCount(); i++) {
    auto key_id = buffered_keys[i].GetId();
    if (key_list->item(i, 0)->checkState() == Qt::Checked &&
        std::find(ret->begin(), ret->end(), key_id) == ret->end()) {
      ret->push_back(key_id);
    }
  }
  return ret;
}

void KeyTable::SetChecked(KeyIdArgsListPtr key_ids) {
  LOG(INFO) << "called";
  checked_key_ids_ = std::move(key_ids);
}

void KeyTable::Refresh(KeyLinkListPtr m_keys) {
  LOG(INFO) << "Called";

  auto& checked_key_list = GetChecked();
  // while filling the table, sort enabled causes errors

  key_list->setSortingEnabled(false);
  key_list->clearContents();

  // Optimization for copy
  KeyLinkListPtr keys = nullptr;
  if (m_keys == nullptr)
    keys = GpgKeyGetter::GetInstance().FetchKey();
  else
    keys = std::move(m_keys);

  auto it = keys->begin();
  int row_count = 0;

  while (it != keys->end()) {
    if (filter != nullptr) {
      if (!filter(*it)) {
        it = keys->erase(it);
        continue;
      }
    }
    if (select_type == KeyListRow::ONLY_SECRET_KEY && !it->IsPrivateKey()) {
      it = keys->erase(it);
      continue;
    }
    row_count++;
    it++;
  }

  key_list->setRowCount(row_count);

  int row_index = 0;
  it = keys->begin();

  auto& table_buffered_keys = buffered_keys;

  table_buffered_keys.clear();

  while (it != keys->end()) {
    table_buffered_keys.push_back(it->Copy());

    auto* tmp0 = new QTableWidgetItem(QString::number(row_index));
    tmp0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    tmp0->setTextAlignment(Qt::AlignCenter);
    tmp0->setCheckState(Qt::Unchecked);
    key_list->setItem(row_index, 0, tmp0);

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
    key_list->setItem(row_index, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(QString::fromStdString(it->GetName()));
    key_list->setItem(row_index, 2, tmp2);
    auto* tmp3 = new QTableWidgetItem(QString::fromStdString(it->GetEmail()));
    key_list->setItem(row_index, 3, tmp3);

    QString usage;
    QTextStream usage_steam(&usage);

    if (it->IsHasActualCertificationCapability()) usage_steam << "C";
    if (it->IsHasActualEncryptionCapability()) usage_steam << "E";
    if (it->IsHasActualSigningCapability()) usage_steam << "S";
    if (it->IsHasActualAuthenticationCapability()) usage_steam << "A";

    auto* temp_usage = new QTableWidgetItem(usage);
    temp_usage->setTextAlignment(Qt::AlignCenter);
    key_list->setItem(row_index, 4, temp_usage);

    auto* temp_validity =
        new QTableWidgetItem(QString::fromStdString(it->GetOwnerTrust()));
    temp_validity->setTextAlignment(Qt::AlignCenter);
    key_list->setItem(row_index, 5, temp_validity);

    auto* temp_fpr =
        new QTableWidgetItem(QString::fromStdString(it->GetFingerprint()));
    temp_fpr->setTextAlignment(Qt::AlignCenter);
    key_list->setItem(row_index, 6, temp_fpr);

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
    it++;
    ++row_index;
  }

  if (!checked_key_list->empty()) {
    for (int i = 0; i < key_list->rowCount(); i++) {
      if (std::find(checked_key_list->begin(), checked_key_list->end(),
                    buffered_keys[i].GetId()) != checked_key_list->end()) {
        key_list->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }

  LOG(INFO) << "End";
}

void KeyTable::UncheckALL() const {
  for (int i = 0; i < key_list->rowCount(); i++) {
    key_list->item(i, 0)->setCheckState(Qt::Unchecked);
  }
}

void KeyTable::CheckALL() const {
  for (int i = 0; i < key_list->rowCount(); i++) {
    key_list->item(i, 0)->setCheckState(Qt::Checked);
  }
}
}  // namespace GpgFrontend::UI
