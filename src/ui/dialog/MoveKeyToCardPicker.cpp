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

#include "MoveKeyToCardPicker.h"

#include "core/function/gpg/GpgSmartCardManager.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSubKey.h"
#include "core/utils/GpgUtils.h"
#include "ui/widgets/KeyTreeView.h"

namespace GpgFrontend::UI {

namespace {

/// a (sub)key can be moved onto a card when it still holds private material and
/// gpg exposes a card slot that matches its capabilities
auto IsMovableSubKey(const GpgSubKey& sk) -> bool {
  return sk.IsSecretKey() && !sk.IsADSK() && !sk.IsCardKey() &&
         !sk.IsRevoked() && !sk.IsExpired() &&
         !GpgSmartCardManager::CandidateSlots(sk).isEmpty();
}

/// a key is worth showing when its primary part or any subkey can be moved
auto KeyHasMovablePart(const GpgKey& key) -> bool {
  if (!key.IsPrivateKey()) return false;
  for (const auto& sk : key.SubKeys()) {
    if (IsMovableSubKey(sk)) return true;
  }
  return false;
}

/// prefer the default database, otherwise the first supported one
auto PickInitialChannel(const QContainer<KeyDatabaseInfo>& dbs) -> int {
  if (dbs.isEmpty()) return kGpgFrontendDefaultChannel;
  for (const auto& db : dbs) {
    if (db.channel == kGpgFrontendDefaultChannel) return db.channel;
  }
  return dbs.front().channel;
}

}  // namespace

auto MoveKeyToCardPicker::SupportedDatabases() -> QContainer<KeyDatabaseInfo> {
  QContainer<KeyDatabaseInfo> ret;
  for (const auto& db : GetGpgKeyDatabaseInfos()) {
    // smart card operations are GnuPG-only; rpgp databases cannot host them
    if (db.backend_type.toLower().trimmed() == "rpgp") continue;
    ret.append(db);
  }
  return ret;
}

MoveKeyToCardPicker::MoveKeyToCardPicker(QWidget* parent)
    : GeneralDialog(typeid(MoveKeyToCardPicker).name(), parent),
      channel_(PickInitialChannel(SupportedDatabases())),
      tree_view_(new KeyTreeView(
          channel_,
          [](GpgAbstractKey* k) -> bool {
            if (k == nullptr) return false;
            if (k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
              auto* sk = dynamic_cast<GpgSubKey*>(k);
              return sk != nullptr && IsMovableSubKey(*sk);
            }
            if (k->KeyType() == GpgAbstractKeyType::kGPG_KEY) {
              auto* gk = dynamic_cast<GpgKey*>(k);
              // the top-level row stands for the primary key, i.e. subkey 0
              return gk != nullptr && !gk->SubKeys().isEmpty() &&
                     IsMovableSubKey(gk->SubKeys().front());
            }
            return false;
          },
          [](const GpgAbstractKey* k) -> bool {
            if (k == nullptr) return false;
            if (k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
              const auto* sk = dynamic_cast<const GpgSubKey*>(k);
              return sk != nullptr && IsMovableSubKey(*sk);
            }
            if (k->KeyType() == GpgAbstractKeyType::kGPG_KEY) {
              const auto* gk = dynamic_cast<const GpgKey*>(k);
              return gk != nullptr && KeyHasMovablePart(*gk);
            }
            return false;
          })) {
#ifdef Q_OS_MACOS
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
#endif

  setWindowTitle(tr("Select Key to Move to Card"));
  setModal(true);
  setMinimumSize(560, 420);
  resize(700, 500);

  auto* title_label = new QLabel(tr("Choose a Key or Subkey to Move"));
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* desc_label = new QLabel(
      tr("Select the single private key or subkey to move onto the smart card. "
         "Expand a key to choose a particular subkey. Only parts that can be "
         "stored on a card are selectable."));
  desc_label->setWordWrap(true);

  auto tips_palette = desc_label->palette();
  tips_palette.setColor(
      QPalette::WindowText,
      palette().color(QPalette::Disabled, QPalette::WindowText));
  desc_label->setPalette(tips_palette);

  // let the user choose which key database to move a key from; only GnuPG
  // databases are listed because smart cards are unsupported by rpgp
  const auto databases = SupportedDatabases();
  auto* db_label = new QLabel(tr("Database") + ": ");
  db_combo_ = new QComboBox(this);
  for (const auto& db : databases) {
    db_combo_->addItem(QString("%1: %2").arg(db.channel).arg(db.name),
                       db.channel);
  }
  const auto current_db = db_combo_->findData(channel_);
  if (current_db >= 0) db_combo_->setCurrentIndex(current_db);

  connect(db_combo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int index) {
            if (index < 0) return;
            channel_ = db_combo_->itemData(index).toInt();
            tree_view_->SetChannel(channel_);
            update_confirm_button_state();
          });

