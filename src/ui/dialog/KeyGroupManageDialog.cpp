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

#include "KeyGroupManageDialog.h"

#include "core/function/gpg/GpgAbstractKeyGetter.h"
#include "ui/widgets/KeyList.h"

//
#include "ui_KeyGroupManageDialog.h"

namespace GpgFrontend::UI {

KeyGroupManageDialog::KeyGroupManageDialog(
    int channel, const QSharedPointer<GpgKeyGroup>& key_group, QWidget* parent)
    : GeneralDialog(typeid(KeyGroupManageDialog).name(), parent),
      ui_(QSharedPointer<Ui_KeyGroupManageDialog>::create()),
      channel_(channel),
      key_group_(key_group) {
  ui_->setupUi(this);

  connect(ui_->addButton, &QPushButton::clicked, this,
          &KeyGroupManageDialog::slot_add_to_key_group);

  connect(ui_->removeButton, &QPushButton::clicked, this,
          &KeyGroupManageDialog::slot_remove_from_key_group);

  ui_->keyGroupKeyList->Init(
      channel, KeyMenuAbility::kCOLUMN_FILTER | KeyMenuAbility::kSEARCH_BAR,
      GpgKeyTableColumn::kTYPE | GpgKeyTableColumn::kNAME |
          GpgKeyTableColumn::kEMAIL_ADDRESS | GpgKeyTableColumn::kKEY_ID);
  ui_->keyGroupKeyList->AddListGroupTab(
      tr("Key Group"), "key-group",
      GpgKeyTableDisplayMode::kPRIVATE_KEY |
          GpgKeyTableDisplayMode::kPUBLIC_KEY,
      [=](const GpgAbstractKey* key) -> bool {
        return key_group_->KeyIds().contains(key->ID());
      });
  ui_->keyGroupKeyList->SlotRefresh();

  ui_->keyList->Init(
      channel, KeyMenuAbility::kCOLUMN_FILTER | KeyMenuAbility::kSEARCH_BAR,
      GpgKeyTableColumn::kTYPE | GpgKeyTableColumn::kNAME |
          GpgKeyTableColumn::kEMAIL_ADDRESS | GpgKeyTableColumn::kKEY_ID);
  ui_->keyList->AddListGroupTab(
      tr("Default"), "default",
      GpgKeyTableDisplayMode::kPRIVATE_KEY |
          GpgKeyTableDisplayMode::kPUBLIC_KEY,
      [=](const GpgAbstractKey* key) -> bool {
        return key->IsHasEncrCap() && key->ID() != key_group_->ID() &&
               !key_group_->KeyIds().contains(key->ID());
      });
  ui_->keyList->SlotRefresh();

  QTimer::singleShot(200, [=]() { slot_notify_invalid_key_ids(); });

  this->setModal(true);
  this->setWindowTitle(tr("Key Group Management"));

  movePosition2CenterOfParent();

  this->show();
  this->raise();
  this->activateWindow();
}

void KeyGroupManageDialog::slot_add_to_key_group() {
  auto keys = ui_->keyList->GetCheckedKeys();
  QSet<QString> set;

  GpgAbstractKeyPtrList failed_keys;
  auto& getter = GpgKeyGroupGetter::GetInstance(channel_);
  for (const auto& key : keys) {
    if (!getter.AddKey2KeyGroup(key_group_->ID(), key)) {
      failed_keys.push_back(key);
    }
  }

  ui_->keyGroupKeyList->RefreshKeyTable(0);
  ui_->keyList->RefreshKeyTable(0);

  if (!failed_keys.isEmpty()) {
    QStringList failed_ids;
    for (const auto& key : failed_keys) {
      failed_ids << key->ID();
    }

    QMessageBox::warning(this, tr("Some Keys Failed"),
                         tr("Some keys could not be added to the group:\n%1")
                             .arg(failed_ids.join(", ")));
  }
}

void KeyGroupManageDialog::slot_remove_from_key_group() {
  auto keys = ui_->keyGroupKeyList->GetCheckedKeys();

  auto& getter = GpgKeyGroupGetter::GetInstance(channel_);
  for (const auto& key : keys) {
    getter.RemoveKeyFromKeyGroup(key_group_->ID(), key->ID());
  }

  ui_->keyGroupKeyList->RefreshKeyTable(0);
  ui_->keyList->RefreshKeyTable(0);
}

void KeyGroupManageDialog::slot_notify_invalid_key_ids() {
  auto key_ids = key_group_->KeyIds();

  QStringList invalid_key_ids;
  for (const auto& key_id : key_ids) {
    auto key = GpgAbstractKeyGetter::GetInstance(channel_).GetKey(key_id);
    if (key == nullptr) invalid_key_ids.push_back(key_id);
  }

  if (invalid_key_ids.isEmpty()) {
    return;
  }

  const QString id_list = invalid_key_ids.join(", ");
  const auto message =
      tr("This Key Group contains some invalid keys:\n\n%1\n\n"
         "These keys are no longer available. Do you want to remove them from "
         "the group?")
          .arg(id_list);

  const auto reply = QMessageBox::question(
      this, tr("Invalid Keys in Group"), message,
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    auto key_ids = key_group_->KeyIds();
    auto& getter = GpgKeyGroupGetter::GetInstance(channel_);
    for (const auto& key_id : invalid_key_ids) {
      getter.RemoveKeyFromKeyGroup(key_group_->ID(), key_id);
    }
  }
}
}  // namespace GpgFrontend::UI
