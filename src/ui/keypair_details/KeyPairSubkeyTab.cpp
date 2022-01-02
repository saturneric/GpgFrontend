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

#include "ui/keypair_details/KeyPairSubkeyTab.h"

#include "gpg/function/GpgKeyGetter.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {

KeyPairSubkeyTab::KeyPairSubkeyTab(const std::string& key_id, QWidget* parent)
    : QWidget(parent), mKey(GpgKeyGetter::GetInstance().GetKey(key_id)) {
  createSubkeyList();
  createSubkeyOperaMenu();

  listBox = new QGroupBox(_("Subkey List"));
  detailBox = new QGroupBox(_("Detail of Selected Subkey"));

  auto uidButtonsLayout = new QGridLayout();

  auto addSubkeyButton = new QPushButton(_("Generate A New Subkey"));
  if (!mKey.is_private_key() || !mKey.has_master_key()) {
    addSubkeyButton->setDisabled(true);
    setHidden(addSubkeyButton);
  }

  uidButtonsLayout->addWidget(addSubkeyButton, 0, 1);

  auto* baseLayout = new QVBoxLayout();

  auto subkeyListLayout = new QGridLayout();
  subkeyListLayout->addWidget(subkeyList, 0, 0);
  subkeyListLayout->addLayout(uidButtonsLayout, 1, 0);
  subkeyListLayout->setContentsMargins(0, 10, 0, 0);

  auto* subkeyDetailLayout = new QGridLayout();

  subkeyDetailLayout->addWidget(new QLabel(QString(_("Key ID")) + ": "), 0, 0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Algorithm")) + ": "), 1,
                                0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Key Size")) + ": "), 2,
                                0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Usage")) + ": "), 3, 0);
  subkeyDetailLayout->addWidget(
      new QLabel(QString(_("Expires On (Local Time)")) + ": "), 4, 0);
  subkeyDetailLayout->addWidget(
      new QLabel(QString(_("Create Date (Local Time)")) + ": "), 5, 0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Existence")) + ": "), 6,
                                0);
  subkeyDetailLayout->addWidget(new QLabel(QString(_("Fingerprint")) + ": "), 7,
                                0);

  keyidVarLabel = new QLabel();
  keySizeVarLabel = new QLabel();
  expireVarLabel = new QLabel();
  algorithmVarLabel = new QLabel();
  createdVarLabel = new QLabel();
  usageVarLabel = new QLabel();
  masterKeyExistVarLabel = new QLabel();
  fingerPrintVarLabel = new QLabel();

  subkeyDetailLayout->addWidget(keyidVarLabel, 0, 1, 1, 1);
  subkeyDetailLayout->addWidget(keySizeVarLabel, 2, 1, 1, 2);
  subkeyDetailLayout->addWidget(expireVarLabel, 4, 1, 1, 2);
  subkeyDetailLayout->addWidget(algorithmVarLabel, 1, 1, 1, 2);
  subkeyDetailLayout->addWidget(createdVarLabel, 5, 1, 1, 2);
  subkeyDetailLayout->addWidget(usageVarLabel, 3, 1, 1, 2);
  subkeyDetailLayout->addWidget(masterKeyExistVarLabel, 6, 1, 1, 2);
  subkeyDetailLayout->addWidget(fingerPrintVarLabel, 7, 1, 1, 2);

  auto* copyKeyIdButton = new QPushButton(_("Copy"));
  copyKeyIdButton->setFlat(true);
  subkeyDetailLayout->addWidget(copyKeyIdButton, 0, 2);
  connect(copyKeyIdButton, &QPushButton::clicked, this, [=]() {
    QString fpr = keyidVarLabel->text().trimmed();
    QClipboard* cb = QApplication::clipboard();
    cb->setText(fpr);
  });

  listBox->setLayout(subkeyListLayout);
  listBox->setContentsMargins(0, 12, 0, 0);
  detailBox->setLayout(subkeyDetailLayout);

  baseLayout->addWidget(listBox);
  baseLayout->addWidget(detailBox);
  baseLayout->addStretch();

  connect(addSubkeyButton, SIGNAL(clicked(bool)), this, SLOT(slotAddSubkey()));
  connect(subkeyList, SIGNAL(itemSelectionChanged()), this,
          SLOT(slotRefreshSubkeyDetail()));

  // key database refresh signal
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slotRefreshKeyInfo()));
  connect(SignalStation::GetInstance(), SIGNAL(KeyDatabaseRefresh()), this,
          SLOT(slotRefreshSubkeyList()));

  baseLayout->setContentsMargins(0, 0, 0, 0);

  setLayout(baseLayout);
  setAttribute(Qt::WA_DeleteOnClose, true);

  slotRefreshSubkeyList();
}

