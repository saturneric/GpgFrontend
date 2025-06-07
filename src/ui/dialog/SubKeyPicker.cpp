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

#include "SubKeyPicker.h"

#include <utility>

#include "core/function/gpg/GpgAbstractKeyGetter.h"

namespace GpgFrontend::UI {

SubKeyPicker::SubKeyPicker(int channel, GpgAbstractKeyPtrList keys,
                           QWidget* parent)
    : GeneralDialog("SubKeyPicker", parent),
      channel_(channel),
      buffered_keys_(std::move(keys)) {
  setWindowTitle(tr("Select Signing Key(s) & Subkey(s)"));
  setModal(true);

  auto* layout = new QVBoxLayout(this);

  layout->addWidget(
      new QLabel(tr("Choose the primary key(s) or one or more subkeys:")));

  QVector<QString> key_ids;

  for (const auto& k : buffered_keys_) {
    if (k->KeyType() != GpgAbstractKeyType::kGPG_KEY) continue;

    auto gpg_key = qSharedPointerDynamicCast<GpgKey>(k);
    if (gpg_key == nullptr) continue;

    for (const auto& s_key : gpg_key->SubKeys()) {
      if (s_key.IsHasSignCap()) key_ids.push_back(s_key.Fingerprint());
    }
  }

  tree_view_ = new KeyTreeView(
      channel_,
      [](GpgAbstractKey* k) {
        return k->KeyType() == GpgAbstractKeyType::kGPG_KEY ||
               k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY;
      },
      [=](const GpgAbstractKey* k) {
        return key_ids.contains(k->Fingerprint());
      });

  connect(tree_view_, &KeyTreeView::SignalKeysChecked, this,
          [=](const GpgAbstractKeyPtrList& keys) {
            confirm_btn_->setDisabled(keys.empty());
          });

  tree_view_->setStyleSheet("QTreeView::item { height: 28px; }");
  layout->addWidget(tree_view_);

  auto* tips_label = new QLabel(tr(
      "Multiple signing subkeys have been detected among your selected keys, "
      "which makes it unclear which key should be used for signing. "
      "Please select the primary key(s) or one or more subkeys with signing "
      "capability to proceed."));
  tips_label->setWordWrap(true);
  tips_label->setStyleSheet("color: #666; margin-bottom: 6px;");

  layout->addWidget(tips_label);

  confirm_btn_ = new QPushButton(tr("Confirm"));
  cancel_btn_ = new QPushButton(tr("Cancel"));

  connect(confirm_btn_, &QPushButton::clicked, this, [=]() {
    if (tree_view_->GetAllCheckedKeys().isEmpty()) {
      QMessageBox::information(this, tr("No Subkeys Selected"),
                               tr("Please select at least one Subkey."));

      return;
    }
    accept();
  });
  connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);

  auto* btn_layout = new QHBoxLayout();
  btn_layout->addStretch();
  btn_layout->addWidget(confirm_btn_);
  btn_layout->addWidget(cancel_btn_);
  layout->addLayout(btn_layout);

  this->setMinimumWidth(480);

  confirm_btn_->setDisabled(true);

  movePosition2CenterOfParent();

  // should not delete itself at closing by default
  setAttribute(Qt::WA_DeleteOnClose, false);
};

[[nodiscard]] auto SubKeyPicker::GetSelectedKeyWithFlags() const
    -> GpgAbstractKeyPtrList {
  const auto keys = tree_view_->GetAllCheckedKeys();

  GpgAbstractKeyPtrList ret;

  auto& getter = GpgAbstractKeyGetter::GetInstance(channel_);

  for (const auto& k : keys) {
    GpgAbstractKeyPtr key = nullptr;

    if (k->KeyType() == GpgAbstractKeyType::kGPG_KEY ||
        k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
      key = getter.GetKey(k->Fingerprint() + "!");
    }

    if (key) ret.push_back(key);
  }

  return ret;
}
}  // namespace GpgFrontend::UI