  auto* db_row = new QHBoxLayout();
  db_row->addWidget(db_label);
  db_row->addWidget(db_combo_, 1);

  // a single database needs no selector, so keep the dialog uncluttered
  const bool show_db_selector = databases.size() > 1;
  db_label->setVisible(show_db_selector);
  db_combo_->setVisible(show_db_selector);

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  confirm_btn_ = button_box->button(QDialogButtonBox::Ok);
  confirm_btn_->setText(tr("Confirm"));
  confirm_btn_->setEnabled(false);

  button_box->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

  connect(button_box, &QDialogButtonBox::accepted, this, [this]() {
    if (!resolve_selection()) {
      QMessageBox::information(
          this, tr("Select One Key"),
          tr("Please select exactly one key or subkey to move to the card."));
      return;
    }
    accept();
  });

  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(tree_view_, &KeyTreeView::SignalKeysChecked, this,
          &MoveKeyToCardPicker::update_confirm_button_state);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);

  layout->addWidget(title_label);
  layout->addWidget(desc_label);
  layout->addLayout(db_row);
  layout->addWidget(tree_view_, 1);
  layout->addWidget(button_box);

  setAttribute(Qt::WA_DeleteOnClose, false);

  update_confirm_button_state();
}

auto MoveKeyToCardPicker::GetSelectedKey() const -> GpgKeyPtr {
  return selected_key_;
}

auto MoveKeyToCardPicker::GetSelectedChannel() const -> int { return channel_; }

auto MoveKeyToCardPicker::GetSelectedSubKeyIndex() const -> int {
  return selected_subkey_index_;
}

auto MoveKeyToCardPicker::resolve_selection() -> bool {
  selected_key_ = nullptr;
  selected_subkey_index_ = -1;

  if (tree_view_ == nullptr) return false;

  // a card move targets a single part; enforce exactly one checked entry
  const auto checked = tree_view_->GetAllCheckedKeys();
  if (checked.size() != 1) return false;

  const auto& item = checked.front();
  if (item == nullptr) return false;

  QString primary_fpr;
  QString target_fpr;  // fingerprint of the part to move
  if (item->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
    auto* s_key = dynamic_cast<GpgSubKey*>(item.get());
    if (s_key == nullptr) return false;
    const auto primary = s_key->Convert2GpgKey();
    if (primary == nullptr) return false;
    primary_fpr = primary->Fingerprint();
    target_fpr = s_key->Fingerprint();
  } else if (item->KeyType() == GpgAbstractKeyType::kGPG_KEY) {
    primary_fpr = item->Fingerprint();
    target_fpr = primary_fpr;  // the primary key part is subkey 0
  } else {
    return false;
  }

  auto key = GpgKeyRepository::GetInstance(channel_).GetKeyPtr(primary_fpr);
  if (key == nullptr) return false;

  const auto subkeys = key->SubKeys();
  for (int i = 0; i < static_cast<int>(subkeys.size()); ++i) {
    if (subkeys[i].Fingerprint() != target_fpr) continue;
    selected_key_ = key;
    selected_subkey_index_ = i;
    return true;
  }
  return false;
}

void MoveKeyToCardPicker::update_confirm_button_state() {
  if (confirm_btn_ == nullptr || tree_view_ == nullptr) return;
  confirm_btn_->setEnabled(tree_view_->GetAllCheckedKeys().size() == 1);
}

void MoveKeyToCardPicker::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);
  if (!isRectRestored()) movePosition2CenterOfParent();
}

}  // namespace GpgFrontend::UI