void KeyPairSubkeyTab::createSubkeyList() {
  subkeyList = new QTableWidget(this);

  subkeyList->setColumnCount(5);
  subkeyList->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  subkeyList->verticalHeader()->hide();
  subkeyList->setShowGrid(false);
  subkeyList->setSelectionBehavior(QAbstractItemView::SelectRows);

  // tableitems not editable
  subkeyList->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  subkeyList->setFocusPolicy(Qt::NoFocus);
  subkeyList->setAlternatingRowColors(true);

  QStringList labels;
  labels << _("Subkey ID") << _("Key Size") << _("Algo")
         << _("Create Date (UTC)") << _("Expire Date (UTC)");

  subkeyList->setHorizontalHeaderLabels(labels);
  subkeyList->horizontalHeader()->setStretchLastSection(false);
}

void KeyPairSubkeyTab::slotRefreshSubkeyList() {
  LOG(INFO) << "Called";
  int row = 0;

  subkeyList->setSelectionMode(QAbstractItemView::SingleSelection);

  this->buffered_subkeys.clear();
  auto sub_keys = mKey.subKeys();
  for (auto& sub_key : *sub_keys) {
    if (sub_key.disabled() || sub_key.revoked()) continue;
    this->buffered_subkeys.push_back(std::move(sub_key));
  }

  subkeyList->setRowCount(buffered_subkeys.size());

  for (const auto& subkeys : buffered_subkeys) {
    auto* tmp0 = new QTableWidgetItem(QString::fromStdString(subkeys.id()));
    tmp0->setTextAlignment(Qt::AlignCenter);
    subkeyList->setItem(row, 0, tmp0);

    auto* tmp1 = new QTableWidgetItem(QString::number(subkeys.length()));
    tmp1->setTextAlignment(Qt::AlignCenter);
    subkeyList->setItem(row, 1, tmp1);

    auto* tmp2 =
        new QTableWidgetItem(QString::fromStdString(subkeys.pubkey_algo()));
    tmp2->setTextAlignment(Qt::AlignCenter);
    subkeyList->setItem(row, 2, tmp2);

    auto* tmp3 = new QTableWidgetItem(
        QString::fromStdString(to_iso_string(subkeys.timestamp())));
    tmp3->setTextAlignment(Qt::AlignCenter);
    subkeyList->setItem(row, 3, tmp3);

    auto* tmp4 = new QTableWidgetItem(
        boost::posix_time::to_time_t(
            boost::posix_time::ptime(subkeys.expires())) == 0
            ? _("Never Expire")
            : QString::fromStdString(to_iso_string(subkeys.expires())));
    tmp4->setTextAlignment(Qt::AlignCenter);
    subkeyList->setItem(row, 4, tmp4);

    if (!row) {
      for (auto i = 0; i < subkeyList->columnCount(); i++) {
        subkeyList->item(row, i)->setForeground(QColor(65, 105, 255));
      }
    }

    row++;
  }

  if (subkeyList->rowCount() > 0) {
    subkeyList->selectRow(0);
  }
}

void KeyPairSubkeyTab::slotAddSubkey() {
  auto dialog = new SubkeyGenerateDialog(mKey.id(), this);
  dialog->show();
}

