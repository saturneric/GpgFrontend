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

#include "ui/widgets/KeyList.h"

#include <utility>

#include "gpg/function/GpgKeyGetter.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {

KeyList::KeyList(QWidget* parent) : QWidget(parent) { init(); }

KeyList::KeyList(KeyListRow::KeyType selectType,
                 KeyListColumn::InfoType infoType,
                 const std::function<bool(const GpgKey&)>& filter,
                 QWidget* parent)
    : QWidget(parent) {
  init();
  addListGroupTab(_("Default"), selectType, infoType, filter);
}

void KeyList::init() {
  mGroupTab = new QTabWidget();
  mGroupTab->setMovable(true);
  mGroupTab->setTabsClosable(false);

  auto* layout = new QVBoxLayout;
  layout->addWidget(mGroupTab);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(3);
  setLayout(layout);

  popupMenu = new QMenu(this);

  // register key database refresh signal
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slotRefresh()));

  setAcceptDrops(true);
}

void KeyList::addListGroupTab(
    const QString& name, KeyListRow::KeyType selectType,
    KeyListColumn::InfoType infoType,
    const std::function<bool(const GpgKey&)>& filter) {
  auto key_list = new QTableWidget(this);
  if (mKeyList == nullptr) {
    mKeyList = key_list;
  }
  mGroupTab->addTab(key_list, name);
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

  // refresh
  mKeyTables.back().Refresh();
}

void KeyList::slotRefresh() {
  LOG(INFO) << _("called");

  for (auto& key_table : mKeyTables) {
    key_table.Refresh();
  }
}

