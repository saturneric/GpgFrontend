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

#include "core/function/openpgp/AbstractKeyRepository.h"

namespace GpgFrontend::UI {

SubKeyPicker::SubKeyPicker(int channel, GpgAbstractKeyPtrList keys,
                           QWidget* parent)
    : GeneralDialog("SubKeyPicker", parent),
      channel_(channel),
      buffered_keys_(std::move(keys)) {
#ifdef Q_OS_MACOS
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
#endif

  setWindowTitle(tr("Select Signing Key(s) && Subkey(s)"));
  setModal(true);
  setMinimumSize(560, 420);
  resize(680, 480);

  auto* title_label = new QLabel(tr("Choose Signing Subkey(s)"));
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_label->setFont(title_font);

  auto* desc_label =
      new QLabel(tr("Choose the primary key(s) or one or more signing-capable "
                    "subkeys to use for this operation."));
  desc_label->setWordWrap(true);

  QVector<QString> key_ids;
  key_ids.reserve(buffered_keys_.size() * 2);

  for (const auto& k : buffered_keys_) {
    if (k == nullptr || k->KeyType() != GpgAbstractKeyType::kGPG_KEY) {
      continue;
    }

    auto gpg_key = qSharedPointerDynamicCast<GpgKey>(k);
    if (gpg_key == nullptr) continue;

    for (const auto& s_key : gpg_key->SubKeys()) {
      if (s_key.IsHasSignCap()) {
        key_ids.push_back(s_key.Fingerprint());
      }
    }
  }

  tree_view_ = new KeyTreeView(
      channel_,
      [](GpgAbstractKey* k) {
        return k != nullptr &&
               (k->KeyType() == GpgAbstractKeyType::kGPG_KEY ||
                k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY);
      },
      [key_ids](const GpgAbstractKey* k) {
        return k != nullptr && key_ids.contains(k->Fingerprint());
      });

  tree_view_->setUniformRowHeights(true);
  tree_view_->setAlternatingRowColors(true);
  tree_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tree_view_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  tree_view_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  auto* tips_label = new QLabel(
      tr("Multiple signing subkeys were detected among the selected keys. "
         "Please choose exactly the key material that should be used for "
         "signing."));
  tips_label->setWordWrap(true);

  auto tips_palette = tips_label->palette();
  tips_palette.setColor(
      QPalette::WindowText,
      palette().color(QPalette::Disabled, QPalette::WindowText));
  tips_label->setPalette(tips_palette);

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  confirm_btn_ = button_box->button(QDialogButtonBox::Ok);
  confirm_btn_->setText(tr("Confirm"));
  confirm_btn_->setEnabled(false);

  cancel_btn_ = button_box->button(QDialogButtonBox::Cancel);
  cancel_btn_->setText(tr("Cancel"));

  connect(button_box, &QDialogButtonBox::accepted, this, [this]() {
    if (tree_view_ == nullptr || tree_view_->GetAllCheckedKeys().isEmpty()) {
      QMessageBox::information(this, tr("No Subkeys Selected"),
                               tr("Please select at least one signing key or "
                                  "signing subkey."));
      return;
    }

    accept();
  });

  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(tree_view_, &KeyTreeView::SignalKeysChecked, this,
          &SubKeyPicker::update_confirm_button_state);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);

  layout->addWidget(title_label);
  layout->addWidget(desc_label);
  layout->addWidget(tree_view_, 1);
  layout->addWidget(tips_label);
  layout->addWidget(button_box);

  setAttribute(Qt::WA_DeleteOnClose, false);

  update_confirm_button_state();

  if (!isRectRestored()) {
    movePosition2CenterOfParent();
  }
}

void SubKeyPicker::update_confirm_button_state() {
  if (confirm_btn_ == nullptr || tree_view_ == nullptr) return;
  confirm_btn_->setEnabled(!tree_view_->GetAllCheckedKeys().isEmpty());
}

[[nodiscard]] auto SubKeyPicker::GetSelectedKeyWithFlags() const
    -> GpgAbstractKeyPtrList {
  if (tree_view_ == nullptr) return {};

  const auto keys = tree_view_->GetAllCheckedKeys();

  GpgAbstractKeyPtrList ret;
  auto& getter = AbstractKeyRepository::GetInstance(channel_);

  for (const auto& k : keys) {
    if (k == nullptr) continue;

    if (k->KeyType() != GpgAbstractKeyType::kGPG_KEY &&
        k->KeyType() != GpgAbstractKeyType::kGPG_SUBKEY) {
      continue;
    }

    auto key = getter.GetKey(k->Fingerprint() + "!");
    if (key != nullptr) {
      ret.push_back(key);
    }
  }

  return ret;
}

}  // namespace GpgFrontend::UI