void KeyPairSubkeyTab::slotRefreshSubkeyDetail() {
  auto& subkey = getSelectedSubkey();

  keyidVarLabel->setText(QString::fromStdString(subkey.id()));
  keySizeVarLabel->setText(QString::number(subkey.length()));

  time_t subkey_time_t =
      boost::posix_time::to_time_t(boost::posix_time::ptime(subkey.expires()));

  expireVarLabel->setText(
      subkey_time_t == 0 ? _("Never Expires")
                         : QLocale::system().toString(QDateTime::fromTime_t(
                               to_time_t(subkey.expires()))));
  if (subkey_time_t != 0 &&
      subkey.expires() < boost::posix_time::second_clock::local_time()) {
    auto paletteExpired = expireVarLabel->palette();
    paletteExpired.setColor(expireVarLabel->foregroundRole(), Qt::red);
    expireVarLabel->setPalette(paletteExpired);
  } else {
    auto paletteValid = expireVarLabel->palette();
    paletteValid.setColor(expireVarLabel->foregroundRole(), Qt::darkGreen);
    expireVarLabel->setPalette(paletteValid);
  }

  algorithmVarLabel->setText(QString::fromStdString(subkey.pubkey_algo()));
  createdVarLabel->setText(QLocale::system().toString(
      QDateTime::fromTime_t(to_time_t(subkey.timestamp()))));

  std::stringstream usage_steam;

  if (subkey.can_certify()) usage_steam << _("Certificate") << " ";
  if (subkey.can_encrypt()) usage_steam << _("Encrypt") << " ";
  if (subkey.can_sign()) usage_steam << _("Sign") << " ";
  if (subkey.can_authenticate()) usage_steam << _("Auth") << " ";

  usageVarLabel->setText(usage_steam.str().c_str());

  // Show the situation that primary key not exists.
  masterKeyExistVarLabel->setText(subkey.secret() ? _("Exists")
                                                  : _("Not Exists"));
  if (!subkey.secret()) {
    auto paletteExpired = masterKeyExistVarLabel->palette();
    paletteExpired.setColor(masterKeyExistVarLabel->foregroundRole(), Qt::red);
    masterKeyExistVarLabel->setPalette(paletteExpired);
  } else {
    auto paletteValid = masterKeyExistVarLabel->palette();
    paletteValid.setColor(masterKeyExistVarLabel->foregroundRole(),
                          Qt::darkGreen);
    masterKeyExistVarLabel->setPalette(paletteValid);
  }

  fingerPrintVarLabel->setText(QString::fromStdString(subkey.fpr()));
}

void KeyPairSubkeyTab::createSubkeyOperaMenu() {
  subkeyOperaMenu = new QMenu(this);
  // auto *revokeSubkeyAct = new QAction(_("Revoke Subkey"));
  auto* editSubkeyAct = new QAction(_("Edit Expire Date"));
  connect(editSubkeyAct, SIGNAL(triggered(bool)), this, SLOT(slotEditSubkey()));

  // subkeyOperaMenu->addAction(revokeSubkeyAct);
  subkeyOperaMenu->addAction(editSubkeyAct);
}

void KeyPairSubkeyTab::slotEditSubkey() {
  LOG(INFO) << "Fpr" << getSelectedSubkey().fpr();

  auto dialog =
      new KeySetExpireDateDialog(mKey.id(), getSelectedSubkey().fpr(), this);
  dialog->show();
}

void KeyPairSubkeyTab::slotRevokeSubkey() {}

void KeyPairSubkeyTab::contextMenuEvent(QContextMenuEvent* event) {
  if (!subkeyList->selectedItems().isEmpty()) {
    subkeyOperaMenu->exec(event->globalPos());
  }
}

const GpgSubKey& KeyPairSubkeyTab::getSelectedSubkey() {
  int row = 0;

  for (int i = 0; i < subkeyList->rowCount(); i++) {
    if (subkeyList->item(row, 0)->isSelected()) break;
    row++;
  }

  return buffered_subkeys[row];
}
void KeyPairSubkeyTab::slotRefreshKeyInfo() {
  mKey = GpgKeyGetter::GetInstance().GetKey(mKey.id());
}

}  // namespace GpgFrontend::UI