KeyIdArgsListPtr KeyList::getChecked(const KeyTable& key_table) {
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_table.key_list->rowCount(); i++) {
    if (key_table.key_list->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(key_table.buffered_keys[i].id());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::getChecked() {
  auto key_list = qobject_cast<QTableWidget*>(mGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[mGroupTab->currentIndex()].buffered_keys;
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(buffered_keys[i].id());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::getAllPrivateKeys() {
  auto key_list = qobject_cast<QTableWidget*>(mGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[mGroupTab->currentIndex()].buffered_keys;
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 1) && buffered_keys[i].is_private_key()) {
      ret->push_back(buffered_keys[i].id());
    }
  }
  return ret;
}

KeyIdArgsListPtr KeyList::getPrivateChecked() {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (mGroupTab->size().isEmpty()) return ret;

  auto key_list = qobject_cast<QTableWidget*>(mGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[mGroupTab->currentIndex()].buffered_keys;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if ((key_list->item(i, 0)->checkState() == Qt::Checked) &&
        (key_list->item(i, 1))) {
      ret->push_back(buffered_keys[i].id());
    }
  }
  return ret;
}

void KeyList::setChecked(const KeyIdArgsListPtr& keyIds,
                         const KeyTable& key_table) {
  if (!keyIds->empty()) {
    for (int i = 0; i < key_table.key_list->rowCount(); i++) {
      if (std::find(keyIds->begin(), keyIds->end(),
                    key_table.buffered_keys[i].id()) != keyIds->end()) {
        key_table.key_list->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }
}

void KeyList::setChecked(const KeyIdArgsListPtr& keyIds) {
  if (mGroupTab->size().isEmpty()) return;
  auto key_list = qobject_cast<QTableWidget*>(mGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[mGroupTab->currentIndex()].buffered_keys;

  if (!keyIds->empty()) {
    for (int i = 0; i < key_list->rowCount(); i++) {
      if (std::find(keyIds->begin(), keyIds->end(), buffered_keys[i].id()) !=
          keyIds->end()) {
        key_list->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }
}

KeyIdArgsListPtr KeyList::getSelected() {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (mGroupTab->size().isEmpty()) return ret;

  auto key_list = qobject_cast<QTableWidget*>(mGroupTab->currentWidget());
  const auto& buffered_keys =
      mKeyTables[mGroupTab->currentIndex()].buffered_keys;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->isSelected() == 1) {
      ret->push_back(buffered_keys[i].id());
    }
  }
  return ret;
}

[[maybe_unused]] bool KeyList::containsPrivateKeys() {
  if (mGroupTab->size().isEmpty()) return false;
  mKeyList = qobject_cast<QTableWidget*>(mGroupTab->currentWidget());

  for (int i = 0; i < mKeyList->rowCount(); i++) {
    if (mKeyList->item(i, 1)) {
      return true;
    }
  }
  return false;
}

void KeyList::setColumnWidth(int row, int size) {
  if (mGroupTab->size().isEmpty()) return;
  mKeyList = qobject_cast<QTableWidget*>(mGroupTab->currentWidget());

  mKeyList->setColumnWidth(row, size);
}

void KeyList::contextMenuEvent(QContextMenuEvent* event) {
  if (mGroupTab->size().isEmpty()) return;
  mKeyList = qobject_cast<QTableWidget*>(mGroupTab->currentWidget());

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
  if (settings.value("general/confirmImportKeys").toBool())
    checkBox->setCheckState(Qt::Unchecked);

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

  if (settings.value("general/confirmImportKeys", Qt::Checked).toBool()) {
    dialog->exec();
    if (dialog->result() == QDialog::Rejected) {
      return;
    }
    if (checkBox->isChecked()) {
      settings.setValue("general/confirmImportKeys", false);
    } else {
      settings.setValue("general/confirmImportKeys", true);
    }
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
      GpgKeyImportExportor::GetInstance().ImportKey(std::move(std_buffer));
  new KeyImportDetailDialog(result, false, this);
}

void KeyList::slotDoubleClicked(const QModelIndex& index) {
  if (mGroupTab->size().isEmpty()) return;
  const auto& buffered_keys =
      mKeyTables[mGroupTab->currentIndex()].buffered_keys;
  if (mAction != nullptr) {
    const auto key =
        GpgKeyGetter::GetInstance().GetKey(buffered_keys[index.row()].id());
    mAction(key, this);
  }
}

void KeyList::setDoubleClickedAction(
    std::function<void(const GpgKey&, QWidget*)> action) {
  this->mAction = std::move(action);
}

std::string KeyList::getSelectedKey() {
  if (mGroupTab->size().isEmpty()) return {};
  const auto& buffered_keys =
      mKeyTables[mGroupTab->currentIndex()].buffered_keys;

  for (int i = 0; i < mKeyList->rowCount(); i++) {
    if (mKeyList->item(i, 0)->isSelected() == 1) {
      return buffered_keys[i].id();
    }
  }
  return {};
}

KeyIdArgsListPtr KeyTable::GetChecked() {
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(buffered_keys[i].id());
    }
  }
  return ret;
}

void KeyTable::SetChecked(const KeyIdArgsListPtr& key_ids) {
  if (!key_ids->empty()) {
    for (int i = 0; i < key_list->rowCount(); i++) {
      if (std::find(key_ids->begin(), key_ids->end(), buffered_keys[i].id()) !=
          key_ids->end()) {
        key_list->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }
}

void KeyTable::Refresh() {
  auto checked_key_list = GetChecked();
  // while filling the table, sort enabled causes errors

  key_list->setSortingEnabled(false);
  key_list->clearContents();

  auto keys = GpgKeyGetter::GetInstance().FetchKey();
  auto it = keys->begin();
  int row_count = 0;

  while (it != keys->end()) {
    if (filter != nullptr) {
      if (!filter(*it)) {
        it = keys->erase(it);
        continue;
      }
    }
    if (select_type == KeyListRow::ONLY_SECRET_KEY && !it->is_private_key()) {
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
    table_buffered_keys.push_back(GpgKeyGetter::GetInstance().GetKey(it->id()));

    auto* tmp0 = new QTableWidgetItem(QString::number(row_index));
    tmp0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    tmp0->setTextAlignment(Qt::AlignCenter);
    tmp0->setCheckState(Qt::Unchecked);
    key_list->setItem(row_index, 0, tmp0);

    QString type_str;
    QTextStream type_steam(&type_str);
    if (it->is_private_key()) {
      type_steam << "pub/sec";
    } else {
      type_steam << "pub";
    }

    if (it->is_private_key() && !it->has_master_key()) {
      type_steam << "#";
    }
    auto* tmp1 = new QTableWidgetItem(type_str);
    key_list->setItem(row_index, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(QString::fromStdString(it->name()));
    key_list->setItem(row_index, 2, tmp2);
    auto* tmp3 = new QTableWidgetItem(QString::fromStdString(it->email()));
    key_list->setItem(row_index, 3, tmp3);

    QString usage;
    QTextStream usage_steam(&usage);

    if (it->CanCertActual()) usage_steam << "C";
    if (it->CanEncrActual()) usage_steam << "E";
    if (it->CanSignActual()) usage_steam << "S";
    if (it->CanAuthActual()) usage_steam << "A";

    auto* temp_usage = new QTableWidgetItem(usage);
    temp_usage->setTextAlignment(Qt::AlignCenter);
    key_list->setItem(row_index, 4, temp_usage);

    auto* temp_validity =
        new QTableWidgetItem(QString::fromStdString(it->owner_trust()));
    temp_validity->setTextAlignment(Qt::AlignCenter);
    key_list->setItem(row_index, 5, temp_validity);

    auto* temp_fpr = new QTableWidgetItem(QString::fromStdString(it->fpr()));
    temp_fpr->setTextAlignment(Qt::AlignCenter);
    key_list->setItem(row_index, 6, temp_fpr);

    // strike out expired keys
    if (it->expired() || it->revoked()) {
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

  SetChecked(checked_key_list);
}
}  // namespace GpgFrontend::UI
